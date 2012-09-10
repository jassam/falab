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
#include "fa_aacpsy.h"
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
    int is_last = 0;
    int read_len = 0;
    int write_total_size= 0;

    uintptr_t h_aac_analysis, h_aac_synthesis;
    uintptr_t h_aacpsy;
    int block_type;
    float pe;

	short wavsamples_in[FRAME_SIZE_MAX];
	short wavsamples_out[FRAME_SIZE_MAX];
	float buf_in[FRAME_SIZE_MAX];
    float buf_out[FRAME_SIZE_MAX];
    float mdct_line[FRAME_SIZE_MAX];

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
    fseek(sourcefile,46,0);
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);

    h_aacpsy = fa_aacpsy_init(48000);
    h_aac_analysis = fa_aacfilterbank_init(block_switch_en);
    h_aac_synthesis = fa_aacfilterbank_init(block_switch_en);


    while(1)
    {
        if(is_last)
            break;

        memset(wavsamples_in, 0, 2*opt_framelen);
        read_len = fread(wavsamples_in, 2, opt_framelen, sourcefile);
        if(read_len < opt_framelen)
            is_last = 1;
       
        for(i = 0 ; i < read_len; i++) {
            buf_in[i] = (float)wavsamples_in[i]/32768;
            buf_out[i] = 0;
        }
        
        if(block_switch_en) {
            block_type = fa_get_aacblocktype(h_aac_analysis);
            pe = fa_aacpsy_calculate_pe(h_aacpsy, wavsamples_in, block_type);
            fa_aacblocktype_switch(h_aac_analysis, h_aacpsy, pe);
        }

        fa_aacfilterbank_analysis(h_aac_analysis, buf_in, mdct_line);

        if(block_switch_en) {
            block_type = fa_get_aacblocktype(h_aac_analysis);
            fa_set_aacblocktype(h_aac_synthesis, block_type);
        }
        fa_aacfilterbank_synthesis(h_aac_synthesis, mdct_line, buf_out);

        for(i = 0 ; i < opt_framelen; i++) {
            float temp;
            temp = buf_out[i] * 32768;

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
