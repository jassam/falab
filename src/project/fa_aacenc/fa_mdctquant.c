#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fa_mdctquant.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#define MAX_QUANT             8191
#define MAGIC_NUMBER          0.4054

#define SF_OFFSET             100

typedef struct _fa_mdctquant_t {

    int   block_type_cof;
    int   mdct_line_num;

    float mdct_line[NUM_MDCT_LINE_MAX];
    float xr_pow[NUM_MDCT_LINE_MAX];
    float mdct_scaled[NUM_MDCT_LINE_MAX];

    float xmin[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX][NUM_WINDOWS_MAX];
    float error_energy[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX][NUM_WINDOWS_MAX];

    int   sfb_num;
    int   swb_low[FA_SWB_NUM_MAX];
    int   swb_high[FA_SWB_NUM_MAX];
    int   sfb_low[NUM_WINDOW_GROUPS_MAX][FA_SWB_NUM_MAX];
    int   sfb_high[NUM_WINDOW_GROUPS_MAX][FA_SWB_NUM_MAX];

    int   common_scalefac;

}fa_mdctquant_t;

uintptr_t fa_mdctquant_init(int mdct_line_num, int sfb_num, int *swb_low, int block_type_cof)
{
    int swb;

    fa_mdctquant_t *f = (fa_mdctquant_t *)malloc(sizeof(fa_mdctquant_t));
    
    f->block_type_cof = block_type_cof;
    f->mdct_line_num  = mdct_line_num;

    memset(f->mdct_line  , 0, sizeof(float)*NUM_MDCT_LINE_MAX);
    memset(f->xr_pow     , 0, sizeof(float)*NUM_MDCT_LINE_MAX);
    memset(f->mdct_scaled, 0, sizeof(float)*NUM_MDCT_LINE_MAX);
    memset(f->xmin       , 0, sizeof(float)*NUM_WINDOW_GROUPS_MAX*NUM_SFB_MAX*NUM_WINDOWS_MAX);

    f->sfb_num = sfb_num;
    memset(f->swb_low    , 0, sizeof(int)*FA_SWB_NUM_MAX);
    memset(f->swb_high   , 0, sizeof(int)*FA_SWB_NUM_MAX);
    memset(f->sfb_low    , 0, sizeof(int)*FA_SWB_NUM_MAX*NUM_WINDOW_GROUPS_MAX);
    memset(f->sfb_high   , 0, sizeof(int)*FA_SWB_NUM_MAX*NUM_WINDOW_GROUPS_MAX);

    for(swb = 0; swb < sfb_num; swb++) {
        f->swb_low[swb] = swb_low[swb];
        if(swb == sfb_num - 1)
            f->swb_high[swb] = mdct_line_num-1;
        else 
            f->swb_high[swb] = swb_low[swb+1] - 1;
    }

    f->common_scalefac = 0;

    return (uintptr_t)f;
}

void      fa_mdctquant_uninit(uintptr_t handle)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    if(f) {
        free(f);
        f = NULL;
    }

}

static int calculate_start_common_scalefac(float max_mdct_line)
{
    int start_common_scalefac;
    float tmp;

    tmp = ceilf(16./3 * (log2f((powf(max_mdct_line, 0.75))/MAX_QUANT)));
    start_common_scalefac = (int)tmp;

    return start_common_scalefac;
}

static int bit_count()
{
    int bits;

    return bits;
}

static void iteration_innerloop(fa_mdctquant_t *f, 
                                int outer_loop_count, int available_bits,
                                int *x_quant, int *used_bits)
{
    int i;
    float cof_quant;
    int quantizer_change;
    int counted_bits;

    float *mdct_scaled = f->mdct_scaled;

    if(outer_loop_count == 0)
        quantizer_change = 64;
    else 
        quantizer_change = 2;

    do {
        /*quantize spectrum*/
        for(i = 0; i < f->block_type_cof*f->mdct_line_num; i++) {
            cof_quant = powf(2, (-3./16)*f->common_scalefac);
            x_quant[i] = (int)(mdct_scaled[i] * cof_quant + MAGIC_NUMBER);
        }
        return;

        counted_bits = bit_count();

        if(counted_bits > available_bits) {
            f->common_scalefac = f->common_scalefac + quantizer_change;
        }else {
            f->common_scalefac = f->common_scalefac - quantizer_change;
        }

        quantizer_change >>= 1;

        if((quantizer_change == 0 ) && (counted_bits > available_bits))
            quantizer_change = 1;

    }while(quantizer_change != 0);


    *used_bits = counted_bits;
}

