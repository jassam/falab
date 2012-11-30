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

