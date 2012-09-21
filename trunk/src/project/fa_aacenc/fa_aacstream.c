#include <stdio.h>
#include <stdlib.h>
#include "fa_aaccfg.h"
#include "fa_aacstream.h"
#include "fa_bitstream.h"
#include "fa_aacenc.h"
#include "fa_huffman.h"
#include "fa_huffmantab.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif

static int find_grouping_bits(aacenc_ctx_t *p_ctx);
static int write_ltp_predictor_data(aacenc_ctx_t *p_ctx, int write_flag);
static int write_predictor_data(aacenc_ctx_t *p_ctx, int write_flag);
static int write_tns_data(aacenc_ctx_t *p_ctx, int write_flag);
static int write_gaincontrol_data(aacenc_ctx_t *p_ctx, int write_flag);
static int write_spectral_data(aacenc_ctx_t *p_ctx, int write_flag);
static int write_aac_fillbits(aacenc_ctx_t *p_ctx, int write_flag, int numBits);

/* returns the maximum bitrate per channel for certain sample rate*/
int get_aac_max_bitrate(long sample_rate)
{
    /*maximum of 6144 bit for a channel*/
    return (int)(6144.0 * (float)sample_rate/(float)1024 + .5);
}

/* returns the minimum bitrate per channel*/
int get_aac_min_bitrate()
{
    return 8000;
}


/* calculate bit_allocation based on PE */
int calculate_bit_allocation(float pe, int block_type)
{
    float pe1;
    float pe2;
    float bit_allocation;
    int bits_alloc;

    if (block_type == ONLY_SHORT_BLOCK) {
        pe1 = 0.6;
        pe2 = 24.0;
    } else {
        pe1 = 0.3;
        pe2 = 6.0;
    }
    bit_allocation = pe1 * pe + pe2 * sqrt(pe);
    bit_allocation = FA_MIN(FA_MAX(0.0, bit_allocation), 6144.0);

    bits_alloc = (int)(bit_allocation + 0.5);
    if (bits_alloc > 3000)
        bits_alloc = 3000;

    return bits_alloc;
}


/* returns the maximum bit reservoir size */
int get_aac_bitreservoir_maxsize(int bit_rate, int sample_rate)
{
    return (6144 - (int)((float)bit_rate/(float)sample_rate*1024));
}

static int write_adtsheader(aaccfg_t *p_cfg, aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 56;

    if (write_flag) {
        /* Fixed ADTS header */
        fa_bitstream_putbits(h_bs, 0xFFFF, 12);
        fa_bitstream_putbits(h_bs, 0xFFFF, 12);                     /* 12 bit Syncword */
        fa_bitstream_putbits(h_bs, p_cfg->mpeg_version, 1);         /* ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC */
        fa_bitstream_putbits(h_bs, 0, 2);                           /* layer == 0 */
        fa_bitstream_putbits(h_bs, 1, 1);                           /* protection absent */
        fa_bitstream_putbits(h_bs, p_cfg->aac_objtype-1, 2);        /* profile */
        fa_bitstream_putbits(h_bs, p_ctx->sample_rate_index, 4);    /* sampling rate */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* private bit */
        fa_bitstream_putbits(h_bs, p_cfg->chn_num, 3);              /* ch. config (must be > 0) */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* original/copy */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* home */

        /* Variable ADTS header */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* copyr. id. bit */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* copyr. id. start */
        fa_bitstream_putbits(h_bs, p_ctx->used_bytes, 13);
        fa_bitstream_putbits(h_bs, 0x7FF, 11);                      /* buffer fullness (0x7FF for VBR) */
        fa_bitstream_putbits(h_bs, 0, 2);                           /* raw data blocks (0+1=1) */
    }

    return bits;
}

