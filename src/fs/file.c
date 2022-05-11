#include "file.h"
#include "config.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "fat/fat16.h"
#include "fs/pparser.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem* filesystems[CHUCHUOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[CHUCHUOS_MAX_FILE_DESCRIPTORS];

//-------------------------------------------------------------
static struct filesystem**  fs_get_free_filesystem()
{
    int i=0;

    for(i=0; i<CHUCHUOS_MAX_FILESYSTEMS; i++)
    {
        if(filesystems[i]==0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

//-------------------------------------------------------------

void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;
    fs = fs_get_free_filesystem();

    if(!fs)
    {
        print("Problem Inserting filesystem");
        while(1) {}
    }

    *fs = filesystem;
}

//-------------------------------------------------------------
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}


//------------------------------------------------------------
void fs_load()
{
    memset(filesystems,0,sizeof(filesystems));
    fs_static_load();
}

//------------------------------------------------------------
void fs_init()
{
    memset(file_descriptors,0,sizeof(file_descriptors));
    fs_load();
}


//------------------------------------------------------------
static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;

    for(int i=0; i< CHUCHUOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if(file_descriptors[i]==0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));

            // Descriptor index always starts with 1
            desc->index = i+1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }
    return res;
}

//-----------------------------------------------------------------
static struct file_descriptor* file_get_descriptor(int index)
{
    if(index < 1 || index > CHUCHUOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    return file_descriptors[index-1];
}

//-----------------------------------------------------------------
struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = 0;

    for(int i=0; i<CHUCHUOS_MAX_FILESYSTEMS; i++)
    {
        if(filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

//-----------------------------------------------------------------
FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode =  FILE_MODE_INVALID;

    if(strncmp(str,"r",1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if(strncmp(str,"w",1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    if(strncmp(str,"a",1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}

//-----------------------------------------------------------------
int fopen(const char* filename, const char* mode_str)
{
    int res;

    struct path_root* file_header = pathparser_parse(filename,NULL);

    if(!file_header)
    {
        res = -EINVARG;
        goto out;
    }

    // we also have to check if there is a valid filepath in subsequent parts of the 
    // list : e.g.  0:/test.txt   vs 0:/  ??

    if(!file_header->first_part)
    {
        res = -EINVARG;
        goto out;
    }

    // Ensure that the disk we are reading from exists
    struct disk* disk = disk_get(file_header->drive_no);
    if(!disk)
    {
        res = -EIO;
        goto out;
    }

    // also ensure that the particular disk has a valid filesystem
    if(!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    // checking if the file mode is valid
    if(mode == FILE_MODE_INVALID)
    {
        return -EINVARG;
        goto out;
    }

    void* descriptor_private_data = disk->filesystem->open(disk,file_header->first_part,mode);
    // above is pointer to the file

    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);  // this function initializes the pointer desc with address

    if(res < 0)
    {
        goto out;
    }

    // here we are adding the information to the file_descriptors array.
    // this is necessary because we maybe reading files from different disks
    // with different filesystems.
    desc->disk = disk;
    desc->privte = descriptor_private_data;
    desc->filesystem = disk->filesystem;
    res = desc->index;


out:

    if (res < 0)
    {
        res = 0;
    }
    return res;
}


