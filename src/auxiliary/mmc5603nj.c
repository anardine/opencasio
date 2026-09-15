//
// Created by Alessandro Nardinelli on 12/01/26.
//
// MMC5603NJ magnetometer driver.
//
// Bring-up notes (on-silicon, 2026-09-12):
//   - CTRL0 (0x1B) is WRITE-ONLY: reads return 0x60. The state is shadowed
//     in software — the original read-modify-write set Auto_st_en and
//     wedged the device in self-test.
//   - Meas_m_done is Status1 bit 6 and Meas_t_done is bit 7 (datasheet
//     register map). The app-note's "Meas_M_Done bit 1" text refers to
//     Meas_m_done_int, a factory bit — polling it never completes.
//

#include "auxiliary/mmc5603nj.h"
#include <math.h>

// MMC5603NJ I2C: our I2C driver always ends with STOP, so register access
// is the two-transaction pattern (write register address, STOP, then read).

uint8_t readFromMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    // Repeated START (no intervening STOP) between the pointer write and the
    // read: a write-STOP-read sequence was observed on silicon to reliably
    // return all-zero XOUT/TOUT bytes even after Status1.MEAS_M_DONE, on two
    // different boards with a clean, NACK-free bus — consistent with the
    // output latch resetting/re-arming on STOP.
    return I2C_MemRead(pToI2CHandle, reg, buf, len, MAG_ADDR);
}

uint8_t writeToMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    return I2C_Transmit(pToI2CHandle, buf, reg, len, MAG_ADDR);
}

// CTRL0 (0x1B) is WRITE-ONLY (datasheet §Internal Control 0): reads return
// 0x60 on silicon. State is tracked here instead of read-modify-write.
static uint8_t ctrl0Shadow;
static float magMinX, magMaxX, magMinY, magMaxY;
static uint16_t magCalibrationSamples;

void magResetCalibration(void) {
    magMinX = 1.0e30f;
    magMaxX = -1.0e30f;
    magMinY = 1.0e30f;
    magMaxY = -1.0e30f;
    magCalibrationSamples = 0;
}

// Initialize: verify product ID, enable Auto_SR, perform SET/RESET.
// On-demand mode (MAG_CONTINUOUS_MODE = 0 by default). Every measurement
// then re-applies the shadow before triggering.
uint8_t magInit(I2C_Handle_t *pToI2CHandle) {
    // Verify device presence via product ID (§Product ID 1, 0x39 = 0x10).
    uint8_t pid;
    uint8_t status = readFromMag(pToI2CHandle, MAG_REG_PRODUCT_ID, &pid, 1);
    if (status != CORE_OK) return status;
    if (pid != MAG_PRODUCT_ID) return MAG_INIT_CFG_ERR;

    // Enable Auto_SR for measurement quality (datasheet: recommended).
    uint8_t ctrl0 = MAG_CTRL0_AUTO_SR;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;
    ctrl0Shadow = ctrl0;

    // Perform initial SET/RESET to clear residual magnetization.
    status = magCalibrate(pToI2CHandle);
    if (status != CORE_OK) return status;
    return CORE_OK;
}

// SET then RESET to clear residual magnetization (§SET/RESET operation).
// Each op is self-clearing after 375 ns and writes the full CTRL0 byte, so
// the shadow is re-applied and ends at Auto_SR after both ops.
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle) {
    uint8_t cmd = MAG_CTRL0_AUTO_SR | MAG_CTRL0_DO_SET;
    uint8_t status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &cmd, 1);
    if (status != CORE_OK) return status;

    cmd = MAG_CTRL0_AUTO_SR | MAG_CTRL0_DO_RESET;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &cmd, 1);
    if (status != CORE_OK) return status;
    ctrl0Shadow = MAG_CTRL0_AUTO_SR;
    return CORE_OK;
}

