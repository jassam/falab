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


  filename: fa_aacpsy.c 
  version : v1.0.0
  time    : 2012/08/22 - 2012/10/05 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_psytab.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#endif 

#ifndef FA_MAX
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#ifndef FA_ABS
#define FA_ABS(a) ( (a) > 0 ? (a) : (-(a)))
#endif


typedef struct _fa_aacpsy_t {
    int   sample_rate;

    uintptr_t h_psy1_long;
    uintptr_t h_psy1_short[8];

    uintptr_t h_psy2_long;
    uintptr_t h_psy2_short[8];

    float mag_prev1_long[AAC_BLOCK_LONG_LEN];
    float mag_prev2_long[AAC_BLOCK_LONG_LEN];
    float phi_prev1_long[AAC_BLOCK_LONG_LEN];
    float phi_prev2_long[AAC_BLOCK_LONG_LEN];

    float mag_prev1_short[AAC_BLOCK_SHORT_LEN];
    float mag_prev2_short[AAC_BLOCK_SHORT_LEN];
    float phi_prev1_short[AAC_BLOCK_SHORT_LEN];
    float phi_prev2_short[AAC_BLOCK_SHORT_LEN];

    /*int   bit_alloc;*/
    /*int   block_type;*/

}fa_aacpsy_t;


uintptr_t fa_aacpsy_init(int sample_rate)
{
    int i;
    fa_aacpsy_t *f = (fa_aacpsy_t *)malloc(sizeof(fa_aacpsy_t));

    memset(f, 0, sizeof(fa_aacpsy_t));

    f->sample_rate = sample_rate;
    /*f->bit_alloc   = 0;*/
    /*f->block_type  = LONG_CODING_BLOCK;*/

    f->h_psy1_long = fa_psychomodel1_init(sample_rate, 2048);
    for (i = 0; i < 8; i++)
        f->h_psy1_short[i] = fa_psychomodel1_init(sample_rate, 256);

    switch (sample_rate) {
        case 32000:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_32k_LONG_NUM,fa_psy_32k_long_wlow,fa_psy_32k_long_barkval,fa_psy_32k_long_qsthr,
                                                   FA_SWB_32k_LONG_NUM,fa_swb_32k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            for (i = 0; i < 8; i++)
            f->h_psy2_short[i] = fa_psychomodel2_init(FA_PSY_32k_SHORT_NUM,fa_psy_32k_short_wlow,fa_psy_32k_short_barkval,fa_psy_32k_short_qsthr,
                                                   FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset,
                                                   AAC_BLOCK_SHORT_LEN); 
            break;
        case 44100:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_44k_LONG_NUM,fa_psy_44k_long_wlow,fa_psy_44k_long_barkval,fa_psy_44k_long_qsthr,
                                                   FA_SWB_44k_LONG_NUM,fa_swb_44k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            for (i = 0; i < 8; i++)
                f->h_psy2_short[i] = fa_psychomodel2_init(FA_PSY_44k_SHORT_NUM,fa_psy_44k_short_wlow,fa_psy_44k_short_barkval,fa_psy_44k_short_qsthr,
                                                       FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset,
                                                       AAC_BLOCK_SHORT_LEN); 
            break;
        case 48000:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_48k_LONG_NUM,fa_psy_48k_long_wlow,fa_psy_48k_long_barkval,fa_psy_48k_long_qsthr,
                                                   FA_SWB_48k_LONG_NUM,fa_swb_48k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            for (i = 0; i < 8; i++)
            f->h_psy2_short[i] = fa_psychomodel2_init(FA_PSY_48k_SHORT_NUM,fa_psy_48k_short_wlow,fa_psy_48k_short_barkval,fa_psy_48k_short_qsthr,
                                                   FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset,
                                                   AAC_BLOCK_SHORT_LEN); 
            break;
        default:
            return (uintptr_t)NULL;
    }

    return (uintptr_t)f;
}

void reset_psy_previnfo(uintptr_t handle)
{
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;

    fa_psychomodel2_reset_mag_prev1(f->h_psy2_short);
    fa_psychomodel2_reset_mag_prev2(f->h_psy2_short);
    fa_psychomodel2_reset_phi_prev1(f->h_psy2_short);
    fa_psychomodel2_reset_phi_prev2(f->h_psy2_short);
    fa_psychomodel2_reset_nb_prev(f->h_psy2_short);

    fa_psychomodel2_reset_mag_prev1(f->h_psy2_long);
    fa_psychomodel2_reset_mag_prev2(f->h_psy2_long);
    fa_psychomodel2_reset_phi_prev1(f->h_psy2_long);
    fa_psychomodel2_reset_phi_prev2(f->h_psy2_long);
    fa_psychomodel2_reset_nb_prev(f->h_psy2_long);

}

