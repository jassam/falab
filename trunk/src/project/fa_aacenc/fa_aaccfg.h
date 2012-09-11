#ifndef _FA_AACCFG_H
#define _FA_AACCFG_H 

//MPEG ID's 
#define MPEG2       1
#define MPEG4       0

//AAC object types 
#define MAIN        1
#define LOW         2
#define SSR         3
#define LTP         4

//channel ID
#define ID_SCE      0
#define ID_CPE      1
#define ID_CCE      2
#define ID_LFE      3
#define ID_DSE      4
#define ID_PCE      5
#define ID_FIL      6
#define ID_END      7

//channels
#define MAX_CHANNELS            64

//scale factor band
#define NSFB_LONG               51
#define NSFB_SHORT              15
#define MAX_SHORT_WINDOWS       8
#define MAX_SCFAC_BANDS         ((NSFB_SHORT+1)*MAX_SHORT_WINDOWS)




typedef struct _fa_aaccfg_t  {
    /* copyright string */
    char *copyright;

    /* MPEG version, 2 or 4 */
    int mpeg_version;

    /* AAC object type */
    int aac_objtype;

    /* mid/side coding */
    int ms_enable;

    /* Use one of the channels as LFE channel */
    int lfe_enable;

    /* Use Temporal Noise Shaping */
    int tns_enable;

    int chn_num;
    /* bitrate / channel of AAC file */
    int bit_rate;

    int sample_rate;

}fa_aaccfg_t;



#endif
