#ifndef CONTROL_H
#define CONTROL_H

#include "stdint.h"
#include "filter.h"

#define Q_CTRL 13

#define Kp 5.5f
#define Kd 0.055f

#define CTRL_LOOP_FREQ 1000

// For now only uses P and D terms
typedef struct 
{
    int32_t output;
    int32_t error[2];
    int32_t derror;
    int32_t ref;
    int32_t KpQ;
    int32_t KdQ;
    int32_t fs;
} PID_t;

void control_PWM_Init(PID_t *pid, int32_t ref);
void control_PWM_Step(PID_t *pid, int32_t meas);

// Setpoint controller

#define SETPOINT_REF_Q 16
#define SETPOINT_REF_SHIFT  (1 << SETPOINT_REF_Q)

#define SETPOINT_INT_GAIN_DEFAULT  ((25 * SETPOINT_REF_SHIFT) / 10000)
#define SETPOINT_MAX_INT_STEP_ERROR_DEFAULT  ((5 * SETPOINT_REF_SHIFT) / 1000)
#define SETPOINT_ABS_INT_LIMIT_DEFAULT  (2500 * SETPOINT_REF_SHIFT)

typedef struct 
{
    int32_t int_x;
    int32_t int_y;
    int32_t int_gain;
    int32_t max_int_step_error;
    int32_t abs_int_limit;
    PID_t *pid_x;
    PID_t *pid_y;
    // Biquad filter
    biquad_t bq_sp_x;
    biquad_t bq_sp_y;
} setpoint_controller_t;

void control_Setpoint_Init(setpoint_controller_t *spc, PID_t *pid_x, PID_t *pid_y, int32_t sp_x, int32_t sp_y);
void control_Setpoint_Step(setpoint_controller_t *spc, int32_t error_x, int32_t error_y);
void control_Setpoint_Reset_Integral(setpoint_controller_t *spc);

#endif // CONTROL_H