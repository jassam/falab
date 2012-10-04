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
#include "fa_psychomodel1.h"

#define FRAME_SIZE_MAX  2048 

#define FRAME_LEN 512
uintptr_t h_fft;
uintptr_t h_psy;
float gthres[FRAME_LEN];
float fft_buf[2*FRAME_LEN];
float win[FRAME_LEN];

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
    fseek(sourcefile,46,0);
    fseek(destfile, 0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);

    /*init*/
    h_fft = fa_fft_init(FRAME_LEN);
    fa_hamming(win, FRAME_LEN);
    h_psy = fa_psychomodel1_init(fmt.samplerate, FRAME_LEN);
    opt_framelen = FRAME_LEN;

    while(1)
    {
        if(is_last)
            break;

        memset(wavsamples_in, 0, 2*opt_framelen);
        read_len = fread(wavsamples_in, 2, opt_framelen, sourcefile);
        if(read_len < opt_framelen)
            is_last = 1;
       
		for(i = 0 ; i < read_len; i++) {
			buf_in[i] = (float)win[i]*wavsamples_in[i]/32768.;
            fft_buf[i+i] = buf_in[i];
            fft_buf[i+i+1] = 0;
			buf_out[i] = 0;
		}

        fa_fft(h_fft, fft_buf);

        fa_psy_global_threshold(h_psy, fft_buf, gthres);

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

    printf("\n");

    return 0;
}
