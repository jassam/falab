#include <stdio.h>
#include <stdlib.h>
#include "fa_aacpsy.h"
#include "fa_aacfilterbank.h"
#include "fa_mdct.h"

typedef struct _fa_aacfilterbank_t {

    int        win_block_switch_en;
    int        block_type;
    int        prev_win_shape;

    float      x_buf[2*AAC_FRAME_LEN];

    float      sin_win_long_left[AAC_BLOCK_LONG_LEN];
    float      sin_win_long_right[AAC_BLOCK_LONG_LEN];
    float      kbd_win_long_left[AAC_BLOCK_LONG_LEN];
    float      kbd_win_long_right[AAC_BLOCK_LONG_LEN];

    float      sin_win_short_left[AAC_BLOCK_SHORT_LEN];
    float      sin_win_short_right[AAC_BLOCK_SHORT_LEN];
    float      kbd_win_short_left[AAC_BLOCK_SHORT_LEN];
    float      kbd_win_short_right[AAC_BLOCK_SHORT_LEN];

    uintptr_t  h_mdct_long;
    float      mdct_long_buf[2*AAC_BLOCK_LONG_LEN];
    uintptr_t  h_mdct_short;
    float      mdct_short_buf[2*AAC_BLOCK_SHORT_LEN];

}fa_aacfilterbank_t;

uintptr_t fa_aacfilterbank_init(int win_block_switch_en)
{
    int   i;
    float *sin_win_long;
    float *kbd_win_long; 
    float *sin_win_short;
    float *kbd_win_short;

    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)malloc(sizeof(fa_aacfilterbank_t));

    f->win_block_switch_en = win_block_switch_en;

    /*initial the long and short block window*/
    sin_win_long  = (float *)malloc(sizeof(float)*2*AAC_BLOCK_LONG_LEN);
    kbd_win_long  = (float *)malloc(sizeof(float)*2*AAC_BLOCK_LONG_LEN);
    sin_win_short = (float *)malloc(sizeof(float)*2*AAC_BLOCK_SHORT_LEN);
    kbd_win_short = (float *)malloc(sizeof(float)*2*AAC_BLOCK_SHORT_LEN);
    memset(sin_win_long , 0, sizeof(float)*2*AAC_BLOCK_LONG_LEN);
    memset(kbd_win_long , 0, sizeof(float)*2*AAC_BLOCK_LONG_LEN);
    memset(sin_win_short, 0, sizeof(float)*2*AAC_BLOCK_SHORT_LEN);
    memset(kbd_win_short, 0, sizeof(float)*2*AAC_BLOCK_SHORT_LEN);

    fa_mdct_sine(sin_win_long , 2*AAC_BLOCK_LONG_LEN);
    fa_mdct_kbd(kbd_win_long  , 2*AAC_BLOCK_LONG_LEN , 4);
    fa_mdct_sine(sin_win_short, 2*AAC_BLOCK_SHORT_LEN);
    fa_mdct_kbd(kbd_win_short , 2*AAC_BLOCK_SHORT_LEN, 6);

    for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) {
        f->sin_win_long_left[i]   = sin_win_long[i];
        f->sin_win_long_right[i]  = sin_win_long[i+AAC_BLOCK_LONG_LEN];
        f->kbd_win_long_left[i]   = kbd_win_long[i];
        f->kbd_win_long_right[i]  = kbd_win_long[i+AAC_BLOCK_LONG_LEN];
    }

    for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++) {
        f->sin_win_short_left[i]  = sin_win_short[i];
        f->sin_win_short_right[i] = sin_win_short[i+AAC_BLOCK_SHORT_LEN];
        f->kbd_win_short_left[i]  = kbd_win_short[i];
        f->kbd_win_short_right[i] = kbd_win_short[i+AAC_BLOCK_SHORT_LEN];
    }

    free(sin_win_long);
    free(kbd_win_long);
    free(sin_win_short);
    free(kbd_win_short);

    f->block_type      = ONLY_LONG_BLOCK;
    f->prev_win_shape  = SINE_WINDOW;

    memset(f->x_buf, 0, sizeof(float)*2*AAC_FRAME_LEN);

    memset(f->mdct_long_buf , 0, sizeof(float)*2*AAC_BLOCK_LONG_LEN);
    memset(f->mdct_short_buf, 0, sizeof(float)*2*AAC_BLOCK_SHORT_LEN);
    f->h_mdct_long  = fa_mdct_init(MDCT_FFT4, 2*AAC_BLOCK_LONG_LEN);
    f->h_mdct_short = fa_mdct_init(MDCT_FFT4, 2*AAC_BLOCK_SHORT_LEN);

    return (uintptr_t)f;
}

