//
// Created by Alessandro Nardinelli on 14/11/25.
//

#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"
#include "driver/i2c.h"
#include "driver/lcd.h"
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

// Bus-scan results.
volatile uint8_t i2cScanAck[128];
volatile uint8_t i2cScanRtc, i2cScanMag, i2cScanBme76, i2cScanBme77;

// RTC read results.
rtc_time_t rtcTime;
rtc_date_t rtcDate;
volatile uint8_t rtcReadStatus;

// Sensor results.
mag_data_t   magData;
bme280_data_t bmeData;
volatile uint8_t magInitStatus, magMeasStatus;
volatile uint8_t bmeInitStatus, bmeMeasStatus;

// LCD status.
volatile uint8_t lcdInitStatus;

// BME280 address resolved from bus scan.
static uint8_t bme280Addr;

// --- Phase 7: event flags set by EXTI ISR callbacks ---
// Each flag is set by the ISR and consumed by the superloop.
volatile uint8_t btnLedFlag;    // PC13 rising
volatile uint8_t btnModeFlag;   // PC3 rising
volatile uint8_t btnAlarmFlag;  // PE4 rising
volatile uint8_t rtcIntFlag;    // PA0 falling (RTC alarm/timer)

// EXTI ISR callback — overrides the weak default in gpio.c.
// Sets the event flag for the superloop to consume. Minimal work in ISR.
void GPIO_IRQCallback(uint8_t pinNumber) {
    switch (pinNumber) {
        case 13: btnLedFlag   = 1; break;  // PC13 = BTN_LED
        case 3:  btnModeFlag  = 1; break;  // PC3  = BTN_MODE
        case 4:  btnAlarmFlag = 1; break;  // PE4  = BTN_ALARM
        case 0:  rtcIntFlag   = 1; break;  // PA0  = RTC_INT
        default: break;
    }
}

// Process a button press: LED toggle + short beep.
static void handleButtonPress(uint8_t buttonId) {
    ledToggle();
    buzzerBeep(20);
    (void)buttonId;
}

// Process RTC interrupt: clear alarm/timer flags on the RTC, refresh time.
static void handleRtcInterrupt(void) {
    // Clear both alarm and timer flags (write 0 to AF and TF bits).
    uint8_t clearVal = 0xFFU & ~(RTC_FLAG_AF | RTC_FLAG_TF);
    writeToRTC(&pToI2C, RTC_REG_CONTROL_INT_FLAG, &clearVal, 1);
    // Refresh time from RTC.
    rtcReadStatus = getTime(&pToI2C, &rtcTime);
    if (rtcReadStatus == CORE_OK)
        rtcReadStatus = getDate(&pToI2C, &rtcDate);
}

int main() {

      if (!initRCC()) return RCC_CFG_ERR;

      uint8_t status = Board_GPIO_Init();
      if (status != CORE_OK) return status;

      status = I2C_Init(&pToI2C);
      if (status != CORE_OK) return status;

      // --- LCD ---
      lcdInitStatus = LCD_Init();
      if (lcdInitStatus != CORE_OK) return lcdInitStatus;

      // --- Bus scan ---
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
      rtcReadStatus = getTime(&pToI2C, &rtcTime);
      if (rtcReadStatus == CORE_OK)
            rtcReadStatus = getDate(&pToI2C, &rtcDate);

      // --- Sensors ---
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

      railOff();

      // Bring-up confirmation.
      ledOn();
      buzzerBeep(50);
      ledOff();

      // --- Phase 7: event-driven superloop ---
      // WFI sleeps until an EXTI interrupt fires. On wake, check event
      // flags, process, then sleep again. All sensor/LCD work happens
      // in the active phase; sleep draws minimal power (GPIO/EXTI/NVIC
      // stay clocked, peripherals gate automatically).
      while (1) {
            __asm volatile ("wfi");

            if (btnLedFlag) {
                  btnLedFlag = 0;
                  handleButtonPress(0);
            }
            if (btnModeFlag) {
                  btnModeFlag = 0;
                  handleButtonPress(1);
            }
            if (btnAlarmFlag) {
                  btnAlarmFlag = 0;
                  handleButtonPress(2);
            }
            if (rtcIntFlag) {
                  rtcIntFlag = 0;
                  handleRtcInterrupt();
            }
      }
}
