//
// Created by Alessandro Nardinelli on 20/12/25.
//

#include "auxiliary/rv-3129-c3.h"

// BCD ↔ binary (§3.3: clock registers are BCD-coded).
// bcd_to_bin: high nibble × 10 + low nibble.
// bin_to_bcd: (bin/10 << 4) | (bin%10).
uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10U + (bcd & 0x0FU);
}

uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10U) << 4) | (bin % 10U);
}

// RV-3129-C3 §6.8: write register address with STOP, then read with STOP.
// Repeated START is forbidden, so this is two separate I2C transactions.
uint8_t readFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t status = I2C_Transmit(pToI2CHandle, 0, reg, 0, RTC_ADDR);
    if (status != CORE_OK) return status;
    return I2C_Receive(pToI2CHandle, buf, len, RTC_ADDR);
}

// Single transaction: START + addr(W) + reg + data[0..len-1] + STOP.
uint8_t writeToRTC(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    return I2C_Transmit(pToI2CHandle, buf, reg, len, RTC_ADDR);
}

// Batch read 3 bytes from 0x08 (seconds, minutes, hours) using auto-increment.
// Hours masked to 6 bits (24-hour mode; bit 6 = 12-24 stays 0 at reset).
uint8_t getTime(I2C_Handle_t *pToI2CHandle, rtc_time_t *t) {
    uint8_t bcd[3];
    uint8_t status = readFromRTC(pToI2CHandle, RTC_REG_SECONDS, bcd, 3);
    if (status != CORE_OK) return status;
    t->seconds = bcd_to_bin(bcd[0] & 0x7FU);
    t->minutes = bcd_to_bin(bcd[1] & 0x7FU);
    t->hours   = bcd_to_bin(bcd[2] & 0x3FU);
    return CORE_OK;
}

// Batch write 3 bytes to 0x08 (seconds, minutes, hours).
uint8_t setTime(I2C_Handle_t *pToI2CHandle, const rtc_time_t *t) {
    uint8_t bcd[3];
    bcd[0] = bin_to_bcd(t->seconds);
    bcd[1] = bin_to_bcd(t->minutes);
    bcd[2] = bin_to_bcd(t->hours);
    return writeToRTC(pToI2CHandle, RTC_REG_SECONDS, bcd, 3);
}

// Batch read 4 bytes from 0x0B (days, weekdays, months, years).
// Weekdays are BCD 1-7 (identical to binary); mask to 3 bits.
uint8_t getDate(I2C_Handle_t *pToI2CHandle, rtc_date_t *d) {
    uint8_t bcd[4];
    uint8_t status = readFromRTC(pToI2CHandle, RTC_REG_DAYS, bcd, 4);
    if (status != CORE_OK) return status;
    d->day      = bcd_to_bin(bcd[0] & 0x3FU);
    d->weekday  = bcd[1] & 0x07U;
    d->month    = bcd_to_bin(bcd[2] & 0x1FU);
    d->year     = bcd_to_bin(bcd[3] & 0x7FU);
    return CORE_OK;
}

// Batch write 4 bytes to 0x0B (days, weekdays, months, years).
uint8_t setDate(I2C_Handle_t *pToI2CHandle, const rtc_date_t *d) {
    uint8_t bcd[4];
    bcd[0] = bin_to_bcd(d->day);
    bcd[1] = d->weekday;  // BCD 1-7 == binary 1-7
    bcd[2] = bin_to_bcd(d->month);
    bcd[3] = bin_to_bcd(d->year);
    return writeToRTC(pToI2CHandle, RTC_REG_DAYS, bcd, 4);
}

// Enable AIE (bit 0) in Control_INT and read back to verify.
uint8_t alarmInit(I2C_Handle_t *pToI2CHandle) {
    uint8_t val = RTC_INT_AIE;
    uint8_t status = writeToRTC(pToI2CHandle, RTC_REG_CONTROL_INT, &val, 1);
    if (status != CORE_OK) return status;
    status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_INT, &val, 1);
    if (status != CORE_OK) return status;
    return (val & RTC_INT_AIE) ? CORE_OK : RTC_ALARM_CFG_ERR;
}

