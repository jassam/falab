#ifndef _FA_AACFILTERBANK_H
#define _FA_AACFILTERBANK_H

#define SINE_WINDOW         0
#define KBD_WINDOW          1

typedef unsigned uintptr_t;

uintptr_t fa_aacfilterbank_init(int win_block_switch_en);
void fa_aacfilterbank_uninit(uintptr_t handle);

int  fa_get_aacblocktype(uintptr_t handle);
void fa_set_aacblocktype(uintptr_t handle, int block_type);

void fa_aacblocktype_switch(uintptr_t h_fltbank, uintptr_t h_psy, float pe);
void fa_aacfilterbank_analysis(uintptr_t handle, float *x, float *mdct_line);
void fa_aacfilterbank_synthesis(uintptr_t handle, float *mdct_line, float *x);

#endif
