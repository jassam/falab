#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_bitbuffer.h"
#include "fa_bitstream.h"

#ifndef FA_ABS
#define FA_ABS(A)    ((A) < 0 ? (-A) : (A))
#endif

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#endif

#ifndef FA_MAX
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

typedef struct _fa_bitstream_t {

    fa_bitbuffer_t bitbuf;

    unsigned char *data;
    int           num_bytes;

}fa_bitstream_t;


/*initial bitstream buffer, size is num_bytes*/
uintptr_t fa_bitstream_init(int num_bytes)
{
    fa_bitstream_t *f  = (fa_bitstream_t *)malloc(sizeof(fa_bitstream_t));

    f->data = (unsigned char *)malloc(sizeof(unsigned char)*num_bytes);
    memset(f->data, 0, sizeof(unsigned char)*num_bytes);
    f->num_bytes = num_bytes;

    fa_bitbuffer_init(&f->bitbuf, f->data, num_bytes);

    return (uintptr_t)f;
}

/*free the bitstream buffer*/
void      fa_bitstream_uninit(uintptr_t handle)
{
    fa_bitstream_t *f = (fa_bitstream_t *)handle;

    if(f) {
        fa_bitbuffer_uninit(&f->bitbuf);

        if(f->data) {
            free(f->data);
            f->data = NULL;
        }

        free(f);
    }
}

/*set the bitstream buffer to zero*/
void fa_bitstream_reset(uintptr_t handle)
{
    fa_bitstream_t *f = (fa_bitstream_t *)handle;
    memset(f->data, 0, sizeof(unsigned char)*f->num_bytes);
    fa_bitbuffer_init(&f->bitbuf, f->data, f->num_bytes);
}

/*fill the bitstream buffer using buf, return how many bytes filled*/
int  fa_bitstream_fillbuffer(uintptr_t handle, unsigned char *buf, int num_bytes)
{
    fa_bitstream_t *f = (fa_bitstream_t *)handle;
    int fill_bytes;

    fill_bytes = FA_MIN(num_bytes, f->num_bytes);

    memcpy(f->data, buf, fill_bytes);

    return fill_bytes;
}


/*put bits*/
int  fa_bitstream_putbits(uintptr_t handle, unsigned int value, int nbits)
{
    int ret;

    fa_bitstream_t *f = (fa_bitstream_t *)handle;

    ret = fa_putbits(&f->bitbuf, value, nbits);

    return ret;
}

/*get bits, return the value*/
int  fa_bitstream_getbits(uintptr_t handle, unsigned int *value, int nbits)
{
    int ret;

    fa_bitstream_t *f = (fa_bitstream_t *)handle;

    *value = fa_getbits(&f->bitbuf, nbits);

    return nbits;
}

int fa_bitstream_getbits_num(uintptr_t handle)
{
    int nbits;

    fa_bitstream_t *f = (fa_bitstream_t *)handle;

    nbits = fa_getbits_num(&f->bitbuf);

    return nbits;
}

int  fa_bitstream_getbufval(uintptr_t handle, unsigned char *buf_out)
{
    fa_bitstream_t *f = (fa_bitstream_t *)handle;
    int bytes_num;

    bytes_num = fa_getbits_num(&f->bitbuf)/8;

    memcpy(buf_out, f->data, bytes_num);

    return bytes_num;
}
