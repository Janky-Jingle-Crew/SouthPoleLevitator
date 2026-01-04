#include <stdint.h>

#define TMAG5273_ADDR 0x35
#define TMAG5273_REG_X_MSB 0x12

void TMAG5273_ISR_TIM14_Init(void);
void TMAG5273_Init(void);

uint8_t TMAG5273_ReadRegister(uint8_t reg);
void    TMAG5273_WriteRegister(uint8_t reg, uint8_t value);

void TMAG5273_ReadRegisters(uint8_t reg, uint8_t *buf, uint8_t len);

void TMAG5273_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