void fa_aacfilterbank_uninit(uintptr_t handle)
{
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)handle;

    if(f) {
        fa_mdct_uninit(f->h_mdct_long);
        fa_mdct_uninit(f->h_mdct_short);

        free(f);
        f = NULL;
    }
}

#define SWITCH_PE 1800

static void aacblocktype_switch(float pe, int prev_block_type, int *cur_block_type)
{
    int prev_coding_block_type;
    int cur_coding_block_type;

    /*get prev coding block type*/
    if(prev_block_type == ONLY_LONG_BLOCK || prev_block_type == LONG_STOP_BLOCK)
        prev_coding_block_type = LONG_CODING_BLOCK;
    else 
        prev_coding_block_type = SHORT_CODING_BLOCK;

    /*use pe to decide current coding block type*/
    if(pe > SWITCH_PE) 
        cur_coding_block_type = SHORT_CODING_BLOCK;
    else 
        cur_coding_block_type = LONG_CODING_BLOCK;
    
    /*use prev coding block type and current coding block type to decide current block type*/
    if(prev_coding_block_type == LONG_CODING_BLOCK && cur_coding_block_type == LONG_CODING_BLOCK)
        *cur_block_type = ONLY_LONG_BLOCK;
    else if(prev_coding_block_type == LONG_CODING_BLOCK && cur_coding_block_type == SHORT_CODING_BLOCK)
        *cur_block_type = LONG_START_BLOCK;
    else if(prev_coding_block_type == SHORT_CODING_BLOCK && cur_coding_block_type == SHORT_CODING_BLOCK)
        *cur_block_type = ONLY_SHORT_BLOCK;
    else 
        *cur_block_type = LONG_STOP_BLOCK;
}


/*this function used in aac encode*/
void fa_aacblocktype_switch(uintptr_t h_fltbank, uintptr_t h_psy, float pe)
{
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)h_fltbank;
    int prev_block_type;
    int cur_block_type;

    prev_block_type = f->block_type;
    aacblocktype_switch(pe, prev_block_type, &cur_block_type);

#if 1 
    if(cur_block_type == LONG_START_BLOCK)
        update_psy_short_previnfo(h_psy);

    if(cur_block_type == LONG_STOP_BLOCK)
        update_psy_long_previnfo(h_psy);
#endif
        
    f->block_type = cur_block_type;
}

int  fa_get_aacblocktype(uintptr_t handle)
{
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)handle;

    return f->block_type;
}

/*this function used in aac decode*/
void fa_set_aacblocktype(uintptr_t handle, int block_type)
{
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)handle;

    f->block_type = block_type;
}