static int write_icsinfo(aacenc_ctx_t *p_ctx, int write_flag, 
                        int objtype,
                        int common_window)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int grouping_bits;
    int bits = 0;

    if (write_flag) {
        fa_bitstream_putbits(h_bs, 0, LEN_ICS_RESERV);   /* ics reserved bit*/
        fa_bitstream_putbits(h_bs, p_ctx->block_type, LEN_WIN_SEQ);  /* window sequence, block type */
        fa_bitstream_putbits(h_bs, p_ctx->window_shape, LEN_WIN_SH);  /* window shape */
    }
    bits += LEN_ICS_RESERV;
    bits += LEN_WIN_SEQ;
    bits += LEN_WIN_SH;

    /* For short windows, write out max_sfb and scale_factor_grouping */
    if(p_ctx->block_type == ONLY_SHORT_BLOCK){
        if(write_flag) {
            fa_bitstream_putbits(h_bs, p_ctx->max_sfb, LEN_MAX_SFBS);
            grouping_bits = find_grouping_bits(p_ctx);
            fa_bitstream_putbits(h_bs, grouping_bits, MAX_SHORT_WINDOWS - 1);  /* the grouping bits */
        }
        bits += LEN_MAX_SFBS;
        bits += MAX_SHORT_WINDOWS - 1;
    } else { /* Otherwise, write out max_sfb and predictor data */
        if(write_flag) {
            fa_bitstream_putbits(h_bs, p_ctx->max_sfb, LEN_MAX_SFBL);
        }
        bits += LEN_MAX_SFBL;
        if(objtype == LTP)
        {
            bits++;
            if(write_flag)
                fa_bitstream_putbits(h_bs, p_ctx->ltpInfo.global_pred_flag, 1); /* Prediction Global used */

            bits += write_ltp_predictor_data(p_ctx, write_flag);
            if (common_window)
                bits += write_ltp_predictor_data(p_ctx, write_flag);
        }else {
            bits++;
            if (write_flag)
                fa_bitstream_putbits(h_bs, p_ctx->pred_global_flag, LEN_PRED_PRES);  /* predictor_data_present */

            bits += write_predictor_data(p_ctx, write_flag);
        }
    }

    return bits;
}

#if 0 
static int WriteICS(aacenc_ctx_t *p_ctx, int write_flag,
                    int objtype,
                    int common_window)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    /* this function writes out an individual_channel_stream to the bitstream and */
    /* returns the number of bits written to the bitstream */
    int bits = 0;

    /* Write the 8-bit global_gain */
    if (write_flag)
        fa_bitstream_putbits(h_bs, p_ctx->common_scalefac, LEN_GLOB_GAIN);
    bits += LEN_GLOB_GAIN;

    /* Write ics information */
    if (!commonWindow) {
        bits += write_icsinfo(p_ctx, write_flag, objtype, common_window);
    }

    bits += SortBookNumbers(coderInfo, bitStream, write_flag);
    bits += WriteScalefactors(coderInfo, bitStream, write_flag);

    bits += WritePulseData(coderInfo, bitStream, write_flag);
    bits += WriteTNSData(coderInfo, bitStream, write_flag);
    bits += WriteGainControlData(coderInfo, bitStream, write_flag);

    bits += WriteSpectralData(coderInfo, bitStream, write_flag);

    /* Return number of bits */
    return bits;
}





