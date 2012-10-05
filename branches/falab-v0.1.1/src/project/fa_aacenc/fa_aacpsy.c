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

typedef struct _fa_aacpsy_t {
    int   sample_rate;

    uintptr_t h_psy2_long;
    uintptr_t h_psy2_short;

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
    fa_aacpsy_t *f = (fa_aacpsy_t *)malloc(sizeof(fa_aacpsy_t));

    f->sample_rate = sample_rate;
    /*f->bit_alloc   = 0;*/
    /*f->block_type  = LONG_CODING_BLOCK;*/

    switch (sample_rate) {
        case 32000:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_32k_LONG_NUM,fa_psy_32k_long_wlow,fa_psy_32k_long_barkval,fa_psy_32k_long_qsthr,
                                                   FA_SWB_32k_LONG_NUM,fa_swb_32k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            f->h_psy2_short = fa_psychomodel2_init(FA_PSY_32k_SHORT_NUM,fa_psy_32k_short_wlow,fa_psy_32k_short_barkval,fa_psy_32k_short_qsthr,
                                                   FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset,
                                                   AAC_BLOCK_SHORT_LEN); 
            break;
        case 44100:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_44k_LONG_NUM,fa_psy_44k_long_wlow,fa_psy_44k_long_barkval,fa_psy_44k_long_qsthr,
                                                   FA_SWB_44k_LONG_NUM,fa_swb_44k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            f->h_psy2_short = fa_psychomodel2_init(FA_PSY_44k_SHORT_NUM,fa_psy_44k_short_wlow,fa_psy_44k_short_barkval,fa_psy_44k_short_qsthr,
                                                   FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset,
                                                   AAC_BLOCK_SHORT_LEN); 
            break;
        case 48000:
            f->h_psy2_long  = fa_psychomodel2_init(FA_PSY_48k_LONG_NUM,fa_psy_48k_long_wlow,fa_psy_48k_long_barkval,fa_psy_48k_long_qsthr,
                                                   FA_SWB_48k_LONG_NUM,fa_swb_48k_long_offset,
                                                   AAC_BLOCK_LONG_LEN); 
            f->h_psy2_short = fa_psychomodel2_init(FA_PSY_48k_SHORT_NUM,fa_psy_48k_short_wlow,fa_psy_48k_short_barkval,fa_psy_48k_short_qsthr,
                                                   FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset,
                                                   AAC_BLOCK_SHORT_LEN); 
            break;
        default:
            return NULL;
    }

    return (uintptr_t)f;
}

/* short block to long block
 * NOTE: I simulate using matlab, and found that the psd of short block less than long block 
 *       about 36dB after fft, because that the length of the input samples is differernt, and 
 *       the longer length will lead to big magnitude(cumulation effect), then raise up the psd level, in my code,
 *       I will use some normalizier to adjust this difference, below is the detail
 *
 *       short block to long block: 36dB psd up, for consideration of fix point in the future, I use 64(2^6) to be
 *                                  the mutiplier of the magnitude of short block
 *       warn: the input samples must be the 16 bits, 1bits is the lsb; 
 *             if you normalize the input samples to the [0,1] scope, the 90.012dB psd up will 
 *             equal to the psd of 16bits after fft
 *
 * Above description is NO THEORY SUPPORTED, it is just simulateed by me(llz) using matlab, I will
 * not responsible for any results of your implemetion, and do not ask me why! you can simulate by
 * yourself, I use this result.
 *                                  
*/
void update_psy_long_previnfo(uintptr_t handle)
{
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;
    int i, j;
    int len;

    /*update mag_prev1*/
    fa_psychomodel2_get_mag_prev1(f->h_psy2_short, f->mag_prev1_short, &len);
    for (i = 0; i < len; i++) 
        for (j = 0; j < 8; j++) 
            f->mag_prev1_long[8*i+j] = 64*f->mag_prev1_short[i];
    fa_psychomodel2_set_mag_prev1(f->h_psy2_long , f->mag_prev1_long , 8*len);

    /*update mag_prev2*/
    fa_psychomodel2_get_mag_prev2(f->h_psy2_short, f->mag_prev2_short, &len);
    for (i = 0; i < len; i++) 
        for (j = 0; j < 8; j++) 
            f->mag_prev2_long[8*i+j] = 64*f->mag_prev2_short[i];
    fa_psychomodel2_set_mag_prev2(f->h_psy2_long , f->mag_prev2_long , 8*len);

    /*update mag_phi1, */
    fa_psychomodel2_get_phi_prev1(f->h_psy2_short, f->phi_prev1_short, &len);
    for (i = 0; i < len; i++) 
        for (j = 0; j < 8; j++) 
            f->phi_prev1_long[8*i+j] = f->phi_prev1_short[i];
    fa_psychomodel2_set_phi_prev1(f->h_psy2_long , f->phi_prev1_long , 8*len);

    /*update mag_phi2*/
    fa_psychomodel2_get_phi_prev2(f->h_psy2_short, f->phi_prev2_short, &len);
    for (i = 0; i < len; i++) 
        for (j = 0; j < 8; j++) 
            f->phi_prev2_long[8*i+j] = f->phi_prev2_short[i];
    fa_psychomodel2_set_phi_prev2(f->h_psy2_long , f->phi_prev2_long , 8*len);


    fa_psychomodel2_reset_nb_prev(f->h_psy2_long);
    fa_psychomodel2_reset_nb_prev(f->h_psy2_short);
}

