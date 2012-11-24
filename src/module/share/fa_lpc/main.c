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
#include "fa_lpc.h"


#define P      10
#define LENGTH 1000

int main(int argc, char *argv[])
{
    int i;
    uintptr_t h_lpc;

#ifdef USE_LPC_HP
    double x[LENGTH];         
    double lpc_cof[P+1];
    double kcof[P+1];
    double err;
    double gain;

    double xp[LENGTH];
    double err_real;
    double x_energy;
    double err_relative;
    int    k, p;

    for( i = 0; i < LENGTH; i++) {
        x[i]  = (double)1000*(sin(2*3.1415926*i/LENGTH))+100;
        /*x[i]  = (double)1000*(sin(2*3.1415926*i/LENGTH));*/
    }
#else
    float x[LENGTH];         
    float lpc_cof[P+1];
    float kcof[P+1];
    float err;
    float gain;

    float xp[LENGTH];
    float err_real;
    float x_energy;
    float err_relative;
    int   k, p;


    for( i = 0; i < LENGTH; i++) {
        x[i]  = (float)1000*(sin(2*3.1415926*i/LENGTH))+100;
        /*x[i]  = (float)(sin(2*3.1415926*i/LENGTH));*/
    }
#endif

    h_lpc = fa_lpc_init(P);

    gain = fa_lpc(h_lpc, x, LENGTH, lpc_cof, kcof, &err);

    for (i = 0; i <= P; i++) {
        printf("lpc_cof[%d] = %f, kcof[%d] = %f\n", i, lpc_cof[i], i, kcof[i]);
    }
    printf("\n");
    printf("err=%f, gain=%f\n", err, gain);


#ifdef USE_LPC_HP
    err_real = 0.0;
    for (i = 1; i < LENGTH; i++) {
        double errtmp;

        xp[i] = 0.0;
        p = (i < P) ? i : P; 
        for (k = 1; k <= p; k++) {
            xp[i] += -lpc_cof[k] * x[i-k]; 
        }
        errtmp = fabs(xp[i] - x[i]);
        err_real += errtmp * errtmp;
    }

    x_energy = 0.0;
    for (i = 0; i < LENGTH; i++)
        x_energy += x[i]*x[i];
    err_relative = err_real/x_energy;

    printf("err real = %f, x_energy=%f, err_relative = %f\n", err_real, x_energy, err_relative);

#else 
    err_real = 0.0;
    for (i = 1; i < LENGTH; i++) {
        float errtmp;

        xp[i] = 0.0;
        p = (i < P) ? i : P; 
        for (k = 1; k <= p; k++) {
            xp[i] += -lpc_cof[k] * x[i-k]; 
        }
        errtmp = fabs(xp[i] - x[i]);
        err_real += errtmp * errtmp;
    }

    x_energy = 0.0;
    for (i = 0; i < LENGTH; i++)
        x_energy += x[i]*x[i];
    err_relative = err_real/x_energy;

    printf("err real = %f, x_energy=%f, err_relative = %f\n", err_real, x_energy, err_relative);

#endif


    fa_lpc_uninit(h_lpc);

    return 0;
}
