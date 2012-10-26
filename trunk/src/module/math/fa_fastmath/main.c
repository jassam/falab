#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fa_fastmath.h"

#define TEST_LOG
#define TEST_LOG10
/*#define TEST_ANGLE*/
#define TEST_SQRT



int main()
{
    float x;

    float y1, y2, dy;

	float y_sin,y_cos,y_tan;
	float y_arcsin , y_arccos , y_arctan;
	float y_sin_true,y_cos_true;
    float sita_true, sita;
    float delta;


    /*x = 123361452.718; //10;//333345.68;*/
    x = 487324.718; //10;//333345.68;

    fa_logtab_init();

#ifdef TEST_LOG 
    y1 = log(x); 
    y2 = FA_LOG(x);
    dy = fabs(y2-y1);
    printf("log:  y1=%f, y2=%f, dy=%f\n", y1, y2, dy);
#endif 

#ifdef TEST_LOG10
    y1 = log10(x); 
    y2 = FA_LOG10(x);
    dy = fabs(y2-y1);
    printf("log10:  y1=%f, y2=%f, dy=%f\n", y1, y2, dy);

#endif

#ifdef TEST_ANGLE
    delta = M_PI/7;
    for (x = 0; x < 2*M_PI; x += delta) {
        /*x = 2*M_PI/3;// + 334*2*PI;//2*PI;//+ */
        
        y_sin = FA_SIN(x);
        y_cos = FA_COS(x);
        
        y_sin_true = sin(x);
        y_cos_true = cos(x);

        sita = FA_ATAN2(y_sin, y_cos);
        sita_true = atan2(y_sin_true, y_cos_true);

        printf("true: x=%f, %f %f, sita=%f, sin=%f, cos=%f\n",x, y_sin_true,y_cos_true, sita_true, sin(sita_true), cos(sita_true));
        printf("simu: x=%f, %f %f, sita=%f, sin=%f, cos=%f\n\n",x, y_sin,y_cos, sita, sin(sita_true), cos(sita_true));
    }
#endif


#ifdef TEST_SQRT 
    y1 = sqrtf(x);
    y2 = FA_SQRTF(x);
    dy = fabs(y2-y1);

    printf("sqrtf:  y1=%f, y2=%f, dy=%f\n", y1, y2, dy);

#endif


}