// Trigger one magnetic measurement and poll until data is ready.
// Status1.MEAS_M_DONE (bit 6) = 1 means X/Y/Z data available.
// Poll bound: each iteration is a write+read I2C pair ≈ 0.5 ms at 100 kHz;
// 50 iterations ≈ 25 ms — far above the 6.6 ms BW=00 measurement time.
uint8_t magGetData(I2C_Handle_t *pToI2CHandle, mag_data_t *data) {
    uint8_t ctrl0 = ctrl0Shadow | MAG_CTRL0_TAKE_MEAS_M;
    uint8_t status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;

    uint8_t status1 = 0;
    uint32_t timeout = 50;
    while (!(status1 & MAG_STATUS_MEAS_DONE)) {
        status = readFromMag(pToI2CHandle, MAG_REG_STATUS1, &status1, 1);
        if (status != CORE_OK) return status;
        if (--timeout == 0U) return I2C_BUS_ERR;
    }

    // Burst read 9 bytes from XOUT0 (0x00) through ZOUT2 (0x08).
    uint8_t raw[9];
    status = readFromMag(pToI2CHandle, MAG_REG_XOUT0, raw, 9);
    if (status != CORE_OK) return status;

    // Assemble 20-bit values (§Xout0/Xout1/Xout2).
    int32_t xRaw = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[6] >> 4);
    int32_t yRaw = ((int32_t)raw[2] << 12) | ((int32_t)raw[3] << 4) | ((int32_t)raw[7] >> 4);
    int32_t zRaw = ((int32_t)raw[4] << 12) | ((int32_t)raw[5] << 4) | ((int32_t)raw[8] >> 4);

    // Convert from offset binary to signed, then to milli-Gauss.
    data->x_mG = (float)(xRaw - MAG_NULL_FIELD) * MAG_LSB_MGAUSS;
    data->y_mG = (float)(yRaw - MAG_NULL_FIELD) * MAG_LSB_MGAUSS;
    data->z_mG = (float)(zRaw - MAG_NULL_FIELD) * MAG_LSB_MGAUSS;

    // Temperature: 8-bit unsigned, 0 = -75°C, 0.8°C/LSB (§Temperature Out).
    uint8_t tout;
    status = readFromMag(pToI2CHandle, MAG_REG_TOUT, &tout, 1);
    if (status != CORE_OK) return status;
    data->temp_C = (int8_t)((int16_t)tout * 4 / 5 - 75);

    return CORE_OK;
}

uint8_t magStandby(I2C_Handle_t *pToI2CHandle) {
    uint8_t zero = 0;
    uint8_t status = writeToMag(pToI2CHandle, MAG_REG_CTRL2, &zero, 1);
    if (status != CORE_OK) return status;
    status = writeToMag(pToI2CHandle, MAG_REG_ODR, &zero, 1);
    if (status != CORE_OK) return status;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &zero, 1);
    if (status == CORE_OK) ctrl0Shadow = 0;
    return status;
}

// U13 rotation maps watch-forward to sensor +X and watch-right to sensor -Y.
// atan2(right, forward) gives clockwise compass degrees from the SWD edge.
// Returns 0-3599 (0.1° resolution).
uint16_t magTransformToHeading(const mag_data_t *data) {
    if (data->x_mG < magMinX) magMinX = data->x_mG;
    if (data->x_mG > magMaxX) magMaxX = data->x_mG;
    if (data->y_mG < magMinY) magMinY = data->y_mG;
    if (data->y_mG > magMaxY) magMaxY = data->y_mG;
    if (magCalibrationSamples < UINT16_MAX) magCalibrationSamples++;

    float x = data->x_mG;
    float y = data->y_mG;
    const float spanX = magMaxX - magMinX;
    const float spanY = magMaxY - magMinY;
    if (magCalibrationSamples >= 4U && spanX > 1.0f && spanY > 1.0f) {
        const float centerX = (magMaxX + magMinX) * 0.5f;
        const float centerY = (magMaxY + magMinY) * 0.5f;
        const float radiusX = spanX * 0.5f;
        const float radiusY = spanY * 0.5f;
        const float radius = (radiusX + radiusY) * 0.5f;
        x = (data->x_mG - centerX) * radius / radiusX;
        y = (data->y_mG - centerY) * radius / radiusY;
    }

    float headingDeg = (float)(atan2((double)-y, (double)x) * 180.0 / M_PI);
    if (headingDeg < 0.0f) headingDeg += 360.0f;
    if (headingDeg >= 360.0f) headingDeg -= 360.0f;
    return (uint16_t)(headingDeg * 10.0f);
}