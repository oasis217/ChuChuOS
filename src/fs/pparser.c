#include "pparser.h"
#include "string/string.h"
#include "kernel.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"

//------------------------------------------------------------------
static int pathparser_path_valid_format_check(const char* filename)
{
    int len = strnlen(filename,CHUCHUOS_MAX_PATH_LENGTH);
    //    Address format  0:/bin/kernel.bin
    // 1) Length must be greater than 3
    // 2) Drive name should start with a number
    // 3) After number, two characters are needed  :/

    char check[2] = {':','/'};

       // condition-1 &&   condition-2          &&         condition-3
    return( (len>=3)  && (isdigit(filename[0])) && (memcmp((void*)&filename[1],(void*)&check[0],2) == 0) );  

}

//------------------------------------------------------------------

static int pathparser_get_drive_of_path(const char** path)
{
    // first establish validity of path
    if( !pathparser_path_valid_format_check(*path))
    {
        return -EBADPATH;
    }

    int drive_no = tonumericdigit(*path[0]);

    // Add 3bytes to skip drive number and associate characters,  eg 0:/ , 1:/  2:/
    *path += 3;

    return drive_no;

}


//---------------------------------------------------------------------

struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* path_r=kzalloc(sizeof(struct path_root));
    path_r->drive_no = drive_number;
    path_r->first_part = 0;

    return path_r;
}

//---------------------------------------------------------------------

static const char* pathparser_get_current_partof_path(const char** path)
{   
    char* current_part = kzalloc(CHUCHUOS_MAX_PATH_LENGTH);

    int i=0;
    while(**path != '/' && **path != 0x00)
    {
        current_part[i]=**path;
        *path += 1;  
        i++;
    }

    if(**path == '/')
    {
        // skip forward slash to avoid problems
        *path += 1;
    }

    if(i == 0)
    {
        kfree(current_part);
        current_part=0;
    }

    return current_part;

}

//---------------------------------------------------------------------
struct path_part* pathparser_addpath_inlist(struct path_part* last_part,const char** path)
{
    const char* current_path_string = pathparser_get_current_partof_path(path);

    if(!current_path_string)
    {
        return 0;
    }

    struct path_part* current_part = kzalloc(sizeof(struct path_part));
    current_part->current_part = current_path_string;
    current_part->next_part = 0x00;

    if(last_part)
    {
        last_part->next_part = current_part;
    }

    return current_part;

}

//---------------------------------------------------------------------
void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first_part;

    while(part)
    {
        struct path_part* next_part = part->next_part;
        kfree((void*)part->current_part);
        kfree(part);
        part = next_part;
    }

    kfree(root);
}

//---------------------------------------------------------------------
struct path_root* pathparser_parse(const char* path, const char* current_directory_path)
{
    int response = 0;
    const char* tmp_path = path;
    struct path_root* path_root = 0;

// checking the total length of the path
    if(strlen(path) > CHUCHUOS_MAX_PATH_LENGTH)
    {
        goto out;
    }


// Getting the drive number
    response = pathparser_get_drive_of_path(&tmp_path);

    if(response < 0)
    {
        goto out;
    }


// Now create root
    path_root = pathparser_create_root(response);

    if(!path_root)
    {
        goto out;
    }

// Now get the first part
    struct path_part* first_part = pathparser_addpath_inlist(NULL,&tmp_path);

    if(!first_part)
    {
        goto out;
    }

    path_root->first_part = first_part;

    struct path_part* part = pathparser_addpath_inlist(first_part,&tmp_path);

    while(part)
    {
        part = pathparser_addpath_inlist(part,&tmp_path);
    }

out:
    return path_root;

}


