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
char tolower(char s1)
{   // converts to lower character for alphabets
    if (s1 >=65 && s1<=90)
    {
        s1 +=32;
    }

    return s1;
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

//------------------------------------------
int strlen_terminator(const char* str,int max,char terminator)
{   // this function tells where a particular character exists in the string
    int i=0;

    for(i=0;i<max;i++)
    {
        if(str[i] == '\0' || str[i] == terminator)
        {
            break;
        }
    }

    return i;
}

//------------------------------------------
int strncmp(const char* str1, const char* str2, int n)
{
    // compares two strings and sends a non-zero value if they are not equal
    unsigned char u1,u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;

        if(u1 != u2)
        {
            return u1-u2;
        }
        if(u1 == '\0')
        {
            return 0;
        }
    }

    return 0;
}

//--------------------------------------------
int istrncmp(const char* s1, const char* s2, int n)
{
    // this function compares and returns 0, while being case- insensitive

    unsigned char u1,u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;

        if( (u1 != u2) && (tolower(u1) != tolower(u2)) )
        {
            return u1-u2;
        }
        if(u1 == '\0')
        {
            return 0;
        }
    }
    return 0;
}