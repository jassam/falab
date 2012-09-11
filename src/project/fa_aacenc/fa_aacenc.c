#include <stdio.h>
#include <stdlib.h>
#include "fa_aacenc.h"
#include "fa_aaccfg.h"
#include "fa_aacpsy.h"
#include "fa_swbtab.h"
#include "fa_aacfilterbank.h"



/* Returns the sample rate index */
int get_samplerate_index(int sample_rate)
{
    if (92017 <= sample_rate) return 0;
    if (75132 <= sample_rate) return 1;
    if (55426 <= sample_rate) return 2;
    if (46009 <= sample_rate) return 3;
    if (37566 <= sample_rate) return 4;
    if (27713 <= sample_rate) return 5;
    if (23004 <= sample_rate) return 6;
    if (18783 <= sample_rate) return 7;
    if (13856 <= sample_rate) return 8;
    if (11502 <= sample_rate) return 9;
    if (9391  <= sample_rate) return 10;

    return 11;
}


/* Max prediction band for backward predictionas function of fs index */
const int _max_pred_sfb[] = { 33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34, 0 };

int get_max_pred_sfb(int sample_rate_index)
{
    return _max_pred_sfb[sample_rate_index];
}



uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, 
                         int ms_enable, int lfe_enable, int tns_enable, int block_switch_enable)
{
    int i;
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)malloc(sizeof(fa_aacenc_ctx_t));

    f->cfg.sample_rate   = sample_rate;
    f->cfg.bit_rate      = bit_rate;
    f->cfg.chn_num       = chn_num;
    f->cfg.mpeg_version  = mpeg_version;
    f->cfg.aac_objtype   = aac_objtype;
    f->cfg.ms_enable     = ms_enable;
    f->cfg.lfe_enable    = lfe_enable;
    f->cfg.tns_enable    = tns_enable;

    for(i = 0; i < chn_num; i++) {
        f->ctx[i].sample_rate_index = get_samplerate_index(sample_rate);
        f->ctx[i].block_type        = ONLY_LONG_BLOCK;
        f->ctx[i].window_shape      = SINE_WINDOW;
        f->ctx[i].global_gain       = 0;
        memset(f->ctx[i].scale_factor, 0, sizeof(int)*MAX_SCFAC_BANDS);
        f->ctx[i].num_window_groups = 1;
        f->ctx[i].window_group_length[0] = 8;
        f->ctx[i].window_group_length[1] = 0;
        f->ctx[i].window_group_length[2] = 0;
        f->ctx[i].window_group_length[3] = 0;
        f->ctx[i].window_group_length[4] = 0;
        f->ctx[i].window_group_length[5] = 0;
        f->ctx[i].window_group_length[6] = 0;
        f->ctx[i].window_group_length[7] = 0;

        f->ctx[i].used_bytes = 0;

        f->ctx[i].h_aacpsy        = fa_aacpsy_init(sample_rate);
        f->ctx[i].h_aac_analysis  = fa_aacfilterbank_init(block_switch_enable);

        switch(sample_rate) {
            case 48000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_48k_LONG_NUM ,fa_swb_48k_long_offset);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_48k_SHORT_NUM,fa_swb_48k_short_offset);
                break;
            case 44100:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_44k_LONG_NUM ,fa_swb_44k_long_offset);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_44k_SHORT_NUM,fa_swb_44k_short_offset);
                break;
            case 32000:
                f->ctx[i].h_mdctq_long = fa_mdctquant_init(1024, FA_SWB_32k_LONG_NUM ,fa_swb_32k_long_offset);
                f->ctx[i].h_mdctq_short= fa_mdctquant_init(128 , FA_SWB_32k_SHORT_NUM,fa_swb_32k_short_offset);
                break;
        }

        f->ctx[i].max_pred_sfb = get_max_pred_sfb(f->ctx[i].sample_rate_index);
    }

    return (uintptr_t)f;
}

void fa_aacenc_uninit(uintptr_t handle)
{
    fa_aacenc_ctx_t *f = (fa_aacenc_ctx_t *)handle;

    if(f) {
        free(f);
        f = NULL;
    }

}
