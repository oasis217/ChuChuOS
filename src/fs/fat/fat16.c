#include "fat16.h" 
#include "string/string.h"
#include "status.h"
#include <stdint.h>
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "kernel.h"


#define CHUCHUOS_FAT16_SIGNATURE 0x29
#define CHUCHUOS_FAT16_FAT_ENTRY_SIZE 0x02
#define CHUCHUOS_FAT16_BAD_SECTOR 0xFF7
#define CHUCHUOS_FAT16_UNUSED 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1
 
// FAT directory entry attributes --> See FAT DIRECTORY ITEM
#define FAT_FILE_READONLY   0x01
#define FAT_FILE_HIDDEN     0x02
#define FAT_FILE_SYSTEM     0x04
#define FAT_FILE_VOLUME_LABEL   0x08
#define FAT_FILE_SUBDIRECTORY   0x10
#define FAT_FILE_ARCHIVED       0x20
#define FAT_FILE_DEVICE     0x40
#define FAT_FILE_RESERVED   0x80



struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));


struct fat_header
{
    uint8_t short_jump_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;
} __attribute__((packed));


struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};


struct fat_directory_item
{
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

//--------------------------------------------
struct fat_directory
{
    struct fat_directory_item* item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

//--------------------------------------------
struct fat_item
{
    union
    {
        struct fat_directory_item* item;     // either a filel      
        struct fat_directory* directory;    // or a directory 
    };

    FAT_ITEM_TYPE type;
};

//--------------------------------------------
struct fat_file_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

//---------------------------------------------
struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    // Stream data clusters
    struct disk_stream* cluster_read_stream;

    // Stream file allocation table
    struct disk_stream* fat_read_stream;

    // Stream the director
    struct disk_stream* directory_stream;

};



int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode );

struct filesystem fat16_fs = 
{
    .resolve = fat16_resolve,
    .open = fat16_open
};


//---------------------------------------------------------------------------
struct filesystem* fat16_init()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

//---------------------------------------------------------------------------
static void fat16_init_private(struct disk* disk, struct fat_private* private)
{
    memset(private,0,sizeof(struct fat_private));

    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
}

//----------------------------------------------------------------------------
int fat16_get_total_items_of_directory_from_disk(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;

   struct fat_private* fat_private = disk->fs_private;

   int res = 0;
   int counter = 0;

   int directory_start_pos = directory_start_sector * disk->sector_size;
   struct disk_stream* stream = fat_private->directory_stream;

   if(diskstreamer_seek(stream, directory_start_pos) != CHUCHUOS_ALL_OK)
   {
       res = -EIO;
       goto out;
   }


   while(1)
   {
       if(diskstreamer_read(stream,&item,sizeof(item)) != CHUCHUOS_ALL_OK)
       {
           res = -EIO;
           goto out;
       }

       if(item.filename[0] == 0x00)  //--> this means current and all subsequent entries are emtpy: see note
       {
           break;
       }

       // un-used item
       if(item.filename[0] == 0xE5)
       {
           continue;
       }

       counter++;
   }

    res = counter;

out:
    return res;
}

//-----------------------------------------------------------------------------

int fat16_sector_to_absolute(struct disk* disk, int sector_no)
{
    return sector_no* disk->sector_size;
}