/*used in encode, kbd is used for short block, sine is used for long block*/
void fa_aacfilterbank_analysis(uintptr_t handle, float *x, float *mdct_line)
{
    int i,k;
    int offset;
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)handle;

    float *win_left, *win_right;
    int block_type = f->block_type;

    /*update x_buf, 50% overlap, copy the remain half data to the beginning position*/
    for(i = 0; i < AAC_FRAME_LEN; i++) 
        f->x_buf[i] = f->x_buf[i+AAC_FRAME_LEN];
    for(i = 0; i < AAC_FRAME_LEN; i++)
        f->x_buf[i+AAC_FRAME_LEN] = x[i];

    /*window shape the input x*/
    if(f->win_block_switch_en) {
        switch(f->prev_win_shape) {
            case SINE_WINDOW:
                if(block_type == ONLY_LONG_BLOCK || block_type == LONG_START_BLOCK)
                    win_left = f->sin_win_long_left;
                else 
                    win_left = f->sin_win_short_left;
                break;
            case KBD_WINDOW:
                if(block_type == ONLY_LONG_BLOCK || block_type == LONG_START_BLOCK)
                    win_left = f->kbd_win_long_left;
                else 
                    win_left = f->kbd_win_short_left;
                break;
        }

        switch(block_type) {
            case ONLY_LONG_BLOCK:
                win_right = f->sin_win_long_right;
                f->prev_win_shape = SINE_WINDOW;
                break;
            case LONG_START_BLOCK:
                win_right = f->kbd_win_short_right;
                f->prev_win_shape = KBD_WINDOW;
                break;
            case ONLY_SHORT_BLOCK:
                win_right = f->kbd_win_short_right;
                f->prev_win_shape = KBD_WINDOW;
                break;
            case LONG_STOP_BLOCK:
                win_right = f->sin_win_long_right;
                f->prev_win_shape = SINE_WINDOW;
                break;
        }
    }else {
        win_left  = f->sin_win_long_left;
        win_right = f->sin_win_long_right;
    }

    switch(block_type) {
        case ONLY_LONG_BLOCK:
            offset = AAC_BLOCK_LONG_LEN;
            for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) {
                f->mdct_long_buf[i]        = f->x_buf[i]        * win_left[i];
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset] * win_right[i];
            }
            fa_mdct(f->h_mdct_long, f->mdct_long_buf, mdct_line);
            break;
        case LONG_START_BLOCK:
            for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) 
                f->mdct_long_buf[i] = f->x_buf[i] * win_left[i];
            offset = AAC_BLOCK_LONG_LEN;
            for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset];
            offset += AAC_BLOCK_TRANS_LEN;
            for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++)
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset] * win_right[i];
            offset += AAC_BLOCK_SHORT_LEN;
            for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                f->mdct_long_buf[i+offset] = 0;
            fa_mdct(f->h_mdct_long, f->mdct_long_buf, mdct_line);
            break;
        case ONLY_SHORT_BLOCK:
            offset = AAC_BLOCK_TRANS_LEN;
            for(k = 0; k < 8; k++) {
                for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++) {
                    f->mdct_short_buf[i]                     = f->x_buf[i+offset]                     * win_left[i];
                    f->mdct_short_buf[i+AAC_BLOCK_SHORT_LEN] = f->x_buf[i+offset+AAC_BLOCK_SHORT_LEN] * win_right[i];
                }
                offset += AAC_BLOCK_SHORT_LEN;
                fa_mdct(f->h_mdct_short, f->mdct_short_buf, mdct_line+k*AAC_BLOCK_SHORT_LEN);
            }
            break;
        case LONG_STOP_BLOCK:
            for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                f->mdct_long_buf[i] = 0;
            offset = AAC_BLOCK_TRANS_LEN;
            for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++)
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset] * win_left[i];
            offset += AAC_BLOCK_SHORT_LEN;
            for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset];
            offset += AAC_BLOCK_TRANS_LEN;
            for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                f->mdct_long_buf[i+offset] = f->x_buf[i+offset] * win_right[i];
            fa_mdct(f->h_mdct_long, f->mdct_long_buf, mdct_line);
            break;
    }

}

