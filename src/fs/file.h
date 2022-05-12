#ifndef FILE_H
#define FILE_H

#include "pparser.h"
#include <stdint.h>
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

enum{
    FILE_STAT_READ_ONLY = (1 << 0),
};

typedef unsigned int FILE_STAT_FLAGS; 


//-------------------------------------------------------

struct disk;  // you can do forward declaration (without including disk.h), as long as you only use pointer types

typedef void* (*FS_OPEN_FUNCTION) (struct disk* disk, struct path_part* path, FILE_MODE mode );

typedef int (*FS_READ_FUNCTION) (struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);

typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

typedef int (*FS_SEEK_FUNCTION) (void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);


struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

typedef int (*FS_STAT_FUNCTION) (struct disk* disk, void* private, struct file_stat* stat);

typedef int (*FS_CLOSE_FUNCTION) (void* private);

//--------------------------------------------------------

struct filesystem
{
    FS_RESOLVE_FUNCTION resolve;  // we typedefed the function pointer
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;
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

    struct disk* disk;   // the disk on which we have the file
};


void fs_init();
void fs_insert_filesystem(struct filesystem* filesystem);
int fopen(const char* filename, const char* mode_str);
int fread(void* read_buf, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
int fstat(int fd, struct file_stat* stat);
int fclose(int fd);
struct filesystem* fs_resolve(struct disk* disk);



#endif