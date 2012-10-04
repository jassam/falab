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


  filename: fa_aacenc.c 
  version : v1.0.0
  time    : 2012/08/22 - 2012/10/05 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fa_aacenc.h"
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_mdctquant.h"
#include "fa_aacfilterbank.h"
#include "fa_aacstream.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#define GAIN_ADJUST 4

/* Returns the sample rate index */
int get_samplerate_index(int sample_rate)
{
    if (92017 <= sample_rate) return 0;
    if (75132 <= sample_rate) return 1;
    if (55426 <= sample_rate) return 2;
    if (46009 <= sample_rate) return 3;
    if (37566 <= sample_rate) return 4;
    if (27713 <= sample_rate) return 5;
    if (23004 <= sample_rate) return 6;
    if (18783 <= sample_rate) return 7;
    if (13856 <= sample_rate) return 8;
    if (11502 <= sample_rate) return 9;
    if (9391  <= sample_rate) return 10;

    return 11;
}


/* Max prediction band for backward predictionas function of fs index */
const int _max_pred_sfb[] = { 33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34, 0 };

int get_max_pred_sfb(int sample_rate_index)
{
    return _max_pred_sfb[sample_rate_index];
}

int get_avaiable_bits(int average_bits, int more_bits, int bitres_bits, int bitres_max_size)
{
    int available_bits;

    if (more_bits >= 0) {
        available_bits = average_bits + FA_MIN(more_bits, bitres_bits);
    } else if (more_bits < 0) {
        available_bits = average_bits + FA_MAX(more_bits, bitres_bits-bitres_max_size);
    }

    if (available_bits == 1) {
        available_bits += 1;
    }

    return available_bits;
}


uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, 
                         int ms_enable, int lfe_enable, int tns_enable, int block_switch_enable)
{
    int i;
    int bits_average;
    int bits_res_maxsize;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)malloc(sizeof(fa_aacenc_ctx_t));
    chn_info_t chn_info_tmp[MAX_CHANNELS];

    /*init configuration*/
    f->cfg.sample_rate   = sample_rate;
    f->cfg.bit_rate      = bit_rate;
    f->cfg.chn_num       = chn_num;
    f->cfg.mpeg_version  = mpeg_version;
    f->cfg.aac_objtype   = aac_objtype;
    f->cfg.ms_enable     = ms_enable;
    f->cfg.lfe_enable    = lfe_enable;
    f->cfg.tns_enable    = tns_enable;
    f->cfg.sample_rate_index = get_samplerate_index(sample_rate);

    f->sample = (float *)malloc(sizeof(float)*chn_num*AAC_FRAME_LEN);
    memset(f->sample, 0, sizeof(float)*chn_num*AAC_FRAME_LEN);

    f->block_switch_en = block_switch_enable;


    memset(chn_info_tmp, 0, sizeof(chn_info_t)*MAX_CHANNELS);
    get_aac_chn_info(chn_info_tmp, chn_num, lfe_enable);

    bits_average  = (bit_rate*1024)/(sample_rate*chn_num);
    bits_res_maxsize = get_aac_bitreservoir_maxsize(bits_average, sample_rate);
    f->h_bitstream = fa_bitstream_init((6144/8)*chn_num);

    /*init psy and mdct quant */
    for (i = 0; i < chn_num; i++) {
        f->ctx[i].pe                = 0.0;
        f->ctx[i].block_type        = ONLY_LONG_BLOCK;
        f->ctx[i].window_shape      = SINE_WINDOW;
        f->ctx[i].common_scalefac   = 0;
        memset(f->ctx[i].scalefactor, 0, sizeof(int)*MAX_SCFAC_BANDS);
        f->ctx[i].num_window_groups = 1;
        f->ctx[i].window_group_length[0] = 1;
        f->ctx[i].window_group_length[1] = 0;
        f->ctx[i].window_group_length[2] = 0;
        f->ctx[i].window_group_length[3] = 0;
        f->ctx[i].window_group_length[4] = 0;
        f->ctx[i].window_group_length[5] = 0;
        f->ctx[i].window_group_length[6] = 0;
        f->ctx[i].window_group_length[7] = 0;

        f->ctx[i].used_bits= 0;

        f->ctx[i].bits_average     = bits_average;
        f->ctx[i].bits_res_maxsize = bits_res_maxsize;
        f->ctx[i].res_buf          = (unsigned char *)malloc(sizeof(unsigned char)*(bits_res_maxsize/8 + 1));
        f->ctx[i].bits_res_size    = 0;

        f->ctx[i].h_aacpsy        = fa_aacpsy_init(sample_rate);
        f->ctx[i].h_aac_analysis  = fa_aacfilterbank_init(block_switch_enable);

        memcpy(&(f->ctx[i].chn_info), &(chn_info_tmp[i]), sizeof(chn_info_t));

        switch (sample_rate) {
            case 48000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset, 8);
                f->ctx[i].sfb_num_long = FA_SWB_48k_LONG_NUM;
                f->ctx[i].sfb_num_short= FA_SWB_48k_SHORT_NUM;
                break;
            case 44100:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset, 8);
                f->ctx[i].sfb_num_long = FA_SWB_44k_LONG_NUM;
                f->ctx[i].sfb_num_short= FA_SWB_44k_SHORT_NUM;
                break;
            case 32000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset, 8);
                f->ctx[i].sfb_num_long = FA_SWB_32k_LONG_NUM;
                f->ctx[i].sfb_num_short= FA_SWB_32k_SHORT_NUM;
                break;
        }

        memset(f->ctx[i].mdct_line, 0, sizeof(float)*2*AAC_FRAME_LEN);

        f->ctx[i].max_pred_sfb = get_max_pred_sfb(f->cfg.sample_rate_index);

        f->ctx[i].quant_ok = 0;
    }

    /*f->bitres_maxsize = get_aac_bitreservoir_maxsize(f->cfg.bit_rate, f->cfg.sample_rate);*/
    

    return (uintptr_t)f;
}

