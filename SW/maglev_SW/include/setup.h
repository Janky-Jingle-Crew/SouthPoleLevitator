#include <stdint.h>

// Debug prints
#define DEBUG_RTT

#define CLK_PLL_DIV (0b001<<29)

void setup_SystemClockInit(void);
void setup_SysTickEnable(void);
void setup_I2CInit(void);
void delay_20us(uint32_t counts);
void delay_ms(uint32_t ms);
