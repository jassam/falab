#ifndef _FA_QUANTPDF_H 
#define _FA_QUANTPDF_H 

typedef struct _fa_qpdf_para_t {
    float alpha;
    float beta;
    float a2;
    float a4;

} fa_qpdf_para_t;

void  fa_protect_db_rom_init();
float fa_get_subband_power(float *X, int kmin, int kmax);
float fa_get_subband_abspower(float *X, int kmin, int kmax);
float fa_get_subband_sqrtpower(float *X, int kmin, int kmax);
float fa_get_scaling_para(int scale_factor);
int fa_mpeg_round(float x);
float fa_inverse_error_func(float alpha);
float fa_get_pdf_beta(float alpha);
int   fa_estimate_sf(float T, int K, float beta,
                     float a2, float a4, float miu, float miuhalf);
float fa_pow2db(float power);
float fa_db2pow(float db);
void fa_adjust_thr(int subband_num, 
                   float *Px, float *Tm, float *G, 
                   float *Ti, float *Ti1);
void fa_quantqdf_para_init(fa_qpdf_para_t *f, float alpha);

#endif