static int iteration_stop(int num_window_groups, int *energy_err_ok, int *sfb_allscale)
{
    int gr;

    for(gr = 0; gr < num_window_groups; gr++) {
        if((energy_err_ok[gr] == 0) && (sfb_allscale[gr] == 0))
            return 0;
    }

    return 1;
}

static void xr_pow34_calculate(float *mdct_line, float mdct_line_num, float *xr_pow)
{
    int i;
    float tmp;

    for(i = 0; i < mdct_line_num; i++) {
        tmp = fabsf(mdct_line[i]); 
        xr_pow[i] = sqrtf(tmp*sqrtf(tmp));
    }
}

static void iteration_outerloop(fa_mdctquant_t *f,   
                                int num_window_groups, int *window_group_length,
                                int available_bits, 
                                int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], int *x_quant, int *used_bits)
{
    int i;

    int gr, win;
    int sfb;
    int sfb_num;
    int *swb_low, *swb_high;
    int swb_width;

    float *mdct_line;
    float *xr_pow;
    float *mdct_scaled;


    float tmp;
    float cof_scale;

    float inv_x_quant;
    float inv_cof;

    int outer_loop_count;

    float error_energy[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX][NUM_WINDOWS_MAX];
    int mdct_line_offset;
    /*three break condition variable*/
    /*no1*/
    int energy_err_ok_cnt[NUM_WINDOW_GROUPS_MAX];
    int energy_err_ok[NUM_WINDOW_GROUPS_MAX];
    /*no2*/
    int sfb_scale_cnt[NUM_WINDOW_GROUPS_MAX];
    int sfb_allscale[NUM_WINDOW_GROUPS_MAX];
    /*no3*/
    int sfb_nb_diff60;

    int scalefactor_ok[NUM_WINDOW_GROUPS_MAX];


    sfb_num  = f->sfb_num;
    swb_low  = f->swb_low;
    swb_high = f->swb_high;

    xr_pow       = f->xr_pow;
    mdct_line    = f->mdct_line;
    mdct_scaled  = f->mdct_scaled;

    outer_loop_count = 0;

    sfb_nb_diff60     = 0;
    xr_pow34_calculate(mdct_line, f->block_type_cof*f->mdct_line_num, xr_pow);

    do {

        memset(energy_err_ok_cnt, 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
        memset(energy_err_ok    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
        memset(sfb_scale_cnt    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
        memset(sfb_allscale     , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
        memset(scalefactor_ok   , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);

        for(gr = 0; gr < num_window_groups; gr++) {
            for(sfb = 0; sfb < sfb_num; sfb++) {
                cof_scale = powf(2, (3./16.) * scalefactor[gr][sfb]);
                for(i = f->sfb_low[gr][sfb]; i <= f->sfb_high[gr][sfb]; i++) 
                    mdct_scaled[i] = xr_pow[i] * cof_scale;
            }
        }

        iteration_innerloop(f, outer_loop_count, available_bits,
                            x_quant, used_bits);

        /*calculate scalefactor band error energy*/
        memset(error_energy, 0, sizeof(float)*NUM_WINDOW_GROUPS_MAX*NUM_SFB_MAX*NUM_WINDOWS_MAX);

        /*calculate error_energy*/
        mdct_line_offset = 0;
        for(gr = 0; gr < num_window_groups; gr++) {
            for(sfb = 0; sfb < sfb_num; sfb++) {
                swb_width = swb_high[sfb] - swb_low[sfb] + 1;
                for(win = 0; win < window_group_length[gr]; win++) {
                    float tmp_xq;
                    error_energy[gr][sfb][win] = 0;
                    for(i = 0; i < swb_width; i++) {
                        inv_cof = powf(2, 0.25*(f->common_scalefac - scalefactor[gr][sfb]));
                        tmp_xq = (float)x_quant[mdct_line_offset+i];
                        inv_x_quant = powf(tmp_xq, 4./3.) * inv_cof; 
                        tmp = fabsf(mdct_line[mdct_line_offset+i]) - inv_x_quant;
                        error_energy[gr][sfb][win] += tmp*tmp;  
                    }
                    mdct_line_offset += swb_width;
               }
            }
        }

        /*judge if scalefactor increase or not*/
        for(gr = 0; gr < num_window_groups; gr++) {
            for(sfb = 0; sfb < sfb_num; sfb++) {
                for(win = 0; win < window_group_length[gr]; win++) {
                    if(error_energy[gr][sfb][win] > f->xmin[gr][sfb][win]) {
                        scalefactor[gr][sfb] += 1;
                        sfb_scale_cnt[gr]++;
                        break;
                    }else {
                        energy_err_ok_cnt[gr]++;
                    }
                }
            }
        }

        /*every scalefactor band has good energy distorit;*/
        for(gr = 0; gr < num_window_groups; gr++) {
            if(energy_err_ok_cnt[gr] >= sfb_num*window_group_length[gr])
                energy_err_ok[gr] = 1;
            else 
                energy_err_ok[gr] = 0;

            if(sfb_scale_cnt[gr] >= sfb_num) {
                sfb_allscale[gr] = 1;
                /*recover the scalefactor*/
                for(sfb = 0; sfb < sfb_num; sfb++) {
                    scalefactor[gr][sfb] -= 1;
                }
            }
            else
                sfb_allscale[gr] = 0;
        }

        outer_loop_count++;
    }while(!iteration_stop(num_window_groups, energy_err_ok, sfb_allscale));

    /* offset the difference of common_scalefac and scalefactors by SF_OFFSET  */
    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            scalefactor[gr][sfb] = f->common_scalefac - scalefactor[gr][sfb] + SF_OFFSET;
        }
    }

    f->common_scalefac = scalefactor[0][0];

}

int fa_mdctline_getmax(uintptr_t handle)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    float max_mdct_line;
    float abs_mdct_line;
    float *mdct_line = f->mdct_line;

    max_mdct_line = 0;

    /*calculate max mdct_line*/
    for(i = 0; i < (f->block_type_cof*f->mdct_line_num); i++) {
        abs_mdct_line = fabsf(mdct_line[i]);
        if(abs_mdct_line > max_mdct_line)
            max_mdct_line = abs_mdct_line;
    }

    return max_mdct_line;
}

int fa_get_start_common_scalefac(int max_mdct_line)
{
    int start_common_scalefac;
    float tmp;

    tmp = ceilf(16./3 * (log2f((powf(max_mdct_line, 0.75))/MAX_QUANT)));
    start_common_scalefac = (int)tmp;

    return start_common_scalefac;
}



void fa_mdctline_pow34(uintptr_t handle)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    xr_pow34_calculate(f->mdct_line, f->block_type_cof*f->mdct_line_num, f->xr_pow);

}