static int write_cpe(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    chn_info_t *p_chn_info = p_ctx->chn_info;
    int bits = 0;

    if(write_flag) {
        /* write ID_CPE, single_element_channel() identifier */
        fa_bitstream_putbits(h_bs, ID_CPE, LEN_SE_ID);

        /* write the element_identifier_tag */
        fa_bitstream_putbits(h_bs, p_chn_info->tag, LEN_TAG);

        /* common_window? */
        fa_bitstream_putbits(h_bs, p_chn_info->common_window, LEN_COM_WIN);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;
    bits += LEN_COM_WIN;

    /* if common_window, write ics_info */
    if (p_chn_info->common_window) {
        int numWindows, maxSfb;

        bits += WriteICSInfo(coderInfoL, bitStream, objectType, p_chn_info->common_window, write_flag);
        numWindows = coderInfoL->num_window_groups;
        maxSfb = coderInfoL->max_sfb;

        if (write_flag) {
            fa_bitstream_putbits(h_bs, p_chn_info->msInfo.is_present, LEN_MASK_PRES);
            if (p_chn_info->msInfo.is_present == 1) {
                int g;
                int b;
                for (g=0;g<numWindows;g++) {
                    for (b=0;b<maxSfb;b++) {
                        fa_bitstream_putbits(h_bs, p_chn_info->msInfo.ms_used[g*maxSfb+b], LEN_MASK);
                    }
                }
            }
        }
        bits += LEN_MASK_PRES;
        if (p_chn_info->msInfo.is_present == 1)
            bits += (numWindows*maxSfb*LEN_MASK);
    }

    /* Write individual_channel_stream elements */
    bits += WriteICS(coderInfoL, bitStream, p_chn_info->common_window, objectType, write_flag);
    bits += WriteICS(coderInfoR, bitStream, p_chn_info->common_window, objectType, write_flag);



}
#endif 


static int find_grouping_bits(aacenc_ctx_t *p_ctx)
{
    /* This function inputs the grouping information and outputs the seven bit
    'grouping_bits' field that the AAC decoder expects.  */

    int grouping_bits = 0;
    int tmp[8];
    int i, j;
    int index = 0;

    for(i = 0; i < p_ctx->num_window_groups; i++){
        for (j = 0; j < p_ctx->window_group_length[i]; j++){
            tmp[index++] = i;
        }
    }

    for(i = 1; i < 8; i++){
        grouping_bits = grouping_bits << 1;
        if(tmp[i] == tmp[i-1]) {
            grouping_bits++;
        }
    }

    return grouping_bits;
}


static int write_ltp_predictor_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int i, last_band;
    int bits;
    LtpInfo *ltpInfo = &p_ctx->ltpInfo;

    bits = 0;

    if (ltpInfo->global_pred_flag)
    {

        if(write_flag)
            fa_bitstream_putbits(h_bs, 1, 1); /* LTP used */
        bits++;

        switch(p_ctx->block_type)
        {
        case ONLY_LONG_BLOCK:
        case LONG_START_BLOCK:
        case LONG_STOP_BLOCK:
            bits += LEN_LTP_LAG;
            bits += LEN_LTP_COEF;
            if(write_flag)
            {
                fa_bitstream_putbits(h_bs, ltpInfo->delay[0], LEN_LTP_LAG);
                fa_bitstream_putbits(h_bs, ltpInfo->weight_idx,  LEN_LTP_COEF);
            }

            last_band = ((p_ctx->nr_of_sfb < MAX_LT_PRED_LONG_SFB) ?
                p_ctx->nr_of_sfb : MAX_LT_PRED_LONG_SFB);

            bits += last_band;
            if(write_flag)
                for (i = 0; i < last_band; i++)
                    fa_bitstream_putbits(h_bs, ltpInfo->sfb_prediction_used[i], LEN_LTP_LONG_USED);
            break;

        default:
            break;
        }
    }

    return (bits);
}

static int write_predictor_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 0;

    /* Write global predictor data present */
    short predictorDataPresent = p_ctx->pred_global_flag;
    int numBands = FA_MIN(p_ctx->max_pred_sfb, p_ctx->nr_of_sfb);

    if (write_flag) {
        if (predictorDataPresent) {
            int b;
            if (p_ctx->reset_group_number == -1) {
                fa_bitstream_putbits(h_bs, 0, LEN_PRED_RST); /* No prediction reset */
            } else {
                fa_bitstream_putbits(h_bs, 1, LEN_PRED_RST);
                fa_bitstream_putbits(h_bs, (unsigned long)p_ctx->reset_group_number,
                    LEN_PRED_RSTGRP);
            }

            for (b=0;b<numBands;b++) {
                fa_bitstream_putbits(h_bs, p_ctx->pred_sfb_flag[b], LEN_PRED_ENAB);
            }
        }
    }
    bits += (predictorDataPresent) ?
        (LEN_PRED_RST +
        ((p_ctx->reset_group_number)!=-1)*LEN_PRED_RSTGRP +
        numBands*LEN_PRED_ENAB) : 0;

    return bits;
}


