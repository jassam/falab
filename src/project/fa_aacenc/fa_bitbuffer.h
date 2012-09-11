#ifndef  _FA_BITBUFFER_H 
#define	 _FA_BITBUFFER_H 

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus 
extern "C"
{ 
#endif  

typedef struct _fa_bitbuffer_t {
	int buf_size;				//the number of bits of this bitbuffer
	int is_valid;

	unsigned char *start;		//point to the start position of bitbuffer 
	unsigned char *end;		    //point to the end position of bitbuffer

	unsigned char *pNextRead;   //point to the next read position
	unsigned char *pNextWrite;  //point to the next write position

	int rpos_of_byte;			//0<=rpos_of_byte<=7
	int wpos_of_byte;		    //0<=wpos_of_byte<=7

	int nbits;		            //number of available bits in the bitstream buffer
	                            //write bits to bitstream buffer  => increment cntBits 
	                            //read bits from bitstream buffer => decrement cntBits 
}fa_bitbuffer_t;


void fa_write_byte(unsigned char in , FILE *fp);
void fa_write_ushort(unsigned short in, FILE *fp);
void fa_write_ulong(unsigned long in, FILE *fp);

unsigned char fa_read_byte(FILE *fp);
unsigned short fa_read_ushort(FILE *fp);
unsigned long fa_read_ulong(FILE *fp);


void fa_bitbuffer_init(fa_bitbuffer_t *bitbuf, unsigned char *start, int sizeofbyte);
void fa_bitbuffer_uninit(fa_bitbuffer_t * bitbuf);
int  fa_bit2byte(int bit);

int  fa_putbits(fa_bitbuffer_t * bitbuf, unsigned int wValue, int nbits);
int  fa_getbits(fa_bitbuffer_t * bitbuf, short noBitsToRead);

#ifdef __cplusplus 
}
#endif  

#endif
