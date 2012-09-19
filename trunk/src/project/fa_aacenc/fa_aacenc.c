#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fa_aacenc.h"
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_aacfilterbank.h"



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



uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, 
                         int ms_enable, int lfe_enable, int tns_enable, int block_switch_enable)
{
    int i;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)malloc(sizeof(fa_aacenc_ctx_t));

    /*init configuration*/
    f->cfg.sample_rate   = sample_rate;
    f->cfg.bit_rate      = bit_rate;
    f->cfg.chn_num       = chn_num;
    f->cfg.mpeg_version  = mpeg_version;
    f->cfg.aac_objtype   = aac_objtype;
    f->cfg.ms_enable     = ms_enable;
    f->cfg.lfe_enable    = lfe_enable;
    f->cfg.tns_enable    = tns_enable;

    f->sample = (float *)malloc(sizeof(float)*chn_num*AAC_FRAME_LEN);
    memset(f->sample, 0, sizeof(float)*chn_num*AAC_FRAME_LEN);

    f->block_switch_en = block_switch_enable;

    /*init psy and mdct quant */
    for(i = 0; i < chn_num; i++) {
        f->ctx[i].sample_rate_index = get_samplerate_index(sample_rate);
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

        f->ctx[i].used_bytes = 0;

        f->ctx[i].h_aacpsy        = fa_aacpsy_init(sample_rate);
        f->ctx[i].h_aac_analysis  = fa_aacfilterbank_init(block_switch_enable);

        switch(sample_rate) {
            case 48000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset, 8);
                break;
            case 44100:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset, 8);
                break;
            case 32000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset, 1);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset, 8);
                break;
        }

        memset(f->ctx[i].mdct_line, 0, sizeof(float)*2*AAC_FRAME_LEN);

        f->ctx[i].max_pred_sfb = get_max_pred_sfb(f->ctx[i].sample_rate_index);
    }

    f->bitres_maxsize = get_aac_bitreservoir_maxsize(f->cfg.bit_rate, f->cfg.sample_rate);
    
    /*f->bitres = (unsigned char *)malloc(;*/

    return (uintptr_t)f;
}

void fa_aacenc_uninit(uintptr_t handle)
{
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;

    if(f) {
        if(f->sample) {
            free(f->sample);
            f->sample = NULL;
        }
        free(f);
        f = NULL;
    }

}


