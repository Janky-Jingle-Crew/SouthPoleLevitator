#include "stm32g030xx.h"
#include "SEGGER_RTT.h"
#include "stdbool.h"
#include "stdlib.h"
#include "setup.h"
#include "TMAG5273.h"
#include "coils.h"
#include "filter.h"
#include "control.h"
#include "coil_cal.h"
#include "led.h"
#include "drive_linearization.h"

// Clamping
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, lower, upper) (MIN((upper), MAX((x), (lower))))

// Starting setpoint
#define START_SETPOINT_X 0
#define START_SETPOINT_Y 0

// Delay in ms before starting setpoint controller
#define SETPOINT_DELAY 2000

#define MAG_IDLE_XY_LIM 5000
#define MAG_IDLE_Z_LIM -3000

#define MAG_OP_XY_LIM 4000
#define MAG_OP_Z_LIM -1000

#define MAG_JINGLE_Z_LIM 30000



void _init(void) {}

typedef enum {
    INIT,
    CALIBRATION,
    IDLE,
    OPERATIONAL,
    TOO_HIGH_MAG_ERROR,
    JINGLE,
    ERROR,
} levitator_states_t;

levitator_states_t current_state = INIT;

biquad_t bqx;
biquad_t bqy;

PID_t pidx;
PID_t pidy;

setpoint_controller_t spc;

led_t status_led;

// IRQ variables
volatile uint32_t ms_tick = 0;  // global millisecond counter
volatile uint32_t us20_tick = 0;
volatile uint32_t run_filter = false;
volatile uint32_t run_ctrl = false;

// Sensor & Control variables
int16_t x, y, z;
int16_t offsets[2] = {0};
int32_t mag_from_coils_x;
int32_t mag_from_coils_y;

int16_t last_pwmx = 0;
int16_t last_pwmy = 0;
int32_t x_u_linearized = 0;
int32_t y_u_linearized = 0;
uint32_t setpoint_counter = 0;
uint32_t jingle_num;


// 5 kHz, Filter IRQ
void TIM14_IRQHandler(void)
{
    if (TIM14->SR & TIM_SR_UIF)
    {
        // Clear IRQ flag
        TIM14->SR &= ~TIM_SR_UIF;
        us20_tick++;
        run_filter = true;
    }
}

// 1 kHz, CTRL / LED IRQ
void TIM16_IRQHandler(void)
{
    if (TIM16->SR & TIM_SR_UIF)
    {
        // Clear IRQ flag
        TIM16->SR &= ~TIM_SR_UIF;
        led_Step(&status_led);
        ms_tick++;
        run_ctrl = true;
    }
}

// Systick
// void SysTick_Handler(void)
// {
    
// }

