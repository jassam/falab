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
    mono                               done                   yes
    stereo(common_window=0)            done                   yes
    stereo(ms)                         done                   yes 
    lfe                                no schecdule           no(easy, need test)
    high frequency optimize            done                   yes(bandwith limited now)
    bitrate control fixed              done                   yes(constant bitrate CBR is OK) 
    TNS                                done                   yes(not very important, little influence only for the strong hit audio point) 
    LTP                                no schecdule           no(very slow, no need support)
    add fast xmin/pe caculate method   done                 
    add new quantize fast method       done 
    optimize the speed performance     done 
     (maybe not use psy)
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "fa_aacenc.h"
#include "fa_wavfmt.h"
#include "fa_parseopt.h"
#include "fa_timeprofile.h"

#define FRAME_SIZE_MAX  2048 

int main(int argc, char *argv[])
{
    int ret;
    int frame_index = 0;

	FILE  * destfile;
	FILE  * sourcefile;
	fa_wavfmt_t fmt;
    int sample_rate;
    int chn_num;

    int is_last = 0;
    int read_len = 0;

    uintptr_t h_aacenc;

	short wavsamples_in[FRAME_SIZE_MAX];
    unsigned char aac_buf[FRAME_SIZE_MAX];
    int aac_out_len;

    int lfe_enable = 0;

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
    printf("\n\nsamplerate = %lu\n", fmt.samplerate);

    sample_rate = fmt.samplerate;
    chn_num     = fmt.channels;

    h_aacenc = fa_aacenc_init(sample_rate, opt_bitrate, chn_num,
                              2, LOW, lfe_enable,
                              opt_bandwidth,
                              opt_speedlevel);
    if (!h_aacenc) {
        printf("initial failed, maybe configuration is not proper!\n");
        return -1;
    }

    FA_CLOCK_START(1);

    while(1)
    {
        if(is_last)
            break;

        memset(wavsamples_in, 0, 2*1024*chn_num);
        read_len = fread(wavsamples_in, 2, 1024*chn_num, sourcefile);
        if(read_len < (1024*chn_num))
            is_last = 1;
       
        /*analysis and encode*/
        fa_aacenc_encode(h_aacenc, (unsigned char *)wavsamples_in, chn_num*2*read_len, aac_buf, &aac_out_len);

        fwrite(aac_buf, 1, aac_out_len, destfile);

        frame_index++;
        fprintf(stderr,"\rthe frame = [%d]", frame_index);
    }

    FA_CLOCK_END(1);
    FA_CLOCK_COST(1);

    fclose(sourcefile);
    fclose(destfile);


    printf("\n");

    FA_GET_TIME_COST(1);
    /*FA_GET_TIME_COST(2);*/
    /*FA_GET_TIME_COST(3);*/
    /*FA_GET_TIME_COST(4);*/
    /*FA_GET_TIME_COST(5);*/
    /*FA_GET_TIME_COST(6);*/

    return 0;
}
