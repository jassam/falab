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


  filename: fa_aacblockswitch.c 
  version : v1.0.0
  time    : 2012/10/27 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "fa_aacblockswitch.h"
#include "fa_aacfilterbank.h"
#include "fa_aacstream.h"
#include "fa_fir.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#endif 

#ifndef FA_MAX
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#ifndef FA_ABS 
#define FA_ABS(a)    ( (a) > 0 ? (a) : (-(a)) )
#endif


/*---------------------------------- psy blockswitch --------------------------------------------------*/
#define SWITCH_PE  400 //2500 //1000 //1800//1800 //300// 1800

static void blockswitch_pe(float pe, int prev_block_type, int *cur_block_type, uintptr_t h_aacpsy)
{
    int prev_coding_block_type;
    int cur_coding_block_type;

    /*get prev coding block type*/

    if (prev_block_type == ONLY_SHORT_BLOCK)
        prev_coding_block_type = SHORT_CODING_BLOCK;
    else 
        prev_coding_block_type = LONG_CODING_BLOCK;

    /*use pe to decide current coding block type*/
    if (pe > SWITCH_PE) 
        cur_coding_block_type = SHORT_CODING_BLOCK;
    else 
        cur_coding_block_type = LONG_CODING_BLOCK;
/*
    if (cur_coding_block_type != prev_coding_block_type)
        reset_psy_previnfo(h_aacpsy);
*/

    if (cur_coding_block_type == LONG_CODING_BLOCK && prev_coding_block_type == SHORT_CODING_BLOCK)
        update_psy_short2long_previnfo(h_aacpsy);
    if (cur_coding_block_type == SHORT_CODING_BLOCK && prev_coding_block_type == LONG_CODING_BLOCK)
        update_psy_long2short_previnfo(h_aacpsy);

    /*use prev coding block type and current coding block type to decide current block type*/
#if 1 
    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            *cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            *cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            *cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            *cur_block_type = LONG_STOP_BLOCK;
    }
#else 

    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            *cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            *cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            *cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK) // || prev_block_type == ONLY_SHORT_BLOCK)
            *cur_block_type = ONLY_SHORT_BLOCK; //LONG_STOP_BLOCK;
        if (prev_block_type == ONLY_SHORT_BLOCK)
            *cur_block_type = LONG_STOP_BLOCK;
    }

#endif

}


/*this function used in aac encode*/
static int aac_blockswitch_psy(int block_type, float pe, uintptr_t h_aacpsy)
{
    int prev_block_type;
    int cur_block_type;

    prev_block_type = block_type;
    blockswitch_pe(pe, prev_block_type, &cur_block_type, h_aacpsy);

    return cur_block_type;
}

int fa_blockswitch_psy(aacenc_ctx_t *s)
{
    if (s->psy_enable) {
        s->block_type = aac_blockswitch_psy(s->block_type, s->pe, s->h_aacpsy);
        s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
        s->bits_more  = s->bits_alloc + 10;//100;
    } else {
        s->block_type = ONLY_LONG_BLOCK;
        s->bits_alloc = s->bits_average;
        s->bits_more  = s->bits_alloc;// - 10;//100;
    }

    return s->block_type;
}


/*---------------------------------- time var fast blockswitch --------------------------------------------------*/
static float frame_var_max(float *x, int len)
{
    int   k, i;
    int   hop;
    float *xp;

    float sum, avg;
    float diff;
    float var;
    float var_max;
    int level;
    int bands;

    level = 1;//4;
    bands = (1<<level);

    hop = len >> level;
    var_max = 0.;

    for (k = 0; k < bands; k++) {
        xp  =  x + k*hop;
        sum =  0.;
        avg =  0.;
        var =  0.;
#if 1 
        for (i = 0; i < hop; i++) {
            float tmp;
            tmp = fabs(xp[i]/32768.);
            sum += tmp;
        }
        avg = sum / hop;

        for (i = 0; i < hop; i++) {
            float tmp;
            tmp  = fabs(xp[i]/32768.);
            diff =  tmp - avg;
            var  += diff  * diff; 
        }
#else 
        for (i = 0; i < hop; i++) {
            float tmp;
            /*tmp = fabsf(xp[i]/32768.);*/
            tmp = FA_ABS(xp[i]);
            sum += tmp;
        }
        avg = sum / hop;

        for (i = 0; i < hop; i++) {
            float tmp;
            /*tmp = fabsf(xp[i]/32768.);*/
            tmp = FA_ABS(xp[i]);
            diff =  tmp - avg;
            var  += diff  * diff; 
        }

#endif

        var /= hop;

        var_max = FA_MAX(var_max, var);
    }

    return var_max;
}


