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
    int index;  // index in the filedescriptors array. Remembers we need to have multiple file descriptors
                // in an array, bcz we can communicate with different storage media each having their
                // own filesystem with particular attributes (even if they are the same).
    struct filesystem* filesystem;   // filesystem of the disk

    void* privte;  //  basically pointer returned by fopen()

    struct disk* disk;   // the disk that the file-descriptor should be used on.
};


void fs_init();
void fs_insert_filesystem(struct filesystem* filesystem);
int fopen(const char* filename, const char* mode_str);
struct filesystem* fs_resolve(struct disk* disk);



#endif