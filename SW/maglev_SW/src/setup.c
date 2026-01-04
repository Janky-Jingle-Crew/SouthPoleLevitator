#include "setup.h"
#include "stm32g030xx.h"

extern volatile uint32_t us20_tick;
extern volatile uint32_t ms_tick;

void delay_20us(uint32_t counts)
{
    uint32_t start = us20_tick;
    while ((us20_tick - start) < counts); // wait
}

void delay_ms(uint32_t ms)
{
    uint32_t start = ms_tick;
    while ((ms_tick - start) < ms); // wait
}

void setup_SystemClockInit()
{
    FLASH->ACR |= 0b010;                                    // Flash latency: two wait states
    
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI                   // set HSI as source for PLL
                 | RCC_PLLCFGR_PLLREN                       // enable PLLR clock output
                 | (0b0001000 << 8)                         // PLL multiplication factor = 8
                 | (0b000 << 4)                             // PLL input divider = 1
                 | CLK_PLL_DIV;                             // PLLR output divider

    RCC->CR |= RCC_CR_PLLON;                                // Enable PLL
    while(!(RCC->CR & RCC_CR_PLLRDY));                      // Wait till PLL is ready      
    RCC->CFGR = 0b010;                                      // PLLR as system clock source
    while((RCC->CFGR & RCC_CFGR_SWS_Msk) != (0b010<<3));    // Wait for PLLR

    SystemCoreClockUpdate(); // Update internals (SystemCoreClock variabe)
}

void setup_SysTickEnable(void) 
{   
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;   // 1 ms reload time
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk      // CPU clock
                  | SysTick_CTRL_TICKINT_Msk        // enable interrupt
                  | SysTick_CTRL_ENABLE_Msk;        // enable SysTick
}


void setup_I2CInit()
{

    // ------------------------
    // GPIO Setup
    // ------------------------
    RCC->IOPENR  |= RCC_IOPENR_GPIOBEN;     // Enable GPIO B

    // Reset pin state and set to alternate function (AF) mode
    GPIOB->MODER &= ~( (0b11 << GPIO_MODER_MODE8_Pos) | (0b11 << GPIO_MODER_MODE9_Pos) );
    GPIOB->MODER |=  ( (0b10 << GPIO_MODER_MODE8_Pos) | (0b10 << GPIO_MODER_MODE9_Pos) );

    // Set output type to OPEN DRAIN
    GPIOB->OTYPER |= ( (1 << GPIO_OTYPER_OT8_Pos) | (1 << GPIO_OTYPER_OT9_Pos) );

    // Reset AF state and set to AF6
    GPIOB->AFR[1] &= ~( (0xF << GPIO_AFRH_AFSEL8_Pos) | (0xF << GPIO_AFRH_AFSEL9_Pos) );
    GPIOB->AFR[1] |=  ( (6U << GPIO_AFRH_AFSEL8_Pos) | (6U << GPIO_AFRH_AFSEL9_Pos) );

    // Set high speed pins
    //GPIOB->OSPEEDR |= ( (0b11 << GPIO_OSPEEDR_OSPEED8_Pos ) | (0b11 << GPIO_OSPEEDR_OSPEED9_Pos) );
    SYSCFG->CFGR1 |= (SYSCFG_CFGR1_I2C_PB8_FMP | SYSCFG_CFGR1_I2C_PB9_FMP);


    // ------------------------
    // I2C clock timings: 
    // ------------------------
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;     // Enable IC2 clock

    // I2C clock timings for 1000 kHz with 64 MHz APB CLK
    // See reference manual 25.4.10 and 25.4.11
    I2C1->TIMINGR |= (7U << I2C_TIMINGR_PRESC_Pos);
    I2C1->TIMINGR |= (3U << I2C_TIMINGR_SCLL_Pos);
    I2C1->TIMINGR |= (1U << I2C_TIMINGR_SCLH_Pos);
    I2C1->TIMINGR |= (0U << I2C_TIMINGR_SDADEL_Pos);
    I2C1->TIMINGR |= (1U << I2C_TIMINGR_SCLDEL_Pos);

    I2C1->CR1 = I2C_CR1_PE;                 // Enable
}

