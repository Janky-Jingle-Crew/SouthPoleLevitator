#include "coils.h"
#include "stm32g030xx.h"
#include "Setup.h"
#include "stdlib.h"

static int16_t max_pwm_dynamic = MAX_PWM_TOTAL;

// Piano note mapping, 12 keys. Saves some flash.
unsigned int freqz[] = {
  0,
  261, 277, 293, 311, 329, 349, 369, 391, 415, 440, 466, 493,
  523, 554, 587, 622, 659, 698, 739, 783, 830, 880, 932, 987
};

const note_tt cal_jingle[] = {{12,150}, {14,150},{17,300}};
const note_tt last_christmas[] = {{0,100},{17,737},{0,64},{17,438},{0,93},{15,108},{0,427},{10,60},{0,206},{17,193},{0,73},{17,202},{0,65},{19,161},{0,107},{15,578},{0,222},{12,191},{0,75},{12,165},{0,97},{17,214},{0,51},{17,198},{0,70},{19,180},{0,354},{15,571},{0,225},{12,64},{0,204},{14,247},{0,20},{15,266},{0,0},{14,208},{0,60},{12,880},{0,455},{19,681},{0,120},{17,806},{0,261},{12,77},{0,191},{19,197},{0,66},{20,194},{0,72},{19,166},{0,101},{17,697},{0,376},{15,63},{0,202},{14,246},{0,21},{15,207},{0,58},{14,63},{0,207},{14,412},{0,120},{15,90},{0,446},{14,218},{0,318},{10,936}, {0,1000},{0,1000}};
const note_tt jinglebell[] = {{0,100},{11,105},{0,233},{11,1},{11,1},{0,166},{11,121},{0,217},{11,113},{0,55},{10,90},{0,248},{10,66},{0,103},{10,87},{0,251},{10,84},{0,84},{8,309},{0,29},{10,108},{0,60},{8,143},{0,195},{3,381},{0,127},{1,129},{0,39},{1,92},{0,76},{4,63},{0,105},{6,74},{0,95},{8,198},{0,140},{10,71},{0,97},{8,158},{0,180},{3,540},{0,137},{6,360},{0,148},{8,328},{0,10},{10,79},{0,90},{8,119},{0,219},{4,423},{0,593},{4,116},{0,52},{1,137},{0,201},{3,76},{0,92},{4,95},{0,243},{6,354},{0,153},{8,338},{0,169},{6,121},{0,47},{1,100},{0,238},{3,74},{0,95},{4,105},{0,233},{6,423},{0,254},{6,254},{0,84},{6,84},{0,84},{8,227},{0,280},{8,296},{0,211},{10,119},{0,219},{8,111},{0,58},{6,105},{0,233},{11,582},{0,1000},{0,1000},};
const note_tt wish_merry[] = {{0,100},{3,394},{0,0},{8,197},{0,197},{8,193},{0,4},{10,197},{0,0},{8,197},{0,0},{7,185},{0,12},{5,234},{0,160},{5,201},{0,193},{5,361},{0,32},{10,197},{0,197},{10,189},{0,8},{12,197},{0,0},{10,197},{0,0},{8,197},{0,0},{7,394},{0,0},{3,189},{0,205},{3,275},{0,119},{12,164},{0,230},{12,197},{0,0},{13,197},{0,0},{12,197},{0,0},{10,185},{0,12},{8,337},{0,57},{5,283},{0,111},{3,106},{0,90},{3,185},{0,12},{5,341},{0,53},{10,304},{0,90},{7,234},{0,160},{8,596},{0,193},{3,378},{0,16},{8,271},{0,123},{8,320},{0,74},{8,312},{0,82},{7,686},{0,102},{7,353},{0,41},{8,337},{0,57},{7,365},{0,28},{5,394},{0,0},{3,670},{0,119},{10,263},{0,131},{12,394},{0,0},{10,127},{0,69},{10,185},{0,12},{8,131},{0,65},{8,197},{0,0},{15,357},{0,37},{3,263},{0,131},{3,119},{0,78},{3,160},{0,37},{5,378},{0,16},{10,349},{0,45},{7,312},{0,82},{8,629},{0,1000}};
const note_tt feliz[] = {{0,100}, {8,196},{0,10},{13,284},{0,129},{12,191},{0,15},{13,202},{0,4},{10,385},{0,28},{8,56},{0,150},{6,64},{0,142},{5,56},{0,150},{1,62},{0,351},{8,206},{0,620},{10,191},{0,15},{15,269},{0,144},{13,174},{0,32},{10,206},{0,0},{8,381},{0,32},{8,60},{0,146},{6,68},{0,137},{5,64},{0,142},{8,68},{0,344},{1,206},{0,620},{8,206},{0,0},{13,262},{0,150},{12,185},{0,21},{13,206},{0,0},{10,450},{0,170},{6,153},{0,53},{10,280},{0,133},{10,185},{0,21},{8,237},{0,176},{8,219},{0,193},{8,183},{0,23},{6,254},{0,159},{6,206},{0,0},{5,413},{0, 1000}, {0,1000}};

