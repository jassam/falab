#ifndef _FA_MDCTQUANT_H
#define _FA_MDCTQUANT_H

#include "fa_swbtab.h"

typedef unsigned uintptr_t;

#define NUM_WINDOW_GROUPS_MAX 8
#define NUM_WINDOWS_MAX       8
#define NUM_SFB_MAX           FA_SWB_NUM_MAX

#define NUM_MDCT_LINE_MAX     1024

#define SF_OFFSET             100

uintptr_t fa_mdctquant_init(int mdct_line_num, int sfb_num, int *swb_low, int block_type_cof);
void      fa_mdctquant_uninit(uintptr_t handle);

float fa_mdctline_getmax(uintptr_t handle);
int fa_get_start_common_scalefac(float max_mdct_line);
void fa_mdctline_pow34(uintptr_t handle);
void fa_mdctline_scaled(uintptr_t handle,
                        int num_window_groups, int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX]);

void fa_mdctline_quant(uintptr_t handle, 
                       int common_scalefac, int *x_quant);
int fa_mdctline_get_sfbnum(uintptr_t handle);

int fa_mdctline_quantize(uintptr_t handle, 
                         int num_window_groups, int *window_group_length,
                         int average_bits, int more_bits, int bitres_bits, int maximum_bitreservoir_size, 
                         int *common_scalefac, int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                         int *x_quant, int *unused_bits);

int fa_mdctline_iquantize(uintptr_t handle, 
                          int num_window_groups, int *window_group_length,
                          int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                          int *x_quant);
void fa_xmin_sfb_arrange(uintptr_t handle, float xmin_swb[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX],
                         int num_window_groups, int *window_group_length);

void fa_mdctline_sfb_arrange(uintptr_t handle, float *mdct_line_swb, 
                             int num_window_groups, int *window_group_length);

void fa_mdctline_sfb_iarrange(uintptr_t handle, float *mdct_line_swb, int *mdct_line_sig,
                              int num_window_groups, int *window_group_length);
#endif
