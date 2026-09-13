//
// Created by Alessandro Nardinelli on 20/12/25.
//

#ifndef OPENCASIO_RV_3129_C3_H
#define OPENCASIO_RV_3129_C3_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"
#include "etc/error.h"

// Raw I2C address byte (pre-shifted write address).
// 7-bit address 0x56 → write 0xAC / read 0xAD (REFERENCE.md §6).
#define RTC_ADDR  0xACU

// RV-3129-C3 register addresses (Application Manual §3.1).
// Pages are selected by bits 7:3; bits 2:0 auto-increment within a page.
#define RTC_REG_CONTROL_1         0x00U
#define RTC_REG_CONTROL_INT       0x01U
#define RTC_REG_CONTROL_INT_FLAG  0x02U
#define RTC_REG_CONTROL_STATUS    0x03U
#define RTC_REG_CONTROL_RESET     0x04U
// Control_STATUS bit positions (§3.2.4). VLF (bit 7): voltage-low flag —
// set at power-on / after battery change; time and date are invalid then.
#define RTC_STATUS_VLF            (1U << 7)

// Clock page (auto-increment 08→0E)
#define RTC_REG_SECONDS           0x08U
#define RTC_REG_MINUTES           0x09U
#define RTC_REG_HOURS             0x0AU
#define RTC_REG_DAYS              0x0BU
#define RTC_REG_WEEKDAYS          0x0CU
#define RTC_REG_MONTHS            0x0DU
#define RTC_REG_YEARS             0x0EU

// Alarm page (auto-increment 10→16)
#define RTC_REG_SEC_ALARM         0x10U
#define RTC_REG_MIN_ALARM         0x11U
#define RTC_REG_HOUR_ALARM        0x12U

// Timer page
#define RTC_REG_TIMER_LOW         0x18U
#define RTC_REG_TIMER_HIGH        0x19U

// Control_1 bit positions (§3.2.1)
#define RTC_CTRL1_TE              (1U << 1)   // Timer enable
#define RTC_CTRL1_TAR             (1U << 2)   // Timer auto-reload
#define RTC_CTRL1_TD1             (1U << 6)   // TD1:TD0 = 10 → 1 Hz
#define RTC_CTRL1_TD0             (1U << 5)

// Control_INT bit positions (§3.2.2)
#define RTC_INT_TIE               (1U << 1)   // Timer interrupt enable
#define RTC_INT_AIE               (1U << 0)   // Alarm interrupt enable

// Control_INT Flag bit positions (§3.2.3)
#define RTC_FLAG_TF               (1U << 1)   // Timer flag (write 0 to clear)
#define RTC_FLAG_AF               (1U << 0)   // Alarm flag (write 0 to clear)

// Time in binary (caller-facing). Hours are 24-hour mode only.
typedef struct {
    uint8_t seconds;  // 0-59
    uint8_t minutes;  // 0-59
    uint8_t hours;    // 0-23
} rtc_time_t;

// Date in binary (caller-facing). Year is 0-99 (20xx).
typedef struct {
    uint8_t day;      // 1-31
    uint8_t weekday;  // 1-7 (1=Sunday, per app manual §3.3.1)
    uint8_t month;    // 1-12
    uint8_t year;     // 0-99
} rtc_date_t;

// Alarm in binary. Only seconds/minutes/hours are used; day/weekday/month/year
// alarm fields exist but are not needed for this project.
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
} rtc_alarm_t;

// BCD ↔ binary conversion (clock registers are BCD-coded per §3.3).
uint8_t bcd_to_bin(uint8_t bcd);
uint8_t bin_to_bcd(uint8_t bin);

// Low-level register access. Propagates I2C_NACK_ERR / I2C_BUS_ERR.
// readFromRTC: write reg address (STOP), then read len bytes (STOP).
//   RV-3129-C3 §6.8 forbids repeated START, so two separate transactions.
// writeToRTC: write reg address + payload in one transaction (STOP).
uint8_t readFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t writeToRTC(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);

// Time/date get/set. Convert BCD internally; caller passes binary structs.
// Uses auto-increment to read/write all fields in one I2C transaction.
// Returns CORE_OK, I2C_NACK_ERR, or I2C_BUS_ERR.
uint8_t getTime(I2C_Handle_t *pToI2CHandle, rtc_time_t *t);
uint8_t setTime(I2C_Handle_t *pToI2CHandle, const rtc_time_t *t);
uint8_t getDate(I2C_Handle_t *pToI2CHandle, rtc_date_t *d);
uint8_t setDate(I2C_Handle_t *pToI2CHandle, const rtc_date_t *d);

// Alarm functions. alarmSet writes the alarm registers (AE bits set) and
// enables AIE. alarmClear clears the AF flag. alarmInit enables AIE only.
// Returns CORE_OK, I2C error, or RTC_ALARM_CFG_ERR.
uint8_t alarmInit(I2C_Handle_t *pToI2CHandle);
uint8_t alarmClear(I2C_Handle_t *pToI2CHandle);
uint8_t alarmSet(I2C_Handle_t *pToI2CHandle, const rtc_alarm_t *a);

// Timer functions. timerInit enables the countdown timer with 1 Hz source
// clock, auto-reload, and TIE interrupt. timerSet loads the 16-bit countdown
// value (1-65536; 0 stops). timerClear clears the TF flag.
// Returns CORE_OK, I2C error, or RTC_TIMER_CFG_ERR.
// timerStop clears TE (1 Hz config and TAR preserved) — used to pause the
// tick engine. Returns CORE_OK or an I2C error.
uint8_t timerInit(I2C_Handle_t *pToI2CHandle);
uint8_t timerStop(I2C_Handle_t *pToI2CHandle);

// Load 16-bit countdown value. n=1..65536 valid; n=0 stops the timer (§4.4).
uint8_t timerClear(I2C_Handle_t *pToI2CHandle);
uint8_t timerSet(I2C_Handle_t *pToI2CHandle, uint16_t countdown);

#endif //OPENCASIO_RV_3129_C3_H