const note_tt * jingles[] = 
{
    cal_jingle,
    last_christmas,
    wish_merry,
    jinglebell,
    feliz
};

#define NUM_JINGLES (sizeof(jingles)/sizeof(jingles[0]))

#define NOTE_SIZE (sizeof(note_tt))

const uint8_t jingle_lengths[NUM_JINGLES] = {
    sizeof(cal_jingle)/NOTE_SIZE,
    sizeof(last_christmas)/NOTE_SIZE,
    sizeof(wish_merry)/NOTE_SIZE,
    sizeof(jinglebell)/NOTE_SIZE,
    sizeof(feliz)/NOTE_SIZE,

};

// NOT TO BE USED DURING CONTROL
void coils_Tone(uint32_t jingle_idx)
{   
    uint16_t tone_ARR;
    uint16_t freq_idx;
    uint16_t ms;
    const note_tt *jingle = jingles[jingle_idx];
    coils_ZeroDuty();

    for (size_t i = 0; i < jingle_lengths[jingle_idx]; i++)
    {   
        freq_idx = jingle[i].note;
        ms = jingle[i].duration;
        if (freqz[freq_idx] == 0) 
        {
            delay_ms(ms);
        }
        else 
        {
            tone_ARR = (uint16_t) (CORE_CLOCK / ((TONE_PSC+1)*freqz[freq_idx]) - 1);
            TIM1->PSC = TONE_PSC;
            TIM1->ARR = tone_ARR;

            TIM3->PSC = TONE_PSC;
            TIM3->ARR = tone_ARR;

            coils_SetDuty(tone_ARR >> 4, COIL_XP);
            coils_SetDuty(tone_ARR >> 4, COIL_XN);
            coils_SetDuty(tone_ARR >> 4, COIL_YP);
            coils_SetDuty(tone_ARR >> 4, COIL_YN);

            delay_ms(ms);
            coils_ZeroDuty();
        }
    }
    // Reconfigure PWM for control again
    coils_TIM1_Init();
    coils_TIM3_Init();
}

void coils_CalculateRMS(int32_t x, int32_t y)
{
    static uint32_t rms_buf[RMS_WINDOW];
    static uint32_t rms_sum = 0;
    static uint16_t rms_idx = 0;

    uint32_t v = (uint32_t)x * x + (uint32_t)y * y;

    /* Sliding RMS window update */
    rms_sum -= rms_buf[rms_idx];
    rms_buf[rms_idx] = v;
    rms_sum += v;

    rms_idx++;
    if (rms_idx >= RMS_WINDOW)
        rms_idx = 0;

    /* Compare sums to avoid division */
    uint32_t sum_high = RMS_LIMIT_HIGH * RMS_WINDOW;
    uint32_t sum_low  = RMS_LIMIT_LOW  * RMS_WINDOW;

    /* Hysteretic dynamic limit */
    if (rms_sum > sum_high)
    {
        if (max_pwm_dynamic > PWM_MIN_DYNAMIC)
            max_pwm_dynamic -= 3;   // strong reduction
    }
    else if (rms_sum < sum_low)
    {
        if (max_pwm_dynamic < MAX_PWM_TOTAL)
            max_pwm_dynamic += 1;   // gentle recovery
    }
}

