#ifndef _FA_PSYTAB_H
#define _FA_PSYTAB_H

#define FA_PSY_32k_LONG_NUM   66
#define FA_PSY_32k_SHORT_NUM  44 

#define FA_PSY_44k_LONG_NUM   70 
#define FA_PSY_44k_SHORT_NUM  42 

#define FA_PSY_48k_LONG_NUM   69 
#define FA_PSY_48k_SHORT_NUM  42 

extern int   fa_psy_32k_long_wlow[FA_PSY_32k_LONG_NUM+1];
extern float fa_psy_32k_long_barkval[FA_PSY_32k_LONG_NUM];
extern float fa_psy_32k_long_qsthr[FA_PSY_32k_LONG_NUM];
extern int   fa_psy_32k_short_wlow[FA_PSY_32k_SHORT_NUM+1];
extern float fa_psy_32k_short_barkval[FA_PSY_32k_SHORT_NUM];
extern float fa_psy_32k_short_qsthr[FA_PSY_32k_SHORT_NUM];

extern int   fa_psy_44k_long_wlow[FA_PSY_44k_LONG_NUM+1];
extern float fa_psy_44k_long_barkval[FA_PSY_44k_LONG_NUM];
extern float fa_psy_44k_long_qsthr[FA_PSY_44k_LONG_NUM];
extern int   fa_psy_44k_short_wlow[FA_PSY_44k_SHORT_NUM+1];
extern float fa_psy_44k_short_barkval[FA_PSY_44k_SHORT_NUM];
extern float fa_psy_44k_short_qsthr[FA_PSY_44k_SHORT_NUM];

extern int   fa_psy_48k_long_wlow[FA_PSY_48k_LONG_NUM+1];
extern float fa_psy_48k_long_barkval[FA_PSY_48k_LONG_NUM];
extern float fa_psy_48k_long_qsthr[FA_PSY_48k_LONG_NUM];
extern int   fa_psy_48k_short_wlow[FA_PSY_48k_SHORT_NUM+1];
extern float fa_psy_48k_short_barkval[FA_PSY_48k_SHORT_NUM];
extern float fa_psy_48k_short_qsthr[FA_PSY_48k_SHORT_NUM];

#endif

