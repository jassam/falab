#include <stdio.h>
#include <stdlib.h>
#include "fa_aacstream.h"
#include "fa_bitstream.h"
#include "fa_aacenc.h"

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif


/* returns the maximum bitrate per channel for certain sample rate*/
unsigned int get_aac_max_bitrate(unsigned long sample_rate)
{
    /*maximum of 6144 bit for a channel*/
    return (unsigned int)(6144.0 * (float)sample_rate/(float)1024 + .5);
}

/* returns the minimum bitrate per channel*/
unsigned int get_aac_min_bitrate()
{
    return 8000;
}


/* calculate bit_allocation based on PE */
unsigned int calculate_bit_allocation(float pe, int short_block)
{
    float pe1;
    float pe2;
    float bit_allocation;

    if (short_block) {
        pe1 = 0.6;
        pe2 = 24.0;
    } else {
        pe1 = 0.3;
        pe2 = 6.0;
    }
    bit_allocation = pe1 * pe + pe2 * sqrt(pe);
    bit_allocation = FA_MIN(FA_MAX(0.0, bit_allocation), 6144.0);

    return (unsigned int)(bit_allocation+0.5);
}


/* returns the maximum bit reservoir size */
unsigned int get_aac_bitreservoir_maxsize(unsigned long bit_rate, unsigned long sample_rate)
{
    return (6144 - (unsigned int)((float)bit_rate/(float)sample_rate*1024));
}

static int write_adtsheader(fa_aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    int bits = 56;

    if (write_flag) {
        /* Fixed ADTS header */
        fa_bitstream_putbits(h_bs, 0xFFFF, 12);
        fa_bitstream_putbits(h_bs, 0xFFFF, 12);                     /* 12 bit Syncword */
        fa_bitstream_putbits(h_bs, p_ctx->cfg.mpeg_version, 1);     /* ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC */
        fa_bitstream_putbits(h_bs, 0, 2);                           /* layer == 0 */
        fa_bitstream_putbits(h_bs, 1, 1);                           /* protection absent */
        fa_bitstream_putbits(h_bs, p_ctx->cfg.aac_objtype-1, 2);    /* profile */
        fa_bitstream_putbits(h_bs, p_ctx->sample_rate_index, 4);    /* sampling rate */
        fa_bitstream_putbits(h_bs, 0, 1);                           /* private bit */
        fa_bitstream_putbits(h_bs, p_ctx->cfg.chn_num, 3);          /* ch. config (must be > 0) */
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
#if 0
static int write_cpe(fa_aacenc_ctx_t *p_ctx, int write_flag)
{
    unsigned long h_bs = p_ctx->h_bitstream;
    chn_info_t *p_chn_info = p_ctx->chn_info;
    int bits = 0;

    if(write_flag) {
        /* write ID_CPE, single_element_channel() identifier */
        fa_bitstream_putbits(h_bs, ID_CPE, LEN_SE_ID);

        /* write the element_identifier_tag */
        fa_bitstream_putbits(h_bs, channelInfo->tag, LEN_TAG);

        /* common_window? */
        fa_bitstream_putbits(h_bs, channelInfo->common_window, LEN_COM_WIN);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;
    bits += LEN_COM_WIN;

    /* if common_window, write ics_info */
    if (channelInfo->common_window) {
        int numWindows, maxSfb;

        bits += WriteICSInfo(coderInfoL, bitStream, objectType, channelInfo->common_window, writeFlag);
        numWindows = coderInfoL->num_window_groups;
        maxSfb = coderInfoL->max_sfb;

        if (writeFlag) {
            fa_bitstream_putbits(h_bs, channelInfo->msInfo.is_present, LEN_MASK_PRES);
            if (channelInfo->msInfo.is_present == 1) {
                int g;
                int b;
                for (g=0;g<numWindows;g++) {
                    for (b=0;b<maxSfb;b++) {
                        fa_bitstream_putbits(h_bs, channelInfo->msInfo.ms_used[g*maxSfb+b], LEN_MASK);
                    }
                }
            }
        }
        bits += LEN_MASK_PRES;
        if (channelInfo->msInfo.is_present == 1)
            bits += (numWindows*maxSfb*LEN_MASK);
    }

    /* Write individual_channel_stream elements */
    bits += WriteICS(coderInfoL, bitStream, channelInfo->common_window, objectType, writeFlag);
    bits += WriteICS(coderInfoR, bitStream, channelInfo->common_window, objectType, writeFlag);



}


static int WriteCPE(CoderInfo *coderInfoL,
                    CoderInfo *coderInfoR,
                    ChannelInfo *channelInfo,
                    BitStream* bitStream,
                    int objectType,
                    int writeFlag)
{
    int bits = 0;

#ifndef DRM
    if (writeFlag) {
        /* write ID_CPE, single_element_channel() identifier */
        PutBit(bitStream, ID_CPE, LEN_SE_ID);

        /* write the element_identifier_tag */
        PutBit(bitStream, channelInfo->tag, LEN_TAG);

        /* common_window? */
        PutBit(bitStream, channelInfo->common_window, LEN_COM_WIN);
    }

    bits += LEN_SE_ID;
    bits += LEN_TAG;
    bits += LEN_COM_WIN;
#endif

    /* if common_window, write ics_info */
    if (channelInfo->common_window) {
        int numWindows, maxSfb;

        bits += WriteICSInfo(coderInfoL, bitStream, objectType, channelInfo->common_window, writeFlag);
        numWindows = coderInfoL->num_window_groups;
        maxSfb = coderInfoL->max_sfb;

        if (writeFlag) {
            PutBit(bitStream, channelInfo->msInfo.is_present, LEN_MASK_PRES);
            if (channelInfo->msInfo.is_present == 1) {
                int g;
                int b;
                for (g=0;g<numWindows;g++) {
                    for (b=0;b<maxSfb;b++) {
                        PutBit(bitStream, channelInfo->msInfo.ms_used[g*maxSfb+b], LEN_MASK);
                    }
                }
            }
        }
        bits += LEN_MASK_PRES;
        if (channelInfo->msInfo.is_present == 1)
            bits += (numWindows*maxSfb*LEN_MASK);
    }

    /* Write individual_channel_stream elements */
    bits += WriteICS(coderInfoL, bitStream, channelInfo->common_window, objectType, writeFlag);
    bits += WriteICS(coderInfoR, bitStream, channelInfo->common_window, objectType, writeFlag);

    return bits;
}


#endif
