//
// Created by Alessandro Nardinelli on 14/01/26.
//
// BME280 driver. Register map and integer compensation formulas from
// docs/bme280_ds.txt (§5.3 register map, §8.2 compensation appendix).
//

#include "auxiliary/bme280.h"

// Raw I2C address byte resolved by the caller's bus scan (bme280Init).
static uint8_t bmeAddr;

// Calibration parameters, read once per init (cold-start-every-use policy:
// the sensor rail is gated off between uses, so init re-reads these).
static uint16_t digT1, digP1;
static int16_t digT2, digT3, digP2, digP3, digP4, digP5, digP6, digP7, digP8, digP9;
static uint8_t digH1, digH3;
static int16_t digH2, digH4, digH5, digH6;
static int32_t tFine;

uint8_t readFromBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t status = I2C_Transmit(pToI2CHandle, 0, reg, 0, bmeAddr);
    if (status != CORE_OK) return status;
    return I2C_Receive(pToI2CHandle, buf, len, bmeAddr);
}

uint8_t writeToBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    return I2C_Transmit(pToI2CHandle, buf, reg, len, bmeAddr);
}

uint8_t bme280Init(I2C_Handle_t *pToI2CHandle, uint8_t deviceAddress) {
    bmeAddr = deviceAddress;
    tFine = 0;

    // Verify presence / identity (§5.4.1: chip_id reset value 0x60).
    uint8_t id;
    uint8_t status = readFromBme(pToI2CHandle, BME280_REG_ID, &id, 1);
    if (status != CORE_OK) return status;
    if (id != BME280_CHIP_ID) return BME280_INIT_CFG_ERR;

    // Read the compensation parameters (§5.4.2 "compensation parameter
    // registers"): T1..P9 at 0x88..0xA1, H1..H6 at 0xE1..0xE7.
    uint8_t cal1[26];
    status = readFromBme(pToI2CHandle, BME280_REG_CALIB_1, cal1, 26);
    if (status != CORE_OK) return status;

    digT1 = (uint16_t)(cal1[0]  | (cal1[1] << 8));
    digT2 = (int16_t)(cal1[2]   | (cal1[3] << 8));
    digT3 = (int16_t)(cal1[4]   | (cal1[5] << 8));
    digP1 = (uint16_t)(cal1[6]  | (cal1[7] << 8));
    digP2 = (int16_t)(cal1[8]   | (cal1[9] << 8));
    digP3 = (int16_t)(cal1[10]  | (cal1[11] << 8));
    digP4 = (int16_t)(cal1[12]  | (cal1[13] << 8));
    digP5 = (int16_t)(cal1[14]  | (cal1[15] << 8));
    digP6 = (int16_t)(cal1[16]  | (cal1[17] << 8));
    digP7 = (int16_t)(cal1[18]  | (cal1[19] << 8));
    digP8 = (int16_t)(cal1[20]  | (cal1[21] << 8));
    digP9 = (int16_t)(cal1[22]  | (cal1[23] << 8));
    digH1 = cal1[25];  // 0xA1: H1, unsigned

    uint8_t cal2[7];
    status = readFromBme(pToI2CHandle, BME280_REG_CALIB_2, cal2, 7);
    if (status != CORE_OK) return status;
    digH2 = (int16_t)(cal2[0] | (cal2[1] << 8));            // H2: 16-bit signed
    digH3 = cal2[2];                                        // H3: unsigned
    digH4 = (int16_t)((cal2[3] << 4) | (cal2[4] & 0x0FU));  // H4: 12-bit signed
    digH5 = (int16_t)((cal2[4] >> 4) | (cal2[5] << 4));     // H5: 12-bit signed
    digH6 = (int8_t)cal2[6];                                // H6: 8-bit signed

    // osrs_h first (§5.4.3: ctrl_hum changes take effect only after a
    // subsequent write to ctrl_meas).
    uint8_t ctrlHum = BME280_OSRS_X1;
    status = writeToBme(pToI2CHandle, BME280_REG_CTRL_HUM, &ctrlHum, 1);
    if (status != CORE_OK) return status;

    // ctrl_meas: oversampling set, mode = sleep until a forced trigger.
    uint8_t ctrlMeas = BME280_CTRL_MEAS_OSRS | BME280_MODE_SLEEP;
    status = writeToBme(pToI2CHandle, BME280_REG_CTRL_MEAS, &ctrlMeas, 1);
    if (status != CORE_OK) return status;

    // config: t_sb/filter irrelevant in forced mode; spi3w_en = 0.
    uint8_t config = 0x00U;
    return writeToBme(pToI2CHandle, BME280_REG_CONFIG, &config, 1);
}

