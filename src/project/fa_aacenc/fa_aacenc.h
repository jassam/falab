#ifndef _FA_AACENC_H
#define _FA_AACENC_H 

#include "fa_aaccfg.h"
#include "fa_aacchn.h"



#define TNS_MAX_ORDER 20
#define DEF_TNS_GAIN_THRESH 1.4
#define DEF_TNS_COEFF_THRESH 0.1
#define DEF_TNS_COEFF_RES 4
#define DEF_TNS_RES_OFFSET 3
#define LEN_TNS_NFILTL 2
#define LEN_TNS_NFILTS 1

#define DELAY 2048
#define LEN_LTP_DATA_PRESENT 1
#define LEN_LTP_LAG 11
#define LEN_LTP_COEF 3
#define LEN_LTP_SHORT_USED 1
#define LEN_LTP_SHORT_LAG_PRESENT 1
#define LEN_LTP_SHORT_LAG 5
#define LTP_LAG_OFFSET 16
#define LEN_LTP_LONG_USED 1
#define MAX_LT_PRED_LONG_SFB 40
#define MAX_LT_PRED_SHORT_SFB 13
#define SHORT_SQ_OFFSET (AAC_BLOCK_LONG_LEN-(AAC_BLOCK_SHORT_LEN*4+AAC_BLOCK_SHORT_LEN/2))
#define CODESIZE 8
#define NOK_LT_BLEN (3 * AAC_BLOCK_LONG_LEN)

#define SBMAX_L 49
#define LPC 2

typedef struct {
    int order;                           /* Filter order */
    int direction;                       /* Filtering direction */
    int coefCompress;                    /* Are coeffs compressed? */
    int length;                          /* Length, in bands */
    double aCoeffs[TNS_MAX_ORDER+1];     /* AR Coefficients */
    double kCoeffs[TNS_MAX_ORDER+1];     /* Reflection Coefficients */
    int index[TNS_MAX_ORDER+1];          /* Coefficient indices */
} TnsFilterData;

typedef struct {
    int numFilters;                             /* Number of filters */
    int coefResolution;                         /* Coefficient resolution */
    TnsFilterData tnsFilter[1<<LEN_TNS_NFILTL]; /* TNS filters */
} TnsWindowData;

typedef struct {
    int tnsDataPresent;
    int tnsMinBandNumberLong;
    int tnsMinBandNumberShort;
    int tnsMaxBandsLong;
    int tnsMaxBandsShort;
    int tnsMaxOrderLong;
    int tnsMaxOrderShort;
    TnsWindowData windowData[MAX_SHORT_WINDOWS]; /* TNS data per window */
} TnsInfo;

typedef struct
{
    int weight_idx;
    double weight;
    int sbk_prediction_used[MAX_SHORT_WINDOWS];
    int sfb_prediction_used[MAX_SCFAC_BANDS];
    int delay[MAX_SHORT_WINDOWS];
    int global_pred_flag;
    int side_info;
    double *buffer;
    double *mdct_predicted;

    double *time_buffer;
    double *ltp_overlap_buffer;
} LtpInfo;


typedef struct
{
    int psy_init_mc;
    double dr_mc[LPC][AAC_BLOCK_LONG_LEN],e_mc[LPC+1+1][AAC_BLOCK_LONG_LEN];
    double K_mc[LPC+1][AAC_BLOCK_LONG_LEN], R_mc[LPC+1][AAC_BLOCK_LONG_LEN];
    double VAR_mc[LPC+1][AAC_BLOCK_LONG_LEN], KOR_mc[LPC+1][AAC_BLOCK_LONG_LEN];
    double sb_samples_pred_mc[AAC_BLOCK_LONG_LEN];
    int thisLineNeedsResetting_mc[AAC_BLOCK_LONG_LEN];
    int reset_count_mc;
} BwpInfo;



typedef struct _fa_aacenc_ctx_t{
    //the configuration of aac encoder
    fa_aaccfg_t cfg;
    chn_info_t  chn_info;

    //the coding status variable 
    int sample_rate_index;

    int block_type;

    int window_shape;

    int global_gain;

    int scale_factor[MAX_SCFAC_BANDS];

    int num_window_groups;

    int window_group_length[8];

    int max_sfb;

    int nr_of_sfb;

    int sfb_offset[250];

    int used_bytes;

    unsigned long h_bitstream;

    ///////////////////////

    int spectral_count;

    /* Huffman codebook selected for each sf band */
    int book_vector[MAX_SCFAC_BANDS];

    /* Data of spectral bitstream elements, for each spectral pair,
       5 elements are required: 1*(esc)+2*(sign)+2*(esc value)=5 */
    int *data;

    /* Lengths of spectral bitstream elements */
    int *len;


    TnsInfo tnsInfo;
    LtpInfo ltpInfo;
    BwpInfo bwpInfo;

    int max_pred_sfb;
    int pred_global_flag;
    int pred_sfb_flag[MAX_SCFAC_BANDS];
    int reset_group_number;

}fa_aacenc_ctx_t;



#endif