#if 1 

#define SWITCH_E_BASE  1 //(32768*32768)
#define SWITCH_E   (0.03*SWITCH_E_BASE)
#define SWITCH_E1  (0.075*SWITCH_E_BASE)
#define SWITCH_E2  (0.003*SWITCH_E_BASE)

int fa_blockswitch_var(aacenc_ctx_t *s)
{
    float x[2*AAC_FRAME_LEN];
    float cur_var_max;
    float var_diff;

    int   prev_block_type;
    int   prev_coding_block_type;
    int   cur_coding_block_type;

    int   cur_block_type;

    fa_aacfilterbank_get_xbuf(s->h_aac_analysis, x);
    cur_var_max = frame_var_max(x, 2*AAC_FRAME_LEN);
    var_diff    = FA_ABS(cur_var_max - s->var_max_prev);
    /*var_diff    = fabsf(cur_var_max - s->var_max_prev);*/
    s->var_max_prev = cur_var_max;

    prev_block_type = s->block_type;
    /*get prev coding block type*/
    if (prev_block_type == ONLY_SHORT_BLOCK)
        prev_coding_block_type = SHORT_CODING_BLOCK;
    else 
        prev_coding_block_type = LONG_CODING_BLOCK;

    if (cur_var_max < SWITCH_E1)
        cur_coding_block_type = LONG_CODING_BLOCK;
    else {
        if (prev_coding_block_type == LONG_CODING_BLOCK) {
            if (var_diff > SWITCH_E)
                cur_coding_block_type = SHORT_CODING_BLOCK;
            else 
                cur_coding_block_type = LONG_CODING_BLOCK;
        } else {
            if (var_diff > SWITCH_E2)
                cur_coding_block_type = SHORT_CODING_BLOCK;
            else 
                cur_coding_block_type = LONG_CODING_BLOCK;
        }
    }
 
    /*use prev coding block type and current coding block type to decide current block type*/
#if 0 
    /*test switch */
    if (prev_block_type == ONLY_LONG_BLOCK)
        cur_block_type = LONG_START_BLOCK;
    if (prev_block_type == LONG_START_BLOCK)
        cur_block_type = ONLY_SHORT_BLOCK;
    if (prev_block_type == ONLY_SHORT_BLOCK)
        cur_block_type = LONG_STOP_BLOCK;
    if (prev_block_type == LONG_STOP_BLOCK)
        cur_block_type = ONLY_LONG_BLOCK;
#else
    #if 0
    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }
    #else 

    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK) // || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK; //LONG_STOP_BLOCK;
        if (prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }


    #endif
#endif

    s->block_type = cur_block_type;

    if (s->psy_enable) {
        s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
        /*s->bits_alloc = s->bits_average;*/
        s->bits_more  = s->bits_alloc+10;// - 10;//100;
    } else {
        s->bits_alloc = s->bits_average;
        s->bits_more  = s->bits_alloc;// - 10;//100;
    }

    return cur_block_type;
}

#else 

#define SWITCH_E   (0.1)
#define SWITCH_E1  (0.5)
#define SWITCH_E2  (0.2)

