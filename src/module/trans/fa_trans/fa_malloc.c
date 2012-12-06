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


  filename: fa_malloc.c 
  version : v1.0.0
  time    : 2012/12/5  
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/

#include "fa_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#ifdef WIN32
#include <string.h>
#endif
#ifdef __GNUC__
#include <strings.h>
#endif

void *fa_malloc(int size)
{
    void *ptr=NULL;
    ptr = malloc(size);
	if(ptr)
		memset(ptr, 0, size);
    return ptr;
}

void *fa_calloc(int num, int size)
{
	void *ptr=NULL;
	ptr = calloc(num,size);
	return ptr;
}

void fa_free(void *ptr)
{
    if (ptr) {
        free(ptr);
		ptr = NULL;
	}
}



char *fa_strdup(const char *s)
{
    char *ptr= NULL;
    if(s) {
        int len = strlen(s) + 1;
        ptr = fa_malloc(len);
        if (ptr)
            memcpy(ptr, s, len);
    }
    return ptr;
}

unsigned int fa_strlcpy(char *dst, const char *src, unsigned int size)
{
    unsigned int len = 0;
    while (++len < size && *src)
        *dst++ = *src++;
    if (len <= size)
        *dst = 0;
    return len + strlen(src) - 1;
}

