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


  filename: main.c 
  version : v1.0.0
  time    : 2012/07/14 22:14 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_fft.h"
#include "fa_fft_fixed.h"

#define N 256 //32
//#define FA_FFT_FIXED

int main(int argc, char *argv[])
{
	int i;

#ifdef		FA_FFT_FIXED
	int buf_in[N];
	int fft_inbuf[2*N];
	uintptr_t handle;
#else
	float buf_in[N];
	float fft_inbuf[2*N];
	uintptr_t handle;
#endif

	for (i = 0 ; i < N ; i++)
		/*buf_in[i] = (1<<25)+i; //i*i;*/
		buf_in[i] = i; //i*i;

	printf("\n");

	for (i = 0 ; i < N ; i++){
		fft_inbuf[2*i] = buf_in[i];
		fft_inbuf[2*i+1] = 0;
	}

#ifdef		FA_FFT_FIXED
	handle = fa_fft_fixed_init(N);
	fa_fft_fixed(handle, fft_inbuf);
	fa_ifft_fixed(handle, fft_inbuf);
#else
	handle = fa_fft_init(N);
	fa_fft(handle, fft_inbuf);
	fa_ifft(handle, fft_inbuf);
#endif


	for (i = 0 ; i < N ; i++)
#ifdef		FA_FFT_FIXED
		printf("%d\t  %d\n",fft_inbuf[2*i],fft_inbuf[2*i+1]);
#else
		printf("%f\t  %f\n",fft_inbuf[2*i],fft_inbuf[2*i+1]);
#endif


#ifdef  FA_FFT_FIXED
	fa_fft_fixed_uninit(handle);
#else
	fa_fft_uninit(handle);
#endif

    return 0;
}
