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


  filename: main.c 
  version : v1.0.0
  time    : 2012/07/22 23:43
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


/*
TODO:
   TASK                                STATUS                SUPPORT 
    ------------------------------------------------------------------
    mono                               complete               yes
    stereo(common_window=0)            complete               yes
    stereo(ms)                         doing                  no 
    lfe                                no schecdule           no(easy, need test)
    high frequency optimize            doing 
    bitrate control fixed              doing 
    TNS                                no schecdule           no(I think no useless, waste time, no need support)
    LTP                                no schecdule           no(very slow, no need support)
    add fast xmin/pe caculate method   doing                 
    add new quantize fast method       doing 
    optimize the speed performance     doing 
     (maybe not use psy)
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_aacenc.h"
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
    FILE  * aacfile;
	fa_wavfmt_t fmt;
    int sample_rate;
    int chn_num;

	int i;
    int k;
    int is_last = 0;
    int read_len = 0;
    int write_total_size= 0;

    uintptr_t h_aacenc;

    uintptr_t  h_aac_synthesis;
    uintptr_t h_mdctiq_long, h_mdctiq_short;
    int block_type;

    int average_bits, more_bits, bitres_bits, maximum_bitreservoir_size; 
    int unused_bits;

    int num_window_groups;
    int window_group_length[8];

	short wavsamples_in[FRAME_SIZE_MAX];
	short wavsamples_out[FRAME_SIZE_MAX];
	float buf_in[FRAME_SIZE_MAX];
    float buf_out[FRAME_SIZE_MAX];
    float mdct_line_inv[FRAME_SIZE_MAX];

    unsigned char aac_buf[FRAME_SIZE_MAX];
    int aac_out_len;

    int ms_enable = MS_DEFAULT;
    int lfe_enable = LFE_DEFAULT;
    int tns_enable = TNS_DEFAULT;
    int block_switch_enable = BLOCK_SWITCH_DEFAULT;

    fa_aacenc_ctx_t *f;

    ret = fa_parseopt(argc, argv);
    if(ret) return -1;

    if ((destfile = fopen(opt_outputfile, "w+b")) == NULL) {
		printf("output file can not be opened\n");
		return 0; 
	}                         

    if ((aacfile = fopen("outaac.aac", "w+b")) == NULL) {
		printf("output file can not be opened\n");
		return 0; 
	}                         

	if ((sourcefile = fopen(opt_inputfile, "rb")) == NULL) {
		printf("input file can not be opened;\n");
		return 0; 
    }

    fmt = fa_wavfmt_readheader(sourcefile);
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);
    printf("\n\nsamplerate=%d\n", fmt.samplerate);

    sample_rate = fmt.samplerate;
    chn_num     = fmt.channels;

    h_aacenc = fa_aacenc_init(sample_rate, 96000, chn_num,
                              2, LOW, 
                              ms_enable, lfe_enable, tns_enable, block_switch_enable);


    h_aac_synthesis = fa_aacfilterbank_init(block_switch_enable);

    switch(fmt.samplerate) {
        case 48000:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset, 1);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset, 8);
            break;
        case 44100:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset, 1);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset, 8);
            break;
        case 32000:
            h_mdctiq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset, 1);
            h_mdctiq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset, 8);
            break;
    }


    while(1)
    {
        if(is_last)
            break;

        memset(wavsamples_in, 0, 2*opt_framelen*chn_num);
        read_len = fread(wavsamples_in, 2, opt_framelen*chn_num, sourcefile);
        if(read_len < (opt_framelen*chn_num))
            is_last = 1;
       
        for(i = 0 ; i < read_len; i++) {
            buf_in[i] = (float)wavsamples_in[i];
            buf_out[i] = 0;
        }

        /*analysis and encode*/
        fa_aacenc_encode(h_aacenc, wavsamples_in, chn_num*2*read_len, aac_buf, &aac_out_len);
        fwrite(aac_buf, 1, aac_out_len, aacfile);

        f = (fa_aacenc_ctx_t *)h_aacenc;

        /*synthesis*/
        memset(mdct_line_inv, 0, FRAME_SIZE_MAX*sizeof(float));
        if(f->ctx[0].block_type == ONLY_SHORT_BLOCK) {
            num_window_groups = 1;
            window_group_length[0] = 8;
            window_group_length[1] = 0;
            window_group_length[2] = 0;
            window_group_length[3] = 0;
            window_group_length[4] = 0;
            window_group_length[5] = 0;
            window_group_length[6] = 0;
            window_group_length[7] = 0;
 
            fa_mdctline_iquantize(h_mdctiq_short, 
                                  num_window_groups, window_group_length, 
                                  f->ctx[0].scalefactor,
                                  f->ctx[0].x_quant);
            fa_mdctline_sfb_iarrange(h_mdctiq_short, mdct_line_inv, f->ctx[0].mdct_line_sign,
                                     num_window_groups, window_group_length);
        }else {
            num_window_groups = 1;
            window_group_length[0] = 1;
            fa_mdctline_iquantize(h_mdctiq_long, 
                                  num_window_groups, window_group_length, 
                                  f->ctx[0].scalefactor,
                                  f->ctx[0].x_quant);
            fa_mdctline_sfb_iarrange(h_mdctiq_long, mdct_line_inv,f->ctx[0].mdct_line_sign,
                                     num_window_groups, window_group_length);
        }

        if(f->block_switch_en) {
            block_type = fa_get_aacblocktype(f->ctx[0].h_aac_analysis);
            fa_set_aacblocktype(h_aac_synthesis, block_type);
        }else {
            block_type = ONLY_LONG_BLOCK;
        }

        fa_aacfilterbank_synthesis(h_aac_synthesis, mdct_line_inv, buf_out);

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
        if (frame_index == 30) {
            i=i+1;
        }
        fprintf(stderr,"\rthe frame = [%d]", frame_index);
    }

    fmt.data_size=write_total_size/fmt.block_align;
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt,destfile);

    fclose(sourcefile);
    fclose(destfile);

    printf("\n");

    return 0;
}