int main(void)
{
    setup_SystemClockInit();
    setup_I2CInit();
    // setup_SysTickEnable();

    // Filter init for X, Y fields
    filter_Biquad_Init(&bqx);
    filter_Biquad_Init(&bqy);

    // Control init
    control_PWM_Init(&pidx, 0);
    control_PWM_Init(&pidy, 0);
    control_Setpoint_Init(&spc, &pidx, &pidy, 0, 0);

    // Coil PWM init
    coils_TIM1_Init();
    coils_TIM3_Init();
    coils_ZeroDuty();

    // Segger RTT init
    SEGGER_RTT_Init();
    SEGGER_RTT_WriteString(0, "RTT ready\n");
    SEGGER_RTT_printf(0, "System Core Clock: %u \r\n", SystemCoreClock);

    // Sensor init
    TMAG5273_ISR_TIM14_Init();
    TMAG5273_Init();

    // Led init
    led_TIM16_Init();
    led_Setup(&status_led);
    led_ChangePattern(&status_led, OFF);
    led_IRQ_Init();

    // Coil calibration routine
    coil_cal_Init();

    #ifdef DEBUG_RTT
    SEGGER_RTT_WriteString(0, "Starting coil calibration...\n");
    bool header_printed = false;
    #endif

    while(1)
    {

        // 5 kHz filtering
        if (run_filter)
        {   
            TMAG5273_ReadXYZ(&x, &y, &z);
            coil_cal_CoilOffsets(fit_params, last_pwmx, -last_pwmx, last_pwmy, -last_pwmy, &mag_from_coils_x, &mag_from_coils_y);
            filter_Biquad_Step(&bqx, (int32_t) x - offsets[0] - mag_from_coils_x );
            filter_Biquad_Step(&bqy, (int32_t) y - offsets[1] - mag_from_coils_y );
            run_filter = false;
        }

        // 1 kHz control
        if (run_ctrl)
        {   

            switch (current_state)
            {
                //==================================================
                case INIT:
                    current_state = CALIBRATION;
                    led_ChangePattern(&status_led, RAMP);
                    break;
                
                //==================================================
                case CALIBRATION:
                    coil_cal_ZeroBias(offsets);
                    #ifdef DEBUG_RTT
                    SEGGER_RTT_printf(0, "Measured offsets: X: %d, Y: %d\r\n", offsets[0], offsets[1]);
                    #endif

                    if (abs(offsets[0]) > 1000 || abs(offsets[1]) > 1000)
                    {
                        current_state = TOO_HIGH_MAG_ERROR;
                        led_ChangePattern(&status_led, CONSTANT);
                        #ifdef DEBUG_RTT
                        SEGGER_RTT_WriteString(0, "Magnetic offsets too high, entering TOO_HIGH_MAG_ERROR state\r\n");
                        #endif
                        break;
                    }

                    int8_t result = coil_cal_MapPWM(fit_params, offsets);
                    if (result == -1)
                    {
                        current_state = TOO_HIGH_MAG_ERROR;
                        led_ChangePattern(&status_led, CONSTANT);
                        #ifdef DEBUG_RTT
                        SEGGER_RTT_WriteString(0, "Coil calibration failed, too high magnetic field detected, \
                                                    entering TOO_HIGH_MAG_ERROR state\r\n");
                        #endif
                        break;
                    }

                    current_state = IDLE;
                    led_ChangePattern(&status_led, BLINK);
                    // Play short jingle to indicate completion 
                    coils_Tone(0);
                    break;
                
                //==================================================    
                case TOO_HIGH_MAG_ERROR:
                    // Continue to check if magnetic field is still too high
                    coil_cal_ZeroBias(offsets);
                    if (abs(offsets[0]) <= 700 && abs(offsets[1]) <= 700)
                    {
                        current_state = CALIBRATION;
                        led_ChangePattern(&status_led, RAMP);
                        // Wait a bit before starting calibration
                        delay_ms(500);
                    }
                    break;

                //==================================================    
                case IDLE:
                    // Idle wait for magnet
                    // Reset regulators / integrators

                    #ifdef DEBUG_RTT
                    if (!header_printed) {
                        SEGGER_RTT_printf(0, "x_meas, y_meas, z_meas\r\n");
                        header_printed = true;
                    }
                    SEGGER_RTT_printf(0, "%6d, %6d, %6d\r", bqx.y[0], bqy.y[0], z);
                    #endif
                    // print debug info

                    // Enter operational if magnet is near center
                    if ((abs(bqx.y[0]) < MAG_OP_XY_LIM) && 
                        (abs(bqy.y[0]) < MAG_OP_XY_LIM) && 
                        (z < (MAG_OP_Z_LIM)) ) 
                    {
                        current_state = OPERATIONAL;
                        led_ChangePattern(&status_led, SINE);
                        control_Setpoint_Reset_Integral(&spc);
                        control_Setpoint_Init(&spc, &pidx, &pidy, START_SETPOINT_X, START_SETPOINT_Y);
                        setpoint_counter = 0;
                    }
                    // Play christmas jingle if upside down with tree on top.
                    else if (z > MAG_JINGLE_Z_LIM)
                    {   
                        current_state = JINGLE;
                        jingle_num = rand() % 4 + 1; // Random jingle (1-4)
                        led_ChangePattern(&status_led, CONSTANT);
                    }

                    break;

                //==================================================    
                case OPERATIONAL:
                    // Start setpoint controller after delay 
                    if (setpoint_counter >= SETPOINT_DELAY)
                    {
                        control_Setpoint_Step(&spc, bqx.y[0], bqy.y[0]);
                    }
                    else 
                    {
                        setpoint_counter++;
                    }

                    control_PWM_Step(&pidx, bqx.y[0]);
                    control_PWM_Step(&pidy, bqy.y[0]);

                    x_u_linearized = linearize_drive((int32_t)pidx.output);
                    y_u_linearized = linearize_drive((int32_t)pidy.output);

                    x_u_linearized  = CLAMP(x_u_linearized, -MAX_TIMER_VAL, MAX_TIMER_VAL);
                    y_u_linearized = CLAMP(y_u_linearized, -MAX_TIMER_VAL, MAX_TIMER_VAL);

                    coils_CalculateRMS(x_u_linearized, y_u_linearized); // Comment to disable dynamic PWM limiter
                    coils_LimitDuty(&x_u_linearized, &y_u_linearized);  // PWM limiter

                    coils_SetDuty(y_u_linearized, COIL_YP);
                    coils_SetDuty(-y_u_linearized, COIL_YN);

                    coils_SetDuty(x_u_linearized, COIL_XP);
                    coils_SetDuty(-x_u_linearized, COIL_XN);

                    last_pwmx = x_u_linearized;
                    last_pwmy = y_u_linearized;

                    // Exit to idle if magnet has been removed from center
                    if ((abs(bqx.y[0]) > MAG_IDLE_XY_LIM) || 
                        (abs(bqy.y[0]) > MAG_IDLE_XY_LIM) || 
                        (z > MAG_IDLE_Z_LIM) ) 
                    {
                        current_state = IDLE;
                        led_ChangePattern(&status_led, BLINK);
                        coils_ZeroDuty();
                        #ifdef DEBUG_RTT
                        header_printed = false;
                        #endif
                    }

                    #ifdef DEBUG_RTT
                    if (!header_printed) {
                        SEGGER_RTT_printf(0, "x_meas, y_meas, x_ref, y_ref, x_u, y_u, x_u_lin, y_u_lin\r\n");
                        header_printed = true;
                    }
                    SEGGER_RTT_printf(0, "%6d, %6d, %6d, %6d, %6d, %6d, %6d, %6d\r", 
                    bqx.y[0], bqy.y[0], pidx.ref, pidy.ref, pidx.output, pidy.output, x_u_linearized, y_u_linearized);
                    #endif
                    break;

                //==================================================
                case JINGLE:
                    // play christmas jingles
                    if (z < 10000)
                    {
                        current_state = IDLE;
                        led_ChangePattern(&status_led, BLINK);
                        break;
                    }
                    coils_Tone(jingle_num);
                    jingle_num = jingle_num % 4 + 1; // Increment 1 to 4
                    break;

                //==================================================
                case ERROR:
                    // indicate error with LED and stay here
                    break;
                    
                default:
                    break;
            }

            run_ctrl = false;
        }


    }

    __asm("NOP"); // how to inline assembly reminder
    return 0;
}