void coils_LimitDuty(int32_t *x, int32_t *y) 
{
    int32_t ax = abs(*x);
    int32_t ay = abs(*y);

    int32_t mag_sq = ax * ax + ay * ay;
    int32_t max_sq = max_pwm_dynamic * max_pwm_dynamic;

    if (mag_sq <= max_sq)
        return;

    // Get lookup index
    uint32_t index = mag_sq >> SQRT_SHIFT;
    if (index >= SQRT_TABLE_SIZE - 1)
        index = SQRT_TABLE_SIZE - 2;

    // Lin. interpolate
    uint32_t frac = mag_sq & ((1UL << SQRT_SHIFT) - 1);
    uint32_t a = sqrt_table[index];
    uint32_t b = sqrt_table[index + 1];

    uint32_t mag = a + (((b - a) * frac) >> SQRT_SHIFT);

    // Q15 scaling 
    int32_t scale = (max_pwm_dynamic * Q15_ONE) / (int32_t)mag;

    *x = (int16_t)((*x * scale) >> 15);
    *y = (int16_t)((*y * scale) >> 15);
}

void coils_SetDuty(int16_t duty, coil_pos_t coil) 
{

    uint16_t duty1, duty2;
    if (duty >= 0) 
    {
        duty1 = duty;
        duty2 = 0;
    } 
    else
    {
        duty1 = 0;
        duty2 = -duty;
    }
    
    switch (coil)
    {
    case COIL_XP:
        TIM3->CCR1 = duty1;
        TIM3->CCR2 = duty2;
        break;
    case COIL_YP:
        TIM1->CCR4 = duty1;
        TIM1->CCR3 = duty2;
        break;
    case COIL_XN:
        TIM1->CCR2 = duty1;
        TIM1->CCR1 = duty2;
        break;
    case COIL_YN:
        TIM3->CCR4 = duty1;
        TIM3->CCR3 = duty2;
        break;

    default:
        break;
    }
} 

void coils_ZeroDuty() 
{
    coils_SetDuty(0, COIL_XP);
    coils_SetDuty(0, COIL_XN);
    coils_SetDuty(0, COIL_YP);
    coils_SetDuty(0, COIL_YN);
}

void coils_TIM1_Init() 
{
    // Enable TIM1 clock
    RCC->APBENR2 |= RCC_APBENR2_TIM1EN;

    // GPIO A enable
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;  

    // Reset pin state and set to alternate function (AF) mode
    GPIOA->MODER &= ~( (0b11 << GPIO_MODER_MODE8_Pos) | (0b11 << GPIO_MODER_MODE9_Pos) );
    GPIOA->MODER |=  ( (0b10 << GPIO_MODER_MODE8_Pos) | (0b10 << GPIO_MODER_MODE9_Pos) );

    // Reset AF state and set to AF2
    GPIOA->AFR[1] &= ~( (0xF << GPIO_AFRH_AFSEL8_Pos) | (0xF << GPIO_AFRH_AFSEL9_Pos) );
    GPIOA->AFR[1] |=  ( (2U << GPIO_AFRH_AFSEL8_Pos) | (2U << GPIO_AFRH_AFSEL9_Pos) );

    // Reset pin state and set to alternate function (AF) mode
    GPIOA->MODER &= ~( (0b11 << GPIO_MODER_MODE10_Pos) | (0b11 << GPIO_MODER_MODE11_Pos) );
    GPIOA->MODER |=  ( (0b10 << GPIO_MODER_MODE10_Pos) | (0b10 << GPIO_MODER_MODE11_Pos) );

    // Reset AF state and set to AF2
    GPIOA->AFR[1] &= ~( (0xF << GPIO_AFRH_AFSEL10_Pos) | (0xF << GPIO_AFRH_AFSEL11_Pos) );
    GPIOA->AFR[1] |=  ( (2U << GPIO_AFRH_AFSEL10_Pos) | (2U << GPIO_AFRH_AFSEL11_Pos) );

    TIM1->CR1 = 0;

    //PWM Mode 1, high if CNT < CCR 
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC2M_Pos);
    TIM1->CCMR2 |= (6U << TIM_CCMR2_OC3M_Pos);
    TIM1->CCMR2 |= (6U << TIM_CCMR2_OC4M_Pos);
    
    // Preload enable
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM1->CCMR1 |= TIM_CCMR1_OC2PE;
    TIM1->CCMR2 |= TIM_CCMR2_OC3PE;
    TIM1->CCMR2 |= TIM_CCMR2_OC4PE;

    // Autoreload
    TIM1->CR1 |= TIM_CR1_ARPE;
    
    // Channel enable
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->CCER |= TIM_CCER_CC2E;
    TIM1->CCER |= TIM_CCER_CC3E;
    TIM1->CCER |= TIM_CCER_CC4E;

    // 25 kHz, ARR = (64 MHz / (timer_freq * (PSC+1))) - 1
    TIM1->PSC = 0;
    TIM1->ARR = (uint16_t) TIMER_ARR;

    // Compare values
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;

    TIM1->CCR3 = 0;
    TIM1->CCR4 = 0;

    // Disengage brake (specific to advanced TIM1)
    TIM1->BDTR |= TIM_BDTR_MOE; 

    // Enable timer
    TIM1->CR1 |= TIM_CR1_CEN;
}

