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
#include <memory.h>
#include "fa_aacenc.h"
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_mdctquant.h"
#include "fa_aacblockswitch.h"
#include "fa_aacfilterbank.h"
#include "fa_bitstream.h"
#include "fa_aacstream.h"
#include "fa_aacms.h"
#include "fa_aacquant.h"
#include "fa_huffman.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#endif 

#ifndef FA_MAX
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#define GAIN_ADJUST   4 


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

typedef struct _rate_cutoff {
    int bit_rate;
    int cutoff;
}rate_cutoff_t;

/*reference to 48000kHz sample rate*/
static rate_cutoff_t rate_cutoff[] = 
{
    {16000, 5000},
    {24000, 5000},
    {32000, 8000},
    {38000, 12000},
    {48000, 20000},
    {64000, 20000},
    {0    , 0},
};

static int get_bandwidth(int chn, int sample_rate, int bit_rate) 
{
    int i;
    int tmpbitrate;
    int bandwidth;
    float ratio;

    ratio = (float) 48000/sample_rate;
    tmpbitrate = bit_rate * ratio;
    tmpbitrate = tmpbitrate/chn;

    for (i = 0; i < rate_cutoff[i].bit_rate; i++) {
        if (rate_cutoff[i].bit_rate >= tmpbitrate)
            break;
    }

    if (i > 0)
        bandwidth = rate_cutoff[i-1].cutoff;
    else 
        bandwidth = 5000;

    if (bandwidth == 0)
        bandwidth = 20000;
    printf("bandwidth = %d\n", bandwidth);
    assert(bandwidth > 0 && bandwidth <= 20000);

    return bandwidth;

}

static int get_cutoff_line(int sample_rate, int fmax_line_offset, int bandwidth)
{
    float fmax;
    float delta_f;
    int offset;

    fmax = (float)sample_rate/2.;
    delta_f = fmax/fmax_line_offset;

    offset = (int)((float)bandwidth/delta_f);

    return offset;

}

static int get_cutoff_sfb(int sfb_offset_max, int *sfb_offset, int cutoff_line)
{
    int i;

    for (i = 0; i < sfb_offset_max; i++) {
        if (sfb_offset[i] >= cutoff_line)
            break;
    }

    return (i+1);
}

static void fa_aacenc_rom_init()
{
    fa_mdctquant_rom_init();
    fa_huffman_rom_init();
}


uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, 
                         int ms_enable, int lfe_enable, int tns_enable, int block_switch_enable, int psy_enable,
                         int blockswitch_method, int quantize_method)
{
    int i;
    int bits_average;
    int bits_res_maxsize;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)malloc(sizeof(fa_aacenc_ctx_t));
    chn_info_t chn_info_tmp[MAX_CHANNELS];

    if (bit_rate > 256000 || bit_rate < 32000)
        return (uintptr_t)NULL;

    /*init rom*/
    fa_aacenc_rom_init();

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
    f->psy_enable      = psy_enable;

    f->band_width = get_bandwidth(chn_num, sample_rate, bit_rate);

    memset(chn_info_tmp, 0, sizeof(chn_info_t)*MAX_CHANNELS);
    get_aac_chn_info(chn_info_tmp, chn_num, lfe_enable);

    bits_average  = (bit_rate*1024)/(sample_rate*chn_num);
    bits_res_maxsize = get_aac_bitreservoir_maxsize(bits_average, sample_rate);
    f->h_bitstream = fa_bitstream_init((6144/8)*chn_num);


    switch (blockswitch_method) {
        case BLOCKSWITCH_PSY:
            f->blockswitch_method = BLOCKSWITCH_PSY;
            f->do_blockswitch  = fa_blockswitch_psy;
            break;
        case BLOCKSWITCH_VAR:
            f->blockswitch_method = BLOCKSWITCH_VAR;
            f->do_blockswitch  = fa_blockswitch_var;
            break;
        default:
            f->blockswitch_method = BLOCKSWITCH_VAR;
            f->do_blockswitch  = fa_blockswitch_var;
            break;

    }

    switch (quantize_method) {
        case QUANTIZE_LOOP:
            f->quantize_method = QUANTIZE_LOOP;
            f->do_quantize = fa_quantize_loop;
            break;
        case QUANTIZE_FAST:
            f->quantize_method = QUANTIZE_FAST;
            break;
        default:
            f->quantize_method = QUANTIZE_LOOP;
            f->do_quantize = fa_quantize_loop;

    }
    /*f->do_quantize;*/

    /*init psy and mdct quant */
    for (i = 0; i < chn_num; i++) {
        f->ctx[i].pe                = 0.0;
        f->ctx[i].var_max_prev      = 0.0;
        f->ctx[i].block_type        = ONLY_LONG_BLOCK;
        f->ctx[i].psy_enable        = psy_enable;
        f->ctx[i].window_shape      = SINE_WINDOW;
        f->ctx[i].common_scalefac   = 0;
        memset(f->ctx[i].scalefactor, 0, sizeof(int)*8*FA_SWB_NUM_MAX);

        f->ctx[i].num_window_groups = 1;
        f->ctx[i].window_group_length[0] = 1;
        f->ctx[i].window_group_length[1] = 0;
        f->ctx[i].window_group_length[2] = 0;
        f->ctx[i].window_group_length[3] = 0;
        f->ctx[i].window_group_length[4] = 0;
        f->ctx[i].window_group_length[5] = 0;
        f->ctx[i].window_group_length[6] = 0;
        f->ctx[i].window_group_length[7] = 0;

        memset(f->ctx[i].lastx, 0, sizeof(int)*8);
        memset(f->ctx[i].avgenergy, 0, sizeof(float)*8);
        f->ctx[i].quality = 100;

        f->ctx[i].used_bits= 0;

        f->ctx[i].bits_average     = bits_average;
        f->ctx[i].bits_res_maxsize = bits_res_maxsize;
        f->ctx[i].res_buf          = (unsigned char *)malloc(sizeof(unsigned char)*(bits_res_maxsize/8 + 1));
        f->ctx[i].bits_res_size    = 0;
        f->ctx[i].last_common_scalefac = 0;

        f->ctx[i].h_aacpsy        = fa_aacpsy_init(sample_rate);
        f->ctx[i].h_aac_analysis  = fa_aacfilterbank_init();

        memcpy(&(f->ctx[i].chn_info), &(chn_info_tmp[i]), sizeof(chn_info_t));
        f->ctx[i].chn_info.common_window = 0;

        switch (sample_rate) {
            case 48000:
                f->ctx[i].cutoff_line_long = get_cutoff_line(48000, 1024, f->band_width);
                f->ctx[i].cutoff_line_short= get_cutoff_line(48000, 128 , f->band_width);
                f->ctx[i].cutoff_sfb_long  = get_cutoff_sfb(FA_SWB_48k_LONG_NUM , fa_swb_48k_long_offset , f->ctx[i].cutoff_line_long);
                f->ctx[i].cutoff_sfb_short = get_cutoff_sfb(FA_SWB_48k_SHORT_NUM, fa_swb_48k_short_offset, f->ctx[i].cutoff_line_short);
                /*f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset, 1);*/
                /*f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset, 8);*/
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, f->ctx[i].cutoff_sfb_long , fa_swb_48k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , f->ctx[i].cutoff_sfb_short, fa_swb_48k_short_offset, 8);
                /*f->ctx[i].sfb_num_long = FA_SWB_48k_LONG_NUM;*/
                /*f->ctx[i].sfb_num_short= FA_SWB_48k_SHORT_NUM;*/
                f->ctx[i].sfb_num_long = f->ctx[i].cutoff_sfb_long;
                f->ctx[i].sfb_num_short= f->ctx[i].cutoff_sfb_short;
                break;
            case 44100:
                f->ctx[i].cutoff_line_long = get_cutoff_line(44100, 1024, f->band_width);
                f->ctx[i].cutoff_line_short= get_cutoff_line(44100, 128 , f->band_width);
                f->ctx[i].cutoff_sfb_long  = get_cutoff_sfb(FA_SWB_44k_LONG_NUM , fa_swb_44k_long_offset , f->ctx[i].cutoff_line_long);
                f->ctx[i].cutoff_sfb_short = get_cutoff_sfb(FA_SWB_44k_SHORT_NUM, fa_swb_44k_short_offset, f->ctx[i].cutoff_line_short);
                /*f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset, 1);*/
                /*f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset, 8);*/
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, f->ctx[i].cutoff_sfb_long , fa_swb_44k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , f->ctx[i].cutoff_sfb_short, fa_swb_44k_short_offset, 8);
                f->ctx[i].sfb_num_long = f->ctx[i].cutoff_sfb_long;
                f->ctx[i].sfb_num_short= f->ctx[i].cutoff_sfb_short;
                break;
            case 32000:
                f->ctx[i].cutoff_line_long = get_cutoff_line(32000, 1024, f->band_width);
                f->ctx[i].cutoff_line_short= get_cutoff_line(32000, 128 , f->band_width);
                f->ctx[i].cutoff_sfb_long  = get_cutoff_sfb(FA_SWB_32k_LONG_NUM , fa_swb_32k_long_offset , f->ctx[i].cutoff_line_long);
                f->ctx[i].cutoff_sfb_short = get_cutoff_sfb(FA_SWB_32k_SHORT_NUM, fa_swb_32k_short_offset, f->ctx[i].cutoff_line_short);
                /*f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset, 1);*/
                /*f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset, 8);*/
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, f->ctx[i].cutoff_sfb_long , fa_swb_32k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , f->ctx[i].cutoff_sfb_short, fa_swb_32k_short_offset, 8);
                /*f->ctx[i].sfb_num_long = FA_SWB_32k_LONG_NUM;*/
                /*f->ctx[i].sfb_num_short= FA_SWB_32k_SHORT_NUM;*/
                f->ctx[i].sfb_num_long = f->ctx[i].cutoff_sfb_long;
                f->ctx[i].sfb_num_short= f->ctx[i].cutoff_sfb_short;
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

