#ifndef STRING_H
#define STRING_H

#include<stdbool.h>


int strlen(const char* ptr);
int strnlen(const char* ptr, int max);
bool isdigit(char c);
int tonumericdigit(char c);
char* strcpy(char* dest, const char* src);
char tolower(char s1);
int strlen_terminator(const char* str,int max,char terminator);
int strncmp(const char* str1, const char* str2, int n);
int istrncmp(const char* s1, const char* s2, int n);



#endif 