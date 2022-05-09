#ifndef IO_H
#define IO_H

unsigned char insb(unsigned short port);    // read a byte from port
unsigned short insw(unsigned short port);    // read a word from port

void outb(unsigned short port, unsigned char val); // output a byte to port
void outw(unsigned short port, unsigned short val); // output a word to port



#endif