// Clear AF (bit 0) in Control_INT Flag. Writing 0 clears; 1 preserves other flags.
uint8_t alarmClear(I2C_Handle_t *pToI2CHandle) {
    uint8_t val = 0xFFU & ~RTC_FLAG_AF;
    return writeToRTC(pToI2CHandle, RTC_REG_CONTROL_INT_FLAG, &val, 1);
}

// Write alarm registers with AE bits set, then enable AIE.
// Only seconds/minutes/hours are used (§3.4: day/weekday/month/year alarm
// fields exist but aren't needed for this project).
uint8_t alarmSet(I2C_Handle_t *pToI2CHandle, const rtc_alarm_t *a) {
    // AE bit (bit 7) = 1 enables each alarm field for comparison.
    uint8_t bcd[3];
    bcd[0] = bin_to_bcd(a->seconds) | 0x80U;
    bcd[1] = bin_to_bcd(a->minutes) | 0x80U;
    bcd[2] = bin_to_bcd(a->hours)   | 0x80U;
    uint8_t status = writeToRTC(pToI2CHandle, RTC_REG_SEC_ALARM, bcd, 3);
    if (status != CORE_OK) return status;
    return alarmInit(pToI2CHandle);
}

// Start the shared UI tick with exact 1 s auto-reload periods. RV-3129-C3
// §4.4 only accepts TD/TAR changes while TE=0, and only accepts the timer
// count while both TE=0 and TAR=0.
uint8_t timerStart1Hz(I2C_Handle_t *pToI2CHandle) {
    uint8_t ctrl1;
    uint8_t status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_1, &ctrl1, 1);
    if (status != CORE_OK) return status;

    ctrl1 &= ~(RTC_CTRL1_TE | RTC_CTRL1_TAR | RTC_CTRL1_TD1 | RTC_CTRL1_TD0);
    status = writeToRTC(pToI2CHandle, RTC_REG_CONTROL_1, &ctrl1, 1);
    if (status != CORE_OK) return status;

    // 32 Hz source, n=31. Auto-reload periods are (n+1)/32 = 1 second.
    uint8_t count[2] = {31U, 0U};
    status = writeToRTC(pToI2CHandle, RTC_REG_TIMER_LOW, count, 2);
    if (status != CORE_OK) return status;

    status = timerClear(pToI2CHandle);
    if (status != CORE_OK) return status;

    uint8_t intEn;
    status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_INT, &intEn, 1);
    if (status != CORE_OK) return status;
    intEn |= RTC_INT_TIE;
    status = writeToRTC(pToI2CHandle, RTC_REG_CONTROL_INT, &intEn, 1);
    if (status != CORE_OK) return status;

    ctrl1 |= RTC_CTRL1_TAR | RTC_CTRL1_TE;
    status = writeToRTC(pToI2CHandle, RTC_REG_CONTROL_1, &ctrl1, 1);
    if (status != CORE_OK) return status;

    uint8_t verifyCtrl;
    status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_1, &verifyCtrl, 1);
    if (status != CORE_OK) return status;
    status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_INT, &intEn, 1);
    if (status != CORE_OK) return status;
    const uint8_t required = RTC_CTRL1_TAR | RTC_CTRL1_TE;
    return ((verifyCtrl & (required | RTC_CTRL1_TD1 | RTC_CTRL1_TD0)) == required &&
            (intEn & RTC_INT_TIE)) ? CORE_OK : RTC_TIMER_CFG_ERR;
}

// Clear TF (bit 1) in Control_INT Flag. Writing 0 clears; 1 preserves others.
uint8_t timerClear(I2C_Handle_t *pToI2CHandle) {
    uint8_t val = 0xFFU & ~RTC_FLAG_TF;
    return writeToRTC(pToI2CHandle, RTC_REG_CONTROL_INT_FLAG, &val, 1);
}

// Stop the countdown timer: clear TE; source selection, TAR and TIE stay set.
uint8_t timerStop(I2C_Handle_t *pToI2CHandle) {
    uint8_t ctrl1;
    uint8_t status = readFromRTC(pToI2CHandle, RTC_REG_CONTROL_1, &ctrl1, 1);
    if (status != CORE_OK) return status;
    ctrl1 &= ~RTC_CTRL1_TE;
    return writeToRTC(pToI2CHandle, RTC_REG_CONTROL_1, &ctrl1, 1);
}
