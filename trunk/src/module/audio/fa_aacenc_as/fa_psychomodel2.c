#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "fa_psychomodel2.h"
#include "fa_fft.h"

#ifndef		M_PI
#define		M_PI							3.14159265358979323846
#endif

#ifndef FA_MIN
#define FA_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define FA_MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif


typedef struct _fa_psychomodel2_t {
    int   cbands_num;
    int   *w_low;
    float *barkval;
    float *qsthr;
    float **spread_val;

    int   swb_num;
    int   *swb_offset;

    int   fft_len;
    uintptr_t h_fft;
    float *fft_buf;
    float *hanning_win;

    float *re, *im;
    float *mag;
    float *phi;

    float *mag_prev1;
    float *mag_prev2;
    float *mag_pred;
    float *phi_prev1;
    float *phi_prev2;
    float *phi_pred;
    float *c;

    float *group_e;
    float *group_c;

    float *en;
    float *cb;

    float *nb;
    float *nb_prev;

    float *epart;
    float *npart;
    float *thrbin;
    float *smr;
}fa_psychomodel2_t;


static float ** matrix_init(int Nrow, int Ncol)
{
    float **A;
    int i;

    /* Allocate the row pointers */
    A = (float **) malloc (Nrow * sizeof (float *));

    /* Allocate the matrix of float values */
    A[0] = (float *) malloc (Nrow * Ncol * sizeof (float));
    memset(A[0], 0, Nrow*Ncol*sizeof(float));

    /* Set up the pointers to the rows */
    for (i = 1; i < Nrow; ++i)
        A[i] = A[i-1] + Ncol;

    return A;
}

static void matrix_uninit(float **A)
{
    /*free the Nrow*Ncol data space*/
    if(A[0])
        free(A[0]);

    /*free the row pointers*/
    if(A)
        free(A);

}

static int hanning(float *w,const int N)
{
	int i,j;
    
	for (i = 0 , j = N-1; i <= j ; i++, j--) {
        /*w[i] = (float)0.5*(1-cos(2*M_PI*i/(N-1)));*/
        w[i] = (float)0.5*(1-cos(2*M_PI*(i+1)/(N-1)));
        w[j] = w[i];
    }

	return N;
}

static float psy2_spread_func(int i, int j)
{
    float tmp;
    float tmpx, tmpy, tmpz;
    float spread_val;

    if(j >= i)
        tmpx = 3 * (j - i);
    else 
        tmpx = 1.5 * (j - i);

    if(tmpx > 0.5 && tmpx < 2.5) {
        tmp  = tmpx - 0.5;
        tmpz = 8 * (tmp*tmp - 2*tmp);
    }else {
        tmpz = 0;
    }

    tmpy = 15.811389 + 7.5*(tmpx+0.474) - 17.5*sqrt(1.0+(tmpx+0.474)*(tmpx+0.474));

    if(tmpy < -100)
        spread_val = 0;
    else 
        spread_val = pow(10, (tmpz+tmpy)/10.);

    return spread_val;
   
}


