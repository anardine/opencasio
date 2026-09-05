//
// Created by Alessandro Nardinelli on 12/01/26.
//

#include "auxiliary/mmc5603nj.h"
#include <math.h>

// MMC5603NJ uses repeated-START I2C (unlike the RV-3129-C3 RTC).
// But our I2C driver sends AUTOEND (STOP) on every transaction, so
// readFromMag is the same two-transaction pattern as readFromRTC:
// write register address (STOP), then read N bytes (STOP).

uint8_t readFromMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    uint8_t status = I2C_Transmit(pToI2CHandle, 0, reg, 0, MAG_ADDR);
    if (status != CORE_OK) return status;
    return I2C_Receive(pToI2CHandle, buf, len, MAG_ADDR);
}

uint8_t writeToMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len) {
    return I2C_Transmit(pToI2CHandle, buf, reg, len, MAG_ADDR);
}

// Initialize: verify product ID, enable Auto_SR, perform SET/RESET.
// On-demand mode by default (MAG_CONTINUOUS_MODE = 0 in device_config.h).
// For continuous mode, define MAG_CONTINUOUS_MODE=1 in device_config.h.
uint8_t magInit(I2C_Handle_t *pToI2CHandle) {
    // Verify device presence via product ID (§Product ID 1, 0x39 = 0x10).
    uint8_t pid;
    uint8_t status = readFromMag(pToI2CHandle, MAG_REG_PRODUCT_ID, &pid, 1);
    if (status != CORE_OK) return status;
    if (pid != MAG_PRODUCT_ID) return MAG_INIT_CFG_ERR;

#if MAG_CONTINUOUS_MODE
    // Enable automatic set/reset and set ODR for continuous mode.
    uint8_t ctrl0 = MAG_CTRL0_AUTO_SR | MAG_CTRL0_CMM_FREQ_EN;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;

    uint8_t odr = 25;  // 25 Hz with BW=00 (75 Hz max for BW=00 + Auto_SR)
    status = writeToMag(pToI2CHandle, MAG_REG_ODR, &odr, 1);
    if (status != CORE_OK) return status;

    uint8_t ctrl2 = MAG_CTRL2_CMM_EN;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL2, &ctrl2, 1);
    if (status != CORE_OK) return status;
#else
    // On-demand mode: enable Auto_SR for measurement quality.
    uint8_t ctrl0 = MAG_CTRL0_AUTO_SR;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;

    // Perform initial SET/RESET to clear residual magnetization.
    status = magCalibrate(pToI2CHandle);
    if (status != CORE_OK) return status;
#endif
    return CORE_OK;
}

// SET then RESET to clear residual magnetization (§SET/RESET operation).
// Each operation is self-clearing after 375 ns; no polling needed.
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle) {
    uint8_t cmd = MAG_CTRL0_DO_SET;
    uint8_t status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &cmd, 1);
    if (status != CORE_OK) return status;

    cmd = MAG_CTRL0_DO_RESET;
    return writeToMag(pToI2CHandle, MAG_REG_CTRL0, &cmd, 1);
}

// Trigger one magnetic measurement and poll until data is ready.
// Status1.MEAS_M_DONE (bit 1) = 1 means X/Y/Z data available.
// Timeout: 100k iterations at 16 MHz ≈ 12 ms — well above the 6.6 ms
// measurement time for BW=00 (the default, §Internal Control 1).
uint8_t magGetData(I2C_Handle_t *pToI2CHandle, mag_data_t *data) {
    // Read current CTRL0, set TAKE_MEAS_M bit (preserve Auto_SR).
    uint8_t ctrl0;
    uint8_t status = readFromMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;
    ctrl0 |= MAG_CTRL0_TAKE_MEAS_M;
    status = writeToMag(pToI2CHandle, MAG_REG_CTRL0, &ctrl0, 1);
    if (status != CORE_OK) return status;

    // Poll Status1 for Meas_M_Done (bit 1).
    uint8_t status1 = 0;
    uint32_t timeout = 100000;
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
    // Xout[19:12] = raw[0], Xout[11:4] = raw[1], Xout[3:0] = raw[6] >> 4.
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

// Heading from X and Y components. atan2(y, x) gives standard math angle;
// compass heading = 90 - math_angle, normalized to 0-360.
// Returns 0-3599 (0.1° resolution).
uint16_t magTransformToHeading(const mag_data_t *data) {
    float headingDeg = 90.0f - (float)(atan2((double)data->y_mG, (double)data->x_mG) * 180.0 / M_PI);
    if (headingDeg < 0.0f) headingDeg += 360.0f;
    if (headingDeg >= 360.0f) headingDeg -= 360.0f;
    return (uint16_t)(headingDeg * 10.0f);
}