int fa_blockswitch_var(aacenc_ctx_t *s)
{
    float x[2*AAC_FRAME_LEN];
    float cur_var_max;
    float var_diff;
    float var_diff_relative = 0.0;

    int   prev_block_type;
    int   prev_coding_block_type;
    int   cur_coding_block_type;

    int   cur_block_type;

    fa_aacfilterbank_get_xbuf(s->h_aac_analysis, x);
    cur_var_max = frame_var_max(x, 2*AAC_FRAME_LEN);
    var_diff    = FA_ABS(cur_var_max - s->var_max_prev);
    var_diff_relative = var_diff / s->var_max_prev;
    /*printf("var_diff_relative = %f\n", var_diff_relative);*/

    /*var_diff    = fabsf(cur_var_max - s->var_max_prev);*/
    s->var_max_prev = cur_var_max;

    prev_block_type = s->block_type;
    /*get prev coding block type*/
    if (prev_block_type == ONLY_SHORT_BLOCK)
        prev_coding_block_type = SHORT_CODING_BLOCK;
    else 
        prev_coding_block_type = LONG_CODING_BLOCK;

    if (var_diff_relative < SWITCH_E1)
        cur_coding_block_type = LONG_CODING_BLOCK;
    else {
#if 0
        if (prev_coding_block_type == LONG_CODING_BLOCK) {
            if (var_diff_relative > SWITCH_E)
                cur_coding_block_type = SHORT_CODING_BLOCK;
            else 
                cur_coding_block_type = LONG_CODING_BLOCK;
        } else {
            if (var_diff_relative > SWITCH_E2)
                cur_coding_block_type = SHORT_CODING_BLOCK;
            else 
                cur_coding_block_type = LONG_CODING_BLOCK;
        }
#else 
        cur_coding_block_type = SHORT_CODING_BLOCK;
#endif
    }
 
    /*use prev coding block type and current coding block type to decide current block type*/
#if 0 
    /*test switch */
    if (prev_block_type == ONLY_LONG_BLOCK)
        cur_block_type = LONG_START_BLOCK;
    if (prev_block_type == LONG_START_BLOCK)
        cur_block_type = ONLY_SHORT_BLOCK;
    if (prev_block_type == ONLY_SHORT_BLOCK)
        cur_block_type = LONG_STOP_BLOCK;
    if (prev_block_type == LONG_STOP_BLOCK)
        cur_block_type = ONLY_LONG_BLOCK;
#else
    #if 0

    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }

    #else 

    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK) // || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK; //LONG_STOP_BLOCK;
        if (prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }


    #endif
#endif

    s->block_type = cur_block_type;

    if (s->psy_enable) {
        s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
        /*s->bits_alloc = s->bits_average;*/
        s->bits_more  = s->bits_alloc;// - 10;//100;
    } else {
        s->bits_alloc = s->bits_average;
        s->bits_more  = s->bits_alloc;// - 10;//100;
    }

    return cur_block_type;
}



#endif

#define WINCNT  4 //8

typedef struct _fa_blockctrl_t {
    uintptr_t  h_flt_fir;
    uintptr_t  h_flt_fir_hp;

    int block_len;
    float *x;
    float *x_flt;
    float *x_flt_hp;

    // 1 means attack, 0 means no attack
    int attack_flag;           
    int lastattack_flag;
    int attack_index;
    int lastattack_index;

    float max_win_enrg;         // max energy for attack when detected, and used in sync short block group info

    float win_enrg[2][WINCNT];
    float win_hfenrg[2][WINCNT];
    float win_accenrg;

    float diff_enrg_ratio;
    int   low_hf_flag;
    float diff_enrg;
    float diff_accenrg;

} fa_blockctrl_t;

uintptr_t fa_blockswitch_init(int block_len)
{
    fa_blockctrl_t *f = (fa_blockctrl_t *)malloc(sizeof(fa_blockctrl_t));
    memset(f, 0, sizeof(fa_blockctrl_t));

    f->h_flt_fir    = fa_fir_filter_hpf_init(block_len, 7, 0.4, KAISER);
    f->h_flt_fir_hp = fa_fir_filter_hpf_init(block_len, 31, 0.6, KAISER);
    f->block_len    = block_len;

    f->x = (float *)malloc(block_len * sizeof(float));
    memset(f->x, 0, block_len * sizeof(float));
    f->x_flt = (float *)malloc(block_len * sizeof(float));
    memset(f->x_flt, 0, block_len * sizeof(float));
    f->x_flt_hp = (float *)malloc(block_len * sizeof(float));
    memset(f->x_flt_hp, 0, block_len * sizeof(float));

    f->diff_enrg_ratio = 0.0;
    f->diff_enrg = 0.0;
    f->diff_accenrg = 0.0;
    f->low_hf_flag = 0;

    return (uintptr_t)f;
}

