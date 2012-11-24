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
#include "fa_levinson.h"


/*#define USE_CORR_HP*/
#define P      14
#define LENGTH 200

int main(int argc, char *argv[])
{
    int i;
    int p;

    float x[LENGTH];         
    float r[P+1];
    float a[P+1], kcof[P+1];
    float err;

    double x1[LENGTH];         
    double r1[P+1];
    double a1[P+1], kcof1[P+1];
    double err1;

    float b[P];
    float a3[P], k3[P];
    float err3;

    double b1[P];
    double a4[P], k4[P];
    double err4;

    p = P;

    for( i = 0; i < LENGTH; i++) {
        /*x[i]  = (float)(1000*sin(2*3.1415926*i/LENGTH));*/
        x[i]  = (float)(sin(2*3.1415926*i/LENGTH));
        x1[i] = (double)x[i];
    }


    fa_autocorr_hp(x1, LENGTH, p, r1);

    fa_levinson_hp(r1, p, a1, kcof1, &err1);
    /*fa_levinson1_hp(r1, p, a1, kcof1, &err1);*/

#ifdef USE_CORR_HP
    for (i = 0; i <= p; i++)
        r[i] = (float)r1[i];
#else
    fa_autocorr(x, LENGTH, p, r);
#endif
    fa_levinson(r, p, a, kcof, &err);
    /*fa_levinson1(r, p, a, kcof, &err);*/

    for(i = 0; i <= p; i++) 
        printf("a[%d]=%f, a1[%d]=%f\n", i, a[i], i, a1[i]);
    printf("\n");
    for(i = 0; i <= p; i++) 
        printf("k[%d]=%f, k1[%d]=%f\n", i, kcof[i], i, kcof1[i]);
    printf("\n");
    printf("err=%f, err1=%f\n", err, err1);
    printf("\n");

     
    for (i = 0; i < p; i++)
        b1[i] = -r1[i+1];
    fa_atlvs_hp(r1, p, b1, a4, k4, &err4);
   
    for (i = 0; i < p; i++)
        b[i] = -r[i+1];
    fa_atlvs(r, p, b, a3, k3, &err3);


    for(i = 0; i < p; i++) 
        printf("a3[%d]=%f, a4[%d]=%f\n", i, a3[i], i, a4[i]);
    printf("\n");
    for(i = 0; i < p; i++) 
        printf("k3[%d]=%f, k4[%d]=%f\n", i, k3[i], i, k4[i]);
    printf("\n");
    printf("err3=%f, err4=%f\n", err3, err4);
    printf("\n");


    return 0;
}