uintptr_t fa_psychomodel2_init(int cbands_num, int *w_low, float *barkval, float *qsthr, 
                               int swb_num   , int *swb_offset,
                               int iblen)
{
    int i, j;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)malloc(sizeof(fa_psychomodel2_t));
    
    f->cbands_num = cbands_num;
    f->w_low      = (int   *)malloc(sizeof(int)*(cbands_num+1));
    for(i = 0; i <= cbands_num; i++)
        f->w_low[i] = w_low[i];
    f->barkval    = (float *)malloc(sizeof(float)*cbands_num);
    f->qsthr      = (float *)malloc(sizeof(float)*cbands_num);
    for(i = 0; i < cbands_num; i++) {
        f->barkval[i] = barkval[i];
        f->qsthr[i]   = qsthr[i];
    }

    f->spread_val = (float **)matrix_init(cbands_num, cbands_num);
    for(i = 0; i < cbands_num; i++)
        for(j = 0; j < cbands_num; j++)
            f->spread_val[i][j] = psy2_spread_func(i,j);


    f->swb_num    = swb_num;
    f->swb_offset = (int *)malloc(sizeof(int)*(swb_num+1));
    for(i = 0; i <= swb_num; i++)
        f->swb_offset[i] = swb_offset[i];

    f->fft_len    = 2*iblen;
    f->h_fft      = fa_fft_init(f->fft_len);
    f->fft_buf    = (float *)malloc(sizeof(float)*f->fft_len*2);
    memset(f->fft_buf , 0, sizeof(float)*f->fft_len*2);

    
    f->hanning_win = (float *)malloc(sizeof(float)*f->fft_len);
    hanning(f->hanning_win, f->fft_len);

    f->re         = (float *)malloc(sizeof(float)*iblen);
    f->im         = (float *)malloc(sizeof(float)*iblen);
    f->mag        = (float *)malloc(sizeof(float)*iblen);
    f->phi        = (float *)malloc(sizeof(float)*iblen);
    memset(f->re , 0, sizeof(float)*iblen);
    memset(f->im , 0, sizeof(float)*iblen);
    memset(f->mag, 0, sizeof(float)*iblen);
    memset(f->phi, 0, sizeof(float)*iblen);

    f->mag_prev1  = (float *)malloc(sizeof(float)*iblen);
    f->mag_prev2  = (float *)malloc(sizeof(float)*iblen);
    f->mag_pred   = (float *)malloc(sizeof(float)*iblen);
    f->phi_prev1  = (float *)malloc(sizeof(float)*iblen);
    f->phi_prev2  = (float *)malloc(sizeof(float)*iblen);
    f->phi_pred   = (float *)malloc(sizeof(float)*iblen);
    f->c          = (float *)malloc(sizeof(float)*iblen);
    memset(f->mag_prev1, 0, sizeof(float)*iblen);
    memset(f->mag_prev2, 0, sizeof(float)*iblen);
    memset(f->mag_pred , 0, sizeof(float)*iblen);
    memset(f->phi_prev1, 0, sizeof(float)*iblen);
    memset(f->phi_prev2, 0, sizeof(float)*iblen);
    memset(f->phi_pred , 0, sizeof(float)*iblen);
    memset(f->c        , 0, sizeof(float)*iblen);

    f->group_e    = (float *)malloc(sizeof(float)*cbands_num);
    f->group_c    = (float *)malloc(sizeof(float)*cbands_num);
    memset(f->group_e, 0, sizeof(float)*cbands_num);
    memset(f->group_c, 0, sizeof(float)*cbands_num);

    f->en         = (float *)malloc(sizeof(float)*cbands_num);
    f->cb         = (float *)malloc(sizeof(float)*cbands_num);
    f->nb         = (float *)malloc(sizeof(float)*cbands_num);
    f->nb_prev    = (float *)malloc(sizeof(float)*cbands_num);
    memset(f->en     , 0, sizeof(float)*cbands_num);
    memset(f->cb     , 0, sizeof(float)*cbands_num);
    memset(f->nb     , 0, sizeof(float)*cbands_num);
    memset(f->nb_prev, 0, sizeof(float)*cbands_num);

    f->epart      = (float *)malloc(sizeof(float)*swb_num);
    f->npart      = (float *)malloc(sizeof(float)*swb_num);
    f->thrbin     = (float *)malloc(sizeof(float)*iblen);
    f->smr        = (float *)malloc(sizeof(float)*swb_num);
    memset(f->epart , 0, sizeof(float)*swb_num);
    memset(f->npart , 0, sizeof(float)*swb_num);
    memset(f->thrbin, 0, sizeof(float)*swb_num);
    memset(f->smr   , 0, sizeof(float)*swb_num);

    return (uintptr_t)f;
}

