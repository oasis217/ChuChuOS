#include "string.h"

//------------------------------------------
int strlen(const char* ptr)
{
    int i=0;

    while(*ptr != 0)
    {   
        i++;
        ptr++;
    }

    return i;
}
//-----------------------------------------
int strnlen(const char* ptr, int max)
{
    int i=0;

    for(i=0; i<max; i++)
    {
        if(ptr[i] == 0) {
            break;
        }
        
    }

        return i;
}
//-----------------------------------------
bool isdigit(char c)
{
    return (c >= 48 && c<=57);  // 48-57 are ascii chracterers of number 0-9 
}
//-----------------------------------------

int tonumericdigit(char c)
{
    return (c-48);
}

//--------------------------------------------
char* strcpy(char* dest, const char* src)
{
    char* res = dest;

    while(*src != 0)
    {
        *dest = *src;
        dest += 1;
        src += 1;
    }

    *dest = 0x00;
    
    return res;
}