static int write_pulse_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 0;

    if (write_flag) {
        fa_bitstream_putbits(h_bs, 0, LEN_PULSE_PRES);  /* no pulse_data_present */
    }

    bits += LEN_PULSE_PRES;

    return bits;
}


static int write_tns_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 0;
    int numWindows;
    int len_tns_nfilt;
    int len_tns_length;
    int len_tns_order;
    int filtNumber;
    int resInBits;
    int bitsToTransmit;
    unsigned long unsignedIndex;
    int w;

    TnsInfo* tnsInfoPtr = &p_ctx->tnsInfo;

    if (write_flag) {
        fa_bitstream_putbits(h_bs,tnsInfoPtr->tnsDataPresent,LEN_TNS_PRES);
    }
    bits += LEN_TNS_PRES;

    /* If TNS is not present, bail */
    if (!tnsInfoPtr->tnsDataPresent) {
        return bits;
    }

    /* Set window-dependent TNS parameters */
    if (p_ctx->block_type == ONLY_SHORT_BLOCK) {
        numWindows = MAX_SHORT_WINDOWS;
        len_tns_nfilt = LEN_TNS_NFILTS;
        len_tns_length = LEN_TNS_LENGTHS;
        len_tns_order = LEN_TNS_ORDERS;
    }
    else {
        numWindows = 1;
        len_tns_nfilt = LEN_TNS_NFILTL;
        len_tns_length = LEN_TNS_LENGTHL;
        len_tns_order = LEN_TNS_ORDERL;
    }

    /* Write TNS data */
    bits += (numWindows * len_tns_nfilt);
    for (w=0;w<numWindows;w++) {
        TnsWindowData* windowDataPtr = &tnsInfoPtr->windowData[w];
        int numFilters = windowDataPtr->numFilters;
        if (write_flag) {
            fa_bitstream_putbits(h_bs,numFilters,len_tns_nfilt); /* n_filt[] = 0 */
        }
        if (numFilters) {
            bits += LEN_TNS_COEFF_RES;
            resInBits = windowDataPtr->coefResolution;
            if (write_flag) {
                fa_bitstream_putbits(h_bs,resInBits-DEF_TNS_RES_OFFSET,LEN_TNS_COEFF_RES);
            }
            bits += numFilters * (len_tns_length+len_tns_order);
            for (filtNumber=0;filtNumber<numFilters;filtNumber++) {
                TnsFilterData* tnsFilterPtr=&windowDataPtr->tnsFilter[filtNumber];
                int order = tnsFilterPtr->order;
                if (write_flag) {
                    fa_bitstream_putbits(h_bs,tnsFilterPtr->length,len_tns_length);
                    fa_bitstream_putbits(h_bs,order,len_tns_order);
                }
                if (order) {
                    bits += (LEN_TNS_DIRECTION + LEN_TNS_COMPRESS);
                    if (write_flag) {
                        fa_bitstream_putbits(h_bs,tnsFilterPtr->direction,LEN_TNS_DIRECTION);
                        fa_bitstream_putbits(h_bs,tnsFilterPtr->coefCompress,LEN_TNS_COMPRESS);
                    }
                    bitsToTransmit = resInBits - tnsFilterPtr->coefCompress;
                    bits += order * bitsToTransmit;
                    if (write_flag) {
                        int i;
                        for (i=1;i<=order;i++) {
                            unsignedIndex = (unsigned long) (tnsFilterPtr->index[i])&(~(~0<<bitsToTransmit));
                            fa_bitstream_putbits(h_bs,unsignedIndex,bitsToTransmit);
                        }
                    }
                }
            }
        }
    }
    return bits;
}


static int write_gaincontrol_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 0;

    if (write_flag) {
        fa_bitstream_putbits(h_bs, 0, LEN_GAIN_PRES);
    }

    bits += LEN_GAIN_PRES;

    return bits;
}