void fa_aacenc_uninit(uintptr_t handle)
{
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;

    if (f) {
        if (f->sample) {
            free(f->sample);
            f->sample = NULL;
        }
        free(f);
        f = NULL;
    }

}

static void calculate_start_common_scalefac(fa_aacenc_ctx_t *f)
{
    int i, chn;
    int chn_num;
    aacenc_ctx_t *s, *sl, *sr;

    chn_num = f->cfg.chn_num;

    i = 0;
    chn = 1;
    while (i < chn_num) {
        s = &(f->ctx[i]);
        s->chn_info.common_window = 0;

        if (s->chn_info.cpe == 1) {
            chn = 2;
            sl = s;
            sr = &(f->ctx[i+1]);
            if (0) {// (sl->block_type == sr->block_type) {
//            if (sl->block_type == sr->block_type) {
                float max_mdct_line;
                sl->chn_info.common_window = 1;
                sr->chn_info.common_window = 1;
                max_mdct_line = FA_MAX(sl->max_mdct_line, sr->max_mdct_line);
                sl->start_common_scalefac = fa_get_start_common_scalefac(max_mdct_line);
                sr->start_common_scalefac = sl->start_common_scalefac;
            } else {
                sl->chn_info.common_window = 0;
                sr->chn_info.common_window = 0;
                sl->start_common_scalefac = fa_get_start_common_scalefac(sl->max_mdct_line);
                sr->start_common_scalefac = fa_get_start_common_scalefac(sr->max_mdct_line);
            }
        } else if (s->chn_info.sce == 1) {
            chn = 1;
            s->start_common_scalefac = fa_get_start_common_scalefac(s->max_mdct_line);
        } else  { //lfe
            chn = 1;
            s->start_common_scalefac = fa_get_start_common_scalefac(s->max_mdct_line);
        }

        i += chn;
    }

}