//----------------------------------------------------------------------------
int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    // directory in argument is our root directory

    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = primary_header->reserved_sectors + (primary_header->fat_copies * primary_header->sectors_per_fat);
    int root_dir_entries = primary_header->root_dir_entries;
    int root_dir_size = root_dir_entries * sizeof(struct fat_directory_item);

    int total_sectors = root_dir_size / disk->sector_size ;

    if( root_dir_size % disk->sector_size)
    {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_of_directory_from_disk(disk,root_dir_sector_pos);

    struct fat_directory_item* dir = kzalloc(root_dir_size);  // variable to get all the items of the root directory
    if(!dir)
    {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;

    if(diskstreamer_seek(stream,fat16_sector_to_absolute(disk,root_dir_sector_pos) ) != CHUCHUOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    if(diskstreamer_read(stream,dir,root_dir_size) != CHUCHUOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;

}

//----------------------------------------------------------------------------
int fat16_resolve(struct disk* disk)
{
    int res = 0;

    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk,fat_private);

    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;
    
    struct disk_stream* stream = diskstreamer_new(disk->id);

    if(!stream)
    {
        res= -ENOMEM;
        goto out;
    }

    // dont need to seek, bcz we are at the start of the file

    if(diskstreamer_read(stream, &fat_private->header,sizeof(fat_private->header)) != CHUCHUOS_ALL_OK )
    {
        res = -EIO;
        goto out;
    }

    if(fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    if(fat16_get_root_directory(disk,fat_private,&fat_private->root_directory)!= CHUCHUOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

out:
    if(stream)
    {
        diskstreamer_close(stream);
    }

    if(res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }

    return res;

}

//-----------------------------------------------------------------------------
struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size)
{
    struct fat_directory_item* item_copy = 0;

    if(size < sizeof(struct fat_directory_item))
    {
        return 0;
    }

    item_copy = kzalloc(size);
    if(!item_copy)
    {
        return 0;
    }

    memcpy(item_copy,item,size);

    return item_copy;

}


//-----------------------------------------------------------------------------
void fat16_to_proper_string(char** out, const char* in)
{   // this function is required, because fat16 names can have empty spaces

    while(*in != 0x00 && *in != 0x20)  // not equal to null character or empty space
    {   
        **out = *in;
        *out += 1;
        in += 1;
    }

    if(*in == 0x20)
    {
        **out = 0x00;   // putting null character when there is space in the filename
    }

}

//-----------------------------------------------------------------------------

void fat16_get_full_filename_from_dir(struct fat_directory_item* item, char* full_name, int mem_reqd_for_name)
{
    memset(full_name,0,mem_reqd_for_name);
    char* tmp_name = full_name;
    fat16_to_proper_string(&tmp_name, (const char*)item->filename);

    if(item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
        *tmp_name++ = '.';
        fat16_to_proper_string(&tmp_name,(const char*)item->ext);
    }
}

//-----------------------------------------------------------------------------
static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster) | (item->low_16_bits_first_cluster) ;
}

//-----------------------------------------------------------------------------
static int fat16_cluster_to_sector(struct fat_private* fat_private,int cluster)
{
    return fat_private->root_directory.ending_sector_pos + ( (cluster-2)*fat_private->header.primary_header.sectors_per_cluster) ;
}

//-----------------------------------------------------------------------------
static uint32_t fat16_get_first_fat_sector(struct fat_private* fat_private)
{
    return fat_private->header.primary_header.reserved_sectors;
}

//-----------------------------------------------------------------------------
static int fat16_get_entry_from_fat_table(struct disk* disk, int cluster)
{
    int res = -1;
    struct fat_private* fat_private = disk->fs_private;
    struct disk_stream* stream = fat_private->fat_read_stream;
    if(!stream)
    {
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_fat_sector(fat_private)*disk->sector_size;

    res = diskstreamer_seek(stream,fat_table_position + (cluster*CHUCHUOS_FAT16_FAT_ENTRY_SIZE));
    if(res < 0)
    {
        goto out;
    }

    uint16_t result = 0;
    res = diskstreamer_read(stream,&result,sizeof(result));
    if(res < 0)
    {
        goto out;
    }

    res = result;

out:
    return res;
}


//-----------------------------------------------------------------------------
static int fat_get_offseted_cluster(struct disk* disk, int starting_cluster, int offset)
{
    int res = 0;
    struct fat_private* fat_private = disk->fs_private;
    int bytes_per_cluster = fat_private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int no_of_clusters_ahead = offset / bytes_per_cluster;

    for(int i=0; i<no_of_clusters_ahead;i++)
    {
        // need function to read fat entry
        int entry = fat16_get_entry_from_fat_table(disk,cluster_to_use);

        if(entry == 0xFF8 || entry == 0xFFF)
        {   // this is the last entry
            res = -EIO;
            goto out;
        }

        if(entry == CHUCHUOS_FAT16_BAD_SECTOR)
        {
            res = -EIO;
            goto out;
        }

        if(entry == 0xFF0 || entry == 0xFF6)
        {
            res = -EIO;
            goto out;
        }

        if(entry == 0x00)
        {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }
    
    res = cluster_to_use;

out: 
    return res;
}

//-----------------------------------------------------------------------------
static int fat16_retrieve_dir_data_from_stream(struct disk* disk, struct disk_stream* stream, int starting_cluster, int offset, int total, void* out_buf)
{
    int res = 0;
    struct fat_private* fat_private = disk->fs_private;

    int size_of_cluster_bytes = fat_private->header.primary_header.sectors_per_cluster * disk->sector_size;

    int cluster_to_use = fat_get_offseted_cluster(disk,starting_cluster,offset);

    if(cluster_to_use < 0)
    {
        res = cluster_to_use;
        goto out;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;

    int starting_sector = fat16_cluster_to_sector(fat_private,cluster_to_use);
    int starting_position = (starting_sector * disk->sector_size) + offset_from_cluster;

    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;

    res = diskstreamer_seek(stream,starting_position);

    if(res != CHUCHUOS_ALL_OK)
    {
        goto out;
    } 

    res = diskstreamer_read(stream,out_buf,total_to_read);

    total -= total_to_read;

    if(total > 0)
    {
        // we still have more data to read
        res = fat16_retrieve_dir_data_from_stream(disk,stream,starting_cluster,offset+total_to_read,total,out_buf+total_to_read);
    }

out:
    return res;
}

//-----------------------------------------------------------------------------
static int fat16_retrieve_dir_data(struct disk* disk, int starting_cluster, int offset, int total, void* out_buf)
{
    struct fat_private* fat_private = disk->fs_private;
    struct disk_stream* stream = fat_private->cluster_read_stream;
    return fat16_retrieve_dir_data_from_stream(disk,stream,starting_cluster,offset,total,out_buf);
}

//-----------------------------------------------------------------------------
void fat_free_directory(struct fat_directory* fat_directory)
{
    if(!fat_directory)
    {
        return;
    }

    if(fat_directory->item)
    {
        kfree(fat_directory->item);
    }

    kfree(fat_directory);
}

//-----------------------------------------------------------------------------
void fat_free_item(struct fat_item* fat_item)
{
    if(fat_item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat_free_directory(fat_item->directory);
    }
    else if(fat_item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(fat_item->item);
    }

    kfree(fat_item);
}

//-----------------------------------------------------------------------------
struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    struct fat_directory* fat_directory = 0;
    struct fat_private* fat_private = disk->fs_private;

    if(!(item->attribute & FAT_FILE_SUBDIRECTORY) )
    {
        res = -EINVARG;
        goto out;
    }

    fat_directory = kzalloc(sizeof(struct fat_directory));
    if(!fat_directory)
    {
        res = -ENOMEM;
        goto out;
    }

    int first_cluster_of_item = fat16_get_first_cluster(item);
    int first_sector_of_item = fat16_cluster_to_sector(fat_private,first_cluster_of_item);
    int total_items_in_dir = fat16_get_total_items_of_directory_from_disk(disk,first_sector_of_item);

    fat_directory->total = total_items_in_dir;

    int directory_size = (fat_directory->total)*sizeof(struct fat_directory_item);

    fat_directory->item = kzalloc(directory_size);
    if(!fat_directory->item)
    {
        res = -ENOMEM;
        goto out;
    }

    res = fat16_retrieve_dir_data(disk,first_cluster_of_item,0x00,directory_size,fat_directory->item);
    if(res != CHUCHUOS_ALL_OK)
    {
        goto out;
    }
out:
    if(res != CHUCHUOS_ALL_OK)
    {
        fat_free_directory(fat_directory);
    }

    return fat_directory;
}

//-----------------------------------------------------------------------------
struct fat_item* fat16_create_new_fat_item_for_directory_item(struct disk* disk,struct fat_directory_item* item)
{
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if(!f_item)
    {
        return 0;
    }

    if(item->attribute && FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk,item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE; 
    // its a file clone the file from disk to memory
    f_item->item = fat16_clone_directory_item(item,sizeof(struct fat_directory_item));

    return f_item; 

}

//-----------------------------------------------------------------------------
struct fat_item* fat16_get_path_item_from_directory(struct disk* disk, struct fat_directory* fat_directory, const char* name)
{
    struct fat_item* fat_item = 0;
    char temp_filename[CHUCHUOS_MAX_PATH];
    // First find the 1st path from the root directory

    for(int i=0; i < fat_directory->total; i++)
    {   
        fat16_get_full_filename_from_dir(&fat_directory->item[i],temp_filename,sizeof(temp_filename));

        if(istrncmp(temp_filename,name,sizeof(temp_filename))==0)
        {
            // we found a match, now create a new fat item
            fat_item = fat16_create_new_fat_item_for_directory_item(disk,&fat_directory->item[i]);
        }
    }

    return fat_item;
}

//-----------------------------------------------------------------------------
struct fat_item* fat16_get_final_file_from_directory(struct disk* disk, struct path_part* path)
{
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* current_item = 0;

    struct fat_item* root_item  = fat16_get_path_item_from_directory(disk,&fat_private->root_directory,path->current_part);
    if(!root_item)
    {
        goto out;
    }

    struct path_part* next_part = path->next_part;
    current_item=root_item;

    while(next_part != 0)
    {
        if(current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }

        struct fat_item* tmp_item = fat16_get_path_item_from_directory(disk,current_item->directory,next_part->current_part);
        fat_free_item(current_item);
        next_part = next_part->next_part;

        current_item = tmp_item;
    }

out:
    return current_item;
}

//-----------------------------------------------------------------------------
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    if(mode != FILE_MODE_READ)
    {
        return ERROR(ERDONLY);
    }

    struct fat_file_descriptor* descriptor = 0;

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if(!descriptor)
    {
        return ERROR(-ENOMEM);
    }

    descriptor->item = fat16_get_final_file_from_directory(disk,path);
    if(!descriptor->item)
    {
        return ERROR(-EIO);
    }

    descriptor->pos = 0;
    return descriptor;

}








