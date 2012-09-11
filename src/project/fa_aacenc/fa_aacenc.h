#ifndef _FA_AACENC_H
#define _FA_AACENC_H 

#include "fa_aaccfg.h"
#include "fa_aacchn.h"


typedef struct _fa_aacenc_ctx_t{
    //the configuration of aac encoder
    fa_aaccfg_t cfg;
    chn_info_t  chn_info[MAX_CHANNELS];

    //the coding status variable 
    int sample_rate_index;

    int window_shape;

    int block_type;

    int global_gain;

    int scale_factor[MAX_SCFAC_BANDS];

    int num_window_groups;

    int window_group_length[8];

    int max_sfb;

    int nr_of_sfb;

    int sfb_offset[250];

    int used_bytes;

    unsigned long h_bitstream;

}fa_aacenc_ctx_t;



#endif
