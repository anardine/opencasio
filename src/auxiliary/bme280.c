//
// BME280 driver — temperature / pressure / humidity (U4, LGA-8).
// Register map and compensation formulas: docs/bme280_ds.txt (Bosch Sensortec
// BME280 datasheet), register map §5.3, compensation §8.2.
//
// Bring-up notes (on-silicon, 2026-09-12):
//   - After a soft reset (0xB6) the calibration registers read 0x00 for a
//     while — wait ≈20 ms before reading them (2 ms was too short and
//     yielded all-zero compensated output).
//   - The first forced trigger after rail churn was ACKed but the
//     conversion did not run (data stayed at reset defaults); the driver
//     re-triggers once when it detects reset-default data.
//   - The Bosch 64-bit pressure variant returns Pa in Q24.8 (×256) — the
//     result is divided by 256 before use.
//

#include "auxiliary/bme280.h"

// Raw I2C address byte resolved by the caller's bus scan (bme280Init).
static uint8_t bmeAddr;

// Calibration parameters, read once per init (cold-start-every-use policy:
// the sensor rail is gated off between uses, so init re-reads these).
static uint16_t digT1, digP1;
static int16_t digT2, digT3, digP2, digP3, digP4, digP5, digP6, digP7, digP8, digP9;
static uint8_t digH1, digH3;
static int16_t digH2, digH6;
static int32_t digH4, digH5;   // 12-bit, sign-extended via the int8 msb
static int32_t tFine;

uint8_t readFromBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t status = I2C_Transmit(pToI2CHandle, 0, reg, 0, bmeAddr);
    if (status != CORE_OK) return status;
    return I2C_Receive(pToI2CHandle, buf, len, bmeAddr);
}

uint8_t writeToBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    return I2C_Transmit(pToI2CHandle, buf, reg, len, bmeAddr);
}

// True when the 8-byte data burst is still the reset default
// (mid-scale "zero field", 0x80000/0x8000) — i.e. no conversion ran.
static uint8_t dataIsResetDefault(const uint8_t *raw) {
    return raw[0] == 0x80U && raw[1] == 0x00U && raw[2] == 0x00U &&
           raw[3] == 0x80U && raw[4] == 0x00U && raw[5] == 0x00U &&
           raw[6] == 0x80U && raw[7] == 0x00U;
}

uint8_t bme280Init(I2C_Handle_t *pToI2CHandle, uint8_t deviceAddress) {
    bmeAddr = deviceAddress;
    tFine = 0;

    // Soft reset first (§5.4.1: write 0xB6 to 0xE0; power-on time ≈ 2 ms,
    // but silicon needs ≈20 ms before the calibration registers are valid).
    uint8_t rst = BME280_RESET_CMD;
    uint8_t status = writeToBme(pToI2CHandle, BME280_REG_RESET, &rst, 1);
    if (status != CORE_OK) return status;
    for (volatile uint32_t d = 0; d < 320000; d++) __asm volatile ("nop");

    // Verify presence / identity (§5.4.1: chip_id reset value 0x60).
    uint8_t id;
    status = readFromBme(pToI2CHandle, BME280_REG_ID, &id, 1);
    if (status != CORE_OK) return status;
    if (id != BME280_CHIP_ID) return BME280_INIT_CFG_ERR;

    // Read the compensation parameters (§5.4.2): T1..P9 at 0x88..0xA1,
    // H1..H6 at 0xE1..0xE7.
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
    // Sanity: a real chip always reports a non-zero H2. Silicon showed an
    // occasional all-zero burst here right after the rail came up — re-read
    // once, then fail with a distinct status instead of emitting 0 %rH.
    digH2 = (int16_t)(cal2[0] | (cal2[1] << 8));            // H2: 16-bit signed
    digH3 = cal2[2];                                        // H3: unsigned
    // H4/H5: 12-bit, sign carried by the msb byte (Bosch SensorAPI parse).
    digH4 = ((int32_t)(int8_t)cal2[3]) * 16 | (cal2[4] & 0x0F);
    digH5 = ((int32_t)(int8_t)cal2[5]) * 16 | (cal2[4] >> 4);
    digH6 = (int8_t)cal2[6];                                // H6: 8-bit signed
    digH2 = (int16_t)(cal2[0] | (cal2[1] << 8));            // H2: 16-bit signed
    // osrs_h first (§5.4.3: ctrl_hum takes effect only after a subsequent
    // ctrl_meas write).
    uint8_t ctrlHum = BME280_OSRS_X1;
    status = writeToBme(pToI2CHandle, BME280_REG_CTRL_HUM, &ctrlHum, 1);
    if (status != CORE_OK) return status;

    // ctrl_meas: oversampling set, mode = sleep until a forced trigger.
    uint8_t ctrlMeas = BME280_CTRL_MEAS_OSRS | BME280_MODE_SLEEP;
    status = writeToBme(pToI2CHandle, BME280_REG_CTRL_MEAS, &ctrlMeas, 1);
    if (status != CORE_OK) return status;
    uint8_t config = 0x00U;
    return writeToBme(pToI2CHandle, BME280_REG_CONFIG, &config, 1);
}

