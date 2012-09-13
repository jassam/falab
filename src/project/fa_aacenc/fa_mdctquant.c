#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

#define MAX_QUANT    8191
#define MAGIC_NUMBER 0.4054

typedef unsigned long uintptr_t;

typedef struct _fa_mdctquant_t {
    int mdct_line_num;
    float *mdct_scaled;

    int sb_num;
    int *sb_low;
    int *sb_high;

    int common_scalefac;

    float *error_energy;
       
}fa_mdctquant_t;

uintptr_t fa_mdctquant_init(int mdct_line_num, int sb_num, int *sb_low)
{
    int sb;

    fa_mdctquant_t *f = (fa_mdctquant_t *)malloc(sizeof(fa_mdctquant_t));
    
    f->mdct_line_num = mdct_line_num;
    f->mdct_scaled = (float *)malloc(sizeof(float)*mdct_line_num);

    f->sb_num = sb_num;
    f->sb_low = (int *)malloc(sizeof(int)*sb_num);
    f->sb_high = (int *)malloc(sizeof(int)*sb_num);
    f->error_energy = (float *)malloc(sizeof(float)*sb_num);

    for(sb = 0; sb < sb_num; sb++) {
        f->sb_low[sb] = sb_low[sb];
        if(sb == sb_num - 1)
            f->sb_high[sb] = mdct_line_num-1;
        else 
            f->sb_high[sb] = sb_low[sb+1] - 1;
    }

    f->common_scalefac = 0;

    return (uintptr_t)f;
}

void      fa_mdctquant_uninit(uintptr_t handle)
{
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;


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
    int mdct_line_num = f->mdct_line_num;

    if(outer_loop_count == 0)
        quantizer_change = 64;
    else 
        quantizer_change = 2;

    do {
        /*quantize spectrum*/
        for(i = 0; i < mdct_line_num; i++) {
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

static int iteration_stop(int energy_err_ok, int sfb_allscale)
{
    if(energy_err_ok)
        return 1;

    if(sfb_allscale)
        return 1;

    return 0;
}

static void iteration_outerloop(fa_mdctquant_t *f,   
                                float *mdct_line, int available_bits, float *xmin,
                                int *scalefactor, int *x_quant, int *used_bits)
{
    int i;
    int sb;

    int sb_num;
    int *sb_low, *sb_high;
    float *mdct_scaled;
    float *error_energy;

    float tmp;
    float xr_pow;
    float cof_scale;

    float inv_x_quant;
    float inv_cof;

    int outer_loop_count;

    /*three break condition variable*/
    /*no1*/
    int energy_err_ok_cnt;
    int energy_err_ok;
    /*no2*/
    int sfb_scale_cnt;
    int sfb_allscale;
    /*no3*/
    int sfb_nb_diff60;


    sb_num = f->sb_num;
    sb_low = f->sb_low;
    sb_high = f->sb_high;
    mdct_scaled = f->mdct_scaled;
    error_energy = f->error_energy;

    outer_loop_count = 0;

    do {

        for(sb = 0; sb < sb_num; sb++) {
            for(i = sb_low[sb]; i <= sb_high[sb]; i++) {
                tmp = fabsf(mdct_line[i]); 
                xr_pow = sqrtf(tmp*sqrtf(tmp));
                cof_scale = powf(2, (3./16.) * scalefactor[sb]);
                mdct_scaled[i] = xr_pow * cof_scale;
            }
        }

        iteration_innerloop(f, outer_loop_count, available_bits,
                            x_quant, used_bits);

        /*calculate scalefactor band error energy*/
        for(sb = 0; sb < sb_num; sb++) {
            float tmp_xq;
            error_energy[sb] = 0;
            for(i = sb_low[sb]; i <= sb_high[sb]; i++) {
                inv_cof = powf(2, 0.25*(f->common_scalefac - scalefactor[sb]));
                tmp_xq = (float)x_quant[i];
                /*inv_x_quant = pow(x_quant[i], 4./3.) * inv_cof; */
                inv_x_quant = powf(tmp_xq, 4./3.) * inv_cof; 
                tmp = fabsf(mdct_line[i]) - inv_x_quant;
                error_energy[sb] = error_energy[sb] + tmp*tmp;  
            }
        }


        energy_err_ok_cnt = 0;
        energy_err_ok     = 0;
        sfb_scale_cnt     = 0;
        sfb_allscale      = 0;
        sfb_nb_diff60     = 0;
        for(sb = 0; sb < sb_num; sb++) {
            if(error_energy[sb] > xmin[sb]) {
                scalefactor[sb] = scalefactor[sb] + 1;
                sfb_scale_cnt++;
            }else {
                energy_err_ok_cnt++;
            }
        }
        /*every scalefactor band has good energy distorit;*/
        if(energy_err_ok_cnt >= sb_num)
            energy_err_ok = 1;
        else
            energy_err_ok = 0;

        if(sfb_scale_cnt >= sb_num)
            sfb_allscale = 1;

        outer_loop_count++;
    }while(!iteration_stop(energy_err_ok, sfb_allscale));

#if  0  
    for(sb = 0; sb < sb_num; sb++) {
        printf("%d,",scalefactor[sb]);
    }
    printf("\n--------------------\n\n");
#endif

}


int mdctline_quantize(uintptr_t handle,
                      float *mdct_line, float *xmin,
                      int average_bits, int more_bits, int bitres_bits, int maximum_bitreservoir_size, 
                      int *common_scalefac, int *scalefactor, int *x_quant, int *unused_bits)
{
    int i;
    float max_mdct_line;
    float abs_mdct_line;
    int used_bits;
    int available_bits;

    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;


    /*calculate available_bits*/
    if(more_bits > 0)
        available_bits = average_bits + FA_MIN(more_bits, bitres_bits);
    else 
        available_bits = average_bits + FA_MAX(more_bits, bitres_bits - maximum_bitreservoir_size);

    /*reset iteration var*/
    max_mdct_line = 0;
    used_bits = 0;

    /*calculate max mdct_line*/
    for(i = 0; i < f->mdct_line_num; i++) {
        x_quant[i] = 0;  //also reset the x_quant value
        abs_mdct_line = fabsf(mdct_line[i]);
        if(abs_mdct_line > max_mdct_line)
            max_mdct_line = abs_mdct_line;
    }

    /*if all mdct_line are zeros, skip this*/
    if(max_mdct_line > 0) {
        f->common_scalefac = calculate_start_common_scalefac(max_mdct_line);

        iteration_outerloop(f, mdct_line, available_bits, xmin, scalefactor, x_quant, &used_bits);
    }

    *common_scalefac = f->common_scalefac;
    *unused_bits = available_bits - used_bits;

    return 0;
}

int mdctline_iquantize(uintptr_t handle, int common_scalefac, int *scalefactor, 
                       int *x_quant, float *mdct_line)
{
    int i;
    int sb;
    int sb_num;
    int *sb_low, *sb_high;
    float tmp_xq, inv_cof;
    fa_mdctquant_t *f = (fa_mdctquant_t *)handle;

    sb_num = f->sb_num;
    sb_low = f->sb_low;
    sb_high = f->sb_high;
 

    for(sb = 0; sb < sb_num; sb++) {
        for(i = sb_low[sb]; i <= sb_high[sb]; i++) {
            inv_cof = powf(2, 0.25*(common_scalefac - scalefactor[sb]));
            tmp_xq = (float)x_quant[i];
            mdct_line[i] = powf(tmp_xq, 4./3.) * inv_cof; 
        }
    }

}
