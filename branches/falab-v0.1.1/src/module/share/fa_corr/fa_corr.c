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


  filename: fa_corr.c 
  version : v1.0.0
  time    : 2012/11/17 15:52
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "fa_corr.h"
/*
 *   p is the order
 *   WARN: p order , the r is the (p+1) demension
*/
void  fa_autocorr(float *x, int n, int p, float *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * x[j];
    }
}


/*
 * high presion 
 * WARN: use levinson method to resolve the relationship matrix will lead to different 
 *       result according to the input vector R, because the recursion of the levinson,
 *       so it is sensitive to the input vector, I simulate in matlab, the float *x and 
 *       doulbe *x will lead to little error R matrix, but totoally different visual 
 *       results resolved by levinson , so I add this function to vertify the results is 
 *       correct using this method
 */
void  fa_autocorr_hp(double *x, int n, int p, double *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * x[j];
    }
}


void  fa_crosscorr(float *x, float *y, int n, int p, float *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * y[j];
    }
}


/*high precison*/
void  fa_crosscorr_hp(double *x, double *y, int n, int p, double *r)
{
    int i, j, k;

    for (k = 0; k <= p; k++) {
        r[k] = 0.0;
        for (i = 0,j = k; j < n; i++,j++)  
            r[k] += x[i] * y[j];
    }
}


/*a and b are the frame which be caculate the correlation coffients*/
float fa_corr_cof(float *a, float *b, int len)
{
	int k;
	float ta,tb,tc;
	float rab;

	ta = tb = tc = 0;

	for (k = 0 ; k < len ; k++) {
		ta += a[k] * b[k];
		tb += a[k] * a[k];
		tc += b[k] * b[k];
	}

	rab = ta/sqrt(tb*tc);

	return rab;
}


static int nextpow2(int n)
{
    int level;
    int n1;

    level = log2(n);
    n1 = (1<<level);

    if (n1 < n)
        level += 1;

    return level;
}


typedef struct _fa_autocorr_fast_t {
    uintptr_t h_fft;

    int   fft_len;
	float *fft_buf1;
    float *fft_buf2;
} fa_autocorr_fast_t;

uintptr_t fa_autocorr_fast_init(int n)
{
    int level;

    fa_autocorr_fast_t *f = (fa_autocorr_fast_t *)malloc(sizeof(fa_autocorr_fast_t));

    level = nextpow2(2*n-1);
    /*printf("level = %d\n", level);*/

    f->fft_len  = (1<<level);
	f->h_fft    = fa_fft_init(f->fft_len);
    f->fft_buf1 = (float *)malloc(sizeof(float)*f->fft_len*2);
    f->fft_buf2 = (float *)malloc(sizeof(float)*f->fft_len*2);
    memset(f->fft_buf1, 0, sizeof(float)*f->fft_len*2);
    memset(f->fft_buf2, 0, sizeof(float)*f->fft_len*2);
 
    return (uintptr_t)f;
}

void      fa_autocorr_fast_uninit(uintptr_t handle)
{
    fa_autocorr_fast_t *f = (fa_autocorr_fast_t *)handle;

    if (f) {
        if (f->h_fft) 
            fa_fft_uninit(f->h_fft);

        if (f->fft_buf1) {
            free(f->fft_buf1);
            f->fft_buf1 = NULL;
        }

        if (f->fft_buf2) {
            free(f->fft_buf2);
            f->fft_buf2 = NULL;
        }

        free(f);
        f = NULL;
    }
}

void  fa_autocorr_fast(uintptr_t handle, float *x, int n, int p, float *r)
{
    fa_autocorr_fast_t *f = (fa_autocorr_fast_t *)handle;
    int fft_len = f->fft_len;
    int i;

    memset(f->fft_buf1, 0, sizeof(float)*f->fft_len*2);
    for (i = 0; i < n; i++) {
        f->fft_buf1[2*i]   = x[i];
        f->fft_buf1[2*i+1] = 0.0;
    }
    fa_fft(f->h_fft, f->fft_buf1);

    memset(f->fft_buf2, 0, sizeof(float)*f->fft_len*2);
    for (i = 0; i < n; i++) {
        f->fft_buf2[2*i]   = f->fft_buf1[2*i]   * f->fft_buf1[2*i]  + 
                             f->fft_buf1[2*i+1] * f->fft_buf1[2*i+1];
        f->fft_buf2[2*i+1] = 0.0;
    }
    fa_ifft(f->h_fft, f->fft_buf2);
    for (i = 0; i <= p; i++) {
        r[i] = f->fft_buf2[2*i] * 2;
        /*printf("r_fast[%d]=%f\n", i, r[i]);*/
    }
}