void coils_TIM3_Init() 
{
    // Enable Timer 3 clock
    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    // GPIO B Enable
    RCC->IOPENR  |= RCC_IOPENR_GPIOBEN;     
    
    // Reset pin state and set to alternate function (AF) mode
    GPIOB->MODER &= ~( (0b11 << GPIO_MODER_MODE0_Pos) | (0b11 << GPIO_MODER_MODE1_Pos) );
    GPIOB->MODER |=  ( (0b10 << GPIO_MODER_MODE0_Pos) | (0b10 << GPIO_MODER_MODE1_Pos) );

    // Reset AF state and set to AF1
    GPIOB->AFR[0] &= ~( (0xF << GPIO_AFRL_AFSEL0_Pos) | (0xF << GPIO_AFRL_AFSEL1_Pos) );
    GPIOB->AFR[0] |=  ( (1U << GPIO_AFRL_AFSEL0_Pos) | (1U << GPIO_AFRL_AFSEL1_Pos) );

    // Reset pin state and set to alternate function (AF) mode
    GPIOB->MODER &= ~( (0b11 << GPIO_MODER_MODE4_Pos) | (0b11 << GPIO_MODER_MODE5_Pos) );
    GPIOB->MODER |=  ( (0b10 << GPIO_MODER_MODE4_Pos) | (0b10 << GPIO_MODER_MODE5_Pos) );

    // Reset AF state and set to AF1
    GPIOB->AFR[0] &= ~( (0xF << GPIO_AFRL_AFSEL4_Pos) | (0xF << GPIO_AFRL_AFSEL5_Pos) );
    GPIOB->AFR[0] |=  ( (1U << GPIO_AFRL_AFSEL4_Pos) | (1U << GPIO_AFRL_AFSEL5_Pos) );
    
    TIM3->CR1 = 0;

    // PWM Mode 1, high if CNT < CCR 
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos);
    TIM3->CCMR1 |= (6U << TIM_CCMR1_OC2M_Pos);
    TIM3->CCMR2 |= (6U << TIM_CCMR2_OC3M_Pos);
    TIM3->CCMR2 |= (6U << TIM_CCMR2_OC4M_Pos);
    
    // Preload enable
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM3->CCMR1 |= TIM_CCMR1_OC2PE;
    TIM3->CCMR2 |= TIM_CCMR2_OC3PE;
    TIM3->CCMR2 |= TIM_CCMR2_OC4PE;
    
    // Channel enable
    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CCER |= TIM_CCER_CC4E;

    // 25 kHz, 25 kHz, ARR = (64 MHz / (timer_freq * (PSC+1))) - 1
    TIM3->PSC = 0;
    TIM3->ARR = (uint16_t) TIMER_ARR;

    // Compare values
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;

    TIM3->CCR3 = 0;
    TIM3->CCR4 = 0;

    // Enable timer
    TIM3->CR1 |= TIM_CR1_CEN;
}


