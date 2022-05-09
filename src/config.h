#ifndef CONFIG_H
#define CONFIG_H

#define KERNEL_CODE_SELECTOR    0x08
#define KERNEL_DATA_SELECTOR    0x10

#define CHUCHUOS_TOTAL_INTERRUPTS 256

//100 mb heap size
#define CHUCHUOS_HEAP_SIZE_BYTES 104857600  // 100 mb in bytes
#define CHUCHUOS_HEAP_BLOCK_SIZE    4096
#define CHUCHUOS_HEAP_ADDRESS   0x01000000   // look at osdev memory map > 1mb
#define CHUCHUOS_HEAP_TABLE_ADDRESS  0x00007E00 // 480.5 kb bytes free at this location, we need 25600 bytes


#define CHUCHUOS_SECTOR_SIZE 512

#define CHUCHUOS_MAX_FILESYSTEMS 12
#define CHUCHUOS_MAX_FILE_DESCRIPTORS 512

#endif
