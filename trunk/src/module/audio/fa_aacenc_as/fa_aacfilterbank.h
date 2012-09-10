#ifndef _FA_AACFILTERBANK_H
#define _FA_AACFILTERBANK_H

enum {
    ONLY_LONG_BLOCK  = 0,
    LONG_START_BLOCK = 1,   //long  to short
    ONLY_SHORT_BLOCK = 2,
    LONG_STOP_BLOCK  = 3,   //short to long
};

enum {
    LONG_CODING_BLOCK  = 0,
    SHORT_CODING_BLOCK = 1,
};

#define AAC_FRAME_LEN       1024
#define AAC_BLOCK_LONG_LEN  1024
#define AAC_BLOCK_SHORT_LEN 128
#define AAC_BLOCK_TRANS_LEN 448   // (1024-128)/2=448

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
