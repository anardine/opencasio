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
// In 1/3 duty with MUX_SEG=0 on VFQFPN68, SEG[42:40] and SEG[24:0]
// are available (28 lines); we use 24 of those (REFERENCE.md §4).
// The glass truth table (pad ↔ digit-segment map) is not yet known
// (REFERENCE.md §7.2), so rendering functions are deferred.

// LCD status return type.
typedef struct {
    uint8_t enabled;       // ENS
    uint8_t ready;         // RDY (step-up converter)
    uint8_t update_done;   // UDD
    uint8_t sof;           // SOF (start of frame)
    uint8_t fcr_synced;    // FCRSF
} lcd_status_t;

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

// Write segment data to LCD RAM for a given COM (0-2).
// Each COM has a 32-bit low word (SEG[31:0]) and 12-bit high word
// (SEG[43:32]). The caller provides both; unused bits are ignored.
// After writing, sets UDR to transfer to display buffer.
// Returns CORE_OK or LCD_CFG_ERR (invalid COM index).
uint8_t lcdDisplayWrite(uint8_t com, uint32_t segLow, uint32_t segHigh);

// Clear all LCD RAM (all segments off) and trigger update.
void lcdDisplayClear(void);

// Trigger UDR: transfers LCD_RAM to display buffer at next frame.
// Must be called after any RAM write for changes to appear.
void lcdDisplayUpdate(void);

#endif //OPENCASIO_LCD_H
