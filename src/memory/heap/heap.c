#include "heap.h"
#include "kernel.h"
#include "status.h" 
#include "memory/memory.h"
#include <stdbool.h>

//--------------------------------------------------------------------------------
static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    // this function ensures that the start and end of the heap, and the total entries in heap 
    // table are initialized correctly by the kernel.
    int res = 0;

    size_t heap_data_size = (size_t)(end-ptr);
    size_t total_blocks = heap_data_size / CHUCHUOS_HEAP_BLOCK_SIZE;

    if(table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

    out:
        return res;
}


//--------------------------------------------------------------------------------
static bool heap_validate_alignment(void* ptr) // validate giving address adhers to 4096 byte alignment aka block
{
    return (((unsigned int)ptr % CHUCHUOS_HEAP_BLOCK_SIZE) == 0) ; // return-1 if condition is true
                            // true means it is aligned correctly i.e. address size is divisible by 4096
}

//--------------------------------------------------------------------------------
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    // 1st arg: heap --> we get an uninitialized heap
    // 2nd arg: start --> pointer to the start of heap
    // 3rd arg: end --> pointer to the end of heap
    // 4th arg: heap_table --> should provide pointer to a valid heap table

    /* In first part, we are first confirming if the given arguments are valid */

    int res = 0;

    //--> Q1: Is the alignment of pointers correct ??
    if(!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    //--> Q2: Is the table initialized correctly
    memset(heap,0,sizeof(struct heap));  // confusion should this be heap_table ??
    heap->saddr = ptr;
    heap->table = table;
    res = heap_validate_table(ptr,end,table);  // validating the argument provided etc

    if(res < 0)
    {
        goto out;
    }

    /* In the 2nd part, now we are initializing the heap table values with zero */

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY)*table->total;
    memset(table->entries,HEAP_BLOCK_TABLE_ENTRY_FREE,table_size);

out:
    return res;
}


//------------------------------------------------------------------------------------------
void heap_mark_block_taken(struct heap* heap, int start_block, int total_blocks)
{
    int end_block = (start_block + total_blocks)-1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if(total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for(int i=start_block; i<= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if(i != end_block-1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }


}


//------------------------------------------------------------------------------------------
void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->saddr + (block*CHUCHUOS_HEAP_BLOCK_SIZE);
}


//------------------------------------------------------------------------------------------
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

//------------------------------------------------------------------------------------------
int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    int block_counter = 0;
    int block_start = -1;
    int check_entry;

    for(size_t i=0; i<table->total; i++)
    {
        check_entry = heap_get_entry_type(table->entries[i]);

        if(check_entry != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            block_counter = 0;
            block_start = -1 ;
            continue;   // go back to the start of loop
        }

        // if we are here, we found a free entry
        if (block_start == -1)
        {
            block_start = i;
        }
        block_counter++;

        if(block_counter == total_blocks)
        {
            break;
        }

    }

    if(block_start == -1)
    {
        return -ENOMEM;
    }

    return block_start;

}

//------------------------------------------------------------------------------------------
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    int start_block = heap_get_start_block(heap,total_blocks); // need function which tells the start block which hasn enough consecutive blocks available

    if(start_block < 0)
    {
        goto out;  // could not find any block which fits our requirement
    }

    address =  heap_block_to_address(heap,start_block);  // if we do get a block, need a func to convert the start block number to address

    // at the end we need to mark the block as taken
    heap_mark_block_taken(heap,start_block,total_blocks);

out:
    return address;
}


//--------------------------------------------------------------------------------
static uint32_t heap_align_value_to_upper_block(uint32_t val)
{
    // this functions aligns the memory requirement to block alignment

    if( (val % CHUCHUOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }
    // if not then -> first align to lower modulus of 4096 and then add one block
    val = (val - (val % CHUCHUOS_HEAP_BLOCK_SIZE));
    val += CHUCHUOS_HEAP_BLOCK_SIZE;
    return val;
}

//--------------------------------------------------------------------------------
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper_block(size);
    uint32_t total_blocks = aligned_size / CHUCHUOS_HEAP_BLOCK_SIZE;

    return(heap_malloc_blocks(heap,total_blocks));  // this function returns the address of the starting block
                                                    // which has enough blocks

    return 0;
}

//-------------------------------------------------------------------------------
void heap_mark_blocks_free(struct heap* heap, int start_block)
{
    struct heap_table* table = heap->table;

    for(int i= start_block; i< (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i]= HEAP_BLOCK_TABLE_ENTRY_FREE;

        if( !(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}


//-------------------------------------------------------------------------------
int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address-heap->saddr))/(CHUCHUOS_HEAP_BLOCK_SIZE);
}


//--------------------------------------------------------------------------------
void heap_free(struct heap* heap, void* ptr)
{
    // first we need a function, which can change the address to block number, so in the heap table entry
    // we can mark the block as free  : heap_address_to_block
    int start_block = heap_address_to_block(heap,ptr);

    return( heap_mark_blocks_free(heap,start_block));

}
