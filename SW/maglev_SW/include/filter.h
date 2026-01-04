#ifndef FILTER_H
#define FILTER_H

#include "stdint.h"

// 70 Hz cutoff
#define A1 -30732
#define A2 14467

#define B0 30
#define B1 60
#define B2 30

#define Q_FILT 14

typedef struct {
    int32_t a[2];
    int32_t b[3];
    int32_t x[3];
    int32_t y[3];
} biquad_t;


void filter_Biquad_Init(biquad_t *bq);  
void filter_Biquad_Step(biquad_t *bq, int32_t input);

#endif // FILTER_H