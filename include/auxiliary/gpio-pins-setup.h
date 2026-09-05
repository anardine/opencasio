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

// Sensor rail control. PB0 = TEMP_EN, PB1 = MAG_EN (REFERENCE.md §5).
// EN polarity assumed active-HIGH (U11/U12 part unknown, REFERENCE.md §7.3).
// railOn raises both rails; railOff gates both off. Call railSettleDelay
// after railOn before I2C access to the mag or BME280.
void railOn(void);
void railOff(void);
void railSettleDelay(void);

#endif // OPENCASIO_GPIO_PINS_SETUP_H