static void zero_cutoff(float *mdct_line, int mdct_line_num, int cutoff_line)
{
    int i;

    for (i = cutoff_line; i < mdct_line_num; i++)
        mdct_line[i] = 0;

}

static void mdctline_reorder(aacenc_ctx_t *s, float xmin[8][FA_SWB_NUM_MAX])
{

    /*use mdct transform*/
    if (s->block_type == ONLY_SHORT_BLOCK) {
#if  1 
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
        s->num_window_groups = 3;
        s->window_group_length[0] = 6;
        s->window_group_length[1] = 1;
        s->window_group_length[2] = 1;
        s->window_group_length[3] = 0;
        s->window_group_length[4] = 0;
        s->window_group_length[5] = 0;
        s->window_group_length[6] = 0;
        s->window_group_length[7] = 0;
#endif
        fa_mdctline_sfb_arrange(s->h_mdctq_short, s->mdct_line, 
                s->num_window_groups, s->window_group_length);
        fa_xmin_sfb_arrange(s->h_mdctq_short, xmin,
                s->num_window_groups, s->window_group_length);

    } else {
        s->num_window_groups = 1;
        s->window_group_length[0] = 1;
        fa_mdctline_sfb_arrange(s->h_mdctq_long, s->mdct_line, 
                s->num_window_groups, s->window_group_length);
        fa_xmin_sfb_arrange(s->h_mdctq_long, xmin,
                s->num_window_groups, s->window_group_length);

    }

}

static void scalefactor_recalculate(fa_aacenc_ctx_t *f, int chn_num)
{
    int i;
    int gr, sfb, sfb_num;
    aacenc_ctx_t *s;

    for (i = 0; i < chn_num ; i++) {
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


}

void fa_aacenc_encode(uintptr_t handle, unsigned char *buf_in, int inlen, unsigned char *buf_out, int *outlen)
{
    int i,j;
    int chn_num;
    short *sample_in;
    float *sample_buf;
    float xmin[8][FA_SWB_NUM_MAX];
    int ms_enable;
    int block_switch_en;
    int psy_enable;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;
    aacenc_ctx_t *s;

    ms_enable   = f->cfg.ms_enable;
    chn_num     = f->cfg.chn_num;
    /*assert(inlen == chn_num*AAC_FRAME_LEN*2);*/

    memset(xmin, 0, sizeof(float)*8*FA_SWB_NUM_MAX);
    /*update sample buffer, ith sample, jth chn*/
    sample_in = (short *)buf_in;
    for (i = 0; i < AAC_FRAME_LEN; i++) 
        for (j = 0; j < chn_num; j++) 
            f->sample[i+j*AAC_FRAME_LEN] = (float)(sample_in[i*chn_num+j]);

    block_switch_en = f->block_switch_en;
    psy_enable      = f->psy_enable;

    /*block switch and use filterbank to generate mdctline*/
    for (i = 0; i < chn_num; i++) {
        s = &(f->ctx[i]);

        /*get the input sample*/
        sample_buf = f->sample+i*AAC_FRAME_LEN;

        /*block switch */
        if (block_switch_en) {
            f->do_blockswitch(s);
#if 1 
            if (s->block_type != 0)
                printf("i=%d, block_type=%d, pe=%f, bits_alloc=%d\n", i+1, s->block_type, s->pe, s->bits_alloc);
#endif
        } else {
            s->block_type = ONLY_LONG_BLOCK;
        }

        /*analysis*/
        fa_aacfilterbank_analysis(s->h_aac_analysis, s->block_type, &(s->window_shape),
                                  sample_buf, s->mdct_line);

        /*cutoff the frequence according to the bitrate*/
        if (s->block_type == ONLY_SHORT_BLOCK) {
            int k;
            for (k = 0; k < 8; k++)
                zero_cutoff(s->mdct_line+k*128, 128, s->cutoff_line_short);
        } else
            zero_cutoff(s->mdct_line, 1024, s->cutoff_line_long);

        /* 
           calculate xmin and pe
           --use current sample_buf calculate pe to decide which block used in the next frame
        */
        if (psy_enable) {
            fa_aacpsy_calculate_pe(s->h_aacpsy, sample_buf, s->block_type, &s->pe);
            fa_aacpsy_calculate_xmin(s->h_aacpsy, s->mdct_line, s->block_type, xmin);
        } else {
            fa_fastquant_calculate_sfb_avgenergy(s);
            fa_fastquant_calculate_xmin(s, xmin);
        }

        /*if is short block , recorder will arrange the mdctline to sfb-grouped*/
        mdctline_reorder(s, xmin);

        /*reset the quantize status*/
        s->quant_ok = 0;
    }

    /*mid/side encoding*/
    if (ms_enable)
        fa_aacmsenc(f);

    /*quantize*/
    f->do_quantize(f);

    /* offset the difference of common_scalefac and scalefactors by SF_OFFSET  */
    scalefactor_recalculate(f, chn_num);

    /*format bitstream*/
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
