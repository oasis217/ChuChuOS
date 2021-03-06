#ifndef KERNEL_H
#define KERNEL_H

#define VGA_WIDTH   80
#define VGA_HEIGHT  20

#define CHUCHUOS_MAX_PATH_LENGTH   108
#define CHUCHUOS_MAX_PATH   108

void kernel_main();
void print(const char* str);

#define ERROR(value) (void*)(value)    
#define ERROR_I(value) (int)(value)
#define ISERR(value) ( (int)value < 0)  // return 1 if val < 0, i.e. it is an error !!

#endif