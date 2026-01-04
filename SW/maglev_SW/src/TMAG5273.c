#include "TMAG5273.h"
#include "stm32g030xx.h"   

void TMAG5273_ISR_TIM14_Init()
{
    RCC->APBENR2 |= RCC_APBENR2_TIM14EN;

    TIM14->CR1 = 0;
    TIM14->CNT = 0;

    // 5 kHz
    TIM14->PSC = 3;
    TIM14->ARR = 3199;

    NVIC_EnableIRQ(TIM14_IRQn);

    TIM14->DIER |= TIM_DIER_UIE;
    TIM14->CR1 |= TIM_CR1_CEN;
}

// Write TMAG5273 settings
void TMAG5273_Init(void)
{
    // Set 2x average sampling 0x1
    TMAG5273_WriteRegister(0, (0x1 << 2));

    // Set low noise mode (higher power) 0x1
    // Set operating mode continuous 0x2
    // I2C glitch filter on
    TMAG5273_WriteRegister(1, (1 << 4) | (2 << 0) | (1 << 3) );

    // Enable channel X, Y, Z
    TMAG5273_WriteRegister(2, (7 << 4));
}

// Single read
uint8_t TMAG5273_ReadRegister(uint8_t reg)
{
    uint8_t val;
    TMAG5273_ReadRegisters(reg, &val, 1);
    return val;
}

// Single write
void TMAG5273_WriteRegister(uint8_t reg, uint8_t value)
{

    // Reset CR2 and configure write transfer
    I2C1->CR2 = 0;
    I2C1->CR2 = ((TMAG5273_ADDR << 1) & I2C_CR2_SADD) |
                (2U << I2C_CR2_NBYTES_Pos); // 2 bytes: reg + value
    I2C1->CR2 &= ~I2C_CR2_RD_WRN; // write mode
    I2C1->CR2 |= I2C_CR2_START;   // generate START

    // Send reg
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = reg;

    // Send value
    while (!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = value;

    // Wait for transfer complete
    while (!(I2C1->ISR & I2C_ISR_TC));
    I2C1->CR2 |= I2C_CR2_STOP;  // Generate STOP
    while (!(I2C1->ISR & I2C_ISR_STOPF));
    I2C1->ICR = I2C_ICR_STOPCF; // Clear STOP flag

}


// Multi-read
void TMAG5273_ReadRegisters(uint8_t reg, uint8_t *buf, uint8_t len)
{
    // WRITE phase: send register address
    I2C1->CR2 = ((TMAG5273_ADDR << 1) & I2C_CR2_SADD) |
                (1U << I2C_CR2_NBYTES_Pos) |
                (0U << I2C_CR2_RD_WRN_Pos);    // write, 1 byte (the register)

    I2C1->CR2 |= I2C_CR2_START; // generate START

    while (!(I2C1->ISR & I2C_ISR_TXIS));   // wait for TX ready
    I2C1->TXDR = reg;                  // send register address

    while (!(I2C1->ISR & I2C_ISR_TC));     // wait for transfer complete

    // READ phase: get data
    I2C1->CR2 = ((TMAG5273_ADDR << 1) & I2C_CR2_SADD) |
                ((uint32_t)len << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_RD_WRN |           // read direction
                I2C_CR2_AUTOEND;           // auto STOP after all bytes

    I2C1->CR2 |= I2C_CR2_START; // repeated START

    // Read len bytes
    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE));   // wait for data
        buf[i] = (uint8_t)I2C1->RXDR;          // read received byte
    }
}


// Read X, Y, Z fields
void TMAG5273_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t data[6];
    TMAG5273_ReadRegisters(TMAG5273_REG_X_MSB, data, 6);

    *x = (int16_t)((data[0] << 8) | data[1]);
    *y = (int16_t)-((data[2] << 8) | data[3]); // Sign reversed here to match hardware config
    *z = (int16_t)((data[4] << 8) | data[5]);
}