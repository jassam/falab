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

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_wavfmt.h"
#include "fa_parseopt.h"
#include "fa_fir.h"
#include "fa_resample.h"

int main(int argc, char *argv[])
{
    int ret;
    int frame_index = 0;

	FILE  * destfile;
	FILE  * sourcefile;
	fa_wavfmt_t fmt;
    int samplerate_in;
    int samplerate_out;

    int is_last          = 0;
    int in_len_bytes     = 0;
    int out_len_bytes    = 0;
    int read_len         = 0;
    int write_total_size = 0;

    uintptr_t h_resflt;

	short wavsamples_in[FA_FRAMELEN_MAX];
	short wavsamples_out[FA_FRAMELEN_MAX];
    unsigned char * p_wavin  = (unsigned char *)wavsamples_in;
    unsigned char * p_wavout = (unsigned char *)wavsamples_out;


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
    samplerate_in = fmt.samplerate;
    fseek(sourcefile,44,0);

    /*init filter*/
    switch(opt_type) {
        case FA_DECIMATE:
            h_resflt = fa_decimate_init(opt_downfactor, opt_gain, BLACKMAN);
            samplerate_out = samplerate_in / opt_downfactor;  
            break;
        case FA_INTERP:
            h_resflt = fa_interp_init(opt_upfactor, opt_gain, BLACKMAN);
            samplerate_out = samplerate_in * opt_upfactor;
            break;
        case FA_RESAMPLE:
            h_resflt = fa_resample_filter_init(opt_upfactor, opt_downfactor, opt_gain, BLACKMAN);
            samplerate_out = (samplerate_in * opt_upfactor)/opt_downfactor;
            break;
    }

    if(ret) {
        printf(" not support! ");
        return -1;
    }

    fseek(destfile  ,  0, SEEK_SET);
    fmt.samplerate = samplerate_out;
    fa_wavfmt_writeheader(fmt, destfile);

    in_len_bytes = fa_get_resample_framelen_bytes(h_resflt);

    while(1)
    {
        if(is_last)
            break;

        memset(p_wavin, 0, in_len_bytes);
        read_len = fread(p_wavin, 1, in_len_bytes, sourcefile);
        if(read_len < in_len_bytes)
            is_last = 1;

        switch(opt_type) {
            case FA_DECIMATE:
                fa_decimate(h_resflt, p_wavin, in_len_bytes, p_wavout, &out_len_bytes);
                break;
            case FA_INTERP:
                fa_interp(h_resflt, p_wavin, in_len_bytes, p_wavout, &out_len_bytes);
                break;
            case FA_RESAMPLE:
                fa_resample(h_resflt, p_wavin, in_len_bytes, p_wavout, &out_len_bytes);
                break;
        }


        fwrite(p_wavout, 1, out_len_bytes, destfile);
        write_total_size += out_len_bytes;

        frame_index++;
        printf("the frame = [%d]\r", frame_index);
    }

    fmt.data_size = write_total_size/fmt.block_align;
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt,destfile);

    fa_resample_filter_uninit(h_resflt);
    fclose(sourcefile);
    fclose(destfile);

    return 0;
}
