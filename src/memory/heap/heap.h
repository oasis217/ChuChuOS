#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stddef.h>
#include <stdint.h>

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN    0x01  // these are the lower 4 bits
#define HEAP_BLOCK_TABLE_ENTRY_FREE     0x00

#define HEAP_BLOCK_HAS_NEXT (1 << 7)    // 7th bit
#define HEAP_BLOCK_IS_FIRST (1 << 6)    // 6th bit

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table
{
    HEAP_BLOCK_TABLE_ENTRY* entries;
    size_t total;   // i.e. total number of entries
};

struct heap
{
    struct heap_table* table;
    void* saddr;  // start address of the heap data pool
};


int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);
void* heap_malloc(struct heap* heap, size_t size);
void  heap_free(struct heap* heap, void* ptr);



#endif