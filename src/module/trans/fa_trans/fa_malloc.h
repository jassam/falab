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


  filename: fa_malloc.h 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#ifndef _FA_MALLOC_H
#define _FA_MALLOC_H 

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef FA_ABS
#define FA_ABS(a)               ((a) >= 0 ? (a) : (-(a)))
#endif 

#ifndef FA_SIGN
#define FA_SIGN(a)              ((a) > 0 ? 1 : -1)
#endif

#ifndef FA_MAX
#define FA_MAX(a,b)             ((a) > (b) ? (a) : (b))
#endif

#ifndef FA_MAX3
#define FA_MAX3(a,b,c)          FFMAX(FFMAX(a,b),c)
#endif 

#ifndef FA_MIN
#define FA_MIN(a,b)             ((a) > (b) ? (b) : (a))
#endif

#ifndef FA_MIN3
#define FA_MIN3(a,b,c)          FFMIN(FFMIN(a,b),c)
#endif

#ifndef FA_SWAP
#define FA_SWAP(type,a,b)       do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#endif 

#ifndef FA_ARRAY_ELEMS
#define FA_ARRAY_ELEMS(a)       (sizeof(a) / sizeof((a)[0]))
#endif 

#ifndef FA_ALIGN
#define FA_ALIGN(x, a)          (((x)+(a)-1)&~((a)-1))
#endif



void    * fa_malloc(int size);

void      fa_free(void *ptr);

void    * fa_calloc(int num, int size);

char    * fa_strdup(const char *s);

unsigned int    fa_strlcpy(char *dst, const char *src, unsigned int size);


#ifdef __cplusplus
}
#endif




#endif
