//
// Created by Alessandro Nardinelli on 14/11/25.
//

// import section
#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"
#include "driver/i2c.h"
#include "auxiliary/mmc5603nj.h"
#include "auxiliary/rv-3129-c3.h"


I2C_Handle_t pToI2C = {
    .pI2Cx = I2C,
    .I2C_PinConfig = {
        .I2C_ClockSpeed = 100000,          // Standard mode per DEVELOPMENT_PLAN Phase 2
        .I2C_Mode       = I2C_MODE_STANDARD,
    },
};

// Bus-scan results, readable from the debugger at the WFI loop below.
// Indexed by raw address byte >> 1 (7-bit address); 1 = ACK, 0 = NACK/absent.
volatile uint8_t i2cScanAck[128];
// Raw addresses probed: RTC always-on; MAG/BME280 behind gated rails.
volatile uint8_t i2cScanRtc, i2cScanMag, i2cScanBme76, i2cScanBme77;

// Phase 2 bring-up aid only: replace with the event-driven superloop in
// Phase 7. Returning CORE_OK lets main continue to WFI even if a device is
// missing; the scan bitmaps above carry the outcome for the debugger.
__attribute__((weak)) uint8_t i2cScanComplete(void) {
    return CORE_OK;
}

// Coarse settle delay after raising a sensor rail. U11/U12 part numbers are
// unknown (REFERENCE.md §7.3), so the rise time is unverified: start with a
// conservative ~1 ms at 16 MHz and tighten it after measuring on the board.
static void railSettleDelay(void) {
    for (volatile uint32_t i = 0; i < 4000; i++) __asm volatile ("nop");
}

// TEMP_EN / MAG_EN share the Board_GPIO_Init pin settings; these handles
// only retarget the write helpers at PB0/PB1.
static const GPIO_Handle_t tempEnPin = {
    .pGPIOx = GPIOB, .GPIO_PinConfig = { .GPIO_PinNumber = 0 },
};
static const GPIO_Handle_t magEnPin = {
    .pGPIOx = GPIOB, .GPIO_PinConfig = { .GPIO_PinNumber = 1 },
};

int main() {

      // set clock to 16MHz
      if (!initRCC()) return RCC_CFG_ERR;             //if clock is unable to be set to HSI, return RCC_CFG_ERR and terminate

      uint8_t status = Board_GPIO_Init();
      if (status != CORE_OK) return status;

      status = I2C_Init(&pToI2C);
      if (status != CORE_OK) return status;

      // Probe order: RTC first (always powered), then raise both sensor
      // rails once and probe MAG + BME280 (both 0x76 and 0x77 — the SDO
      // strap is unknown, REFERENCE.md §7.5), then gate the rails off.
      i2cScanRtc = (I2C_Transmit(&pToI2C, 0, 0, 0, RTC_ADDR) == CORE_OK);
      i2cScanAck[RTC_ADDR >> 1] = i2cScanRtc;

      GPIO_WriteToOutputPin(&tempEnPin, 1);
      GPIO_WriteToOutputPin(&magEnPin, 1);
      railSettleDelay();

      i2cScanMag   = (I2C_Transmit(&pToI2C, 0, 0, 0, MAG_ADDR) == CORE_OK);
      i2cScanBme76 = (I2C_Transmit(&pToI2C, 0, 0, 0, 0xEC) == CORE_OK); // 0x76<<1
      i2cScanBme77 = (I2C_Transmit(&pToI2C, 0, 0, 0, 0xEE) == CORE_OK); // 0x77<<1
      i2cScanAck[MAG_ADDR >> 1] = i2cScanMag;
      i2cScanAck[0x76]          = i2cScanBme76;
      i2cScanAck[0x77]          = i2cScanBme77;

      GPIO_WriteToOutputPin(&tempEnPin, 0);
      GPIO_WriteToOutputPin(&magEnPin, 0);

      status = i2cScanComplete();
      if (status != CORE_OK) return status;

      // Bring-up checkpoint: inspect i2cScanAck[] / the four result bytes
      // through ST-Link. EXTI wake sources (3 buttons rising, RTC_INT
      // falling) are armed; WFI returns on each event until Phase 7 adds
      // the event-driven superloop.
      while (1)
      {
            __asm volatile ("wfi");

      }

}