void fa_mdctline_scaled(uintptr_t handle,
                        int num_window_groups, int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX])
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    int gr;
    int sfb;
    int sfb_num;

    sfb_num      = f->sfb_num;
    xr_pow       = f->xr_pow;
    mdct_scaled  = f->mdct_scaled;

    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            cof_scale = powf(2, (3./16.) * scalefactor[gr][sfb]);
            for(i = f->sfb_low[gr][sfb]; i <= f->sfb_high[gr][sfb]; i++) 
                mdct_scaled[i] = xr_pow[i] * cof_scale;
        }
    }

}

void fa_mdctline_quant(uintptr_t handle, 
                       int common_scalefac, int *x_quant)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    float mdct_scaled = f->mdct_scaled;

    for(i = 0; i < f->block_type_cof*f->mdct_line_num; i++) {
        cof_quant = powf(2, (-3./16)*common_scalefac);
        x_quant[i] = (int)(mdct_scaled[i] * cof_quant + MAGIC_NUMBER);
    }
}

void fa_calculate_quant_noise(uintptr_t handle,
                             int num_window_groups, int *window_group_length,
                             int common_scalefac, int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                             int *x_quant)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    int i;

    int gr, win;
    int sfb;
    int sfb_num;
    int *swb_low, *swb_high;
    int swb_width;

    float *mdct_line;

    float tmp;

    float inv_x_quant;
    float inv_cof;


    int mdct_line_offset;
    float *error_energy;

    sfb_num  = f->sfb_num;
    swb_low  = f->swb_low;
    swb_high = f->swb_high;

    mdct_line    = f->mdct_line;
    error_energy = f->error_energy;

    /*calculate scalefactor band error energy*/
    memset(error_energy, 0, sizeof(float)*NUM_WINDOW_GROUPS_MAX*NUM_SFB_MAX*NUM_WINDOWS_MAX);

    /*calculate error_energy*/
    mdct_line_offset = 0;
    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            swb_width = swb_high[sfb] - swb_low[sfb] + 1;
            for(win = 0; win < window_group_length[gr]; win++) {
                float tmp_xq;
                error_energy[gr][sfb][win] = 0;
                for(i = 0; i < swb_width; i++) {
                    inv_cof = powf(2, 0.25*(f->common_scalefac - scalefactor[gr][sfb]));
                    tmp_xq = (float)x_quant[mdct_line_offset+i];
                    inv_x_quant = powf(tmp_xq, 4./3.) * inv_cof; 
                    tmp = fabsf(mdct_line[mdct_line_offset+i]) - inv_x_quant;
                    error_energy[gr][sfb][win] += tmp*tmp;  
                }
                mdct_line_offset += swb_width;
           }
        }
    }

}

