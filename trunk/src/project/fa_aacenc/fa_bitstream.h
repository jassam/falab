#ifndef _FA_BITSTREAM_H
#define _FA_BITSTREAM_H 

typedef unsigned uintptr_t;

uintptr_t fa_bitstream_init(int num_bytes);
void      fa_bitstream_uninit(uintptr_t handle);

void fa_bitstream_reset(uintptr_t handle);
int  fa_bitstream_fillbuffer(uintptr_t handle, unsigned char *buf, int num_bytes);

int  fa_bitstream_putbits(uintptr_t handle, unsigned int value, int nbits);
int  fa_bitstream_getbits(uintptr_t handle, unsigned int *value, int nbits);

#endif
