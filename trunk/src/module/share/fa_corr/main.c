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
  time    : 2012/11/17 16:22 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_corr.h"
#include "fa_fft.h"
#include "fa_timeprofile.h"


#define P      14   //256 //14
#define LENGTH 256  //256 //1024 

#define TEST_TIME_CNT 50000

int main(int argc, char *argv[])
{
    int i;
    int p;

    float x[LENGTH];         
    float r[P+1];

    double x1[LENGTH];         
    double r1[P+1];

    float r2[P+1];
    uintptr_t h_acfast;


    p = P;

    for( i = 0; i < LENGTH; i++) {
        /*x[i]  = (float)(1000*sin(2*3.1415926*i/LENGTH));*/
        x[i]  = (float)(sin(2*3.1415926*i/LENGTH));
        x1[i] = (double)x[i];
    }


    fa_autocorr_hp(x1, LENGTH, p, r1);


    FA_CLOCK_START(1);
    for (i = 0; i < TEST_TIME_CNT; i++)
        fa_autocorr(x, LENGTH, p, r);
    FA_CLOCK_END(1);
    FA_CLOCK_COST(1);

    for(i = 0; i <= p; i++) 
        printf("r[%d]=%f, r1[%d]=%f\n", i, r[i], i, r1[i]);
    printf("\n");

    h_acfast = fa_autocorr_fast_init(LENGTH);


    FA_CLOCK_START(2);
    for (i = 0; i < TEST_TIME_CNT; i++)
        fa_autocorr_fast(h_acfast, x, LENGTH, p, r2);
    FA_CLOCK_END(2);
    FA_CLOCK_COST(2);

    for (i = 0; i <= p; i++) 
        printf("r_fast[%d]=%f, r1[%d]=%f\n", i, r2[i], i, r1[i]);

    FA_GET_TIME_COST(1);
    FA_GET_TIME_COST(2);

    return 0;
}