static int write_spectral_data(aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int i, bits = 0;

    /* set up local pointers to data and len */
    /* data array contains data to be written */
    /* len array contains lengths of data words */
    int* data = p_ctx->data;
    int* len  = p_ctx->len;

    if (write_flag) {
        for(i = 0; i < p_ctx->spectral_count; i++) {
            if (len[i] > 0) {  /* only send out non-zero codebook data */
                fa_bitstream_putbits(h_bs, data[i], len[i]); /* write data */
                bits += len[i];
            }
        }
    } else {
        for(i = 0; i < p_ctx->spectral_count; i++) {
            bits += len[i];
        }
    }

    return bits;
}


static int write_aac_fillbits(aacenc_ctx_t *p_ctx, int write_flag, int numBits)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int numberOfBitsLeft = numBits;

    /* Need at least (LEN_SE_ID + LEN_F_CNT) bits for a fill_element */
    int minNumberOfBits = LEN_SE_ID + LEN_F_CNT;

    while (numberOfBitsLeft >= minNumberOfBits)
    {
        int numberOfBytes;
        int maxCount;

        if (write_flag) {
            fa_bitstream_putbits(h_bs, ID_FIL, LEN_SE_ID);   /* Write fill_element ID */
        }
        numberOfBitsLeft -= minNumberOfBits;    /* Subtract for ID,count */

        numberOfBytes = (int)(numberOfBitsLeft/LEN_BYTE);
        maxCount = (1<<LEN_F_CNT) - 1;  /* Max count without escaping */

        /* if we have less than maxCount bytes, write them now */
        if (numberOfBytes < maxCount) {
            int i;
            if (write_flag) {
                fa_bitstream_putbits(h_bs, numberOfBytes, LEN_F_CNT);
                for (i = 0; i < numberOfBytes; i++) {
                    fa_bitstream_putbits(h_bs, 0, LEN_BYTE);
                }
            }
            /* otherwise, we need to write an escape count */
        }
        else {
            int maxEscapeCount, maxNumberOfBytes, escCount;
            int i;
            if (write_flag) {
                fa_bitstream_putbits(h_bs, maxCount, LEN_F_CNT);
            }
            maxEscapeCount = (1<<LEN_BYTE) - 1;  /* Max escape count */
            maxNumberOfBytes = maxCount + maxEscapeCount;
            numberOfBytes = (numberOfBytes > maxNumberOfBytes ) ? (maxNumberOfBytes) : (numberOfBytes);
            escCount = numberOfBytes - maxCount;
            if (write_flag) {
                fa_bitstream_putbits(h_bs, escCount, LEN_BYTE);
                for (i = 0; i < numberOfBytes-1; i++) {
                    fa_bitstream_putbits(h_bs, 0, LEN_BYTE);
                }
            }
        }
        numberOfBitsLeft -= LEN_BYTE*numberOfBytes;
    }

    return numberOfBitsLeft;
}

