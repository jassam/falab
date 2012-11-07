#ifndef _FA_AACQUANT_H
#define _FA_AACQUANT_H 

#include "fa_aacenc.h"
#include "fa_swbtab.h"

#ifndef NUM_SFB_MAX
#define NUM_SFB_MAX           FA_SWB_NUM_MAX
#endif

void fa_quantize_loop(fa_aacenc_ctx_t *f);
void fa_quantize_fast(fa_aacenc_ctx_t *f);

void fa_fastquant_calculate_sfb_avgenergy(aacenc_ctx_t *s);
void fa_fastquant_calculate_xmin(aacenc_ctx_t *s, float xmin[8][NUM_SFB_MAX]);

#endif
