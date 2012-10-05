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


  filename: fa_mdct_fixed.h 
  version : v1.0.0
  time    : 2012/07/19 - 2012/07/20  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#ifndef _FA_MDCT_FIXED_H
#define _FA_MDCT_FIXED_H


typedef unsigned uintptr_t;

/*
    origin: the naive mdct using the original formular, this mdct is leading to you learning mdct
    fft   : normally called fast mdct, use fft transform to compute mdct
    fft4  : the wildly used, N/4 point FFT to compute mdct
*/
enum{
    MDCT_FIXED_ORIGIN = 0,
    MDCT_FIXED_FFT,
    MDCT_FIXED_FFT4,
};

uintptr_t fa_mdct_fixed_init(int type, int len);
void      fa_mdct_fixed_uninit(uintptr_t handle);

void fa_mdct_fixed(uintptr_t handle, int *x, int *X);
void fa_imdct_fixed(uintptr_t handle, int *X, int *x);

#endif
