#include "filter.h"
#include "stm32g030xx.h"

void filter_Biquad_Init(biquad_t *bq) 
{   

    bq->a[0] = (int32_t) A1;
    bq->a[1] = (int32_t) A2;

    bq->b[0] = (int32_t) B0;
    bq->b[1] = (int32_t) B1;
    bq->b[2] = (int32_t) B2;

    for (int i = 0; i < 3; i++)
    {
        bq->x[i] = 0;
        bq->y[i] = 0;
    }
    
}

void filter_Biquad_Step(biquad_t *bq, int32_t input)
{   
    int32_t acc = 0;
    bq->x[0] = input;

    // Filter equation
    acc += bq->b[0]*bq->x[0];
    acc += bq->b[1]*bq->x[1];
    acc += bq->b[2]*bq->x[2];

    acc -= bq->a[0]*bq->y[1];
    acc -= bq->a[1]*bq->y[2];

    // Result
    bq->y[0] = (acc >> Q_FILT);

    // Set states
    bq->x[2] = bq->x[1];
    bq->x[1] = bq->x[0];

    bq->y[2] = bq->y[1];
    bq->y[1] = bq->y[0];
}
