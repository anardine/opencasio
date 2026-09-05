//
// Created by Alessandro Nardinelli on 14/11/25.
//

#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"
#include "driver/i2c.h"
#include "auxiliary/rv-3129-c3.h"
#include "auxiliary/mmc5603nj.h"
#include "auxiliary/bme280.h"


I2C_Handle_t pToI2C = {
    .pI2Cx = I2C,
    .I2C_PinConfig = {
        .I2C_ClockSpeed = 100000,
        .I2C_Mode       = I2C_MODE_STANDARD,
    },
};

// Bus-scan results for debugger inspection at WFI.
volatile uint8_t i2cScanAck[128];
volatile uint8_t i2cScanRtc, i2cScanMag, i2cScanBme76, i2cScanBme77;

// Phase 3: RTC read results.
rtc_time_t rtcTime;
rtc_date_t rtcDate;
volatile uint8_t rtcReadStatus;

// Phase 4: sensor measurement results for debugger inspection at WFI.
mag_data_t   magData;
bme280_data_t bmeData;
volatile uint8_t magInitStatus;
volatile uint8_t magMeasStatus;
volatile uint8_t bmeInitStatus;
volatile uint8_t bmeMeasStatus;

// BME280 address resolved from bus scan (0x76 or 0x77).
static uint8_t bme280Addr;

int main() {

      if (!initRCC()) return RCC_CFG_ERR;

      uint8_t status = Board_GPIO_Init();
      if (status != CORE_OK) return status;

      status = I2C_Init(&pToI2C);
      if (status != CORE_OK) return status;

      // --- Bus scan ---
      // RTC first (always powered), then raise sensor rails and probe
      // MAG + BME280 (both addresses — SDO strap unknown, REFERENCE.md §7.5).
      i2cScanRtc = (I2C_Transmit(&pToI2C, 0, 0, 0, RTC_ADDR) == CORE_OK);
      i2cScanAck[RTC_ADDR >> 1] = i2cScanRtc;

      railOn();
      railSettleDelay();

      i2cScanMag   = (I2C_Transmit(&pToI2C, 0, 0, 0, MAG_ADDR) == CORE_OK);
      i2cScanBme76 = (I2C_Transmit(&pToI2C, 0, 0, 0, BME280_ADDR_76) == CORE_OK);
      i2cScanBme77 = (I2C_Transmit(&pToI2C, 0, 0, 0, BME280_ADDR_77) == CORE_OK);
      i2cScanAck[MAG_ADDR >> 1] = i2cScanMag;
      i2cScanAck[0x76]          = i2cScanBme76;
      i2cScanAck[0x77]          = i2cScanBme77;

      // --- RTC ---
      // Always-powered; read time/date regardless of rail state.
      rtcReadStatus = getTime(&pToI2C, &rtcTime);
      if (rtcReadStatus == CORE_OK)
            rtcReadStatus = getDate(&pToI2C, &rtcDate);

      // --- Sensors (rails still ON from scan) ---
      if (i2cScanMag) {
            magInitStatus = magInit(&pToI2C);
            if (magInitStatus == CORE_OK)
                  magMeasStatus = magGetData(&pToI2C, &magData);
      }

      if (i2cScanBme76) {
            bme280Addr = BME280_ADDR_76;
            bmeInitStatus = bme280Init(&pToI2C, bme280Addr);
      } else if (i2cScanBme77) {
            bme280Addr = BME280_ADDR_77;
            bmeInitStatus = bme280Init(&pToI2C, bme280Addr);
      }
      if (bmeInitStatus == CORE_OK)
            bmeMeasStatus = bme280Measure(&pToI2C, &bmeData);

      // Gate sensor rails off — power discipline.
      railOff();

      // Bring-up checkpoint: inspect all result globals via ST-Link.
      // EXTI wake sources (3 buttons rising, RTC_INT falling) are armed;
      // WFI returns on each event until Phase 7 adds the superloop.
      while (1) {
            __asm volatile ("wfi");
      }
}
