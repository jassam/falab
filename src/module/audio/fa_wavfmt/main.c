/*
  falab - free algorithm lab 
  Copyright (C) 2012 luolongzhi 罗龙智(Chengdu, China)

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
  time    : 2012/07/08 20:33 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fa_wavfmt.h"
#include "fa_parseopt.h"

#define BUF_MAX_SIZE  8192

int main(int argc, char *argv[])
{
    int ret;
    int is_last = 0;
    int read_len = 0;
    int read_total_size = 0;
		
	unsigned char buf[BUF_MAX_SIZE];
	fa_wavfmt_t fmt;

	FILE  * destfile;
	FILE  * sourcefile;

    ret = fa_parseopt(argc, argv);
    if(ret < 0) return -1;

    if ((destfile   = fopen(opt_outputfile, "w+b")) == NULL) {
		printf("FAIL: output file can not be opened\n");
		return -1; 
	}                         

	if ((sourcefile = fopen(opt_inputfile, "rb"))   == NULL) {
		printf("FAIL: input file can not be opened;\n");
		return -1; 
    }

    fmt = fa_wavfmt_readheader(sourcefile);

    printf("NOTE: the sourcefile fmt is below:\n");
    printf("NOTE: wav fmt         : %u\n"  , fmt.format);
    printf("NOTE: total file size : %lu\n" , fmt.data_size*fmt.block_align+36);
    printf("NOTE: channels        : %u\n"  , fmt.channels);
    printf("NOTE: samplerate      : %lu\n" , fmt.samplerate);
    printf("NOTE: bytes per sample: %u\n"  , fmt.bytes_per_sample);
    printf("NOTE: block align     : %u\n"  , fmt.block_align);
    printf("NOTE: wav data size   : %lu\n" , fmt.data_size);


    fseek(sourcefile, 44, SEEK_SET);
    fseek(destfile  ,  0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);

    while(1)
    {
        if(is_last)
            break;

        read_len = fread(buf, 1, opt_framelen, sourcefile); 
        if(read_len < opt_framelen)
            is_last = 1;

        read_total_size += read_len;
        fwrite(buf, 1, read_len, destfile);
    }

    fmt.data_size = read_total_size / fmt.block_align;
    
    fseek(destfile  ,  0, SEEK_SET);
    fa_wavfmt_writeheader(fmt, destfile);

    fclose(sourcefile);
    fclose(destfile);

    printf("SUCC: copy wav fmt data successfully\n");

    return 0;
}
