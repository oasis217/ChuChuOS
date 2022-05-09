#ifndef PAGING_H
#define PAGING_H

#include<stdint.h>
#include<stddef.h>
#include<stdbool.h>



#define PAGING_CACHE_DISABLED   (1 << 4)
#define PAGING_WRITE_THROUGH    (1 << 3)
#define PAGING_ACCESS_FROM_ALL  (1 << 2)
#define PAGING_IS_WRITEABLE     (1 << 1)
#define PAGING_IS_PRESENT       (1 << 0)


#define PAGING_TOTAL_ENTRIES_PER_TABLE  1024
#define PAGING_PAGE_SIZE    4096


struct paging_4gb_chunk
{
    uint32_t* directory_address;
};


struct paging_4gb_chunk* paging_create_new_4gb_chunk(uint8_t flags);
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk);

void paging_switch(uint32_t* directory);
void enable_paging();

bool paging_is_aligned(void* address);
int paging_set(uint32_t* directory, void* virt_addr, uint32_t val);




#endif