static void init_quant_change(int outer_loop_count, aacenc_ctx_t *s)
{
    if (outer_loop_count == 0) {
        s->common_scalefac = s->start_common_scalefac;
        s->quant_change = 64;
    } else {
        s->quant_change = 2;
    }

}

static void quant_innerloop(fa_aacenc_ctx_t *f, int outer_loop_count)
{
    int i, chn;
    int chn_num;
    aacenc_ctx_t *s, *sl, *sr;
    int counted_bits;
    int available_bits;
    int available_bits_l, available_bits_r;
    int find_globalgain;

    chn_num = f->cfg.chn_num;

    i = 0;
    chn = 1;
    while (i < chn_num) {
        s = &(f->ctx[i]);

        if (s->chn_info.cpe == 1) {
            chn = 2;
            sl = s;
            sr = &(f->ctx[i+1]);
            find_globalgain = 0;
            init_quant_change(outer_loop_count, sl);
            init_quant_change(outer_loop_count, sr);
        } else {
            chn = 1;
            find_globalgain = 0;
            init_quant_change(outer_loop_count, s);
        }

        do {
            if (s->chn_info.cpe == 1) {
                if (sl->chn_info.common_window == 1) {
                    if (sl->quant_ok && sr->quant_ok) 
                        break;

                    if (s->block_type == ONLY_SHORT_BLOCK) {
                        fa_mdctline_quant(sl->h_mdctq_short, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_short, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, sl->x_quant_code, sl->x_quant_bits);
                        fa_mdctline_quant(sr->h_mdctq_short, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_short, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, sr->x_quant_code, sr->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sl->h_mdctq_long, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_long, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, sl->x_quant_code, sl->x_quant_bits);
                        fa_mdctline_quant(sr->h_mdctq_long, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_long, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, sr->x_quant_code, sr->x_quant_bits);
                    }

                } else {
                    if (sl->quant_ok && sr->quant_ok)
                        break;

                    if (sl->block_type == ONLY_SHORT_BLOCK) {
                        fa_mdctline_quant(sl->h_mdctq_short, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_short, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, sl->x_quant_code, sl->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sl->h_mdctq_long, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_long, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, sl->x_quant_code, sl->x_quant_bits);
                    }

                    if (sr->block_type == ONLY_SHORT_BLOCK) {
                        fa_mdctline_quant(sr->h_mdctq_short, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_short, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, sr->x_quant_code, sr->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sr->h_mdctq_long, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_long, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, sr->x_quant_code, sr->x_quant_bits);
                    }

                }

                counted_bits  = fa_bits_count(f->h_bitstream, &f->cfg, sl, sr);
                available_bits_l = get_avaiable_bits(sl->bits_average, sl->bits_more, sl->bits_res_size, sl->bits_res_maxsize);
                available_bits_r = get_avaiable_bits(sr->bits_average, sr->bits_more, sr->bits_res_size, sr->bits_res_maxsize);
                available_bits = available_bits_l + available_bits_r;

                if (counted_bits > available_bits) { 
                    sl->common_scalefac += sl->quant_change;
                    sr->common_scalefac += sr->quant_change;
                } else {
                    if (sl->quant_change > 1)
                        sl->common_scalefac -= sl->quant_change;
                    if (sr->quant_change > 1)
                        sr->common_scalefac -= sr->quant_change;
                }

                sl->quant_change >>= 1;
                sr->quant_change >>= 1;

                if (sl->quant_change == 0 && sr->quant_change == 0 &&
                   counted_bits>available_bits) {
                   sl->quant_change = 1;
                   sr->quant_change = 1;
                }

                if (sl->quant_change == 0 && sr->quant_change == 0)
                    find_globalgain = 1;
                else 
                    find_globalgain = 0;

            } else if (s->chn_info.sce == 1) {
                chn = 1;
                if (s->quant_ok)
                    break;
                if (s->block_type == ONLY_SHORT_BLOCK) {
                    fa_mdctline_quant(s->h_mdctq_short, s->common_scalefac, s->x_quant);
                    s->spectral_count = fa_mdctline_encode(s->h_mdctq_short, s->x_quant, s->num_window_groups, s->window_group_length, 
                                                           s->hufftab_no, s->x_quant_code, s->x_quant_bits);
                } else {
                    fa_mdctline_quant(s->h_mdctq_long, s->common_scalefac, s->x_quant);
                    s->spectral_count = fa_mdctline_encode(s->h_mdctq_long, s->x_quant, s->num_window_groups, s->window_group_length, 
                                                           s->hufftab_no, s->x_quant_code, s->x_quant_bits);
                }
#if 1  
                counted_bits  = fa_bits_count(f->h_bitstream, &f->cfg, s, NULL);

                available_bits = get_avaiable_bits(s->bits_average, s->bits_more, s->bits_res_size, s->bits_res_maxsize);
                if (counted_bits > available_bits) 
                    s->common_scalefac += s->quant_change;
                else {
                    if (s->quant_change > 1)
                        s->common_scalefac -= s->quant_change;
                }

                s->quant_change >>= 1;

                if (s->quant_change == 0 && 
                   counted_bits>available_bits)
                    s->quant_change = 1;
#endif 
                if (s->quant_change == 0)
                    find_globalgain = 1;
                else 
                    find_globalgain = 0;

            } else {
                ; // lfe left
            }

        } while (find_globalgain == 0);
#if 1     
        if (s->chn_info.cpe == 1) {
            sl = s;
            sr = &(f->ctx[i+1]);
            sl->used_bits = counted_bits>>1;
            sl->bits_res_size += sl->bits_average - sl->used_bits;
            if (sl->bits_res_size >= sl->bits_res_maxsize)
                sl->bits_res_size = sl->bits_res_maxsize;
            sr->used_bits = counted_bits - sl->used_bits;
            sr->bits_res_size += sr->bits_average - sr->used_bits;
            if (sr->bits_res_size >= sr->bits_res_maxsize)
                sr->bits_res_size = sr->bits_res_maxsize;
        } else {
            s->used_bits = counted_bits;
            s->bits_res_size += s->bits_average - counted_bits;
            if (s->bits_res_size >= s->bits_res_maxsize)
                s->bits_res_size = s->bits_res_maxsize;
        }
#else 
        if (!s->quant_ok) {
            s->used_bits = counted_bits;
            s->bits_res_size += s->bits_average - counted_bits;
            if (s->bits_res_size >= s->bits_res_maxsize)
                s->bits_res_size = s->bits_res_maxsize;
        }


#endif

        i += chn;
    } 


}