/*long to short*/
void update_psy_short_previnfo(uintptr_t handle)
{
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;
    int i, j;
    int len;
    float tmp;

    /*update mag_prev1*/
    fa_psychomodel2_get_mag_prev1(f->h_psy2_long , f->mag_prev1_long, &len);
    for (i = 0; i < (len>>3); i++) {
        tmp = 0;
        for (j = 0; j < 8; j++) 
            tmp = tmp + f->mag_prev1_long[8*i+j];
        f->mag_prev1_short[i] = tmp/(64*8);
    }
    fa_psychomodel2_set_mag_prev1(f->h_psy2_short, f->mag_prev1_short, (len>>3));

    /*update mag_prev2*/
    fa_psychomodel2_get_mag_prev2(f->h_psy2_long , f->mag_prev2_long, &len);
    for (i = 0; i < (len>>3); i++) {
        tmp = 0;
        for (j = 0; j < 8; j++) 
            tmp = tmp + f->mag_prev2_long[8*i+j];
        f->mag_prev2_short[i] = tmp/(64*8);
    }
    fa_psychomodel2_set_mag_prev2(f->h_psy2_short, f->mag_prev2_short, (len>>3));

    /*update phi_prev1, use the phi of the lowest mag line to simulate*/
    fa_psychomodel2_get_phi_prev1(f->h_psy2_long , f->phi_prev1_long, &len);
    for (i = 0; i < (len>>3); i++) 
        f->phi_prev1_short[i] = f->phi_prev1_long[8*i];
    fa_psychomodel2_set_mag_prev1(f->h_psy2_short, f->mag_prev1_short, (len>>3));

    /*update mag_prev2*/
    fa_psychomodel2_get_mag_prev2(f->h_psy2_long , f->mag_prev2_long, &len);
    for (i = 0; i < (len>>3); i++) 
        f->phi_prev2_short[i] = f->phi_prev2_long[8*i];
    fa_psychomodel2_set_mag_prev2(f->h_psy2_short, f->mag_prev2_short, (len>>3));

    fa_psychomodel2_reset_nb_prev(f->h_psy2_short);
    fa_psychomodel2_reset_nb_prev(f->h_psy2_long);


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
            fa_psychomodel2_calculate_pe(f->h_psy2_short, xp, &pe);
            pe_sum += pe;
        }
    } else {
        fa_psychomodel2_calculate_pe(f->h_psy2_long , x, &pe);
        pe_sum = pe;
    }

    *pe_block = pe_sum;
}

void fa_aacpsy_calculate_xmin(uintptr_t handle, float *mdct_line, int block_type, float xmin[8][FA_SWB_NUM_MAX])
{
    int k;
    fa_aacpsy_t *f = (fa_aacpsy_t *)handle;

    if (block_type == ONLY_SHORT_BLOCK) {
        for (k = 0; k < 8; k++) 
            fa_psychomodel2_calculate_xmin(f->h_psy2_short, mdct_line+k*AAC_BLOCK_SHORT_LEN, &(xmin[k][0]));
    } else {
        fa_psychomodel2_calculate_xmin(f->h_psy2_long, mdct_line, &(xmin[0][0]));
    }
}

