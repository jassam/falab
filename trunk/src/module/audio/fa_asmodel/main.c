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

  this main.c is the a good study tools to let you know how the fir 
  filter working, and how we use these filter to process signal.
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_wavfmt.h"
#include "fa_parseopt.h"
#include "fa_asmodel.h"

/*#define USE_AS_FFT*/
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

    uintptr_t h_analysis, h_synthesis;

	short wavsamples_in[FRAME_SIZE_MAX];
	short wavsamples_out[FRAME_SIZE_MAX];
	float buf_in[FRAME_SIZE_MAX];
    float buf_out[FRAME_SIZE_MAX];
    float re[FRAME_SIZE_MAX+1], im[FRAME_SIZE_MAX+1];
    float xf[FRAME_SIZE_MAX];


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

#ifdef USE_AS_FFT
    h_analysis  = fa_analysis_fft_init(opt_overlap, opt_framelen, opt_fftwintype);
    h_synthesis = fa_synthesis_fft_init(opt_overlap, opt_framelen, opt_fftwintype);
#else
    h_analysis  = fa_analysis_mdct_init(opt_framelen, opt_mdctwintype);
    h_synthesis = fa_synthesis_mdct_init(opt_framelen, opt_mdctwintype);
#endif

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
		
#ifdef USE_AS_FFT
        fa_analysis_fft(h_analysis, buf_in, re, im);
        fa_synthesis_fft(h_synthesis, re, im, buf_out);
#else
        fa_analysis_mdct(h_analysis, buf_in, xf);
        fa_synthesis_mdct(h_synthesis, xf, buf_out);
#endif

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

#ifdef USE_AS_FFT
    fa_analysis_fft_uninit(h_analysis);
    fa_synthesis_fft_uninit(h_synthesis);
#else 
    fa_analysis_mdct_uninit(h_analysis);
    fa_synthesis_mdct_uninit(h_synthesis);
#endif

    printf("\n");

    return 0;
}
