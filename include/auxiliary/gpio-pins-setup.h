#ifndef OPENCASIO_GPIO_PINS_SETUP_H
#define OPENCASIO_GPIO_PINS_SETUP_H

#include "driver/gpio.h"

// Phase 1 only: board inputs and inactive sensor/LED enables.
// Returns CORE_OK or GPIO_CFG_ERR; no I2C/LCD/buzzer activation.
uint8_t Board_GPIO_Init(void);
uint8_t Btn_GPIO_Init(void);

void LCD_GPIO_Init(void);
void I2C_GPIO_Init(void);
void Buzzer_GPIO_Init(void);

#endif // OPENCASIO_GPIO_PINS_SETUP_H
