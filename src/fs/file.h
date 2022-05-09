#ifndef FILE_H
#define FILE_H

#include "pparser.h"

//-------------------------------------------------------
typedef unsigned int FILE_SEEK_MODE;  
enum
{
    SEEK_SET,  // set pointer to beginning of a file
    SEEK_CUR,   // set pointer to a given location 
    SEEK_END    // set pointer to the end of a file
};

//-------------------------------------------------------
typedef unsigned int FILE_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

//-------------------------------------------------------

struct disk;  // you can do forward declaration (without including disk.h), as long as you only use pointer types

typedef void* (*FS_OPEN_FUNCTION) (struct disk* disk, struct path_part* path, FILE_MODE mode );

typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

//--------------------------------------------------------

struct filesystem
{
    FS_RESOLVE_FUNCTION resolve;  // we typedefed the function pointer
    FS_OPEN_FUNCTION open;
    char name[20];
};

//-------------------------------------------------------

struct file_descriptor
{
    int index;  // file descriptor index
    struct filesystem* filesystem;

    void* privte;  //  

    struct disk* disk;   // the disk that the file-descriptor should be used on.
};


void fs_init();
void fs_insert_filesystem(struct filesystem* filesystem);
int fopen(const char* filename, const char* mode);
struct filesystem* fs_resolve(struct disk* disk);



#endif