static void calculate_win_enrg(fa_blockctrl_t *f)
{
    int win;
    int i;
    int win_len;

    float win_enrg_tmp;
    float win_hfenrg_tmp;
    float x_tmp, x_flt_tmp;

    float enrg_tmp;
    float hfenrg_hp_tmp;
    float x_flt_hp_tmp;

    win_len = f->block_len / WINCNT;
    f->diff_enrg_ratio = 0.0;

    fa_fir_filter(f->h_flt_fir, f->x, f->x_flt, f->block_len);
    fa_fir_filter(f->h_flt_fir_hp, f->x, f->x_flt_hp, f->block_len);

    enrg_tmp    = 0.0;
    hfenrg_hp_tmp = 0.0;
    for (win = 0; win < WINCNT; win++) {
        win_enrg_tmp   = 0.0;
        win_hfenrg_tmp = 0.0;

        for (i = 0; i < win_len; i++) {
            x_tmp     = f->x    [win*win_len + i];
            x_flt_tmp = f->x_flt[win*win_len + i];
            x_flt_hp_tmp = f->x_flt_hp[win*win_len + i];

            win_enrg_tmp   += x_tmp     * x_tmp;
            win_hfenrg_tmp += x_flt_tmp * x_flt_tmp;
            hfenrg_hp_tmp  += x_flt_hp_tmp * x_flt_hp_tmp;

        }

#if 1 
        f->win_enrg[1][win]   = win_enrg_tmp;
        f->win_hfenrg[1][win] = win_hfenrg_tmp;
#else 
        f->win_enrg[1][win]   = log10(win_enrg_tmp);
        f->win_hfenrg[1][win] = log10(win_hfenrg_tmp);
#endif

        enrg_tmp      += win_enrg_tmp;
    }

    hfenrg_hp_tmp += 1.1;
    enrg_tmp      += 1.1;

    /*printf("hf enrg=%f, enrg=%f\n", hfenrg_hp_tmp, enrg_tmp);*/
    if (hfenrg_hp_tmp < 200000)
    /*if (hfenrg_hp_tmp < 10000)*/
        f->low_hf_flag = 1;
    
    /*f->diff_enrg_ratio = log10(hfenrg_hp_tmp)/log10(enrg_tmp);*/
    f->diff_enrg_ratio = log10(hfenrg_hp_tmp)/log10(enrg_tmp);
    /*f->diff_enrg_ratio = hfenrg_hp_tmp/enrg_tmp;*/

}

                            //lastattack-attack-blocktype
static const int win_sequence[2][2][4] =
{
   /*  ONLY_LONG_BLOCK    LONG_START_BLOCK   ONLY_SHORT_BLOCK   LONG_STOP_BLOCK   */  
   /*last no attack*/
   { 
       /*no attack*/
       {ONLY_LONG_BLOCK,   ONLY_SHORT_BLOCK,  LONG_STOP_BLOCK,   ONLY_LONG_BLOCK,  },   
       /*attack*/
       {LONG_START_BLOCK,  ONLY_SHORT_BLOCK,  ONLY_SHORT_BLOCK,  LONG_START_BLOCK, } 
   }, 
   /*last attack*/
   { 
       /*no attack*/
       {ONLY_LONG_BLOCK,   ONLY_SHORT_BLOCK,  ONLY_SHORT_BLOCK,  ONLY_LONG_BLOCK,  },  
       /*attack*/
       {LONG_START_BLOCK,  ONLY_SHORT_BLOCK,  ONLY_SHORT_BLOCK,  LONG_START_BLOCK, } 
   }  
};


static int select_block(int prev_block_type, int attack_flag)
{
    int prev_coding_block_type;
    int cur_coding_block_type;
    int cur_block_type;

    if (attack_flag)
        cur_coding_block_type = SHORT_CODING_BLOCK;
    else 
        cur_coding_block_type = LONG_CODING_BLOCK;

    /*get prev coding block type*/
    if (prev_block_type == ONLY_SHORT_BLOCK)
        prev_coding_block_type = SHORT_CODING_BLOCK;
    else 
        prev_coding_block_type = LONG_CODING_BLOCK;

#if 0 
    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }

