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


  filename: fa_aacstream.h 
  version : v1.0.0
  time    : 2012/08/22 - 2012/10/05 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/


#ifndef _FA_AACSTREAM_H
#define _FA_AACSTREAM_H 

#include "fa_aaccfg.h"
#include "fa_aacenc.h"

#ifdef __cplusplus 
extern "C"
{ 
#endif  

int get_aac_bitreservoir_maxsize(int bit_rate, int sample_rate);

int get_avaiable_bits(int average_bits, int more_bits, int bitres_bits, int bitres_max_size);

int fa_bits_sideinfo_est(int chn_num);

int calculate_bit_allocation(float pe, int block_type);

int fa_bits_count(uintptr_t h_bs, aaccfg_t *c, aacenc_ctx_t *s, aacenc_ctx_t *sr);

int fa_write_bitstream(fa_aacenc_ctx_t *f);

#ifdef __cplusplus 
}
#endif  


#endif
