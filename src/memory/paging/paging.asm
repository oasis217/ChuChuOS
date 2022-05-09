[BITS 32]

section .asm

global paging_load_directory
global enable_paging

paging_load_directory:
    push ebp
    mov ebp,esp
    mov eax, [ebp+8]  ; moving the value of directory address to register eax
    mov cr3, eax    ; moving the address of directory to cr3 register
    pop ebp
    ret 


enable_paging:
    push ebp
    mov ebp,esp
    mov eax, cr0
    or eax, 0x80000000   ; enabling 31st bit 
    mov cr0, eax
    pop ebp
    ret 