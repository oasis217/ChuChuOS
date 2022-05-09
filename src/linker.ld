ENTRY(_start)  
OUTPUT_FORMAT(binary)

SECTIONS
{
    . = 1M;   /* this is the 1024*1024 bytes = 0x1000000  address at which kernel 
                is located  */

                /* Dan say this will make sure that our origin start at 1 megabyte*/

                /* if you look at your linker document there we defined our memory
                , here this is basically like a shotcut way of doing that */

    .text :     ALIGN(4096)
    {
        *(.text)   /* this * basically says to include text files of all object files */

    }

    .rodata :   ALIGN(4096)
    {
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss :  ALIGN(4096)
    {
        *(COMMON)   /* COMMON is included to make sure all un-initialized variables
                       are in bss section, see linker video of fastbit */
        *(.bss)
    }

    .asm :
    {
        *(.asm)
    }


}
