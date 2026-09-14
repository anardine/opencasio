//
// Created by Alessandro Nardinelli on 14/01/26.
//

#ifndef OPENCASIO_LCD_H
#define OPENCASIO_LCD_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/rcc.h"
#include "auxiliary/gpio-pins-setup.h"
#include "etc/error.h"

// The F-91W glass has 3 commons (COM0-2) and 24 segment lines.
// 1/3 duty, 1/3 bias. The glass truth table (pad ↔ digit-segment map)
// is adapted from the Sensor-Watch project (joeycastillo/Sensor-Watch),
// which uses the same Casio F-91W glass.

// LCD status return type.
typedef struct {
    uint8_t enabled;
    uint8_t ready;
    uint8_t update_done;
    uint8_t sof;
    uint8_t fcr_synced;
} lcd_status_t;

// Indicator segments (F-91W glass, from Sensor-Watch).
typedef enum {
    LCD_INDICATOR_SIGNAL = 0,  // hourly signal / sensor-on
    LCD_INDICATOR_BELL,        // alarm set
    LCD_INDICATOR_PM,          // PM indicator
    LCD_INDICATOR_24H,         // 24-hour mode
    LCD_INDICATOR_LAP          // stopwatch lap
} lcd_indicator_t;

#define LCD_NUM_POSITIONS  10  // 10 digit positions (0=day-of-week, 2-3=day, 4-9=time)

// Initialize the LCD controller:
// - Route LSI1 to RTCCLK (BDCR.RTCSEL) for LCDCLK
// - Enable LCD APB clock, configure GPIO pins (AF11)
// - 1/3 duty, 1/3 bias, internal step-up (VSEL=0), buffered mode
// - Frame rate ~31 Hz (PS=4, DIV=6 per Table 115)
// - Wait for RDY, then enable display
// Returns CORE_OK or LCD_CFG_ERR.
uint8_t LCD_Init(void);

// Disable the LCD: clear LCDEN, wait for ENS to clear.
void lcdDisable(void);

// Read LCD status register into struct.
lcd_status_t lcdGetStatus(void);

// Set/clear a single pixel by COM (0-2) and SEG (0-23).
void lcdSetPixel(uint8_t com, uint8_t seg);
void lcdClearPixel(uint8_t com, uint8_t seg);

// Write segment data to LCD RAM for a given COM (0-2).
// segLow = SEG[31:0], segHigh = SEG[43:32] (only bits 11:0 used).
uint8_t lcdDisplayWrite(uint8_t com, uint32_t segLow, uint32_t segHigh);

// Clear all LCD RAM (all segments off). Call lcdDisplayUpdate() after
// rendering the complete frame.
void lcdDisplayClear(void);

// Trigger UDR: transfers LCD_RAM to display buffer at next frame.
void lcdDisplayUpdate(void);

// Display a single character at a digit position (0-9).
// Uses the F-91W glass segment map from Sensor-Watch.
void lcdDisplayChar(char c, uint8_t position);

// Display a null-terminated string starting at position.
// Space clears the digit. Max 10 characters.
void lcdDisplayString(const char *str, uint8_t position);

// Colon between hours and minutes.
void lcdSetColon(void);
void lcdClearColon(void);

// Indicator segments.
void lcdSetIndicator(lcd_indicator_t indicator);
void lcdClearIndicator(lcd_indicator_t indicator);
void lcdClearAllIndicators(void);

#endif //OPENCASIO_LCD_H