void fa_aacpsy_calculate_pe(uintptr_t handle, float *x, int block_type, float *pe_block)
{
    int   win;
    float *xp;
    float pe;
    float pe_sum;
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;

    if (block_type == ONLY_SHORT_BLOCK) {
        pe_sum = 0;
        for (win = 0; win < 8; win++) {
            xp = x + AAC_BLOCK_TRANS_LEN + win*128;
            fa_psychomodel2_calculate_pe(f->h_psy2_short[win], xp, &pe);
            pe_sum += pe;
        }
    } else {
        fa_psychomodel2_calculate_pe(f->h_psy2_long , x, &pe);
        pe_sum = pe;
    }

    *pe_block = pe_sum;
}

void update_psy_short_previnfo(uintptr_t handle, int index)
{
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;
    int len;

    if (index == 0) {
        fa_psychomodel2_get_mag_prev1(f->h_psy2_short[7], f->mag_prev1_short, &len);
        fa_psychomodel2_set_mag_prev1(f->h_psy2_short[0], f->mag_prev1_short, len);
        fa_psychomodel2_get_phi_prev1(f->h_psy2_short[7], f->mag_prev1_short, &len);
        fa_psychomodel2_set_phi_prev1(f->h_psy2_short[0], f->mag_prev1_short, len);

        fa_psychomodel2_get_mag_prev2(f->h_psy2_short[7], f->mag_prev2_short, &len);
        fa_psychomodel2_set_mag_prev2(f->h_psy2_short[0], f->mag_prev2_short, len);
        fa_psychomodel2_get_phi_prev2(f->h_psy2_short[7], f->mag_prev2_short, &len);
        fa_psychomodel2_set_phi_prev2(f->h_psy2_short[0], f->mag_prev2_short, len);
    }

    if (index >= 1 && index <= 7) {
        fa_psychomodel2_get_mag_prev1(f->h_psy2_short[index-1], f->mag_prev2_short, &len);
        fa_psychomodel2_set_mag_prev1(f->h_psy2_short[index]  , f->mag_prev2_short, len);
        fa_psychomodel2_get_phi_prev1(f->h_psy2_short[index-1], f->mag_prev2_short, &len);
        fa_psychomodel2_set_phi_prev1(f->h_psy2_short[index]  , f->mag_prev2_short, len);

        fa_psychomodel2_get_mag_prev2(f->h_psy2_short[index-1], f->mag_prev2_short, &len);
        fa_psychomodel2_set_mag_prev2(f->h_psy2_short[index]  , f->mag_prev2_short, len);
        fa_psychomodel2_get_phi_prev2(f->h_psy2_short[index-1], f->mag_prev2_short, &len);
        fa_psychomodel2_set_phi_prev2(f->h_psy2_short[index]  , f->mag_prev2_short, len);
    }

}


void fa_aacpsy_calculate_xmin(uintptr_t handle, float *mdct_line, int block_type, float xmin[8][FA_SWB_NUM_MAX])
{
    int k;
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;

    if (block_type == ONLY_SHORT_BLOCK) {
#if 1 
        for (k = 0; k < 8; k++) {
            update_psy_short_previnfo(f, k);
            fa_psychomodel2_calculate_xmin(f->h_psy2_short[k], mdct_line+k*AAC_BLOCK_SHORT_LEN, &(xmin[k][0]));
        }
#else 
        for (k = 0; k < 8; k++) 
            fa_psychomodel2_calculate_xmin(f->h_psy2_short, mdct_line+AAC_BLOCK_TRANS_LEN+k*AAC_BLOCK_SHORT_LEN, &(xmin[k][0]));

#endif
    } else {
        fa_psychomodel2_calculate_xmin(f->h_psy2_long, mdct_line, &(xmin[0][0]));
    }
}

void fa_aacpsy_calculate_xmin_usepsych1(uintptr_t handle, float *mdct_line, int block_type, float xmin[8][FA_SWB_NUM_MAX])
{
    int k;
    int i,j;
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;
    float gthr[1024];
    int   swb_num    = FA_PSY_44k_LONG_NUM;
    int   *swb_offset= fa_swb_44k_long_offset;

    memset(gthr, 0, sizeof(float)*1024);
    if (block_type == ONLY_SHORT_BLOCK) {
        for (k = 0; k < 8; k++) {
            fa_psy_global_threshold_usemdct(f->h_psy1_short[k], mdct_line+k*AAC_BLOCK_SHORT_LEN, gthr+k*AAC_BLOCK_SHORT_LEN);
        }
    } else {
        fa_psy_global_threshold_usemdct(f->h_psy1_long, mdct_line, gthr);
    }

    for (i = 0; i < swb_num; i++) {
        xmin[0][i] = 10000000000;
        for (j = swb_offset[i]; j < swb_offset[i+1]; j++) {
            xmin[0][i] = FA_MIN(xmin[0][i], gthr[j]);
            /*printf("xmin[%d]=%f\n", j, xmin[0][i]);*/
        }
    }

}