void fa_psychomodel2_uninit(uintptr_t handle)
{
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    if(f) {
        if(f->w_low) {
            free(f->w_low);
            f->w_low = NULL;
        }
        if(f->barkval) {
            free(f->barkval);
            f->barkval = NULL;
        }
        if(f->qsthr) {
            free(f->qsthr);
            f->qsthr = NULL;
        }
        if(f->spread_val) {
            matrix_uninit(f->spread_val);
        }
        if(f->swb_offset) {
            free(f->swb_offset);
            f->swb_offset = NULL;
        }
        if(f->h_fft) {
            fa_fft_uninit(f->h_fft);
        }
        if(f->fft_buf) {
            free(f->fft_buf);
            f->fft_buf = NULL;
        }
        if(f->hanning_win) {
            free(f->hanning_win);
            f->hanning_win = NULL;
        }
        if(f->re) {
            free(f->re);
            f->re = NULL;
        }
        if(f->im) {
            free(f->im);
            f->im = NULL;
        }
        if(f->mag) {
            free(f->mag);
            f->mag = NULL;
        }
        if(f->phi) {
            free(f->phi);
            f->phi = NULL;
        }
        if(f->mag_prev1) {
            free(f->mag_prev1);
            f->mag_prev1 = NULL;
        }
        if(f->mag_prev2) {
            free(f->mag_prev2);
            f->mag_prev2 = NULL;
        }
        if(f->mag_pred) {
            free(f->mag_pred);
            f->mag_pred = NULL;
        }
        if(f->phi_prev1) {
            free(f->phi_prev1);
            f->phi_prev1 = NULL;
        }
        if(f->phi_prev2) {
            free(f->phi_prev2);
            f->phi_prev2 = NULL;
        }
        if(f->phi_pred) {
            free(f->phi_pred);
            f->phi_pred = NULL;
        }
        if(f->c) {
            free(f->c);
            f->c = NULL;
        }
        if(f->group_e) {
            free(f->group_e);
            f->group_e = NULL;
        }
        if(f->group_c) {
            free(f->group_c);
            f->group_c = NULL;
        }
        if(f->en) {
            free(f->en);
            f->en = NULL;
        }
        if(f->cb) {
            free(f->cb);
            f->cb = NULL;
        }
        if(f->nb) {
            free(f->nb);
            f->nb = NULL;
        }
        if(f->nb_prev) {
            free(f->nb_prev);
            f->nb_prev = NULL;
        }
        if(f->epart) {
            free(f->epart);
            f->epart = NULL;
        }
        if(f->npart) {
            free(f->npart);
            f->npart = NULL;
        }
        if(f->thrbin) {
            free(f->thrbin);
            f->thrbin = NULL;
        }
        if(f->smr) {
            free(f->smr);
            f->smr = NULL;
        }

        free(f);
        f = NULL;

    }


}
/*
    WARN: the x must be short type, for the table of psy according to the SPL of x
          the short type of x means 16 bits, and the table is also working well with 
          the 16 bits (the table is measurd by 4kHz 16 bits , +- 1 bit of the signal 
                       means the lsb)
*/
void fa_psychomodel2_calculate(uintptr_t handle, short *x, float *pe)
{
    int i,j;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    float *hanning_win = f->hanning_win;
    int   fft_len    = f->fft_len;
    float *fft_buf   = f->fft_buf;
    float *re        = f->re;
    float *im        = f->im;
    float *mag       = f->mag;
    float *phi       = f->phi;
    float *mag_pred  = f->mag_pred;
    float *phi_pred  = f->phi_pred;
    float *mag_prev1 = f->mag_prev1;
    float *mag_prev2 = f->mag_prev2;
    float *phi_prev1 = f->phi_prev1;
    float *phi_prev2 = f->phi_prev2;
    float *c         = f->c;

    int   cbands_num = f->cbands_num;
    float *group_e   = f->group_e;
    float *group_c   = f->group_c;
    int   *w_low     = f->w_low;

    float *en        = f->en;
    float *cb        = f->cb;

    float *nb        = f->nb;
    float *nb_prev   = f->nb_prev;
    float *qsthr     = f->qsthr;

    int   bit_alloc;

    int   swb_num    = f->swb_num;
    int   *swb_offset= f->swb_offset;
    float *epart     = f->epart;
    float *npart     = f->npart;
    float *thrbin    = f->thrbin;
    float *smr       = f->smr;

    /*calculate fft frequence line*/
    for(i = 0; i < fft_len; i++) {
        fft_buf[i+i]   = x[i] * hanning_win[i];
        fft_buf[i+i+1] = 0;
    }

    fa_fft(f->h_fft, fft_buf);

    /*calculate prediction mag and phi*/
    for(i = 0; i < (fft_len>>1); i++) {
        float tmp, tmp1, tmp2;

        re[i]  = fft_buf[i+i];
        im[i]  = fft_buf[i+i+1];
        mag[i] = sqrt(re[i]*re[i] + im[i]*im[i]);
        phi[i] = atan2(im[i], re[i]);

        mag_pred[i] = 2*mag_prev1[i] - mag_prev2[i];
        phi_pred[i] = 2*phi_prev1[i] - phi_prev2[i];

        tmp1   = re[i] - mag_pred[i] * cos(phi_pred[i]);
        tmp2   = im[i] - mag_pred[i] * sin(phi_pred[i]);
        tmp    = sqrt(tmp1*tmp1+tmp2*tmp2);
        c[i]   = tmp/(mag[i] + fabs(mag_pred[i]));

        mag_prev2[i] = mag_prev1[i];
        mag_prev1[i] = mag[i];
        phi_prev2[i] = phi_prev1[i];
        phi_prev1[i] = phi[i];
    }

    /*calculate critical band grouped energy and unpreditable coffients*/
    for(i = 0; i < cbands_num; i++) {
        group_e[i] = 0;
        group_c[i] = 0;
        for(j = w_low[i]; j < w_low[i+1]; j++) {
            float etmp;

            etmp       = mag[j]*mag[j];
            group_e[i] = group_e[i] + etmp;
            group_c[i] = group_c[i] + etmp*c[j]; 
        }
    }

    /*calculate each critical bands conv with spread function value*/
    for(i = 0; i < cbands_num; i++) {
        float ecb, ct;
        float tmp;
        float rnorm;

        ecb   = 0;
        ct    = 0;
        tmp   = 0;
        rnorm = 1;
         
        for(j = 0; j < cbands_num; j++) {
            ecb = ecb + group_e[j]*f->spread_val[j][i];
            ct  = ct  + group_c[j]*f->spread_val[j][i];
            tmp = tmp + f->spread_val[j][i];
        }

        if(0 != tmp) 
            rnorm = 1./tmp;
        else 
            rnorm = 1;

        en[i] = ecb * rnorm;

        if(0 != ecb)
            cb[i] = ct/ecb;
        else 
            cb[i] = 0;
    }

    /*calculate tone index*/
    for(i = 0; i < cbands_num; i++) {
        float tb, snr, bc;

        tb = -0.299 - 0.43*log(cb[i]);
        if(tb < 0)
            tb = 0;

        if(tb > 1)
            tb = 1;

        snr = tb*18 + (1-tb)*6;
        bc  = pow(10, -snr/10);
        nb[i] = en[i] * bc;
        nb[i] = FA_MAX(qsthr[i], FA_MIN(nb[i], nb_prev[i]*2));

        nb_prev[i] = nb[i];
    }

    /*calculate pe*/
    *pe = 0.;
    for(i = 0; i < cbands_num; i++) {
        float tmp;

        tmp = FA_MIN(0, log10(nb[i]/(group_e[i]+1)));
        *pe  = *pe - (w_low[i+1]-1-w_low[i])*tmp;
    }

    /*calculate bits allocation*/
/*
    if(block_type == LONG_BLOCK)
        bit_alloc = (int)(0.3*pe + 6*sqrt(pe));
    else 
        bit_alloc = (int)(0.6*pe + 24*sqrt(pe)); 

    if(bit_alloc < 0)       bit_alloc = 0;
    if(bit_alloc > 3000)    bit_alloc = 3000;
*/
    /*calculate epart*/
    for(i = 0; i < swb_num; i++) {
        epart[i] = 0;
        for(j = swb_offset[i]; j < swb_offset[i+1]; j++)
            epart[i] = epart[i] + mag[j]*mag[j];
    }

    /*calculate threshold of each frequence line and npart*/
    for(i = 0; i < cbands_num; i++) {
        for(j = w_low[i]; j < w_low[i+1]; j++)
            thrbin[j] = nb[i]/(w_low[i+1]-w_low[i]);
    }

    for(i = 0; i < swb_num; i++) {
        float tmp, tmpmin;
        tmpmin = 1000;
        for(j = swb_offset[i]; j < swb_offset[i+1]; j++) {
            tmp = FA_MIN(tmpmin, thrbin[j]);
            if(tmp > 0)
                tmpmin = tmp;
        }
        npart[i] = tmpmin*(swb_offset[i+1]-swb_offset[i]);
    }

    /*calculate smr*/
    for(i = 0; i < swb_num; i++)
        smr[i] = epart[i]/npart[i];

}

