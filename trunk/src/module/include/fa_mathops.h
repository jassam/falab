#ifndef _FA_MATHOPS_H
#define _FA_MATHOPS_H

#include <math.h>
#include "fa_inttypes.h"

static inline float fa_truncf(float x)
{
    return (x > 0) ? floor(x) : ceil(x);
}

static inline float fa_roundf(float x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

static inline int fa_rintf(float x)
{
    float xx;

    xx = fa_roundf(x);

    if(xx > INT32_MAX)      return INT32_MAX;
    else if(xx < INT32_MIN) return INT32_MIN;
    else                    return (int)xx;
}

static inline int fa_clip_int(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

#define FA_SCALE_FLOAT(a, bits)     fa_rint((a) * (float)(1 << (bits)))
#define FA_FIX15(a)                 fa_clip_int(FA_SCALE_FLOAT(a, 15), -32767, 32767)
#define FA_FIXMUL_32X15(a,b)        ((int)(((int64_t)(a) *(int64_t)(b))>>(15)))








#endif