static void quant_outerloop(fa_aacenc_ctx_t *f)
{
    int i, chn;
    int chn_num;
    aacenc_ctx_t *s, *sl, *sr;
    int quant_ok_cnt;
    int outer_loop_count;

    chn_num = f->cfg.chn_num;

    quant_ok_cnt = 0;
    outer_loop_count = 0;
    do {
        /*scale the mdctline firstly using scalefactor*/
        i = 0;
        chn = 1;
        while (i < chn_num) {
            s = &(f->ctx[i]);

            if (s->chn_info.cpe == 1) {
                chn = 2;
                sl = s;
                sr = &(f->ctx[i+1]);
                if (s->chn_info.common_window == 1) {
                    if (!sl->quant_ok || !sr->quant_ok) {
                        if (s->block_type == ONLY_SHORT_BLOCK) {
                            fa_mdctline_scaled(sl->h_mdctq_short, s->num_window_groups, s->scalefactor);
                            fa_mdctline_scaled(sr->h_mdctq_short, s->num_window_groups, s->scalefactor);
                        } else {
                            fa_mdctline_scaled(sl->h_mdctq_long, s->num_window_groups, s->scalefactor);
                            fa_mdctline_scaled(sr->h_mdctq_long, s->num_window_groups, s->scalefactor);
                        }
                    }
                } else {
                    if (!sl->quant_ok || !sr->quant_ok) {
                        if (sl->block_type == ONLY_SHORT_BLOCK)
                            fa_mdctline_scaled(sl->h_mdctq_short, sl->num_window_groups, sl->scalefactor);
                        else 
                            fa_mdctline_scaled(sl->h_mdctq_long, sl->num_window_groups, sl->scalefactor);
                        if (sr->block_type == ONLY_SHORT_BLOCK)
                            fa_mdctline_scaled(sr->h_mdctq_short, sr->num_window_groups, sr->scalefactor);
                        else 
                            fa_mdctline_scaled(sr->h_mdctq_long, sr->num_window_groups, sr->scalefactor);
                    }
                }
            } else if (s->chn_info.sce == 1) {
                chn = 1;
                if (!s->quant_ok) {
                    if (s->block_type == ONLY_SHORT_BLOCK) 
                        fa_mdctline_scaled(s->h_mdctq_short, s->num_window_groups, s->scalefactor);
                    else 
                        fa_mdctline_scaled(s->h_mdctq_long, s->num_window_groups, s->scalefactor);
                }
            } else {
                chn = 1;
            }

            i += chn;
        }

        /*inner loop, search common_scalefac to fit the available_bits*/
        quant_innerloop(f, outer_loop_count);

        /*calculate quant noise */
        for (i = 0; i < chn_num; i++) {
            s = &(f->ctx[i]);
            if (!s->quant_ok) {
                if (s->block_type == ONLY_SHORT_BLOCK) {
                    fa_calculate_quant_noise(s->h_mdctq_short,
                                             s->num_window_groups, s->window_group_length,
                                             s->common_scalefac, s->scalefactor, 
                                             s->x_quant);
                } else {
                    fa_calculate_quant_noise(s->h_mdctq_long,
                                             s->num_window_groups, s->window_group_length,
                                             s->common_scalefac, s->scalefactor, 
                                             s->x_quant);
                }
            }
        }

        /*search scalefactor to fix quant noise*/
        i = 0;
        chn = 1;
        quant_ok_cnt = 0;
        while (i < chn_num) {
            s = &(f->ctx[i]);

            if (s->chn_info.cpe == 1) {
                chn = 2;
                sl = s;
                sr = &(f->ctx[i+1]);
                if (s->chn_info.common_window == 1) {
                    if (!sl->quant_ok || !sr->quant_ok) {
                        if (s->block_type == ONLY_SHORT_BLOCK) {
                            sl->quant_ok = fa_fix_quant_noise_couple(sl->h_mdctq_short, sr->h_mdctq_short, outer_loop_count,
                                                                     s->num_window_groups, s->window_group_length,
                                                                     s->scalefactor, 
                                                                     s->x_quant);
                            sr->quant_ok = sl->quant_ok;
                            quant_ok_cnt += sl->quant_ok * 2;
                        } else {
                            sl->quant_ok = fa_fix_quant_noise_couple(sl->h_mdctq_long, sr->h_mdctq_long, outer_loop_count,
                                                                     s->num_window_groups, s->window_group_length,
                                                                     s->scalefactor, 
                                                                     s->x_quant);
                            sr->quant_ok = sl->quant_ok;
                            quant_ok_cnt += sl->quant_ok * 2;
                        }
                    } else {
                        quant_ok_cnt += 2;
                    }
                } else {
                    if (!sl->quant_ok || !sr->quant_ok) {
                        if (sl->block_type == ONLY_SHORT_BLOCK) {
                            sl->quant_ok = fa_fix_quant_noise_single(sl->h_mdctq_short, outer_loop_count,
                                                                     sl->num_window_groups, sl->window_group_length,
                                                                     sl->scalefactor, 
                                                                     sl->x_quant);
                            quant_ok_cnt += sl->quant_ok;
                        } else {
                            sl->quant_ok = fa_fix_quant_noise_single(sl->h_mdctq_long, outer_loop_count,
                                                                     sl->num_window_groups, sl->window_group_length,
                                                                     sl->scalefactor, 
                                                                     sl->x_quant);
                            quant_ok_cnt += sl->quant_ok;
                        }
                        if (sr->block_type == ONLY_SHORT_BLOCK) {
                            sr->quant_ok = fa_fix_quant_noise_single(sr->h_mdctq_short, outer_loop_count,
                                                                     sr->num_window_groups, sr->window_group_length,
                                                                     sr->scalefactor, 
                                                                     sr->x_quant);
                            quant_ok_cnt += sr->quant_ok;
                        } else {
                            sr->quant_ok = fa_fix_quant_noise_single(sr->h_mdctq_long, outer_loop_count,
                                                                     sr->num_window_groups, sr->window_group_length,
                                                                     sr->scalefactor, 
                                                                     sr->x_quant);
                            quant_ok_cnt += sr->quant_ok;
                        }
                    } else {
                        quant_ok_cnt += 2;
                    }
                }
            } else if (s->chn_info.sce == 1) {
                chn = 1;
                if (!s->quant_ok) {
                    if (s->block_type == ONLY_SHORT_BLOCK) {
                        s->quant_ok = fa_fix_quant_noise_single(s->h_mdctq_short, outer_loop_count,
                                                                s->num_window_groups, s->window_group_length,
                                                                s->scalefactor, 
                                                                s->x_quant);
                        quant_ok_cnt += sr->quant_ok;
                    } else {
                        s->quant_ok = fa_fix_quant_noise_single(s->h_mdctq_long, outer_loop_count,
                                                                s->num_window_groups, s->window_group_length,
                                                                s->scalefactor, 
                                                                s->x_quant);
                        quant_ok_cnt += s->quant_ok;
                    }
                } else {
                    quant_ok_cnt += 1;
                }
            } else {
                chn = 1;
            }

            i += chn;
        }

        outer_loop_count++;

    } while (quant_ok_cnt < chn_num);

}


