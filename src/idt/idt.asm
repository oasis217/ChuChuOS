; here we tell the processor to load the idtr register, so it could know where the interrupt table
; is

section .asm 

extern int21h_handler
extern no_interrupt_handler 

global idt_load
global enable_interrupts
global disable_interrupts 
global no_interrupt


global int21h

;-----------------------------
enable_interrupts:
    sti 
    ret

;-----------------------------
disable_interrupts:
    sti
    ret 

;-----------------------------
idt_load:
    push ebp
    mov ebp,esp
    
    mov ebx, [ebp+8]
    lidt [ebx]

    pop ebp
    ret

;-----------------------------
int21h:
    cli
    pushad   ; pushes all the registers to the stack
    call int21h_handler
    popad
    sti
    iret 


;-----------------------------
no_interrupt:
    cli
    pushad   ; pushes all the registers to the stack
    call no_interrupt_handler
    popad
    sti
    iret 