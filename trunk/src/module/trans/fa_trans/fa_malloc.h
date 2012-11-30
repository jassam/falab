#ifndef _FA_MALLOC_H
#define _FA_MALLOC_H 

#include <stdio.h>
#include <stdlib.h>

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



#endif
