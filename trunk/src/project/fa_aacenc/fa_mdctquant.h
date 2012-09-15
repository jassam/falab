#ifndef _FA_MDCTQUANT_H
#define _FA_MDCTQUANT_H

#include "fa_swbtab.h"

typedef unsigned long uintptr_t;

#define NUM_WINDOW_GROUPS_MAX 8
#define NUM_WINDOWS_MAX       8
#define NUM_SFB_MAX           FA_SWB_NUM_MAX

#define NUM_MDCT_LINE_MAX     1024


enum {
    MDCTQ_LONG_BLOCK  = 0,
    MDCTQ_SHORT_BLOCK = 1,
};


uintptr_t fa_mdctquant_init(int mdct_line_num, int sfb_num, int *swb_low, int block_type_cof);



#endif