static int write_hufftab_no(aacenc_ctx_t *s, int write_flag)
{
    unsigned long h_bs = s->h_bitstream;

    int repeat_counter;
    int bit_count = 0;
    int previous;
    int max, bit_len/*,sfbs*/;
    int gr, sfb;
    int sect_cb_bits = 4;
    int sfb_num;

    /* Set local pointers to coderInfo elements */

    if (s->block_type == ONLY_SHORT_BLOCK){
        max = 7;
        bit_len = 3;
        sfb_num = s->sfb_num_short;
    } else {  /* the block_type is a long,start, or stop window */
        max = 31;
        bit_len = 5;
        sfb_num = s->sfb_num_long;
    }

    for (gr = 0; gr < s->num_window_groups; gr++) {
        repeat_counter=1;

        previous = s->hufftab_no[gr][0];
        if (write_flag) {
            fa_bitstream_putbits(h_bs, s->hufftab_no[gr][0],sect_cb_bits);
        }
        bit_count += sect_cb_bits;

        for (sfb = 1; sfb < sfb_num; sfb++) {
            if ((s->hufftab_no[gr][sfb] != previous)) {
                if (write_flag) {
                    fa_bitstream_putbits(h_bs, repeat_counter, bit_len);
                }
                bit_count += bit_len;

                if (repeat_counter == max){  /* in case you need to terminate an escape sequence */
                    if (write_flag)
                        fa_bitstream_putbits(h_bs, 0, bit_len);
                    bit_count += bit_len;
                }

                if (write_flag)
                    fa_bitstream_putbits(h_bs, s->hufftab_no[gr][sfb], sect_cb_bits);
                bit_count += sect_cb_bits;
                previous = s->hufftab_no[gr][sfb];
                repeat_counter=1;
            }
            /* if the length of the section is longer than the amount of bits available in */
            /* the bitsream, "max", then start up an escape sequence */
            else if ((s->hufftab_no[gr][sfb] == previous) && (repeat_counter == max)) {
                if (write_flag) {
                    fa_bitstream_putbits(h_bs, repeat_counter, bit_len);
                }
                bit_count += bit_len;
                repeat_counter = 1;
            }
            else {
                repeat_counter++;
            }
        }

        if (write_flag)
            fa_bitstream_putbits(h_bs, repeat_counter, bit_len);
        bit_count += bit_len;

        if (repeat_counter == max) {  /* special case if the last section length is an */
            /* escape sequence */
            if (write_flag)
                fa_bitstream_putbits(h_bs, 0, bit_len);
            bit_count += bit_len;
        }
    }  /* Bottom of group iteration */

    return bit_count;
}



static int write_scalefactor(aacenc_ctx_t *s, int write_flag) 
{
    /* this function takes care of counting the number of bits necessary */
    /* to encode the scalefactors.  In addition, if the writeFlag == 1, */
    /* then the scalefactors are written out the bitStream output bit */
    /* stream.  it returns k, the number of bits written to the bitstream*/

    unsigned long h_bs = s->h_bitstream;
    int gr, sfb;

    int bit_count=0;
    int diff,length,codeword;
    int previous_scale_factor;
    int previous_is_factor;       /* Intensity stereo */
    int index = 0;
    int sfb_num;

    if (s->block_type == ONLY_SHORT_BLOCK) {
        sfb_num = s->sfb_num_short;
    } else {
        sfb_num = s->sfb_num_long;
    }

    previous_scale_factor = s->common_scalefac;
    previous_is_factor = 0;

    for (gr = 0; gr < s->num_window_groups; gr++) {
        for (sfb = 0; sfb < sfb_num; sfb++) {
            /* test to see if any codebooks in a group are zero */
            if ((s->hufftab_no[gr][sfb] == INTENSITY_HCB) ||
                (s->hufftab_no[gr][sfb] == INTENSITY_HCB2) ) {
                /* only send scalefactors if using non-zero codebooks */
                diff = s->scalefactor[gr][sfb] - previous_is_factor;
                if ((diff < 60)&&(diff >= -60))
                    length = fa_hufftab12[diff+60][0];
                else 
                    length = 0;
                bit_count += length;
                previous_is_factor = s->scalefactor[index];
                if (write_flag) {
                    codeword = fa_hufftab12[diff+60][1];
                    fa_bitstream_putbits(h_bs, codeword, length);
                }
            } else if (s->hufftab_no[gr][sfb]) {
                /* only send scalefactors if using non-zero codebooks */
                diff = s->scalefactor[gr][sfb] - previous_scale_factor;
                if ((diff < 60)&&(diff >= -60))
                    length = fa_hufftab12[diff+60][0];
                else 
                    length = 0;
                bit_count+=length;
                previous_scale_factor = s->scalefactor[gr][sfb];
                if (write_flag) {
                    codeword = fa_hufftab12[diff+60][1];
                    fa_bitstream_putbits(h_bs, codeword, length);
                }
            }
            index++;
        }
    }

    return bit_count;
}



