#include "control.h"
#include "stm32g030xx.h"
#include "filter.h"
#include "stdlib.h"

void control_PWM_Init(PID_t *pid, int32_t ref) 
{
    pid->output = 0;
    pid->ref = ref;
    pid->error[0] = 0;
    pid->error[1] = 0;
    pid->derror = 0;

    pid->fs = (int32_t) CTRL_LOOP_FREQ;
    pid->KpQ = (int32_t) (Kp * (float)(1 << Q_CTRL));
    pid->KdQ = (int32_t) (Kd * (float)(1 << Q_CTRL)*pid->fs);
}

void control_PWM_Step(PID_t *pid, int32_t meas)
{
    int64_t acc = 0;

    pid->error[0] = (pid->ref - meas);
    pid->derror = (pid->error[0] - pid->error[1]);

    acc += pid->KpQ*pid->error[0];
    acc += pid->KdQ*pid->derror;

    pid->output = (int32_t) (acc >> Q_CTRL);
    pid->error[1] = pid->error[0];
}

// Setpoint controller

#ifndef CLAMP
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, lower, upper) (MIN((upper), MAX((x), (lower))))
#endif

void control_Setpoint_Init(setpoint_controller_t *spc, PID_t *pid_x, PID_t *pid_y, int32_t sp_x, int32_t sp_y)
{
    // Initialize with default values
    spc->int_x = sp_x << SETPOINT_REF_Q;
    spc->int_y = sp_y << SETPOINT_REF_Q;
    spc->int_gain = SETPOINT_INT_GAIN_DEFAULT;
    spc->max_int_step_error = SETPOINT_MAX_INT_STEP_ERROR_DEFAULT;
    spc->abs_int_limit = SETPOINT_ABS_INT_LIMIT_DEFAULT;

    pid_x->ref = sp_x;
    pid_y->ref = sp_y;

    spc->pid_x = pid_x;
    spc->pid_y = pid_y;

    // Biquad filter
    // Init bq_sp_x and bq_sp_y
    filter_Biquad_Init(&spc->bq_sp_x);  
    filter_Biquad_Init(&spc->bq_sp_y);


}

void control_Setpoint_Step(setpoint_controller_t *spc, int32_t meas_x, int32_t meas_y)
{
    int32_t error_x = meas_x - spc->pid_x->ref;
    int32_t error_y = meas_y - spc->pid_y->ref;

    // Update filtered error
    filter_Biquad_Step(&spc->bq_sp_x, error_x);
    filter_Biquad_Step(&spc->bq_sp_y, error_y);

    // If the error is less than 100 start decreasing the integral gain linearly to zero at 0
    if (abs(spc->bq_sp_x.y[0] && spc->bq_sp_y.y[0]) < 100)
    {
        spc->int_gain = (SETPOINT_INT_GAIN_DEFAULT * abs(spc->bq_sp_x.y[0])) / (1000);

        // If error is less than 10, set integral gain to zero
        if ((abs(spc->bq_sp_x.y[0]) < 50) && (abs(spc->bq_sp_y.y[0]) < 50))
        {
            spc->int_gain = 0;
        }
    }
    else
    {
        spc->int_gain = SETPOINT_INT_GAIN_DEFAULT;
    }

    int32_t clamped_error_x = CLAMP(error_x, -spc->max_int_step_error, spc->max_int_step_error);
    int32_t clamped_error_y = CLAMP(error_y, -spc->max_int_step_error, spc->max_int_step_error);

    int32_t int_x_step = clamped_error_x * spc->int_gain;
    int32_t int_y_step = clamped_error_y * spc->int_gain;

    spc->int_x -= int_x_step;
    spc->int_y -= int_y_step;

    // Clamp integral to avoid windup
    if (spc->int_x > spc->abs_int_limit) spc->int_x = spc->abs_int_limit;
    if (spc->int_x < -spc->abs_int_limit) spc->int_x = -spc->abs_int_limit;

    if (spc->int_y > spc->abs_int_limit) spc->int_y = spc->abs_int_limit;
    if (spc->int_y < -spc->abs_int_limit) spc->int_y = -spc->abs_int_limit;

    spc->pid_x->ref = (spc->int_x >> SETPOINT_REF_Q);
    spc->pid_y->ref = (spc->int_y >> SETPOINT_REF_Q);
}

void control_Setpoint_Reset_Integral(setpoint_controller_t *spc)
{
    // spc->int_x = 1650 << SETPOINT_REF_Q;
    // spc->int_y = 250 << SETPOINT_REF_Q;
    // Emriks kort
    // spc->int_x = 1000 << SETPOINT_REF_Q; 
    // spc->int_y = 500 << SETPOINT_REF_Q;

    // 
    // spc->int_x = -150 << SETPOINT_REF_Q;
    // spc->int_y = 1865 << SETPOINT_REF_Q;

    // 
    // spc->int_x = 560 << SETPOINT_REF_Q;
    // spc->int_y = 0 << SETPOINT_REF_Q;

    // 
    // spc->int_x = 615 << SETPOINT_REF_Q;
    // spc->int_y = -495 << SETPOINT_REF_Q;

    // 
    //spc->int_x = 1030 << SETPOINT_REF_Q;
    //spc->int_y = 370 << SETPOINT_REF_Q;

    spc->int_x = 0 << SETPOINT_REF_Q;
    spc->int_y = 0 << SETPOINT_REF_Q;

    spc->pid_x->ref = (spc->int_x >> SETPOINT_REF_Q);
    spc->pid_y->ref = (spc->int_y >> SETPOINT_REF_Q);
}





