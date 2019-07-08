#include "do_crc6.h"

unsigned char crc6_table[64]={
	0  ,3  ,6  ,5  ,12  ,15  ,10  ,
	9  ,24  ,27  ,30  ,29  ,20  ,23  ,
	18  ,17  ,48  ,51  ,54  ,53  ,60  ,
	63  ,58  ,57  ,40  ,43  ,46  ,45  ,
	36  ,39  ,34  ,33  ,35  ,32  ,37  ,
	38  ,47  ,44  ,41  ,42  ,59  ,56  ,
	61  ,62  ,55  ,52  ,49  ,50  ,19  ,
	16  ,21  ,22  ,31  ,28  ,25  ,26  ,
	11  ,8  ,13  ,14  ,7  ,4  ,1  ,2
};

unsigned short do_crc6(unsigned char *message, unsigned int len)
{
    int i, j;
    unsigned short crc_reg = 0;
    unsigned short current = 0;
       
    for (i = 0; i < len; i++)
    {
        current |= message[i];
        for (j = 0; j < 8; j++)
        {
        	 current <<= 1;    
            if (current&0x4000){
            	crc_reg = (current&0x7f00) ^ 0x4300;
            	current = crc_reg|(current&0xff);
            }
                  
        }
    }

    return ((current&0x3f00)>>8)^(0x3F);
}

unsigned short do_crc6_2(unsigned char *message, unsigned int len)
{
    int i, j;
    unsigned short crc_reg = 0;
    unsigned short current = 0;

    for (i = 0; i < len; i++)
    {
        current |= message[i];
        for (j = 0; j < 8; j++)
        {
        	 current <<= 1;
            if (current&0x4000){
            	crc_reg = (current&0x7f00) ^ 0x4300;
            	current = crc_reg|(current&0xff);
            }

        }
    }
    for (i = 0; i < 6; i++)
    {
            current <<= 1;
            if (current&0x4000){
            	crc_reg = (current&0x7f00) ^ 0x4300;
            	current = crc_reg|(current&0xff);
            }
    }
    return ((current&0x3f00)>>8)^(0x3F);
}

/*input with 6zero,input 6bit*/
unsigned short fast_crc6_2(unsigned char *message, unsigned int len)
{
    unsigned char i,out=0;

	for(i=0;i<len;i++){
		out = message[i] ^ crc6_table[out];
	}

    return out^0x3f;

}

