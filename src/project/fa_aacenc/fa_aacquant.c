#include <stdio.h>
#include <stdlib.h>
#include "fa_aaccfg.h"
#include "fa_aacquant.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_mdctquant.h"
#include "fa_timeprofile.h"


#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#endif 

#ifndef FA_MAX
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif



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
        /*s->chn_info.common_window = 0;*/

        if (s->chn_info.cpe == 1) {
            chn = 2;
            sl = s;
            sr = &(f->ctx[i+1]);
            if (sl->chn_info.common_window == 1) {
                float max_mdct_line;
                max_mdct_line = FA_MAX(sl->max_mdct_line, sr->max_mdct_line);
                sl->start_common_scalefac = fa_get_start_common_scalefac(max_mdct_line);
                sr->start_common_scalefac = sl->start_common_scalefac;
            } else {
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
    int head_end_sideinfo_avg;
    int inner_loop_cnt = 0;

    chn_num = f->cfg.chn_num;
    head_end_sideinfo_avg = fa_bits_sideinfo_est(chn_num);

    i = 0;
    chn = 1;
    while (i < chn_num) {
        inner_loop_cnt = 0;
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
                                                                sl->hufftab_no, &(sl->max_sfb), sl->x_quant_code, sl->x_quant_bits);
                        fa_mdctline_quant(sr->h_mdctq_short, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_short, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, &(sr->max_sfb), sr->x_quant_code, sr->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sl->h_mdctq_long, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_long, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, &(sl->max_sfb), sl->x_quant_code, sl->x_quant_bits);
                        fa_mdctline_quant(sr->h_mdctq_long, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_long, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, &(sr->max_sfb), sr->x_quant_code, sr->x_quant_bits);
                    }
                    
                    sr->max_sfb = FA_MAX(sr->max_sfb, sl->max_sfb);
                    sl->max_sfb = sr->max_sfb;

                } else {
                    if (sl->quant_ok && sr->quant_ok)
                        break;

                    if (sl->block_type == ONLY_SHORT_BLOCK) {
                        fa_mdctline_quant(sl->h_mdctq_short, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_short, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, &(sl->max_sfb), sl->x_quant_code, sl->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sl->h_mdctq_long, sl->common_scalefac, sl->x_quant);
                        sl->spectral_count = fa_mdctline_encode(sl->h_mdctq_long, sl->x_quant, sl->num_window_groups, sl->window_group_length, 
                                                                sl->hufftab_no, &(sl->max_sfb), sl->x_quant_code, sl->x_quant_bits);
                    }

                    if (sr->block_type == ONLY_SHORT_BLOCK) {
                        fa_mdctline_quant(sr->h_mdctq_short, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_short, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, &(sr->max_sfb), sr->x_quant_code, sr->x_quant_bits);
                    } else {
                        fa_mdctline_quant(sr->h_mdctq_long, sr->common_scalefac, sr->x_quant);
                        sr->spectral_count = fa_mdctline_encode(sr->h_mdctq_long, sr->x_quant, sr->num_window_groups, sr->window_group_length, 
                                                                sr->hufftab_no, &(sr->max_sfb), sr->x_quant_code, sr->x_quant_bits);
                    }

                }

                counted_bits  = fa_bits_count(f->h_bitstream, &f->cfg, sl, sr) + head_end_sideinfo_avg*2;
                available_bits_l = get_avaiable_bits(sl->bits_average, sl->bits_more, sl->bits_res_size, sl->bits_res_maxsize);
                available_bits_r = get_avaiable_bits(sr->bits_average, sr->bits_more, sr->bits_res_size, sr->bits_res_maxsize);
                available_bits = available_bits_l + available_bits_r;

                if (counted_bits > available_bits) { 
                    sl->common_scalefac += sl->quant_change;
                    sr->common_scalefac += sr->quant_change;
                    sl->common_scalefac = FA_MIN(sl->common_scalefac, 255);
                    sr->common_scalefac = FA_MIN(sr->common_scalefac, 255);
                } else {
                    if (sl->quant_change > 1) {
                        sl->common_scalefac -= sl->quant_change;
                        sl->common_scalefac = FA_MAX(sl->common_scalefac, 0);
                    }
                    if (sr->quant_change > 1) {
                        sr->common_scalefac -= sr->quant_change;
                        sr->common_scalefac = FA_MAX(sr->common_scalefac, 0);
                    }
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
                                                           s->hufftab_no, &(s->max_sfb), s->x_quant_code, s->x_quant_bits);
                } else {
                    fa_mdctline_quant(s->h_mdctq_long, s->common_scalefac, s->x_quant);
                    s->spectral_count = fa_mdctline_encode(s->h_mdctq_long, s->x_quant, s->num_window_groups, s->window_group_length, 
                                                           s->hufftab_no, &(s->max_sfb), s->x_quant_code, s->x_quant_bits);
                }