void fa_aacenc_encode(uintptr_t handle, unsigned char *buf_in, int inlen, unsigned char *buf_out, int *outlen)
{
    int i,j;
    int chn;
    int chn_num;
    short *sample_in;
    float *sample_buf;
    float xmin[8][FA_SWB_NUM_MAX];
    int block_switch_en;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;
    aacenc_ctx_t *s, *sl, *sr;
    int quant_ok_cnt;
    int outer_loop_count;

    float pe;

    chn_num = f->cfg.chn_num;
    /*assert(inlen == chn_num*AAC_FRAME_LEN*2);*/

    sample_in = (short *)buf_in;
    /*ith sample, jth chn*/
    for(i = 0; i < AAC_FRAME_LEN; i++) 
        for(j = 0; j < chn_num; j++) 
            f->sample[i+j*AAC_FRAME_LEN] = (float)(sample_in[i*chn_num+j]);

    block_switch_en = f->block_switch_en;

    /*psy analysis and use filterbank to generate mdctline*/
    for(i = 0; i < chn_num; i++) {
        s = &(f->ctx[i]);
        sample_buf = f->sample+i*AAC_FRAME_LEN;
        /*analysis*/
        if(block_switch_en) {
            s->block_type = fa_get_aacblocktype(s->h_aac_analysis);
            fa_aacpsy_calculate_pe(s->h_aacpsy, sample_buf, s->block_type, &pe);
            printf("block_type=%d, pe=%f\n", s->block_type, pe);
            fa_aacblocktype_switch(s->h_aac_analysis, s->h_aacpsy, pe);
        }else {
            s->block_type = ONLY_LONG_BLOCK;
        }
        fa_aacfilterbank_analysis(s->h_aac_analysis, sample_buf, s->mdct_line);
        fa_aacpsy_calculate_xmin(s->h_aacpsy, s->mdct_line, s->block_type, xmin);

        /*use mdct transform*/
        if(s->block_type == ONLY_SHORT_BLOCK) {
            s->num_window_groups = 1;
            s->window_group_length[0] = 8;
            s->window_group_length[1] = 0;
            s->window_group_length[2] = 0;
            s->window_group_length[3] = 0;
            s->window_group_length[4] = 0;
            s->window_group_length[5] = 0;
            s->window_group_length[6] = 0;
            s->window_group_length[7] = 0;
            fa_mdctline_sfb_arrange(s->h_mdctq_short, s->mdct_line, 
                                    s->num_window_groups, s->window_group_length);
            fa_xmin_sfb_arrange(s->h_mdctq_short, xmin,
                                s->num_window_groups, s->window_group_length);
            s->max_mdct_line = fa_mdctline_getmax(s->h_mdctq_short);
            fa_mdctline_pow34(s->h_mdctq_short);

            memset(s->scalefactor, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
        }else {
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
    }

    /*check common_window and start_common_scalefac*/
    i = 0;
    chn = 1;
    while (i < chn_num) {
        s = &(f->ctx[i]);
        s->chn_info.common_window = 0;

        if (s->chn_info.cpe == 1) {
            chn = 2;
            sl = s;
            sr = &(f->ctx[i+1]);
            if (sl->block_type == sr->block_type) {
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

    /*outer loop*/
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
                if (s->common_window == 1) {
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
                    if (!sl->quant_ok) {
                        if (sl->block_type == ONLY_LONG_BLOCK)
                            fa_mdctline_scaled(sl->h_mdctq_short, sl->num_window_groups, sl->scalefactor);
                        else 
                            fa_mdctline_scaled(sl->h_mdctq_long, sl->num_window_groups, sl->scalefactor);
                    }
                    if (!sr->quant_ok) {
                        if (sr->block_type == ONLY_LONG_BLOCK)
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

        /*inner loop*/
        do {
            i = 0;
            chn = 1;

            while (i < chn_num) {
                s = &(f->ctx[i]);

                if (outer_loop_count == 0) {
                    s->common_scalefac = s->start_common_scalefac;
                    quant_change = 64;
                } else {
                    quant_change = 2;
                }

                if (!s->quant_ok) {
                    if (s->chn_info.cpe == 1) {
                        chn = 2;
                        sl = s;
                        sr = &(f->ctx[i+1]);
                        if (sl->common_window == 1) {
                            if (!sl->quant_ok || !sr->quant_ok) {
                                if (s->block_type == ONLY_SHORT_BLOCK) {
                                    fa_mdctline_quant(sl->h_mdctq_short, s->common_scalefac, s->x_quant);
                                    fa_mdctline_quant(sr->h_mdctq_short, s->common_scalefac, s->x_quant);
                                } else {
                                    fa_mdctline_quant(sl->h_mdctq_long, s->common_scalefac, s->x_quant);
                                    fa_mdctline_quant(sr->h_mdctq_long, s->common_scalefac, s->x_quant);
                                }
                            }
                        } else {
                            if (!sl->quant_ok) {
                                if (sl->block_type == ONLY_LONG_BLOCK)
                                    fa_mdctline_quant(sl->h_mdctq_short, sl->common_scalefac, sl->x_quant);
                                else 
                                    fa_mdctline_quant(sl->h_mdctq_long, sl->common_scalefac, sl->x_quant);
                            }
                            if (!sr->quant_ok) {
                                if (sr->block_type == ONLY_LONG_BLOCK)
                                    fa_mdctline_quant(sr->h_mdctq_short, sr->common_scalefac, sr->x_quant);
                                else 
                                    fa_mdctline_quant(sr->h_mdctq_long, sr->common_scalefac, sr->x_quant);
                            }
                        }

                    } else if (s->chn_info.sce == 1) {
                        chn = 1;
                        if (!s->quant_ok) {
                            if (s->block_type == ONLY_SHORT_BLOCK) 
                                fa_mdctline_quant(s->h_mdctq_short, s->common_scalefac, s->x_quant);
                            else 
                                fa_mdctline_quant(s->h_mdctq_long, s->common_scalefac, s->x_quant);
                        }
                    } else {
                        chn = 1;
                    }
                }

                i += chn;
            }
             
            counted_bits = fa_bit_count();
            if (counted_bits > s->available_bits) 
                s->common_scalefac += quant_change;
            else
                s->common_scalefac -= quant_change;

            quant_change >>= 1;

            if(quant_change == 0 && counted_bits>available_bits)
                quant_change = 1;
   
        } while (quant_change != 0)

        quant_ok_cnt = 0;
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

        i = 0;
        chn = 1;
        while (i < chn_num) {
            s = &(f->ctx[i]);

            if (s->chn_info.cpe == 1) {
                chn = 2;
                sl = s;
                sr = &(f->ctx[i+1]);
                if (s->common_window == 1) {
                    if (!sl->quant_ok || !sr->quant_ok) {
                        if (s->block_type == ONLY_SHORT_BLOCK) {
                            fa_fix_quant_noise_couple(sl->h_mdctq_short, sr->h_mdctq_short, 
                                                      s->num_window_groups, s->window_group_length,
                                                      s->scalefactor, 
                                                      s->x_quant);
                        } else {
                            fa_fix_quant_noise_couple(sl->h_mdctq_long, sr->h_mdctq_long, 
                                                      s->num_window_groups, s->window_group_length,
                                                      s->scalefactor, 
                                                      s->x_quant);
                        }
                    }
                } else {
                    if (!sl->quant_ok) {
                        if (sl->block_type == ONLY_SHORT_BLOCK) {
                            fa_fix_quant_noise_single(sl->h_mdctq_short, 
                                                      sl->num_window_groups, sl->window_group_length,
                                                      sl->scalefactor, 
                                                      sl->x_quant);
                        } else {
                            fa_fix_quant_noise_single(sl->h_mdctq_long, 
                                                      sl->num_window_groups, sl->window_group_length,
                                                      sl->scalefactor, 
                                                      sl->x_quant);
                        }
                    }
                    if (!sr->quant_ok) {
                        if (sr->block_type == ONLY_SHORT_BLOCK) {
                            fa_fix_quant_noise_single(sr->h_mdctq_short, 
                                                      sr->num_window_groups, sr->window_group_length,
                                                      sr->scalefactor, 
                                                      sr->x_quant);
                        } else {
                            fa_fix_quant_noise_single(sr->h_mdctq_long, 
                                                      sr->num_window_groups, sr->window_group_length,
                                                      sr->scalefactor, 
                                                      sr->x_quant);
                        }
                    }
                }
            } else (s->chn_info.sce == 1) {
                chn = 1;
                if (!s->quant_ok) {
                    if (s->block_type == ONLY_SHORT_BLOCK) {
                        fa_fix_quant_noise_single(s->h_mdctq_short, 
                                                  s->num_window_groups, s->window_group_length,
                                                  s->scalefactor, 
                                                  s->x_quant);
                    } else {
                        fa_fix_quant_noise_single(s->h_mdctq_long, 
                                                  s->num_window_groups, s->window_group_length,
                                                  s->scalefactor, 
                                                  s->x_quant);
                    }
                }
            } else {
                chn = 1;
            }

            i += chn;
        }


        outer_loop_count++;
    } while (quant_ok_cnt < chn_num)

    for(i = 0; i < 1024; i++) {
        if(s->mdct_line[i] >= 0)
            s->mdct_line_sig[i] = 1;
        else
            s->mdct_line_sig[i] = -1;
    }


}
