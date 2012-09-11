/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi (Chengdu, China)

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


  filename: main.c 
  version : v1.0.0
  time    : 2012/07/22 23:43
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )

  this main.c is the a good study tools to let you know how the fir 
  filter working, and how we use these filter to process signal.
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_wavfmt.h"
#include "fa_parseopt.h"
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_aacfilterbank.h"

#define FRAME_SIZE_MAX  2048 

int main(int argc, char *argv[])
{
    int ret;
    int frame_index = 0;

	FILE  * destfile;
	FILE  * sourcefile;
	fa_wavfmt_t fmt;

	int i;
    int k;
    int is_last = 0;
    int read_len = 0;
    int write_total_size= 0;

    uintptr_t h_aac_analysis, h_aac_synthesis;
    uintptr_t h_aacpsy;
    uintptr_t h_mdctq_long, h_mdctq_short;
    uintptr_t h_mdctiq_long, h_mdctiq_short;
    int block_type;
    float pe;

    float xmin[1024];
    int average_bits, more_bits, bitres_bits, maximum_bitreservoir_size; 
    int common_scalefac_long;
    int common_scalefac_short[8];
    int scalefactor_long[FA_SWB_NUM_MAX];
    int scalefactor_short[8][FA_SWB_NUM_MAX];
    int x_quant[1024];
    int mdct_ling_sig[1024];
    int unused_bits;

	short wavsamples_in[FRAME_SIZE_MAX];
	short wavsamples_out[FRAME_SIZE_MAX];
	float buf_in[FRAME_SIZE_MAX];
    float buf_out[FRAME_SIZE_MAX];
    float mdct_line[FRAME_SIZE_MAX];
    float mdct_line_inv[FRAME_SIZE_MAX];

    int block_switch_en = 1;

    ret = fa_parseopt(argc, argv);
    if(ret) return -1;

    if ((destfile = fopen(opt_outputfile, "w+b")) == NULL) {
		printf("output file can not be opened\n");
		return 0; 
	}                         

	if ((sourcefile = fopen(opt_inputfile, "rb")) == NULL) {
		printf("input file can not be opened;\n");
		return 0; 
    }

    fmt = fa_wavfmt_readheader(sourcefile);
    fseek(sourcefile,44,0);
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);
    printf("\n\nsamplerate=%d\n", fmt.samplerate);

    h_aacpsy = fa_aacpsy_init(fmt.samplerate);
    h_aac_analysis = fa_aacfilterbank_init(block_switch_en);
    h_aac_synthesis = fa_aacfilterbank_init(block_switch_en);

    switch(fmt.samplerate) {
        case 48000:
            h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset);
            h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset);
            break;
        case 44100:
            h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset);
            h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset);
            break;
        case 32000:
            h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset);
            h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset);
            break;
    }

    switch(fmt.samplerate) {
        case 48000:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset);
            break;
        case 44100:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset);
            break;
        case 32000:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset);
            break;
    }



    while(1)
    {
        if(is_last)
            break;

        memset(wavsamples_in, 0, 2*opt_framelen);
        read_len = fread(wavsamples_in, 2, opt_framelen, sourcefile);
        if(read_len < opt_framelen)
            is_last = 1;
       
        for(i = 0 ; i < read_len; i++) {
            buf_in[i] = (float)wavsamples_in[i];
            buf_out[i] = 0;
        }
       
        /*analysis*/
        if(block_switch_en) {
            block_type = fa_get_aacblocktype(h_aac_analysis);
            fa_aacpsy_calculate_pe(h_aacpsy, buf_in, block_type, &pe);
            /*printf("block_type=%d, pe=%f\n", block_type, pe);*/
            fa_aacblocktype_switch(h_aac_analysis, h_aacpsy, pe);
        }else {
            block_type = ONLY_LONG_BLOCK;
        }

        fa_aacfilterbank_analysis(h_aac_analysis, buf_in, mdct_line);
        fa_aacpsy_calculate_xmin(h_aacpsy, mdct_line, block_type, xmin);

        if(block_type == ONLY_SHORT_BLOCK) {
            memset(scalefactor_short, 0, 8*FA_SWB_NUM_MAX*sizeof(int));
            for(k = 0; k < 8; k++) {
                mdctline_quantize(h_mdctq_short,
                                  mdct_line+k*AAC_BLOCK_SHORT_LEN, xmin+k*AAC_BLOCK_SHORT_LEN,
                                  0, 0, 0, 0, 
                                  &(common_scalefac_short[k]), scalefactor_short[k], x_quant+k*AAC_BLOCK_SHORT_LEN, &unused_bits);
            }
        }else {
            memset(scalefactor_long, 0, FA_SWB_NUM_MAX*sizeof(int));
            mdctline_quantize(h_mdctq_long,
                              mdct_line, xmin,
                              0, 0, 0, 0, 
                              &common_scalefac_long, scalefactor_long, x_quant, &unused_bits);
        }

        for(i = 0; i < 1024; i++) {
            if(mdct_line[i] >= 0)
                mdct_ling_sig[i] = 1;
            else
                mdct_ling_sig[i] = -1;
        }
        /*synthesis*/
#if 1 
        memset(mdct_line_inv, 0, FRAME_SIZE_MAX*sizeof(float));
        if(block_type == ONLY_SHORT_BLOCK) {
            for(k = 0; k < 8; k++) {
                mdctline_iquantize(h_mdctq_short, common_scalefac_short[k], scalefactor_short[k],
                                   x_quant+k*AAC_BLOCK_SHORT_LEN, mdct_line_inv+k*AAC_BLOCK_SHORT_LEN);
            }
        }else {
            mdctline_iquantize(h_mdctq_long, common_scalefac_long, scalefactor_long,
                               x_quant, mdct_line_inv);
        }
        for(i = 0; i < 1024; i++) {
            mdct_line[i] = mdct_ling_sig[i] * mdct_line_inv[i];
        }
#endif

        if(block_switch_en) {
            block_type = fa_get_aacblocktype(h_aac_analysis);
            fa_set_aacblocktype(h_aac_synthesis, block_type);
        }else {
            block_type = ONLY_LONG_BLOCK;
        }
        fa_aacfilterbank_synthesis(h_aac_synthesis, mdct_line, buf_out);

        for(i = 0 ; i < opt_framelen; i++) {
            float temp;
            temp = buf_out[i];

            if (temp >= 32767)
                temp = 32767;
            if (temp < -32768)
                temp = -32768;

            wavsamples_out[i] = temp;
        }

        fwrite(wavsamples_out, 2, opt_framelen, destfile);

        write_total_size += 2 * opt_framelen;

        frame_index++;
        fprintf(stderr,"\rthe frame = [%d]", frame_index);
    }

    fmt.data_size=write_total_size/fmt.block_align;
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt,destfile);

    fclose(sourcefile);
    fclose(destfile);

    /*fa_aacfilterbank_uninit();*/
    printf("\n");

    return 0;
}
