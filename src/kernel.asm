[BITS 32]

global _start
global problem
extern  kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    ; in boot.asm we have already initially CS register by jmp instruction. The rest of the segment registers
    ; which are now actually segmenet select registers are need to point to the global descriptor table relevant
    ; sections.
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp,ebp

    ; Enabling the A-20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Remap the master PIC
    mov al, 00010001b
    out 0x20, al ; Sending master PIC command, 0x20 is command port for PIC

    mov al, 0x20    ; interrupt 0x20 (decimal: 32) is where master ISR should start
    out 0x21, al

    mov al, 00000001b
    out 0x21, al 
    ; End remap of master PIC
 
    call kernel_main

    jmp $


    times 512-($ - $$) db 0 