void fa_aacenc_encode(uintptr_t handle, unsigned char *buf_in, int inlen, unsigned char *buf_out, int *outlen)
{
    int i,j;
    int sample_rate;
    int bit_rate;
    int chn;
    int chn_num;
    short *sample_in;
    float *sample_buf;
    float xmin[8][FA_SWB_NUM_MAX];
    int block_switch_en;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;
    aacenc_ctx_t *s, *sl, *sr;

    chn_num = f->cfg.chn_num;
    sample_rate = f->cfg.sample_rate;
    bit_rate = f->cfg.bit_rate;
    /*assert(inlen == chn_num*AAC_FRAME_LEN*2);*/

    sample_in = (short *)buf_in;
    /*ith sample, jth chn*/
    for (i = 0; i < AAC_FRAME_LEN; i++) 
        for (j = 0; j < chn_num; j++) 
            f->sample[i+j*AAC_FRAME_LEN] = (float)(sample_in[i*chn_num+j]);

    block_switch_en = f->block_switch_en;

    /*psy analysis and use filterbank to generate mdctline*/
    for (i = 0; i < chn_num; i++) {
        s = &(f->ctx[i]);
        sample_buf = f->sample+i*AAC_FRAME_LEN;
        /*analysis*/
        if (block_switch_en) {
            s->block_type = fa_aacblocktype_switch(s->h_aac_analysis, s->h_aacpsy, s->pe);
            s->bits_alloc = calculate_bit_allocation(s->pe, s->block_type);
            s->bits_more  = s->bits_alloc - 90;
            /*printf("block_type=%d, pe=%f, bits_alloc=%d\n", s->block_type, s->pe, s->bits_alloc);*/
        } else {
            s->block_type = ONLY_LONG_BLOCK;
        }

        fa_aacfilterbank_analysis(s->h_aac_analysis, sample_buf, s->mdct_line);
#if 0 
        fa_reshape_highfreqline(s->h_aac_analysis, 
                                s->block_type, sample_rate, bit_rate, 
                                s->mdct_line);
#endif 
        /* calculate xmin and pe
         * use current sample_buf calculate pe to decide which block used in the next frame*/
        if (block_switch_en) {
            fa_aacpsy_calculate_xmin(s->h_aacpsy, s->mdct_line, s->block_type, xmin);
            fa_aacpsy_calculate_pe(s->h_aacpsy, sample_buf, s->block_type, &s->pe);
        }

        /*use mdct transform*/
        if (s->block_type == ONLY_SHORT_BLOCK) {
#if 0
            s->num_window_groups = 1;
            s->window_group_length[0] = 8;
            s->window_group_length[1] = 0;
            s->window_group_length[2] = 0;
            s->window_group_length[3] = 0;
            s->window_group_length[4] = 0;
            s->window_group_length[5] = 0;
            s->window_group_length[6] = 0;
            s->window_group_length[7] = 0;
#else 
            s->num_window_groups = 4;
            s->window_group_length[0] = 2;
            s->window_group_length[1] = 2;
            s->window_group_length[2] = 2;
            s->window_group_length[3] = 2;
            s->window_group_length[4] = 0;
            s->window_group_length[5] = 0;
            s->window_group_length[6] = 0;
            s->window_group_length[7] = 0;
#endif
            fa_mdctline_sfb_arrange(s->h_mdctq_short, s->mdct_line, 
                                    s->num_window_groups, s->window_group_length);
            fa_xmin_sfb_arrange(s->h_mdctq_short, xmin,
                                s->num_window_groups, s->window_group_length);
            s->max_mdct_line = fa_mdctline_getmax(s->h_mdctq_short);
            fa_mdctline_pow34(s->h_mdctq_short);

            memset(s->scalefactor, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
        } else {
            s->num_window_groups = 1;
            s->window_group_length[0] = 1;
            fa_mdctline_sfb_arrange(s->h_mdctq_long, s->mdct_line, 
                                    s->num_window_groups, s->window_group_length);
            fa_xmin_sfb_arrange(s->h_mdctq_long, xmin,
                                s->num_window_groups, s->window_group_length);
            s->max_mdct_line = fa_mdctline_getmax(s->h_mdctq_long);
            fa_mdctline_pow34(s->h_mdctq_long);

            memset(s->scalefactor, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
        }

        s->quant_ok = 0;
    }

    /*check common_window and start_common_scalefac*/
    calculate_start_common_scalefac(f);

    /*outer loop*/
    quant_outerloop(f);

    /* offset the difference of common_scalefac and scalefactors by SF_OFFSET  */
    for (i = 0; i < chn_num ; i++) {
        int gr, sfb, sfb_num;
        s = &(f->ctx[i]);
        if (s->block_type == ONLY_SHORT_BLOCK) 
            sfb_num = fa_mdctline_get_sfbnum(s->h_mdctq_short);
        else 
            sfb_num = fa_mdctline_get_sfbnum(s->h_mdctq_long);

        for (gr = 0; gr < s->num_window_groups; gr++) {
            for (sfb = 0; sfb < sfb_num; sfb++) {
                s->scalefactor[gr][sfb] = s->common_scalefac - s->scalefactor[gr][sfb] + GAIN_ADJUST + SF_OFFSET;
            }
        }
        s->common_scalefac = s->scalefactor[0][0];
    }

    fa_write_bitstream(f);

    for (i = 0; i < 1024; i++) {
        if (s->mdct_line[i] >= 0)
            s->mdct_line_sign[i] = 1;
        else
            s->mdct_line_sign[i] = -1;
    }

    *outlen = fa_bitstream_getbufval(f->h_bitstream, buf_out);

    fa_bitstream_reset(f->h_bitstream);

}
