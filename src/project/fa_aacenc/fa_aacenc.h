#ifndef _FA_AACENC_H
#define _FA_AACENC_H 

#include "fa_aaccfg.h"
#include "fa_aacchn.h"
#include "fa_swbtab.h"

typedef unsigned uintptr_t;

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



typedef struct _aacenc_ctx_t{

    //chn info
    chn_info_t  chn_info;

    //the coding status variable 
    int sample_rate_index;

    int block_type;

    int window_shape;

    int num_window_groups;

    int window_group_length[8];

    int sfb_num_long;

    int sfb_num_short;

    int max_sfb;

    int nr_of_sfb;

    int sfb_offset[250];

    int used_bytes;

    uintptr_t h_aacpsy;
    uintptr_t h_aac_analysis;
    uintptr_t h_mdctq_long, h_mdctq_short;
    uintptr_t h_bitstream;
 
    ///////////////////////

    float max_mdct_line;
    float mdct_line[2*AAC_FRAME_LEN];

    int spectral_count;

    int scalefactor[8][FA_SWB_NUM_MAX];
    int start_common_scalefac;
    int common_scalefac;
    int x_quant[1024];
    int mdct_line_sig[1024];

    int bits_alloc;
    int bits_average;
    int bits_more;
    int bits_res_maxsize;
    int bits_res_size;
    unsigned char *res_buf;

    int unused_bits;
     
    int quant_ok;

    int hufftab_no[8][FA_SWB_NUM_MAX];

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

}aacenc_ctx_t;


typedef struct _fa_aacenc_ctx_t{

    float *sample;

    int block_switch_en;
    //the configuration of aac encoder
    aaccfg_t cfg;

    //encode ctx for each channel
    aacenc_ctx_t ctx[MAX_CHANNELS];

    int bitres_maxsize;
    unsigned char *bitres;


}fa_aacenc_ctx_t;


#define MS_DEFAULT              0
#define LFE_DEFAULT             0
#define TNS_DEFAULT             0
#define BLOCK_SWITCH_DEFAULT    1

uintptr_t fa_aacenc_init(int sample_rate, int bit_rate, int chn_num,
                         int mpeg_version, int aac_objtype, 
                         int ms_enable, int lfe_enable, int tns_enable, int block_switch_enable);

void fa_aacenc_uninit(uintptr_t handle);

void fa_aacenc_encode(uintptr_t handle, unsigned char *buf_in, int inlen, unsigned char *buf_out, int *outlen);


#endif
