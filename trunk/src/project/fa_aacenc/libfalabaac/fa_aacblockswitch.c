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
#define SWITCH_PE  1000 //1800//1800 //300// 1800

static void blockswitch_pe(float pe, int prev_block_type, int *cur_block_type)
{
    /*int prev_coding_block_type;*/
    int cur_coding_block_type;

    /*get prev coding block type*/
/*
    if (prev_block_type == ONLY_SHORT_BLOCK)
        prev_coding_block_type = SHORT_CODING_BLOCK;
    else 
        prev_coding_block_type = LONG_CODING_BLOCK;
*/
    /*use pe to decide current coding block type*/
    if (pe > SWITCH_PE) 
        cur_coding_block_type = SHORT_CODING_BLOCK;
    else 
        cur_coding_block_type = LONG_CODING_BLOCK;
    
    /*use prev coding block type and current coding block type to decide current block type*/
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

}


/*this function used in aac encode*/
static int aac_blockswitch_psy(int block_type, float pe)
{
    int prev_block_type;
    int cur_block_type;

    prev_block_type = block_type;
    blockswitch_pe(pe, prev_block_type, &cur_block_type);

    return cur_block_type;
}

int fa_blockswitch_psy(aacenc_ctx_t *s)
{
    if (s->psy_enable) {
        s->block_type = aac_blockswitch_psy(s->block_type, s->pe);
        s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
        s->bits_more  = s->bits_alloc - 100;
    } else {
        s->block_type = ONLY_LONG_BLOCK;
        s->bits_alloc = s->bits_average;
        s->bits_more  = s->bits_alloc - 100;
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

    hop = len >> 4;
    var_max = 0.;

    for (k = 0; k < 16; k++) {
        xp  =  x + k*hop;
        sum =  0.;
        avg =  0.;
        var =  0.;
#if 0
        for (i = 0; i < hop; i++) {
            float tmp;
            tmp = fabsf(xp[i]/32768.);
            sum += tmp*tmp;
        }
        avg = sum / hop;

        for (i = 0; i < hop; i++) {
            float tmp;
            tmp = fabsf(xp[i]/32768.);
            diff =  tmp*tmp - avg;
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

#define SWITCH_E_BASE  (32768*32768)
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
#endif

    s->block_type = cur_block_type;

    if (s->psy_enable) {
        s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
        /*s->bits_alloc = s->bits_average;*/
        s->bits_more  = s->bits_alloc - 100;
    } else {
        s->bits_alloc = s->bits_average;
        s->bits_more  = s->bits_alloc - 100;
    }

    return cur_block_type;
}

