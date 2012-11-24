#ifndef _FA_TNS_H 
#define _FA_TNS_H 

#include "fa_aacenc.h"

#ifdef __cplusplus 
extern "C"
{ 
#endif  

typedef unsigned uintptr_t;

#define TNS_MAX_ORDER 20
#define DEF_TNS_GAIN_THRESH  10 //15 //1.4
#define DEF_TNS_COEFF_THRESH 0.1
#define DEF_TNS_COEFF_RES 4
#define DEF_TNS_RES_OFFSET 3
#define LEN_TNS_NFILTL 2
#define LEN_TNS_NFILTS 1


typedef struct _tns_flt_t{
    int order;                           /* Filter order */
    int direction;                       /* Filtering direction */
    int coef_compress;                    /* Are coeffs compressed? */
    int length;                          /* Length, in bands */
    float acof[TNS_MAX_ORDER+1];     /* AR Coefficients */
    float kcof[TNS_MAX_ORDER+1];     /* Reflection Coefficients */
    int   index[TNS_MAX_ORDER+1];          /* Coefficient indices */
} tns_flt_t;

typedef struct _tns_win_t{
    int num_flt;                             /* Number of filters */
    int coef_resolution;                         /* Coefficient resolution */
    tns_flt_t tns_flt[1<<LEN_TNS_NFILTL]; /* TNS filters */
} tns_win_t;

typedef struct _tns_info_t{

    int tns_data_present;
    int tns_minband_long;
    int tns_minband_short;
    int tns_maxband_long;
    int tns_maxband_short;
    int tns_maxorder_long;
    int tns_maxorder_short;
    tns_win_t tns_win[8];

    uintptr_t h_lpc_short;
    uintptr_t h_lpc_long;

} tns_info_t;

 
uintptr_t fa_tns_init(int mpeg_version, int objtype, int sr_index);

void fa_tns_uninit(uintptr_t handle);

void fa_tns_encode_frame(aacenc_ctx_t *f);



#ifdef __cplusplus 
}
#endif  

#endif