/*used in decode*/
void fa_aacfilterbank_synthesis(uintptr_t handle, float *mdct_line, float *x)
{
    int i,k;
    int offset;
    fa_aacfilterbank_t *f = (fa_aacfilterbank_t *)handle;

    float *win_left, *win_right;
    int block_type = f->block_type;

    if(f->win_block_switch_en) {
        switch(block_type) {
            case ONLY_LONG_BLOCK:
                win_left  = f->sin_win_long_left;
                win_right = f->sin_win_long_right;
                fa_imdct(f->h_mdct_long, mdct_line, f->mdct_long_buf);
                offset = AAC_BLOCK_LONG_LEN;
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) {
                    f->x_buf[i]        += f->mdct_long_buf[i] * win_left[i];
                    f->x_buf[i+offset] =  f->mdct_long_buf[i+offset] * win_right[i];
                }

                for(i = 0; i < AAC_FRAME_LEN; i++)
                    x[i] = f->x_buf[i];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) 
                    f->x_buf[i] = f->x_buf[i+AAC_BLOCK_LONG_LEN];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i+AAC_BLOCK_LONG_LEN] = 0;

                break;
            case LONG_START_BLOCK:
                win_left  = f->sin_win_long_left;
                win_right = f->kbd_win_short_right;
                fa_imdct(f->h_mdct_long, mdct_line, f->mdct_long_buf);
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i] += f->mdct_long_buf[i] * win_left[i];
                offset = AAC_BLOCK_LONG_LEN;
                for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                    f->x_buf[i+offset] = f->mdct_long_buf[i+offset];
                offset += AAC_BLOCK_TRANS_LEN;
                for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++)
                    f->x_buf[i+offset] = f->mdct_long_buf[i+offset] * win_right[i];
                offset += AAC_BLOCK_SHORT_LEN;
                for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                    f->x_buf[i+offset] = 0;

                for(i = 0; i < AAC_FRAME_LEN; i++)
                    x[i] = f->x_buf[i];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i] = f->x_buf[i+AAC_BLOCK_LONG_LEN];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i+AAC_BLOCK_LONG_LEN] = 0;

                break;
            case ONLY_SHORT_BLOCK:
                win_left  = f->kbd_win_short_left;
                win_right = f->kbd_win_short_right;
                offset = AAC_BLOCK_TRANS_LEN;
                for(k = 0; k < 8; k++) {
                    for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++) {
                        fa_imdct(f->h_mdct_short, mdct_line+k*AAC_BLOCK_SHORT_LEN, f->mdct_short_buf);
                        f->x_buf[i+offset] += f->mdct_short_buf[i] * win_left[i];
                        f->x_buf[i+offset+AAC_BLOCK_SHORT_LEN] = f->mdct_short_buf[i+AAC_BLOCK_SHORT_LEN] * win_right[i];
                    }
                    offset += AAC_BLOCK_SHORT_LEN;
                }
                for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                    f->x_buf[i+offset+AAC_BLOCK_SHORT_LEN] = 0;

                for(i = 0; i < AAC_FRAME_LEN; i++)
                    x[i] = f->x_buf[i];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) 
                    f->x_buf[i] = f->x_buf[i+AAC_BLOCK_LONG_LEN];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i+AAC_BLOCK_LONG_LEN] = 0;
                break;
            case LONG_STOP_BLOCK:
                win_left  = f->kbd_win_short_left;
                win_right = f->sin_win_long_right;
                offset = AAC_BLOCK_TRANS_LEN;
                fa_imdct(f->h_mdct_long, mdct_line, f->mdct_long_buf);
                for(i = 0; i < AAC_BLOCK_SHORT_LEN; i++)
                    f->x_buf[i+offset] += f->mdct_long_buf[i+offset] * win_left[i];
                offset += AAC_BLOCK_SHORT_LEN;
                for(i = 0; i < AAC_BLOCK_TRANS_LEN; i++)
                    f->x_buf[i+offset] = f->mdct_long_buf[i+offset];
                offset += AAC_BLOCK_TRANS_LEN;
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i+offset] = f->mdct_long_buf[i+offset] * win_right[i];

                for(i = 0; i < AAC_FRAME_LEN; i++)
                    x[i] = f->x_buf[i];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i] = f->x_buf[i+AAC_BLOCK_LONG_LEN];
                for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
                    f->x_buf[i+AAC_BLOCK_LONG_LEN] = 0;

                break;
        }

    }else {
        win_left  = f->sin_win_long_left;
        win_right = f->sin_win_long_right;
        fa_imdct(f->h_mdct_long, mdct_line, f->mdct_long_buf);
        offset = AAC_BLOCK_LONG_LEN;
        for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) {
            f->x_buf[i]        += f->mdct_long_buf[i] * win_left[i];
            f->x_buf[i+offset] =  f->mdct_long_buf[i+offset] * win_right[i];
        }

        for(i = 0; i < AAC_FRAME_LEN; i++)
            x[i] = f->x_buf[i];
        for(i = 0; i < AAC_BLOCK_LONG_LEN; i++) 
            f->x_buf[i] = f->x_buf[i+AAC_BLOCK_LONG_LEN];
        for(i = 0; i < AAC_BLOCK_LONG_LEN; i++)
            f->x_buf[i+AAC_BLOCK_LONG_LEN] = 0;
    }
}
