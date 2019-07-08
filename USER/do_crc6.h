#ifndef __CRC6_MW_H
#define __CRC6_MW_H

#include <stdio.h> 
#include <stdlib.h>

unsigned short do_crc6(unsigned char *message, unsigned int len);
unsigned short do_crc6_2(unsigned char *message, unsigned int len);
unsigned short fast_crc6_2(unsigned char *message, unsigned int len);


#endif


