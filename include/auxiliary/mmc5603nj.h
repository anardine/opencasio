//
// Created by Alessandro Nardinelli on 12/01/26.
//

#ifndef OPENCASIO_MMC5603NJ_H
#define OPENCASIO_MMC5603NJ_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"
#include "etc/error.h"

// Raw I2C address byte (pre-shifted write address).
// 7-bit address 0x30 → write 0x60 / read 0x61 (REFERENCE.md §6).
#define MAG_ADDR  0x60U

// Register addresses (MMC5603NJ Rev.B §REGISTER MAP).
#define MAG_REG_XOUT0         0x00U  // Xout[19:12]
#define MAG_REG_XOUT1         0x01U  // Xout[11:4]
#define MAG_REG_YOUT0         0x02U
#define MAG_REG_YOUT1         0x03U
#define MAG_REG_ZOUT0         0x04U
#define MAG_REG_ZOUT1         0x05U
#define MAG_REG_XOUT2         0x06U  // Xout[3:0] in bits 7:4
#define MAG_REG_YOUT2         0x07U
#define MAG_REG_ZOUT2         0x08U
#define MAG_REG_TOUT          0x09U
#define MAG_REG_STATUS1       0x18U
#define MAG_REG_ODR           0x1AU
#define MAG_REG_CTRL0         0x1BU
#define MAG_REG_CTRL1         0x1CU
#define MAG_REG_CTRL2         0x1DU
#define MAG_REG_PRODUCT_ID    0x39U

// Product ID (§Product ID 1: reset value 0x10).
#define MAG_PRODUCT_ID        0x10U

// Status1 done bits (datasheet §Status1 register map — the app-note text
// "Meas_M_Done bit 1" refers to Meas_m_done_int, a factory bit; the real
// flags live at bit6/7, verified on silicon: Status1 = 0x50 while a
// measurement was pending).
#define MAG_STATUS_MEAS_DONE  (1U << 6)
#define MAG_STATUS_T_DONE     (1U << 7)

// Control 0 bits (§Internal Control 0).
#define MAG_CTRL0_TAKE_MEAS_M  (1U << 0)  // Trigger magnetic measurement
#define MAG_CTRL0_TAKE_MEAS_T  (1U << 1)  // Trigger temperature measurement
#define MAG_CTRL0_DO_SET       (1U << 3)  // SET operation
#define MAG_CTRL0_DO_RESET     (1U << 4)  // RESET operation
#define MAG_CTRL0_AUTO_SR      (1U << 5)  // Automatic set/reset
#define MAG_CTRL0_CMM_FREQ_EN  (1U << 7)  // Continuous-mode frequency calc

// Control 2 bits (§Internal Control 2).
#define MAG_CTRL2_CMM_EN       (1U << 3)  // Enable continuous mode

// Raw 20-bit output is offset binary: subtract 2^19 to center at zero.
#define MAG_NULL_FIELD         524288L

// 20-bit output: 0.0625 mG/LSB (§SPECIFICATIONS).
#define MAG_LSB_MGAUSS         0.0625f

// Mag data: 20-bit signed X, Y, Z in milli-Gauss, plus temperature in °C.
typedef struct {
    float x_mG;
    float y_mG;
    float z_mG;
    int8_t temp_C;
} mag_data_t;

// Low-level register access. Propagates I2C error codes.
uint8_t readFromMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t writeToMag(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);

// Initialize the mag. On-demand mode (default): no continuous sampling.
// Reads product ID to verify the device is present.
// Returns CORE_OK, I2C error, or MAG_INIT_CFG_ERR.
uint8_t magInit(I2C_Handle_t *pToI2CHandle);

// SET/RESET calibration to clear residual magnetization (§SET/RESET).
// Recommended after exposure to strong external fields.
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle);

// Trigger one on-demand measurement and read X/Y/Z + temperature.
// Polls Status1.MEAS_M_DONE with a bounded timeout.
// Returns CORE_OK or I2C error.
uint8_t magGetData(I2C_Handle_t *pToI2CHandle, mag_data_t *data);

// Explicitly stop continuous operation and return to the 1 µA power-down
// state. The supply stays on because this board's unpowered sensor clamps
// the shared RTC I2C bus.
uint8_t magStandby(I2C_Handle_t *pToI2CHandle);

// Reset the in-session hard/soft-iron calibration bounds. New MAG samples
// expand the bounds while the watch is rotated through the environment.
void magResetCalibration(void);

// Calculate watch heading from mag data. U13 is rotated 90° on the PCB:
// sensor +X points toward the SWDIO/SWCLK edge (watch north/forward), and
// sensor -Y points toward watch-right. Returns 0-3599 (0.1° resolution).
uint16_t magTransformToHeading(const mag_data_t *data);

#endif //OPENCASIO_MMC5603NJ_H
