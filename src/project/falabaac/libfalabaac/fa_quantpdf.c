#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI			3.14159265358979323846
#endif

#define M_PI_SQRT       1.7724538509
#define M_PI_SQRT_HALF  0.886226925451
#define SQRT2           1.41421356237


float fa_get_subband_power(float *X, int kmin, int kmax)
{
    int k;
    float Px;

    Px = 0.0;
    for (k = kmin; k <= kmax; k++) 
        Px += X[k] * X[k];

    return Px;
}


float fa_get_subband_sqrtpower(float *X, int kmin, int kmax)
{
    int k;
    float tmp;
    float Px;

    Px = 0.0;
    for (k = kmin; k <= kmax; k++) {
        tmp = sqrt(X[k]);
        Px += tmp * tmp;
    }

    return Px;
}



float fa_get_scaling_para(int scale_factor)
{
    return pow(2, (float)scale_factor/4.0);
}

int fa_mpeg_round(float x)
{
    int rx;

    if (x > 0) {
        rx = (int)(x + 0.4054);
    } else {
        rx = -1 * (int)(-x + 0.4054);
    }

    return rx;
}

float fa_inverse_error_func(float alpha)
{
    float erf_inv = 0.0;

    erf_inv = alpha + 
              (M_PI/12.) * alpha * alpha * alpha + 
              ((7*M_PI*M_PI)/480.) * alpha * alpha * alpha * alpha * alpha +
              ((127*M_PI*M_PI*M_PI)/40320.) * alpha * alpha * alpha * alpha * alpha * alpha * alpha;

    erf_inv = erf_inv * M_PI_SQRT_HALF;

    return erf_inv;
}

float fa_get_pdf_beta(float alpha)
{
    float beta;

    beta = SQRT2 * fa_inverse_error_func(2.*alpha - 1);

    return beta;
}

int    fa_estimate_sf(float T, int K, float beta,
                     float a2, float a4, float miu, float miuhalf)
{
    float ratio;
    float diff;
    int sf;


    diff = a4*miu - a2*a2*miuhalf*miuhalf;
    assert(diff >= 0);

    ratio = T/(K*a2*miuhalf + beta*sqrt(2*K*diff));

    sf = fa_mpeg_round((8./3.) * log2(ratio));

    return sf;
}

float fa_get_power_db(float power)
{
    return 10*log10(power);
}


void fa_adjust_thr(int subband_num, 
                   float *Px, float *Tm, float *G, 
                   float *Ti, float *Ti1)
{
    int s;
    float Ti1_tmp;
    float mi;
    float r1, r2;

    r1 = 1.0;
    r2 = 0.25;

    mi = 10000000;
    for (s = 0; s < subband_num; s++) {
        if (Ti1[s] < mi)
            mi = Ti1[s];
    }

    for (s = 0; s < subband_num; s++) {
        if (Px[s] <= Tm[s]) {                           //masked by thr
            Ti[s] = Px[s];
        } else {                                        //unmasked 
            if ((Ti[s] - Tm[s]) < 6.0) {                //high SNR, use constant NMR adjust
                Ti1_tmp = Ti[s] + r1;
                Ti[s]   = FA_MIN(Ti1_tmp, G[s]);
                Ti1[s]  = Ti1_tmp;
            } else if (Ti[s] < G[s]) {                  //low SNR, use water-filling adjust
                Ti1_tmp = FA_MAX(Ti1[s], mi+r1);
                Ti[s]   = FA_MIN(Ti1_tmp, G[s]);
            } else {                                    //very low SNR, small constant adjust
                Ti[s] += r2;
            }
        }
    }

}


