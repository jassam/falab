#ifndef _FA_AACPSY_H
#define _FA_AACPSY_H 

#include "fa_swbtab.h"
#include "fa_psychomodel2.h"

typedef unsigned uintptr_t;

uintptr_t fa_aacpsy_init(int sample_rate);

/*short to long*/
void update_psy_long_previnfo(uintptr_t handle);

/*long to short*/
void update_psy_short_previnfo(uintptr_t handle);

//the x must be 16bits sample quantize
/*
void fa_aacpsy_calculate_pe_long(uintptr_t handle, float *x, float *pe_long);
void fa_aacpsy_calculate_pe_short(uintptr_t handle, float *x, float *pe_short);
void fa_aacpsy_calculate_xmin_long(uintptr_t handle, float *mdct_line, float *xmin_long);
void fa_aacpsy_calculate_xmin_short(uintptr_t handle, float *mdct_line, float xmin_short[8][]);
*/
void fa_aacpsy_calculate_pe(uintptr_t handle, float *x, int block_type, float *pe_block);
//void fa_aacpsy_calculate_xmin(uintptr_t handle, float *mdct_line, int block_type, float *xmin);
void fa_aacpsy_calculate_xmin(uintptr_t handle, float *mdct_line, int block_type, float xmin[8][FA_SWB_NUM_MAX]);
#endif
