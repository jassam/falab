#ifndef _FA_AACPSY_H
#define _FA_AACPSY_H 

#include "fa_psychomodel2.h"

enum {
    AACPSY_LONG_BLOCK = 0,
    AACPSY_SHORT_BLOCK,
};

typedef unsigned uintptr_t;

uintptr_t fa_aacpsy_init(int sample_rate);

/*short to long*/
void update_psy_long_previnfo(uintptr_t handle);

/*long to short*/
void update_psy_short_previnfo(uintptr_t handle);

float fa_aacpsy_calculate_pe(uintptr_t handle, short *x, int block_type);

#endif
