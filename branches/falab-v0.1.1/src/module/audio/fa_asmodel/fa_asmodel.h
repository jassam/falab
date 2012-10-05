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


  filename: fa_asmodel.h 
  version : v1.0.0
  time    : 2012/07/21 - 2012/07/23  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#ifndef _FA_ASMODEL_H
#define _FA_ASMODEL_H 

#include "fa_fir.h"
#include "fa_fft.h"
#include "fa_mdct.h"

#ifdef __cplusplus 
extern "C"
{ 
#endif  


enum {
    FA_OVERLAP_HIGH = 0,       //3/4 overlap
    FA_OVERLAP_LOW,            //1/2 overlap
};

typedef unsigned uintptr_t;

/*
    in fact, the fa_analysis_fft_init is same as fa_synthesis_fft_init 
    I use 2 functions in consider that maybe someone want use one only
*/
uintptr_t fa_analysis_fft_init(int overlap_hint, int frame_len, win_t win_type);
void      fa_analysis_fft_uninit(uintptr_t handle);
void      fa_analysis_fft(uintptr_t handle, float *x, float *re, float *im);

uintptr_t fa_synthesis_fft_init(int overlap_hint, int frame_len, win_t win_type);
void      fa_synthesis_fft_uninit(uintptr_t handle);
void      fa_synthesis_fft(uintptr_t handle, float *re, float *im, float *x);


uintptr_t fa_analysis_mdct_init(int frame_len, mdct_win_t win_type);
void      fa_analysis_mdct_uninit(uintptr_t handle);
void      fa_analysis_mdct(uintptr_t handle, float *x, float *X);

uintptr_t fa_synthesis_mdct_init(int frame_len, mdct_win_t win_type);
void      fa_synthesis_mdct_uninit(uintptr_t handle);
void      fa_synthesis_mdct(uintptr_t handle, float *X, float *x);

#ifdef __cplusplus 
}
#endif  


#endif