// Bounded wait for status.measuring to clear (§5.4.3). Each poll iteration
// is an I2C transaction (~0.4 ms at 100 kHz) — 50 iterations ≈ 20 ms, far
// above the ≈8 ms x1/x1/x1 conversion time.
static uint8_t waitConversionDone(I2C_Handle_t *pToI2CHandle) {
    uint32_t guard = 50;
    uint8_t st;
    do {
        uint8_t status = readFromBme(pToI2CHandle, BME280_REG_STATUS, &st, 1);
        if (status != CORE_OK) return status;
        if (!(st & BME280_STATUS_MEASURING)) break;
    } while (--guard);
    return guard ? CORE_OK : BME280_INIT_CFG_ERR;
}

uint8_t bme280Measure(I2C_Handle_t *pToI2CHandle, bme280_data_t *data) {
    // Re-latch osrs_h and trigger the forced conversion in one sequence
    // (§5.4.3: ctrl_hum takes effect only on a subsequent ctrl_meas write).
    uint8_t ctrlHum = BME280_OSRS_X1;
    uint8_t status = writeToBme(pToI2CHandle, BME280_REG_CTRL_HUM, &ctrlHum, 1);
    if (status != CORE_OK) return status;
    uint8_t ctrlMeas = BME280_CTRL_MEAS_OSRS | BME280_MODE_FORCED;
    status = writeToBme(pToI2CHandle, BME280_REG_CTRL_MEAS, &ctrlMeas, 1);
    if (status != CORE_OK) return status;

    status = waitConversionDone(pToI2CHandle);
    if (status != CORE_OK) return status;

    // Burst-read the 8 data bytes with auto-increment from 0xF7 (§5.3.3).
    uint8_t raw[8];
    status = readFromBme(pToI2CHandle, BME280_REG_PRESS_MSB, raw, 8);
    if (status != CORE_OK) return status;

    // On silicon the first trigger after rail churn was ACKed but the
    // conversion did not run (data stayed at reset defaults) — re-trigger
    // once when the burst is still the reset default.
    if (dataIsResetDefault(raw)) {
        status = writeToBme(pToI2CHandle, BME280_REG_CTRL_MEAS, &ctrlMeas, 1);
        if (status != CORE_OK) return status;
        status = waitConversionDone(pToI2CHandle);
        if (status != CORE_OK) return status;
        status = readFromBme(pToI2CHandle, BME280_REG_PRESS_MSB, raw, 8);
        if (status != CORE_OK) return status;
        if (dataIsResetDefault(raw)) return BME280_INIT_CFG_ERR;
    }

    int32_t adcP = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adcT = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
    int32_t adcH = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

    // --- Temperature (§8.2, 32-bit integer variant). t_fine feeds the
    // other two conversions; result is °C × 100. ---
    int32_t var1 = ((((adcT >> 3) - ((int32_t)digT1 << 1))) * ((int32_t)digT2)) >> 11;
    int32_t var2 = (((((adcT >> 4) - ((int32_t)digT1)) * ((adcT >> 4) - ((int32_t)digT1))) >> 12) * ((int32_t)digT3)) >> 14;
    tFine = var1 + var2;
    int32_t t100 = (tFine * 5 + 128) >> 8;
    data->temp_C = (float)t100 / 100.0f;

    // --- Pressure (§8.2, 64-bit integer variant; Pa in Q24.8). ---
    int64_t var1L = ((int64_t)tFine) - 128000;
    int64_t var2L = var1L * var1L * (int64_t)digP6;
    var2L = var2L + ((var1L * (int64_t)digP5) << 17);
    var2L = var2L + (((int64_t)digP4) << 35);
    var1L = ((var1L * var1L * (int64_t)digP3) >> 8) + ((var1L * (int64_t)digP2) << 12);
    var1L = (((((int64_t)1) << 47) + var1L)) * ((int64_t)digP1) >> 33;
    if (var1L == 0) {
        // Avoid division by zero (invalid calibration).
        data->pressure_Pa = 0.0f;
    } else {
        int64_t p = 1048576 - adcP;
        p = (((p << 31) - var2L) * 3125) / var1L;
        var1L = (((int64_t)digP9) * (p >> 13) * (p >> 13)) >> 25;
        var2L = (((int64_t)digP8) * p) >> 19;
        p = ((p + var1L + var2L) >> 8) + (((int64_t)digP7) << 4);
        data->pressure_Pa = (float)p / 256.0f;   // Q24.8 → Pa
    }

    // --- Humidity: Bosch SensorAPI compensate_humidity (int32 variant),
    // ported verbatim; output var5/4096 = %rH × 1024, 102400 = 100 %rH. ---
    int32_t hv1 = tFine - ((int32_t)76800);
    int32_t hv2 = adcH * 16384;
    int32_t var3 = digH4 * 1048576;
    int32_t var4 = digH5 * hv1;
    int32_t var5 = (((hv2 - var3 - var4) + ((int32_t)16384)) / 32768);
    hv2 = (hv1 * digH6) / 1024;
    var3 = (hv1 * digH3) / 2048;
    hv2 = ((var4 * digH2) + 8192) / 16384;
    var3 = var5 * hv2;
    var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
    var5 = var3 - ((var4 * digH1) / 16);
    if (var5 < 0)         var5 = 0;
    if (var5 > 419430400) var5 = 419430400;
    data->humidity_pct = (float)(var5 / 4096) / 1024.0f;

    return CORE_OK;
}