int  fa_fix_quant_noise_single(uintptr_t handle, 
                               int num_window_groups, int *window_group_length,
                               int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                               int *x_quant)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    int i;

    int gr, win;
    int sfb;
    int sfb_num;

    /*three break condition variable*/
    /*no1*/
    int energy_err_ok_cnt[NUM_WINDOW_GROUPS_MAX];
    int energy_err_ok[NUM_WINDOW_GROUPS_MAX];
    /*no2*/
    int sfb_scale_cnt[NUM_WINDOW_GROUPS_MAX];
    int sfb_allscale[NUM_WINDOW_GROUPS_MAX];
    /*no3*/
    int sfb_nb_diff60;


    sfb_num  = f->sfb_num;


    sfb_nb_diff60     = 0;

    memset(energy_err_ok_cnt, 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(energy_err_ok    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(sfb_scale_cnt    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(sfb_allscale     , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);

    /*judge if scalefactor increase or not*/
    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            for(win = 0; win < window_group_length[gr]; win++) {
                if(error_energy[gr][sfb][win] > f->xmin[gr][sfb][win]) {
                    scalefactor[gr][sfb] += 1;
                    sfb_scale_cnt[gr]++;
                    break;
                }else {
                    energy_err_ok_cnt[gr]++;
                }
            }
        }
    }

    /*every scalefactor band has good energy distorit;*/
    for(gr = 0; gr < num_window_groups; gr++) {
        if(energy_err_ok_cnt[gr] >= sfb_num*window_group_length[gr])
            energy_err_ok[gr] = 1;
        else 
            energy_err_ok[gr] = 0;

        if(sfb_scale_cnt[gr] >= sfb_num) {
            sfb_allscale[gr] = 1;
            /*recover the scalefactor*/
            for(sfb = 0; sfb < sfb_num; sfb++) {
                scalefactor[gr][sfb] -= 1;
            }
        }
        else
            sfb_allscale[gr] = 0;
    }

    for(gr = 0; gr < num_window_groups; gr++) {
        if((energy_err_ok[gr] == 0) && (sfb_allscale[gr] == 0))
            return 0;
    }

    return 1;

}