void fa_psychomodel2_get_mag_prev1(uintptr_t handle, float *mag, int *len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    *len = f->fft_len>>1;
    for(i = 0; i < (*len); i++)
        mag[i] = f->mag_prev1[i];
}

void fa_psychomodel2_get_mag_prev2(uintptr_t handle, float *mag, int *len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    *len = f->fft_len>>1;
    for(i = 0; i < (*len); i++)
        mag[i] = f->mag_prev2[i];
}


void fa_psychomodel2_get_phi_prev1(uintptr_t handle, float *phi, int *len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    *len = f->fft_len>>1;
    for(i = 0; i < (*len); i++)
        phi[i] = f->phi_prev1[i];
}

void fa_psychomodel2_get_phi_prev2(uintptr_t handle, float *phi, int *len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    *len = f->fft_len>>1;
    for(i = 0; i < (*len); i++)
        phi[i] = f->phi_prev2[i];
}


void fa_psychomodel2_set_mag_prev1(uintptr_t handle, float *mag, int len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    for(i = 0; i < len; i++)
        f->mag_prev1[i] = mag[i];
}

void fa_psychomodel2_set_mag_prev2(uintptr_t handle, float *mag, int len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    for(i = 0; i < len; i++)
        f->mag_prev2[i] = mag[i];
}


void fa_psychomodel2_set_phi_prev1(uintptr_t handle, float *phi, int len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    for(i = 0; i < len; i++)
        f->phi_prev1[i] = phi[i];
}

void fa_psychomodel2_set_phi_prev2(uintptr_t handle, float *phi, int len)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    for(i = 0; i < len; i++)
        f->phi_prev2[i] = phi[i];
}

void fa_psychomodel2_reset_nb_prev(uintptr_t handle)
{
    int i;
    fa_psychomodel2_t *f = (fa_psychomodel2_t *)handle;

    for(i = 0; i < f->cbands_num; i++)
        f->nb_prev[i] = 0;
}


