//
// Created by Alessandro Nardinelli on 14/01/26.
//

#ifndef OPENCASIO_LCD_H
#define OPENCASIO_LCD_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/rcc.h"

typedef struct {
      LCD_RegTypeDef *pToLCD;
} LCD_Handler_t;


uint8_t LCD_Init(LCD_Handler_t *pToLCDHandler);

void lcdBlink(void);
void lcdDisable(void);
void lcdGetStatus(void);
void lcdDisplayLow(void);
void lcdDisplayHigh(void);


#endif //OPENCASIO_LCD_H