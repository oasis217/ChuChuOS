#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"

struct idtr_desc idtr_descriptor; // this structure holds the address and size of the interrupt table
struct idt_desc idt_descriptors[CHUCHUOS_TOTAL_INTERRUPTS];  // info of each interrupt

extern void idt_load(struct idtr_desc *ptr);
extern void int21h();
extern void no_interrupt();


void int21h_handler()
{
    print("Keyboard pressed!\n");
    outb(0x20, 0x20);    // send acknowledgement to PIC that the interrupt has been handled
                        // otherwise no further interrupts would be processed by PIC
                        // 0x20 is the command code for acknowledgement

}


void no_interrupt_handler()
{
    outb(0x20, 0x20);
}

void idt_zero()
{
    print("Divide by zero error\n");
}


void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0;
    desc->type_attr = 0xEE;  //lower four bit i.e. E(1110) ensures a 32 bit interrupt gate
                            // uper 3 bit i.e. E(1110) ensures ring3 protection and interrupt is used
    desc->offset_2 = (uint32_t)address >> 16;    
}



void idt_init()
{
    // first make sure that the memory which contains the idt descriptors do not have
    // any bad information, can cause crashes

    //--> we set the information of IDTR register

    memset(idt_descriptors,0,sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors)-1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;

    for(int i=0; i<CHUCHUOS_TOTAL_INTERRUPTS; i++)
    {
        idt_set(i,no_interrupt);
    }

    idt_set(0, idt_zero);
    idt_set(0x21, int21h);        //remember we have remapped PIC to start from 0x20,
                                        // so 0x21 is keyboard interrupt.

    idt_load(&idtr_descriptor);

}

