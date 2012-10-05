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
#include "fa_mdct.h"
#include "fa_mdct_fixed.h"

#define N  256 //32
#define N2 (N >> 1)

int main(int argc, char *argv[])
{
	int i;
    int type = 0;

	float buf_in[N];
	float buf_out[N];
    float mdct_buf[N2];

	int   buf_in_int[N];
	int   buf_out_int[N];
    int   mdct_buf_int[N2];

	uintptr_t handle;
	uintptr_t handle_int;

	for (i = 0 ; i < N ; i++) {
		/*buf_in[i] = (1<<25)+i; //i*i;*/
		buf_in[i] = (1<<21)+i+1; //i*i;
		buf_in_int[i] = (1<<21)+i+1; //i*i;
    }

	printf("\n");

	handle     = fa_mdct_init(type, N);
	handle_int = fa_mdct_fixed_init(type, N);

    memset(mdct_buf, 0, sizeof(float)*N2);
    memset(mdct_buf_int, 0, sizeof(int)*N2);

	fa_mdct(handle, buf_in, mdct_buf);
	fa_mdct_fixed(handle_int, buf_in_int, mdct_buf_int);
    for(i = 0; i < N2; i++) 
        printf("mdct[%d] = %f\t mdct_int[%d] = %d\t err_mdct = %f%%\n", 
                i, mdct_buf[i], i, mdct_buf_int[i], 
                100*fabs(((float)(mdct_buf[i])-(float)(mdct_buf_int[i]))/mdct_buf[i]));

    memset(buf_out, 0, sizeof(float)*N);
    memset(buf_out_int, 0, sizeof(int)*N);
	fa_imdct(handle, mdct_buf, buf_out);
	fa_imdct_fixed(handle_int, mdct_buf_int, buf_out_int);

    for (i = 0 ; i < N ; i++)
        printf("float(%f\t%f)\t int(%d\t%d)\t err_imdct = %f%%\n",
                buf_in[i], buf_out[i], buf_in_int[i], buf_out_int[i], 
                100*fabs(((float)(buf_out[i])-(float)(buf_out_int[i]))/buf_out[i]));


	fa_mdct_uninit(handle);
	fa_mdct_fixed_uninit(handle_int);

    return 0;
}

