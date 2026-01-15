//
// Created by Alessandro Nardinelli on 14/01/26.
//

#ifndef OPENCASIO_LCD_H
#define OPENCASIO_LCD_H
#pragma once

#include "driver/stm32wb55xx.h"


void lcdInit(void);
void lcdBlink(void);
void lcdDisable(void);
void lcdGetStatus(void);
void lcdDisplayLow(void);
void lcdDisplayHigh(void);


#endif //OPENCASIO_LCD_H