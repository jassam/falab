#ifndef _FA_PSYCHOMODEL2_H
#define _FA_PSYCHOMODEL2_H 

typedef unsigned uintptr_t;

uintptr_t fa_psychomodel2_init(int cbands_num, int *w_low, float *barkval, float *qsthr, 
                               int swb_num   , int *swb_offset,
                               int iblen);
void fa_psychomodel2_uninit(uintptr_t handle);
void fa_psychomodel2_calculate(uintptr_t handle, short *x, float *pe);
 

void fa_psychomodel2_get_mag_prev1(uintptr_t handle, float *mag, int *len);
void fa_psychomodel2_get_mag_prev2(uintptr_t handle, float *mag, int *len);
void fa_psychomodel2_get_phi_prev1(uintptr_t handle, float *phi, int *len);
void fa_psychomodel2_get_phi_prev2(uintptr_t handle, float *phi, int *len);

void fa_psychomodel2_set_mag_prev1(uintptr_t handle, float *mag, int len);
void fa_psychomodel2_set_mag_prev2(uintptr_t handle, float *mag, int len);
void fa_psychomodel2_set_phi_prev1(uintptr_t handle, float *phi, int len);
void fa_psychomodel2_set_phi_prev2(uintptr_t handle, float *phi, int len);

void fa_psychomodel2_reset_nb_prev(uintptr_t handle);

#endif