int  fa_fix_quant_noise_couple(uintptr_t handle1, uintptr_t handle2,
                               int num_window_groups, int *window_group_length,
                               int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                               int *x_quant)
{
    fa_mdctquant_t *f1 = (fa_mdctquant_t *)handle1;
    fa_mdctquant_t *f2 = (fa_mdctquant_t *)handle2;
    int i;

    int gr, win;
    int sfb;
    int sfb_num;

    float *error_energy1;
    float *error_energy2;

    /*three break condition variable*/
    /*no1*/
    int energy_err_ok_cnt[NUM_WINDOW_GROUPS_MAX];
    int energy_err_ok[NUM_WINDOW_GROUPS_MAX];
    /*no2*/
    int sfb_scale_cnt[NUM_WINDOW_GROUPS_MAX];
    int sfb_allscale[NUM_WINDOW_GROUPS_MAX];
    /*no3*/
    int sfb_nb_diff60;


    sfb_num  = f1->sfb_num;
    error_energy1 = f1->error_energy;
    error_energy2 = f2->error_energy;


    sfb_nb_diff60     = 0;

    memset(energy_err_ok_cnt, 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(energy_err_ok    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(sfb_scale_cnt    , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);
    memset(sfb_allscale     , 0, sizeof(int)*NUM_WINDOW_GROUPS_MAX);

    /*judge if scalefactor increase or not*/
    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            for(win = 0; win < window_group_length[gr]; win++) {
                if((error_energy1[gr][sfb][win] > f1->xmin[gr][sfb][win]) ||
                   (error_energy2[gr][sfb][win] > f2->xmin[gr][sfb][win])) {
                    scalefactor[gr][sfb] += 1;
                    sfb_scale_cnt[gr]++;
                    break;
                }else {
                    energy_err_ok_cnt[gr]++;
                }
            }
        }
    }

    /*every scalefactor band has good energy distorit;*/
    for(gr = 0; gr < num_window_groups; gr++) {
        if(energy_err_ok_cnt[gr] >= sfb_num*window_group_length[gr])
            energy_err_ok[gr] = 1;
        else 
            energy_err_ok[gr] = 0;

        if(sfb_scale_cnt[gr] >= sfb_num) {
            sfb_allscale[gr] = 1;
            /*recover the scalefactor*/
            for(sfb = 0; sfb < sfb_num; sfb++) {
                scalefactor[gr][sfb] -= 1;
            }
        }
        else
            sfb_allscale[gr] = 0;
    }

    for(gr = 0; gr < num_window_groups; gr++) {
        if((energy_err_ok[gr] == 0) && (sfb_allscale[gr] == 0))
            return 0;
    }

    return 1;

}





int fa_mdctline_quantize(uintptr_t handle, 
                         int num_window_groups, int *window_group_length,
                         int average_bits, int more_bits, int bitres_bits, int maximum_bitreservoir_size, 
                         int *common_scalefac, int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                         int *x_quant, int *unused_bits)
{
    int i;
    float max_mdct_line;
    float abs_mdct_line;
    int used_bits;
    int available_bits;

    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    float *mdct_line = f->mdct_line;
    int mdct_line_num;

    /*calculate available_bits*/
    if(more_bits > 0)
        available_bits = average_bits + FA_MIN(more_bits, bitres_bits);
    else 
        available_bits = average_bits + FA_MAX(more_bits, bitres_bits - maximum_bitreservoir_size);

    /*reset iteration var*/
    max_mdct_line = 0;
    used_bits = 0;

    /*calculate max mdct_line*/
    for(i = 0; i < (f->block_type_cof*f->mdct_line_num); i++) {
        x_quant[i] = 0;  //also reset the x_quant value
        abs_mdct_line = fabsf(mdct_line[i]);
        if(abs_mdct_line > max_mdct_line)
            max_mdct_line = abs_mdct_line;
    }

    /*if all mdct_line are zeros, skip this*/
    if(max_mdct_line > 0) {
        f->common_scalefac = calculate_start_common_scalefac(max_mdct_line);

        iteration_outerloop(f, num_window_groups, window_group_length, available_bits, scalefactor, x_quant, &used_bits);
    }


    *common_scalefac = f->common_scalefac;
    *unused_bits = available_bits - used_bits;

    return 0;
}


int fa_mdctline_iquantize(uintptr_t handle, 
                          int num_window_groups, int *window_group_length,
                          int scalefactor[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX], 
                          int *x_quant)
{
    int i;
    int gr, win;
    int sfb;
    int sfb_num;
    int *swb_low, *swb_high;
    int swb_width;
    float tmp_xq, inv_cof;
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    float *mdct_line = f->mdct_line;
    int mdct_line_offset;
    float inv_x_quant;
    float tmp;

    sfb_num = f->sfb_num;
    swb_low = f->swb_low;
    swb_high = f->swb_high;

    mdct_line_offset = 0;
    for(gr = 0; gr < num_window_groups; gr++) {
        for(sfb = 0; sfb < sfb_num; sfb++) {
            swb_width = swb_high[sfb] - swb_low[sfb] + 1;
            for(win = 0; win < window_group_length[gr]; win++) {
                for(i = 0; i < swb_width; i++) {
                    /*inv_cof = powf(2, 0.25*(common_scalefac - scalefactor[gr][sfb]));*/
                    inv_cof = powf(2, 0.25*(scalefactor[gr][sfb] - SF_OFFSET));
                    tmp_xq = (float)x_quant[mdct_line_offset+i];
                    inv_x_quant = powf(tmp_xq, 4./3.) * inv_cof; 
                    mdct_line[mdct_line_offset+i] = inv_x_quant;
                }
                mdct_line_offset += swb_width;
           }

        }
    }

    return 0;
}

