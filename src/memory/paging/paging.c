#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

extern void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

//------------------------------------------------------------------------------------------------
struct paging_4gb_chunk* paging_create_new_4gb_chunk(uint8_t flags)
{
    //1. First create directory
    uint32_t* directory = kzalloc(sizeof(uint32_t)*PAGING_TOTAL_ENTRIES_PER_TABLE);

    int offset = 0;

    //2. Now need to populate the directory, but to do that we need to initialize page table and 
    //   populate their values as well

    for(int i=0; i<PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        //3a) initialize the page_table_entry
        //         Each page table has 1024 entries of address | flags
        uint32_t* page_table_entry = kzalloc(sizeof(uint32_t)*PAGING_TOTAL_ENTRIES_PER_TABLE);

        for(int b=0; b<PAGING_TOTAL_ENTRIES_PER_TABLE;b++)
        {
            page_table_entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }

        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE*PAGING_PAGE_SIZE);

        directory[i] = (uint32_t)page_table_entry | flags | PAGING_IS_WRITEABLE;

    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));

    chunk_4gb->directory_address = directory;

    return chunk_4gb;

}

//------------------------------------------------------------------------------------------------
void paging_switch(uint32_t* directory)
{   
    // loading a directory requires an assembly function, to enable reqd in cr3 register
    paging_load_directory(directory);
    current_directory = directory;
}

//------------------------------------------------------------------------------------------------

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return (chunk->directory_address);
}

//-------------------------------------------------------------------------------------------------
bool paging_is_aligned(void* address)
{
    return (((uint32_t)address % 4096) == 0 );  // if condition true, paging is aligned
}


//------------------------------------------------------------------------------------------------
int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    // Remember directory_index --> page table address
    //          table_index     --> page number in that table

    int res = 0;

    if(!paging_is_aligned(virtual_address))
    {
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = (uint32_t)(virtual_address) / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);

    *table_index_out = ((uint32_t)(virtual_address) % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE)) / PAGING_PAGE_SIZE;
    
out:
    return res;
}

//------------------------------------------------------------------------------------------------

int paging_set(uint32_t* directory, void* virt_addr, uint32_t val)
{
    // here val = physical address | flags

    if(!paging_is_aligned(virt_addr))
    {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;

    int res = paging_get_indexes(virt_addr,&directory_index,&table_index);

    if(res < 0)
    {
        return res;
    }

    uint32_t entry = directory[directory_index];  
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    table[table_index] = val;

    return 0;
}



