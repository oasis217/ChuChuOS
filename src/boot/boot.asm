ORG 0x7c00   ; telling the assembler where to load the instructions and data into the memory
        ; origin is zero, means all our labels would be offsetted from zero
        
BITS 16      ; need to boot in 16 bit mode, legacy booting

CODE_SEG equ gdt_code - gdt_start  ; offset for Code Segment
DATA_SEG equ gdt_data - gdt_start  ; offset for data segment

;Initially we are in real mode, where by default we have 16 bit instruction, and access to only 1mb of ram.
jmp short start
nop

; FAT16 Header : https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
OEMIdentifier           db 'CHUCHUOS'
BytesPerSector          dw 0x200    ; 512 bytes
SectorsPerCluster       db 0x80     ; 128 sectors per cluster
ReservedSectors         dw 200      ; 512 reserved sector, this is to store our kernel
FATCopies               db 0x02     ; default value
RootDirEntries          dw 0x40     ; root directory entries
NumSectors              dw 0x00
MediaType               db 0xF8
SectorsPerFat           dw 0x100    ; number of sectors per fat
SectorsPerTrack         dw 0x20
NumberOfHeads           dw 0x40
HiddenSectors           dd 0x00
SectorsBig              dd 0x773594

; Extended BPB (Dos 4.0)
DriveNumber             db 0x80
WinNTBit                db 0x00
Signature               db 0x29
VolumeID                dd 0xD105
VolumeIDString          db 'CHUOS1 BOOT'
SystemIDString          db 'FAT16   '
;---------------------------------------------
start:
    jmp 0:step2  ; jump code is required to set the code segment to 0x7c0 (see ORG) as well
                    ; this basically means Code Segment CS is initialized to 0x7c0, and IP (intruction pointer)
                    ; is initialized to the address of step2



step2:   ; we initialize all the segment register as a matter of good practice, so no  buggy info is left
        ; within these registers. Also remember everything is offset from origin.
    cli ; clear interrupt
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti ; Enable interrupt

; --> Now we are making preparation to switch to 32 bit mode
.load_protected:
    cli  
    lgdt[gdt_descriptor] ; here we load the gdt descriptor tabe
    mov eax, cr0
    or eax, 0x01
    mov cr0, eax  ; enable the last bit in control reg to go to protected mode
    jmp CODE_SEG:load32   ; 
    jmp $

; GDT definition
; -> Remember we are adhering to the flat model, so there is actually no segmentation. Therefore
;    the value of base registers for both code and data section is initialized to zero, and the 
;    limits are initialized to their maximum value of 4gb. 
gdt_start:
gdt_null:
    dd 0x0   ; allocate 4 bytes of zero
    dd 0x0   ; allocate 4 bytes of zero again

; offset : 0x08
gdt_code:   ; CS SHOULD point to this
    dw 0xffff   ; segment limit
    dw 0        ; base-first 0-15 bits
    db 0        ; base 16-23 bits
    db 0x9a     ; access byte
    db 11001111b   ; limit last bytes + Flags
    db 0        ; base 24-31

; offset : 0x10 (16 in decimal)
gdt_data:   ; DS,SS,ES,FS, GS
    dw 0xffff   ; segment limit
    dw 0        ; base-first 0-15 bits
    db 0        ; base 16-23 bits
    db 0x92     ; access byte
    db 11001111b   ; limit last bytes + Flags
    db 0        ; base 24-31  

gdt_end:

;----------------------
gdt_descriptor:
    dw gdt_end - gdt_start -1
    dd gdt_start

[BITS 32]
load32:
    mov eax, 1 ; starting sector to load from, sector-1, bcz sector-0 contains our bootloader
    mov ecx, 100 ; total no of sectors to load, remember in make file we dd'ed 100 sector of 512 bytes each
    mov edi, 0x0100000   ; edi contains the address in the ram where the code needs to be loaded
    call ata_lba_read 
    ; so our disk driver has loaded the code from hard-disk to a specified ram address, see edi above.
    jmp CODE_SEG:0x0100000  ; here we now set the code segment register, which should now point to the global   
                            ; descriptor section of code. Now our segment register have become segment select
                            ; register.

ata_lba_read:
    mov ebx, eax   ; Backup of the LBA
    ; Send the highest 8 bits of the lba to the harddisk controller
    shr eax, 24
    or eax, 0xE0   ; selects the masters drive
    mov dx, 0x1F6
    out dx, al
    ; Finished reading the highest 8 bit of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of the LBA
    mov eax, ebx
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx ; restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

    ; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

 ; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw 
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret  


times 510-($ - $$) db 0  ; this command basically fills zero uptil the 510th byte
dw 0xAA55   ; this is the bootloader signature : 0x55AA which the bios looks for, reversed
            ; bcz intel machines have little endianess