void fa_xmin_sfb_arrange(uintptr_t handle, float xmin_swb[NUM_WINDOW_GROUPS_MAX][NUM_SFB_MAX],
                         int num_window_groups, int *window_group_length)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;
    int sfb_num = f->sfb_num;

    int gr, swb, win;
    int win_offset;

    win_offset = 0;
    for(gr = 0; gr < num_window_groups; gr++) {
        for(swb = 0; swb < sfb_num; swb++) {
            for(win = 0; win < window_group_length[gr]; win++) {
                f->xmin[gr][swb][win] = xmin_swb[win_offset+win][swb];
            }
        }
        win_offset += window_group_length[gr];
    }
}

void fa_mdctline_sfb_arrange(uintptr_t handle, float *mdct_line_swb, 
                             int num_window_groups, int *window_group_length)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    int mdct_line_num = f->mdct_line_num;
    int sfb_num       = f->sfb_num;
    int *swb_low      = f->swb_low;
    int *swb_high     = f->swb_high;

    float *mdct_line_sfb = f->mdct_line;

    int swb_width;
    int group_offset;
    int gr, swb, win, i, k;
    int sfb;
    int index;

    k = 0;
    group_offset = 0;
    /*order rearrage:  swb[gr][win][sfb][k] ---> sfb[gr][sfb][win][k]*/
    for(gr = 0; gr < num_window_groups; gr++) {
        for(swb = 0; swb < sfb_num; swb++) {
            swb_width = swb_high[swb] - swb_low[swb] + 1;
            for(win = 0; win < window_group_length[gr]; win++) {
                for(i = 0; i < swb_width; i++) {
                    index = group_offset + swb_low[swb] + win*mdct_line_num + i;
                    mdct_line_sfb[k++] = mdct_line_swb[index];
                }
            }
        }
        group_offset += mdct_line_num * window_group_length[gr];
    }

    /*calcualte sfb width and sfb_low and sfb_high*/
    group_offset = 0;
    for(gr = 0; gr < num_window_groups; gr++) {
        sfb = 0;
        f->sfb_low[gr][sfb++] = group_offset;
        for(swb = 0; swb < sfb_num; swb++) {
            swb_width           = swb_high[swb]  - swb_low[swb] + 1;
            f->sfb_low[gr][sfb]    = f->sfb_low[gr][sfb-1] + swb_width*window_group_length[gr];
            f->sfb_high[gr][sfb-1] = f->sfb_low[gr][sfb]   - 1;
            sfb++;
        }
        group_offset += mdct_line_num * window_group_length[gr];
    }

}


void fa_mdctline_sfb_iarrange(uintptr_t handle, float *mdct_line_swb, int *mdct_line_sig,
                              int num_window_groups, int *window_group_length)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    int mdct_line_num = f->mdct_line_num;
    int sfb_num       = f->sfb_num;
    int *swb_low      = f->swb_low;
    int *swb_high     = f->swb_high;

    float *mdct_line_sfb = f->mdct_line;

    int swb_width;
    int group_offset;
    int gr, swb, win, i, k;
    int sfb;
    int index;


    k = 0;
    group_offset = 0;

    /*order rearrage:  sfb[gr][sb][win][k] ---> swb[gr][win][sb][k]*/
    for(gr = 0; gr < num_window_groups; gr++) {
        for(swb = 0; swb < sfb_num; swb++) {
            swb_width = swb_high[swb] - swb_low[swb] + 1;
            for(win = 0; win < window_group_length[gr]; win++) {
                for(i = 0; i < swb_width; i++) {
                    index = group_offset + swb_low[swb] + win*mdct_line_num + i;
                    mdct_line_swb[index] = mdct_line_sfb[k++];
                }
            }
        }
        group_offset += mdct_line_num * window_group_length[gr];
    }

    for(i = 0; i < 1024; i++)
        mdct_line_swb[i] = mdct_line_swb[i] * mdct_line_sig[i];

}
