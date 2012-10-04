/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙智 (Chengdu, China)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


  filename: fa_fft_fixed.c 
  version : v1.0.0
  time    : 2012/07/15 14:14 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  WARN    : for the fix precision reason, 
            the input data(int *data) CAN NOT LARGER THAN 2^25
            (I test, maybe not exactly right)

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fa_fft_fixed.h"

#ifndef M_PI
#define  M_PI		3.14159265358979323846
#endif

typedef struct  _fa_fft_fixed_ctx_t {
    int base;
    int length;
    int *bit_rvs;
    int *fft_work;
    short *cos_ang;
    short *sin_ang;
}fa_fft_fixed_ctx_t;


static int bit_reverse_fixed(int *buf_rvs,int size)
{
    int r = 0;              // counter
    int s = 0;              // bit-reversal of r/2
    int N2 = size << 1;     // N<<1 == N*2
    int i = 0;

    do {
        buf_rvs[i++] = s; 
        r += 2;
        s ^= size - (size / (r&-r));
    }
    while (r < N2);

    return s;

}

/*
    decimation-in-freq radix-2 in-place butterfly
    data:   (array of int , order below)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 !!

    useage:
    intput in normal order
    output in bit-reversed order
*/
static void dif_butterfly_fixed(int *data, long size, short *cos_ang, short *sin_ang)
{

    long  angle, astep, dl;
    int   xr, yr, xi, yi, dr, di;
    short wr, wi;
    int   *l1, *l2, *end, *ol2;


    astep = 1;
    end   = data + size + size;
    for (dl = size; dl > 1; dl >>= 1, astep += astep) {
        l1 = data;
        l2 = data + dl;
        for (; l2 < end; l1 = l2, l2 = l2 + dl) {
            ol2 = l2;
            for (angle = 0; l1 < ol2; l1 += 2, l2 += 2) {
                wr = cos_ang[angle];
                wi = -sin_ang[angle];

                xr = *l1     + *l2;
                xi = *(l1+1) + *(l2+1);
                dr = *l1     - *l2;
                di = *(l1+1) - *(l2+1);

                yr = FA_FIXMUL_32X15(dr, wr) - FA_FIXMUL_32X15(di, wi);
                yi = FA_FIXMUL_32X15(dr, wi) + FA_FIXMUL_32X15(di, wr);

                *(l1) = xr; *(l1 + 1) = xi;
                *(l2) = yr; *(l2 + 1) = yi;
                angle += astep;
            }
        }
    }
}


/*
    decimation-in-time radix-2 in-place inverse butterfly
    data:   (array of float, below order)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 

    useage:
    intput in bit-reversed order
    output in normal order
*/
static void inverse_dit_butterfly_fixed(int *data, long size, short *cos_ang, short *sin_ang)
{

    long  angle, astep, dl;
    int   xr, yr, xi, yi, dr, di;
    short wr, wi;
    int   *l1, *l2, *end, *ol2;

    astep = size >> 1;
    end   = data + size + size;
    for (dl = 2; astep > 0; dl += dl, astep >>= 1) {
        l1 = data;
        l2 = data + dl;
        for (;l2 < end; l1 = l2, l2 = l2 + dl) {
            ol2 = l2;
            for (angle = 0; l1 < ol2; l1 += 2, l2 += 2) {
                wr = cos_ang[angle];
                wi = sin_ang[angle];
                xr = *l1;
                xi = *(l1 + 1);
                yr = *l2;
                yi = *(l2 + 1);

                dr = FA_FIXMUL_32X15(yr, wr) - FA_FIXMUL_32X15(yi, wi);
                di = FA_FIXMUL_32X15(yr, wi) + FA_FIXMUL_32X15(yi, wr);

                *(l1) = xr + dr; *(l1 + 1) = xi + di;
                *(l2) = xr - dr; *(l2 + 1) = xi - di;
                angle += astep;
            }
        }
    }
}



/*
    in-place Radix-2 FFT for complex values
    data:   (array of int, below order)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 !!

    output is in similar order, normalized by array length
*/
void fa_fft_fixed(uintptr_t handle, int *data)
{
    int i,bit;
    fa_fft_fixed_ctx_t *f = (fa_fft_fixed_ctx_t *)handle;
    int *temp      = f->fft_work;
    int size       = f->length;
    int *bit_rvs   = f->bit_rvs;

    short *cos_ang = f->cos_ang;
    short *sin_ang = f->sin_ang;


    dif_butterfly_fixed(data,size,cos_ang,sin_ang);

    for (i = 0 ; i < size ; i++) {
        bit = bit_rvs[i];
        temp[i+i] = data[bit+bit]; temp[i+i+1] = data[bit+bit+1];
    }

    for (i = 0; i < size ; i++) {
        data[i+i] = temp[i+i];
        data[i+i+1] = temp[i+i+1];
    }

}


/*
    in-place Radix-2 inverse FFT for complex values
    data:   (array of int, below order)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 !!

    output is in similar order, NOT normalized by array length
*/
void fa_ifft_fixed(uintptr_t handle, int * data)
{
    int i,bit;
    fa_fft_fixed_ctx_t *f = (fa_fft_fixed_ctx_t *)handle;
    int *temp      = f->fft_work;
    int size       = f->length;
    int *bit_rvs   = f->bit_rvs;

    short *cos_ang = f->cos_ang;
    short *sin_ang = f->sin_ang;

    int base = f->base;

    for (i = 0 ; i < size ; i++) {
        bit = bit_rvs[i];
        temp[i+i] = data[bit+bit]; temp[i+i+1] = data[bit+bit+1];
    }

    for (i = 0; i < size ; i++) {
        data[i+i] = temp[i+i];
        data[i+i+1] = temp[i+i+1];
    }

    inverse_dit_butterfly_fixed(data,size,cos_ang,sin_ang);

    for (i = 0; i < size ; i++) {
        data[i+i] = data[i+i] >> base;
        data[i+i+1] = data[i+i+1] >> base;
    }


}



uintptr_t fa_fft_fixed_init(int size)
{
    int i;
    float ang;
    fa_fft_fixed_ctx_t *f = NULL;

    f = (fa_fft_fixed_ctx_t *)malloc(sizeof(fa_fft_fixed_ctx_t));

    f->length  = size;
    f->base    = (int)(log(size)/log(2));

    if ((1<<f->base) < size)
        f->base += 1;

    f->bit_rvs  = (int *)malloc(sizeof(int)*size);
    f->fft_work = (int *)malloc(sizeof(int)*size*2);
    f->cos_ang  = (short *)malloc(sizeof(short)*size);
    f->sin_ang  = (short *)malloc(sizeof(short)*size);

    bit_reverse_fixed(f->bit_rvs,size);

    for (i = 0 ; i < size ; i++) {
        ang = (2*M_PI*i)/size;
        f->cos_ang[i] = FA_FIX15(cos(ang));
        f->sin_ang[i] = FA_FIX15(sin(ang));
    }

    return (uintptr_t)f;
}

void fa_fft_fixed_uninit(uintptr_t handle)
{
    fa_fft_fixed_ctx_t *f = (fa_fft_fixed_ctx_t *)handle;

    free(f->bit_rvs);
    free(f->fft_work);
    free(f->cos_ang);
    free(f->sin_ang);

    f->bit_rvs = NULL;
    f->fft_work = NULL;
    f->cos_ang = NULL;
    f->sin_ang = NULL;

    free(f);
    f = NULL;
}



