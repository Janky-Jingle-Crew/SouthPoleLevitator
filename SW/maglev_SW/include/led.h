#include "stdint.h"

#define LED_FREQ 1000
#define LED_PSC 16
#define LED_ARR (64000000/(LED_FREQ*(LED_PSC+1))) - 1

#define SINE_FREQ 0.7f // Hz
#define BLINK_FREQ 1.5f // Hz
#define RAMP_FREQ 0.4f // Hz 

#define RAMP_MAX 256U

// 3 quarters on, one quarter off
#define RAMP_PERIOD (uint32_t) (RAMP_MAX * 1.5f)

#define NOMINAL_DUTY 128

// LED_FREQ / SINE_FREQ*LUT SIZE gives number of counts per step
#define SINE_COUNT (uint32_t)(LED_FREQ / (64*SINE_FREQ))

// BLINK_COUNT is between two ON-states so need additional factor 2
#define BLINK_COUNT (uint32_t)(LED_FREQ / (2*BLINK_FREQ))

#define RAMP_COUNT (uint32_t)(LED_FREQ / (RAMP_PERIOD*RAMP_FREQ))

typedef enum {
    BLINK = 0,
    RAMP,
    SINE,
    CONSTANT,
    OFF,
    NOT_SET
} led_pattern_t;

typedef struct 
{
  uint32_t counter;
  led_pattern_t pattern;
  led_pattern_t prev_pattern;
} led_t;

void led_TIM16_Init();
void led_SetDuty(uint32_t duty);
void led_Setup(led_t *led); 
void led_ChangePattern(led_t *led, led_pattern_t new_pattern) ;
void led_Step(led_t *led); 
void led_IRQ_Init();