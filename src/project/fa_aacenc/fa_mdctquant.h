#ifndef _FA_MDCTQUANT_H
#define _FA_MDCTQUANT_H

enum {
    MDCTQ_LONG_BLOCK  = 0,
    MDCTQ_SHORT_BLOCK = 1,
};


uintptr_t fa_mdctquant_init(int mdct_line_num, int swb_num, int *swb_low);



#endif
