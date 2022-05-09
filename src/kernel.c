#include "kernel.h"
#include<stdint.h>
#include<stddef.h>
#include"idt/idt.h"
#include "memory/heap/kheap.h"
#include"io/io.h"
#include"memory/paging/paging.h"
#include"disk/disk.h"
#include"string/string.h"
#include"fs/pparser.h"
#include"disk/streamer.h"
#include"fs/file.h"


uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

//-----------------------------------------------------
uint16_t terminal_make_char(char c, char color)
{
    return (color << 8 | c);
}

//----------------------------------------------------
void terminal_putchar(int x,int y, char c, char color)
{
    video_mem[(y*VGA_WIDTH)+x]= terminal_make_char(c,color);
}

//-----------------------------------------------------
void terminal_writechar(char c, char color)
{
    if(c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }



    terminal_putchar(terminal_col,terminal_row,c,color);
    terminal_col += 1;

    if(terminal_row >= VGA_WIDTH)
    {
        terminal_row += 1;
        terminal_col = 0;
    }
}


//-----------------------------------------------------
void terminal_initialize()
{
    video_mem = (uint16_t *)(0xB8000);

    for(int y=0; y < VGA_HEIGHT; y++)
    {
        for(int x=0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x,y,' ',15);
        }

    }
} 

//-----------------------------------------------------
void print(const char* str)
{
    size_t len = strlen(str);
    
    for(int i=0; i<len; i++)
    {
        terminal_writechar(str[i],15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0; 

//-----------------------------------------------------
void kernel_main()
{
    terminal_initialize();
    print("Hello World!\n");

    // initialize the heap
    kheap_init();

    // initialize the file-system
    fs_init();

    // search and initialize disk
    disk_search_and_init();

    // initializing the interrupt descriptor table
    idt_init();


    // Setup paging
    kernel_chunk = paging_create_new_4gb_chunk(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    // switch to kernel_paging_chunk
    paging_switch(kernel_chunk->directory_address);

    // enable paging
    enable_paging();

    // after initializing the IDT, now enabling interrupts
     enable_interrupts();

    struct disk* disk2 = disk_get(0);

    if(disk2)
    {
        
    }
    while(1)
    {

    }



}