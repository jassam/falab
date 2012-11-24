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


  filename: fa_aacapi.h 
  version : v1.0.0
  time    : 2012/11/24 17:58 
  author  : luolongzhi ( falab2012@gmail.com luolongzhi@gmail.com )
  code URL: http://code.google.com/p/falab/

*/



#ifndef _FA_AACAPI_H
#define _FA_AACAPI_H 

#ifdef __cplusplus 
extern "C"
{ 
#endif  


typedef unsigned uintptr_t;

#define FA_AACENC_MPEG_VER_DEF   1
#define FA_AACENC_OBJ_TYPE_DEF   2

/*
    For you attention:

    1.sample rate only support 32, 44.1, 48kHz(I think enough, high is no useless, low sample rate sound terrible),
      if you want to support more sample rate, complete fa_swbtab and add code in fa_aacenc_init by yourself, very easy

    2.bit rate can support 16~160 per channel, means that if encode stereo audio, can support (32 ~320kbps)
      stereo : I tested, >=128kbps (noloss, can not hear difference between aac and flac or wav file)
                           96kbps  (almost noloss, for some certain audio. high frequency maybe not very good)
               I didn't limit the bandwidth for the high bitrate, >=96kbps, I always use 20kHz audio band width
               you can use (-w) option for your certain background
      you can download the EBU test audio file frome the web site below to do sound quality test:
      http://tech.ebu.ch/public-cations/sqamcd

    3.64 chn can support, lfe support (need you test, lfe I didn't test more)

    4.now only support mpeg_version 2(mpeg2aac), sbr and ps not support now, is developing
        MPEG2       1
        MPEG4       0
      and only ADTS format support 

    5.aac_objtype only support MAIN and LC, and ltp of MAIN can not support(useless, very slow and influece quality little) , SSR is not support 
        MAIN        1
        LOW         2
        SSR         3
        LTP         4

    6.ms encode is support, but if you use fast quantize method(according to you speed_level choice), I close ms encode

    7.band_width you can change by yourself, I use 20kHz in high bitrate(>=96kbps) comprare with FAAC which only use 10kHz when bitrate <=96kbps,
      but if the audio polluted by white noise(sound little) and not very pure, in 96kbps the high frequency can sound a little quantize noise caused
      by white noise quantiztion, at this time, you can use (-w 10) option to limit the band width, and the high frequency can be removed, and the 
      sound quality recoverd better 
    
    8.I give 6 speed level(1~6), you can choose according to your application enviroment, 1 is the lowest but good quality, 6 is fatest but low quality,
      default is 2(strongly recommend), I think 2 is the good choice(good quality, the speed also is fast I think). if you want to use fast version, 
      use 5 choice, for speed_level 6 I limit the band_width to 10kHz in high bit rate.

    9.when you use speed_level 1, the tns option is enable, and if the audio is the change fastly signal(not stable signal), and the attack signal 
      frequently happen, then I suggest you use (-w 15) to limit the high frequency if you use high bit rate, because the attack will lead pre-echo 
      noise spread in time domain, and the high frequency band will be heared like(sa sa sa) sound. and at this time, use TNS will highlight the 
      attack and surppress the "sa sa sa" sound, but when do quantize, the sa sa sa will not good be quantized, so I suggeset use (-w 15) to mannally 
      suprress the high frequency, worsely, use (-w 10) can totally suppress the "sa sa sa " pre-echo and quantize noise
      Or, you can use 128kbps to encode the audio file polluted by white noise, in 128kbps, quantize noise can be masked
*/
uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, int lfe_enable,
                         int band_width,
                         int speed_level);

void fa_aacenc_uninit(uintptr_t handle);

//WARN: the inlen must be 1024*chn_num*2 (2 means 2 bytes for per sample, so your sample should be 16 bits short type), 
void fa_aacenc_encode(uintptr_t handle, unsigned char *buf_in, int inlen, unsigned char *buf_out, int *outlen);


#ifdef __cplusplus 
}
#endif  



#endif
