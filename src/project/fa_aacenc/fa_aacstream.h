#ifndef _FA_AACSTREAM_H
#define _FA_AACSTREAM_H 

#include "fa_aaccfg.h"
#include "fa_aacenc.h"

int calculate_bit_allocation(float pe, int block_type);

int fa_bits_count(aaccfg_t *c, aacenc_ctx_t *s, aacenc_ctx_t *sr);

#endif