#if 1  
                counted_bits  = fa_bits_count(f->h_bitstream, &f->cfg, s, NULL) + head_end_sideinfo_avg;

                available_bits = get_avaiable_bits(s->bits_average, s->bits_more, s->bits_res_size, s->bits_res_maxsize);
                if (counted_bits > available_bits) {
                    s->common_scalefac += s->quant_change;
                    s->common_scalefac = FA_MIN(s->common_scalefac, 255);
                }
                else {
                    if (s->quant_change > 1) {
                        s->common_scalefac -= s->quant_change;
                        s->common_scalefac = FA_MAX(s->common_scalefac, 0);
                    }
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

            inner_loop_cnt++;
        } while (find_globalgain == 0);
        /*printf("the %d chn inner loop cnt=%d\n", i, inner_loop_cnt);*/
#if 1     
        if (s->chn_info.cpe == 1) {
            sl = s;
            sr = &(f->ctx[i+1]);
            sl->used_bits = counted_bits>>1;
            sl->bits_res_size = available_bits_l - sl->used_bits;
            if (sl->bits_res_size < 0)
                sl->bits_res_size = 0;
            if (sl->bits_res_size >= sl->bits_res_maxsize)
                sl->bits_res_size = sl->bits_res_maxsize;
            sr->used_bits = counted_bits - sl->used_bits;
            sr->bits_res_size = available_bits_r - sr->used_bits;
            if (sr->bits_res_size < 0)
                sr->bits_res_size = 0;
            if (sr->bits_res_size >= sr->bits_res_maxsize)
                sr->bits_res_size = sr->bits_res_maxsize;
        } else {
            s->used_bits = counted_bits;
            s->bits_res_size = available_bits - counted_bits;
            if (s->bits_res_size >= s->bits_res_maxsize)
                s->bits_res_size = s->bits_res_maxsize;
        }
#else 
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

        FA_CLOCK_START(3);
        quant_innerloop(f, outer_loop_count);
        FA_CLOCK_END(3);
        FA_CLOCK_COST(3);


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
                                                                     sl->scalefactor, sr->scalefactor, 
                                                                     s->x_quant);
                            sr->quant_ok = sl->quant_ok;
                            quant_ok_cnt += sl->quant_ok * 2;
                        } else {
                            sl->quant_ok = fa_fix_quant_noise_couple(sl->h_mdctq_long, sr->h_mdctq_long, outer_loop_count,
                                                                     s->num_window_groups, s->window_group_length,
                                                                     sl->scalefactor, sr->scalefactor,
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

    /*printf("outer loop cnt= %d\n", outer_loop_count);*/
}


void fa_quantize_loop(fa_aacenc_ctx_t *f)
{
    int i;
    int chn_num;
    aacenc_ctx_t *s;

    chn_num = f->cfg.chn_num;

    for (i = 0; i < chn_num; i++) {
        s = &(f->ctx[i]);
        if (s->block_type == ONLY_SHORT_BLOCK) {
            s->max_mdct_line = fa_mdctline_getmax(s->h_mdctq_short);
            fa_mdctline_pow34(s->h_mdctq_short);
            memset(s->scalefactor, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
        } else {
            s->max_mdct_line = fa_mdctline_getmax(s->h_mdctq_long);
            fa_mdctline_pow34(s->h_mdctq_long);
            memset(s->scalefactor, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
        }
    }
     
    /*check common_window and start_common_scalefac*/
    calculate_start_common_scalefac(f);

    /*outer loop*/

    FA_CLOCK_START(2);
    quant_outerloop(f);
    FA_CLOCK_END(2);
    FA_CLOCK_COST(2);



}