#else 

    if (cur_coding_block_type == SHORT_CODING_BLOCK) {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = LONG_START_BLOCK;
        if (prev_block_type == LONG_START_BLOCK || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK;
    } else {
        if (prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
            cur_block_type = ONLY_LONG_BLOCK;
        if (prev_block_type == LONG_START_BLOCK) // || prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = ONLY_SHORT_BLOCK; //LONG_STOP_BLOCK;
        if (prev_block_type == ONLY_SHORT_BLOCK)
            cur_block_type = LONG_STOP_BLOCK;
    }

#endif



    return cur_block_type;
}


#define ATTACK_ENRG_MIN  (2000) 

int fa_blockswitch_robust(aacenc_ctx_t *s, float *sample_buf)
{
    int i;
    fa_blockctrl_t *f = (fa_blockctrl_t *)(s->h_blockctrl);
    float win_enrg_max;
    float win_enrg_prev;

    /*save current attack info to last attack info*/
    f->lastattack_flag  = f->attack_flag;
    f->lastattack_index = f->attack_index;
    if (f->attack_flag)
        f->max_win_enrg = f->win_enrg[0][f->lastattack_index];
    else 
        f->max_win_enrg = 0.0;

    /*save current analysis win energy to last analysis win energy*/
    for (i = 0; i < WINCNT; i++) {
        f->win_enrg[0][i]   = f->win_enrg[1][i];
        f->win_hfenrg[0][i] = f->win_hfenrg[1][i];
    }

    /*get current time signals*/
    fa_aacfilterbank_get_xbuf(s->h_aac_analysis, f->x);
    for (i = 0; i < 1024; i++) {
        f->x[i]      = f->x[i+1024];
        f->x[i+1024] = sample_buf[i];
    }

    calculate_win_enrg(f);

    f->attack_flag = 0;

    win_enrg_max = 0.0;
    win_enrg_prev = f->win_hfenrg[0][WINCNT-1];

    for (i = 0; i < WINCNT; i++) {
        float frac  = 0.3;
        float ratio = 0.1;

        /*the accenrg is the smooth energy threshold*/
        f->win_accenrg = (1-frac)*f->win_accenrg + frac*win_enrg_prev;

        if ((f->win_hfenrg[1][i]*ratio) > f->win_accenrg) {
            f->attack_flag  = 1;
            f->attack_index = i;
        }

        win_enrg_prev = f->win_hfenrg[1][i];
        win_enrg_max  = FA_MAX(win_enrg_max, win_enrg_prev);
    }

    /*printf("win_enrg_max = %f\n", win_enrg_max);*/
    /*if (win_enrg_max < ATTACK_ENRG_MIN)*/
        /*f->attack_flag = 0;*/

    /*check if last prev attack spread to this frame*/
#if 1 
    if (f->lastattack_flag && !f->attack_flag) {
        if  ((f->win_hfenrg[0][WINCNT-1] > f->win_hfenrg[1][0]) &&
             (f->lastattack_index == WINCNT-1)) {
            f->attack_flag  = 1;
            f->attack_index = 0;
        }
    }
#else 
    if (f->lastattack_flag && !f->attack_flag) {
        /*if  ((f->win_hfenrg[0][WINCNT-1] > f->win_hfenrg[1][0]) &&*/
        if  ((f->lastattack_index >= 5)) {
            f->attack_flag  = 1;
            f->attack_index = 0;
        }
    }

#endif

    /*s->block_type = select_block(s->block_type, f->attack_flag);*/
    s->block_type = win_sequence[f->lastattack_flag][f->attack_flag][s->block_type];

    return s->block_type;
}

/*TODO :sync flag, use the short block to sync all attack flag*/


int fa_blockswitch_robust_test(aacenc_ctx_t *s, float *sample_buf)
{
    int i;
    fa_blockctrl_t *f = (fa_blockctrl_t *)(s->h_blockctrl);
    float win_enrg_max;
    float win_enrg_prev;
    float frac; //  = 0.6;
    float ratio; //  = 0.2;

    float xtmp[2048];

    /*save current attack info to last attack info*/
    f->lastattack_flag  = f->attack_flag;
    f->lastattack_index = f->attack_index;
    if (f->attack_flag)
        f->max_win_enrg = f->win_enrg[0][f->lastattack_index];
    else 
        f->max_win_enrg = 0.0;

    /*save current analysis win energy to last analysis win energy*/
    for (i = 0; i < WINCNT; i++) {
        f->win_enrg[0][i]   = f->win_enrg[1][i];
        f->win_hfenrg[0][i] = f->win_hfenrg[1][i];
    }

    /*get current time signals*/
#if 0
    fa_aacfilterbank_get_xbuf(s->h_aac_analysis, f->x);
    for (i = 0; i < 1024; i++) {
        f->x[i]      = f->x[i+1024];
        f->x[i+1024] = sample_buf[i];
    }
#else 
    /*fa_aacfilterbank_get_xbuf(s->h_aac_analysis, xtmp);*/
    for (i = 0; i < 1024; i++) {
        f->x[i] = sample_buf[i];
        /*f->x[i] = xtmp[i];*/
    }
#endif

    calculate_win_enrg(f);

    f->attack_flag = 0;

    win_enrg_max = 0.0;
    win_enrg_prev = f->win_hfenrg[0][WINCNT-1];

#if  1 
    if (f->lastattack_flag) {
        if (f->diff_enrg_ratio > 0.35) {
            frac  = 0.8;
            ratio = 0.8;

            if (f->lastattack_index > 1) {
                /*frac  += (f->lastattack_index - 1) * 0.1;*/
                ratio += (f->lastattack_index - 1) * 0.2;
            }
        } else {
            frac  = 0.3;
            ratio = 0.3;
        }
    } else  {

        if (f->diff_enrg_ratio > 0.4) {
            frac  = 0.4;
            ratio = 0.3;
        } else {
            frac  = 0.3;
            ratio = 0.1;
        }

        if (f->low_hf_flag) {
            frac  = 0.2;
            ratio = 0.05;
        }
    }
#else 
    frac  = 0.4;
    ratio = 0.3;
#endif

    for (i = 0; i < WINCNT; i++) {
        float diff;

        /*the accenrg is the smooth energy threshold*/
        f->win_accenrg = (1-frac)*f->win_accenrg + frac*win_enrg_prev;
        /*f->win_accenrg = 0.7*f->win_accenrg + frac*win_enrg_prev;*/

        if ((f->win_hfenrg[1][i]*ratio) > f->win_accenrg) {
        /*if ((diff*ratio) > f->win_accenrg) {*/
            f->attack_flag  = 1;
            f->attack_index = i;
        }

        win_enrg_prev = f->win_hfenrg[1][i];
        win_enrg_max  = FA_MAX(win_enrg_max, win_enrg_prev);
    }

    /*printf("win_enrg_max = %f\n", win_enrg_max);*/
    /*if (win_enrg_max < ATTACK_ENRG_MIN)*/
        /*f->attack_flag = 0;*/

    /*printf("diff ratio=%f\n", f->diff_enrg_ratio);*/
    /*if (f->low_hf_flag)*/
        /*f->attack_flag = 0;*/

    /*check if last prev attack spread to this frame*/
#if 1 
    if (f->lastattack_flag && !f->attack_flag) {
        if  (((f->win_hfenrg[0][WINCNT-1] > f->win_hfenrg[1][0]) &&
             (f->lastattack_index == WINCNT-1)) ||
             ((f->win_hfenrg[0][WINCNT-2] > f->win_hfenrg[1][0]) &&
             (f->lastattack_index == WINCNT-2)) //||
             /*((f->win_hfenrg[0][WINCNT-3] > f->win_hfenrg[1][0]) &&*/
             /*(f->lastattack_index == WINCNT-3))*/
             )  {
            f->attack_flag  = 1;
            f->attack_index = 0;
        }
    }
#else 
    if (f->lastattack_flag && !f->attack_flag) {
        /*if  ((f->win_hfenrg[0][WINCNT-1] > f->win_hfenrg[1][0]) &&*/
        if  ((f->lastattack_index >= 4)) {
            f->attack_flag  = 1;
            f->attack_index = 0;
        }
    }

#endif
    
    /*s->block_type = select_block(s->block_type, f->attack_flag);*/
    s->block_type = win_sequence[f->lastattack_flag][f->attack_flag][s->block_type];
    /*printf("diff ratio=%f\n", f->diff_enrg_ratio);*/

    return s->block_type;
}

#if 0

static void signal_shaping(fa_blockctrl_t *f)
{
    int i;
    int win_len;

    float win_enrg_tmp;
    float win_hfenrg_tmp;
    /*float diff_enrg;*/
    float x_tmp, x_flt_tmp;
    float irt;

    const float boost_thr = 10000;
    float offset, weight;

    win_len = f->block_len;

    fa_fir_filter(f->h_flt_fir, f->x, f->x_flt, f->block_len);

    win_enrg_tmp   = 0.0;
    win_hfenrg_tmp = 0.0;

    for (i = 0; i < win_len; i++) {
        x_tmp     = f->x    [i];
        x_flt_tmp = f->x_flt[i];

        win_enrg_tmp   += x_tmp     * x_tmp;
        win_hfenrg_tmp += x_flt_tmp * x_flt_tmp;
    }

    win_enrg_tmp   += 1.;
    win_hfenrg_tmp += 1.;

    f->diff_enrg = win_enrg_tmp - win_hfenrg_tmp;
    f->diff_accenrg = 0.7 * f->diff_accenrg + 0.3 * f->diff_enrg;

#if 0
    if (f->diff_enrg < f->diff_accenrg) {
        offset = 1000; //f->diff_accenrg * 0.1;
        weight = 5; //0.8;
    } else {
        offset = 200; //f->diff_enrg * 0.1;
        weight = 1; //0.3;
    }
#else 
    if (f->diff_enrg < f->diff_accenrg) {
        offset = f->diff_accenrg * 0.1;
        weight = 0.8;
    } else {
        offset = f->diff_enrg * 0.1;
        weight = 0.3;
    }

#endif

    irt = offset + log(win_hfenrg_tmp) * weight;

    for (i = 0; i < f->block_len; i++) {
        float tmp;
        float sign;

        tmp = FA_ABS(f->x_flt[i]);
        tmp = tmp - irt;
        tmp = FA_MAX(tmp, 0);

        sign = (f->x_flt[i] > 0.) ? 1. : (-1.);
        f->x_flt[i] = sign * f->x_flt[i];
    }

}

static int cmpab(const void *a, const void *b) 
{
    return *(float *)a - *(float *)b;
}

void fa_blockswitch_robust_test1(aacenc_ctx_t *s, float *sample_buf)
{
    int win;
    int i;
    int win_len;

    /*float win_enrg_tmp;*/
    float win_hfenrg_tmp;
    float x_tmp, x_flt_tmp;

    float win_max_hfenrg;
    float win_hfenrg_sort[8];
    float ratio;
    float offset, weight;
    float cct;

    fa_blockctrl_t *f = (fa_blockctrl_t *)(s->h_blockctrl);

    /*get current time signals*/
    fa_aacfilterbank_get_xbuf(s->h_aac_analysis, f->x);
    for (i = 0; i < 1024; i++) {
        f->x[i]      = f->x[i+1024];
        f->x[i+1024] = sample_buf[i];
    }

    f->lastattack_flag  = f->attack_flag;

    signal_shaping(f);

    win_len = f->block_len / WINCNT;

    win_max_hfenrg = 0.;
    memset(win_hfenrg_sort, 0, sizeof(float)*8);
    for (win = 0; win < WINCNT; win++) {
        /*win_enrg_tmp   = 0.0;*/
        win_hfenrg_tmp = 0.0;

        for (i = 0; i < win_len; i++) {
            /*x_tmp     = f->x    [win*win_len + i];*/
            x_flt_tmp = f->x_flt[win*win_len + i];

            /*win_enrg_tmp   += x_tmp     * x_tmp;*/
            win_hfenrg_tmp += x_flt_tmp * x_flt_tmp;
        }

        /*f->win_enrg[1][win]   = win_enrg_tmp;*/
        /*f->win_hfenrg[1][win] = win_hfenrg_tmp;*/
        win_hfenrg_sort[win]  = win_hfenrg_tmp;
        /*printf("before sort: %d=%f\n", win, win_hfenrg_tmp);*/
        win_max_hfenrg = FA_MAX(win_hfenrg_tmp, win_max_hfenrg);
    }

    qsort(win_hfenrg_sort, WINCNT, sizeof(float), cmpab);
/*
    for (win = 0; win < WINCNT; win++)
        printf("after sort: %d=%f\n", win, win_hfenrg_sort[win]);
*/
    for (win = 0; win < WINCNT; win++) {
        if (win_hfenrg_sort[win] > 0) {
            float tmp;
            if (win < WINCNT-3)
                tmp = (win_hfenrg_sort[win]+win_hfenrg_sort[win+1]+win_hfenrg_sort[win+2])/3;
            else if (win < WINCNT-2)
                tmp = (win_hfenrg_sort[win]+win_hfenrg_sort[win+1])/2;
            else 
                tmp = win_hfenrg_sort[win];
            ratio = win_max_hfenrg / tmp; 
            break;
        }
    }

    offset = 30; //log(f->diff_accenrg);
    weight = 0.4;
    cct = (offset - log(f->diff_enrg)) * weight;
    /*printf("##### ratio=%f, cct=%f, offset=%f, diff=%f\n", ratio, cct, offset, log(f->diff_enrg));*/

    if (ratio > cct) {
        f->attack_flag = 1;
    } else {
        f->attack_flag = 0;
    }

    s->block_type = win_sequence[f->lastattack_flag][f->attack_flag][s->block_type];
    /*s->block_type = select_block(s->block_type, f->attack_flag);*/

    /*printf("\n\n");*/

}

#endif



