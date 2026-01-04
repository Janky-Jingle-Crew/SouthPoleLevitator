#include "led.h"
#include "stm32g030xx.h"


/** Generated using Dr LUT - Free Lookup Table Generator
  * https://github.com/ppelikan/drlut
  **/
// Formula: sin(2*pi*t/T) 
const uint8_t sine_lut[64] = {
127,139,152,164,176,187,198,208,217,225,233,239,244,
249,252,253,254,253,252,249,244,239,233,225,217,208,
198,187,176,164,152,139,127,115,102, 90, 78, 67, 56,
 46, 37, 29, 21, 15, 10,  5,  2,  1,  0,  1,  2,  5,
 10, 15, 21, 29, 37, 46, 56, 67, 78, 90,102,115 };

 // Make general LED function. For now locked to TIM16.
void led_TIM16_Init() 
{
    // Enable TIM16 clock
    RCC->APBENR2 |= RCC_APBENR2_TIM16EN;

    // GPIO A enable
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;  

    // Reset pin state and set to alternate function (AF) mode
    GPIOA->MODER &= ~( (0b11 << GPIO_MODER_MODE6_Pos) );
    GPIOA->MODER |=  ( (0b10 << GPIO_MODER_MODE6_Pos) );

    // Reset AF state and set to AF5
    GPIOA->AFR[0] &= ~( (0xF << GPIO_AFRL_AFSEL6_Pos) );
    GPIOA->AFR[0] |=  ( (5U << GPIO_AFRL_AFSEL6_Pos) );

    TIM16->CR1 = 0;

    //PWM Mode 1, high if CNT < CCR 
    TIM16->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);

    // Preload enable
    TIM16->CCMR1 |= TIM_CCMR1_OC1PE;

    // Autoreload
    TIM16->CR1 |= TIM_CR1_ARPE;
    
    // Channel enable
    TIM16->CCER |= TIM_CCER_CC1E;

    // 1 kHz, ARR = (64 MHz / (led_freq * (PSC+1))) - 1
    TIM16->PSC = LED_PSC;
    TIM16->ARR = (uint16_t) LED_ARR;

    // Compare values
    TIM16->CCR1 = 0;

    // Disengage brake (specific to advanced TIM)
    TIM16->BDTR |= TIM_BDTR_MOE; 

    // Enable timer
    TIM16->CR1 |= TIM_CR1_CEN;
}

void led_SetDuty(uint32_t duty) 
{
    TIM16->CCR1 = duty;
}

void led_Setup(led_t *led) 
{
    led->counter = 0;
    led->pattern = NOT_SET;
    led->prev_pattern = NOT_SET;
} 

void led_ChangePattern(led_t *led, led_pattern_t new_pattern) 
{
    led->prev_pattern = led->pattern;
    led->pattern = new_pattern; 
}

void led_Step(led_t *led)
{   
    static uint32_t lut_idx = 0;
    static uint32_t blink_value = 0;
    static uint32_t ramp_val = 0;

    led->counter++; //overflow ok
    switch (led->pattern)
    {
    case BLINK:
        if (led->counter >= BLINK_COUNT) 
        {   
            blink_value ^= NOMINAL_DUTY;
            led_SetDuty(blink_value);
            led->counter = 0;
        }
        break;
    case RAMP:
        if (led->counter >= RAMP_COUNT) 
        {   
            (ramp_val < RAMP_MAX) ? led_SetDuty(ramp_val) : led_SetDuty(0); 
            ramp_val = (ramp_val + 1) % RAMP_PERIOD;
            led->counter = 0;
        }
        break;
    case SINE:
        if (led->counter >= SINE_COUNT) 
        {   
            led_SetDuty(sine_lut[lut_idx]);
            lut_idx = (lut_idx + 1) % 64;
            led->counter = 0;
        }
        break;
    case CONSTANT:
        // Only set duty on first entry
        if (led->pattern != led->prev_pattern) 
        {
            led_SetDuty(NOMINAL_DUTY);
            led->prev_pattern = led->pattern;
        }
    break;
    case OFF:
        // Only set duty on first entry
        if (led->pattern != led->prev_pattern) 
        {
            led_SetDuty(0);
            led->prev_pattern = led->pattern;
        }
    break;
    
    default:
        break;
    }
}


void led_IRQ_Init()
{
    NVIC_EnableIRQ(TIM16_IRQn);
    TIM16->DIER |= TIM_DIER_UIE;
    TIM16->CR1 |= TIM_CR1_CEN;
}
