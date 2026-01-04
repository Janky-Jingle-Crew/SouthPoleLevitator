#include "stdint.h"

#ifndef COIL_CALIBRATION_H
#define COIL_CALIBRATION_H

#define CALIBRATION_NUM_STEPS 15
#define CALIBRATION_MAX_DRIVE 700
#define CALIBRATION_NUM_MEASUREMENTS_PER_STEP 100
extern int32_t calibration_drive_values[CALIBRATION_NUM_STEPS];

extern int32_t duty_vs_mag_per_coil[4][CALIBRATION_NUM_STEPS][3]; // [coil][step][drive,x,y]
extern float fit_params[4][2][2]; // [coil][0=x, 1=y][k,b]

void coil_cal_ZeroBias(int16_t offsets[2]);
void coil_cal_LinearFit(int16_t* x, int16_t* y, int n, float* k, float* b);
void coil_cal_CoilOffsets(float fit_params[4][2][2], int16_t drive_xp, int16_t drive_xn, int16_t drive_yp, int16_t drive_yn, int32_t* mag_x, int32_t* mag_y);
int8_t coil_cal_MapPWM(float fit_params[4][2][2], int16_t offsets[2]);
void coil_cal_Init(void);

#endif
