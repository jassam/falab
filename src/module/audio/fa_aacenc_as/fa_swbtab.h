#ifndef _FA_SWBTAB_H
#define _FA_SWBTAB_H


#define FA_SWB_32k_LONG_NUM  51
#define FA_SWB_32k_SHORT_NUM 14

#define FA_SWB_44k_LONG_NUM  49 
#define FA_SWB_44k_SHORT_NUM 14

#define FA_SWB_48k_LONG_NUM  49 
#define FA_SWB_48k_SHORT_NUM 14

extern int  fa_swb_32k_long_offset[FA_SWB_32k_LONG_NUM+1];
extern int  fa_swb_32k_short_offset[FA_SWB_32k_SHORT_NUM+1];
extern int  fa_swb_44k_long_offset[FA_SWB_44k_LONG_NUM+1];
extern int  fa_swb_44k_short_offset[FA_SWB_44k_SHORT_NUM+1];
extern int  fa_swb_48k_long_offset[FA_SWB_48k_LONG_NUM+1];
extern int  fa_swb_48k_short_offset[FA_SWB_48k_SHORT_NUM+1];

#endif
