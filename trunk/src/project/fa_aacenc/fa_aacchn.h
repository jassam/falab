#ifndef _FA_AACCHN_H
#define _FA_AACCHN_H 

#include "fa_aaccfg.h"

typedef struct _ms_info_t{

    int is_present;

    int ms_used[MAX_SCFAC_BANDS];

}ms_info_t;

typedef struct _chn_info_t{

    int tag;

    int present;

    int ch_is_left;

    int paired_ch;

    int common_window;

    int cpe;

    int sce;

    int lfe;

    ms_info_t ms_info;

}chn_info_t;

void get_aac_chn_info(chn_info_t *chn_info, int nchn, int lfe_enable);



#endif
