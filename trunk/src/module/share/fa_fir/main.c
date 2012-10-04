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
  time    : 2012/07/10 23:42 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

  this main.c is the a good study tools to let you know how the fir 
  filter working, and how we use these filter to process signal.
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_wavfmt.h"
#include "fa_parseopt.h"
#include "fa_fir.h"

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

    uintptr_t  h_flt;

	short wavsamples_in[FRAME_SIZE_MAX];
	short wavsamples_out[FRAME_SIZE_MAX];
	float buf_in[FRAME_SIZE_MAX];
    float buf_out[FRAME_SIZE_MAX];


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
    fseek(destfile  ,  0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);

    /*init filter*/
    switch(opt_firtype) {
        case FA_FIR_LPF:
            h_flt = fa_fir_filter_lpf_init(opt_framelen, opt_winlen, opt_fc, opt_wintype);
            break;
        case FA_FIR_HPF:
            h_flt = fa_fir_filter_hpf_init(opt_framelen, opt_winlen, opt_fc, opt_wintype);
            break;
        case FA_FIR_BANDPASS:
            h_flt = fa_fir_filter_bandpass_init(opt_framelen, opt_winlen, opt_fc1, opt_fc2, opt_wintype);
            break;
        case FA_FIR_BANDSTOP:
            h_flt = fa_fir_filter_bandstop_init(opt_framelen, opt_winlen, opt_fc1, opt_fc2, opt_wintype);
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
			buf_in[i] = (float)wavsamples_in[i]/32768;
			buf_out[i] = 0;
		}
		
		fa_fir_filter(h_flt, buf_in, buf_out, read_len);

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
        printf("the frame = [%d]\r", frame_index);
    }

    ret = fa_fir_filter_flush(h_flt, buf_out);
    for(i = 0 ; i < ret; i++) {
        float temp;
        temp = buf_out[i] * 32768;

        if (temp >= 32767)
            temp = 32767;
        if (temp < -32768)
            temp = -32768;

        wavsamples_out[i] = temp;
    }

    fwrite(wavsamples_out, 2, ret, destfile);

    write_total_size += 2*ret;

    frame_index++;
    printf("the frame = [%d]\n", frame_index);

    fmt.data_size=write_total_size/fmt.block_align;
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt,destfile);

	fa_fir_filter_uninit(h_flt);
    fclose(sourcefile);
    fclose(destfile);

    return 0;
}