uint8_t bme280Measure(I2C_Handle_t *pToI2CHandle, bme280_data_t *data) {
    // Trigger a forced-mode conversion (§5.4.3: mode = 01 runs one
    // measurement, then the device returns to sleep).
    uint8_t ctrlMeas = BME280_CTRL_MEAS_OSRS | BME280_MODE_FORCED;
    uint8_t status = writeToBme(pToI2CHandle, BME280_REG_CTRL_MEAS, &ctrlMeas, 1);
    if (status != CORE_OK) return status;

    // Bounded poll of status.measuring (bit 3). Each iteration is an I2C
    // transaction, which self-throttles the 16 MHz busy loop.
    uint32_t guard = BME280_MEAS_TIMEOUT;
    uint8_t st;
    do {
        status = readFromBme(pToI2CHandle, BME280_REG_STATUS, &st, 1);
        if (status != CORE_OK) return status;
        if (!(st & BME280_STATUS_MEASURING)) break;
    } while (--guard);
    if (!guard) return BME280_INIT_CFG_ERR;

    // Burst-read the 8 data bytes with auto-increment from 0xF7 (§5.3.3):
    // press_msb/lsb/xlsb, temp_msb/lsb/xlsb, hum_msb/lsb.
    uint8_t raw[8];
    status = readFromBme(pToI2CHandle, BME280_REG_PRESS_MSB, raw, 8);
    if (status != CORE_OK) return status;

    int32_t adcP = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adcT = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
    int32_t adcH = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

    // --- Temperature (§8.2, 32-bit integer variant). t_fine feeds the
    // other two conversions and the result is °C × 100. ---
    int32_t var1 = ((((adcT >> 3) - ((int32_t)digT1 << 1))) * ((int32_t)digT2)) >> 11;
    int32_t var2 = (((((adcT >> 4) - ((int32_t)digT1)) * ((adcT >> 4) - ((int32_t)digT1))) >> 12) * ((int32_t)digT3)) >> 14;
    tFine = var1 + var2;
    int32_t t100 = (tFine * 5 + 128) >> 8;
    data->temp_C = (float)t100 / 100.0f;

    // --- Pressure (§8.2, 64-bit integer variant; Pa). ---
    int64_t var1L = ((int64_t)tFine) - 128000;
    int64_t var2L = var1L * var1L * (int64_t)digP6;
    var2L = var2L + ((var1L * (int64_t)digP5) << 17);
    var2L = var2L + (((int64_t)digP4) << 35);
    var1L = ((var1L * var1L * (int64_t)digP3) >> 8) + ((var1L * (int64_t)digP2) << 12);
    var1L = (((((int64_t)1) << 47) + var1L)) * ((int64_t)digP1) >> 33;
    if (var1L == 0) {
        // Avoid division by zero (datasheet: invalid chip / calibration).
        data->pressure_Pa = 0.0f;
    } else {
        int64_t p = 1048576 - adcP;
        p = (((p << 31) - var2L) * 3125) / var1L;
        var1L = (((int64_t)digP9) * (p >> 13) * (p >> 13)) >> 25;
        var2L = (((int64_t)digP8) * p) >> 19;
        p = ((p + var1L + var2L) >> 8) + (((int64_t)digP7) << 4);
        data->pressure_Pa = (float)p;
    }

    // --- Humidity (§8.2, 32-bit integer variant; %rH × 1024). ---
    int32_t h = tFine - ((int32_t)768000);
    // Stepwise per §8.2: v = (((((h*H6)>>10) * (((h*H3)>>11)+32768))>>10 + 2097152) * H2 + 8192) >> 14
    int32_t a = (h * ((int32_t)digH6)) >> 10;
    int32_t b = ((h * ((int32_t)digH3)) >> 11) + ((int32_t)32768);
    int32_t v = ((((a * b) >> 10) + ((int32_t)2097152)) * ((int32_t)digH2) + 8192) >> 14;
    h = ((((((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)digH1)) >> 4) + ((int32_t)2097152)) * v) >> 14) - ((int32_t)524288);
    if (h < 0)        h = 0;
    if (h > 419430400) h = 419430400;  // > 100 %rH
    data->humidity_pct = (float)h / 1024.0f;

    return CORE_OK;
}