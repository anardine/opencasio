//
// BME280 driver — temperature / pressure / humidity (U4, LGA-8).
// Register map and compensation formulas: docs/bme280_ds.txt (Bosch Sensortec
// BME280 datasheet), register map §5.3, compensation §8.2 (integer appendix).
//

#ifndef OPENCASIO_BME280_H
#define OPENCASIO_BME280_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"
#include "etc/error.h"

// Raw I2C address bytes (pre-shifted write address, project convention).
// 7-bit 0x76 (SDO = GND) → write 0xEC / read 0xED.
// 7-bit 0x77 (SDO = VDDIO) → write 0xEE / read 0xEF. SDO strap not visible
// in the netlist (REFERENCE.md §7.5); the bus scan resolves which one ACKs.
#define BME280_ADDR_76  0xECU
#define BME280_ADDR_77  0xEEU

// Register addresses (datasheet §5.3 register map).
#define BME280_REG_ID            0xD0U  // chip_id, reset value 0x60
#define BME280_REG_RESET         0xE0U  // write 0xB6 = full reset
#define BME280_REG_CTRL_HUM      0xF2U  // osrs_h
#define BME280_REG_STATUS        0xF3U  // measuring, im_update
#define BME280_REG_CTRL_MEAS     0xF4U  // osrs_t, osrs_p, mode
#define BME280_REG_CONFIG        0xF5U  // t_sb, filter
#define BME280_REG_PRESS_MSB     0xF7U  // press_msb/msb/lsb + temp + hum (8 bytes)
#define BME280_REG_CALIB_1       0x88U  // T1..P9 dig params (0x88..0xA1)
#define BME280_REG_CALIB_2       0xE1U  // H1..H6 dig params (0xE1..0xE7)

#define BME280_CHIP_ID           0x60U
#define BME280_RESET_CMD         0xB6U

// Oversampling settings: x1 for all three channels (fastest forced-mode
// conversion, ~2.3 ms per channel at x1 + 1.25 ms overhead).
#define BME280_OSRS_X1           0x1U
// ctrl_meas: osrs_t=x1 (bit5..2: 001<<5), osrs_p=x1 (001<<2), mode bits1:0.
#define BME280_CTRL_MEAS_OSRS    ((BME280_OSRS_X1 << 5) | (BME280_OSRS_X1 << 2))
#define BME280_MODE_SLEEP        0x0U
#define BME280_MODE_FORCED       0x1U

// status.measuring (bit 3): '1' while a conversion is running.
#define BME280_STATUS_MEASURING  (1U << 3)

// Datasheet §8.2: max conversion time at x1/x1/x1 ≈ 8 ms. 16 MHz busy-poll
// with an I2C status read per iteration self-throttles; 50 k iterations is
// a generous bound.
#define BME280_MEAS_TIMEOUT      50000UL

// Measurement results (converted). Pressure in Pa (typically 30000-120000),
// temperature in °C, relative humidity in %.
typedef struct {
    float temp_C;
    float pressure_Pa;
    float humidity_pct;
} bme280_data_t;

// Low-level register access. Propagates I2C error codes.
// readFromBme: write reg address (STOP), then read len bytes (STOP) —
// same pattern as the RTC/mag drivers (our I2C driver always ends with STOP).
uint8_t readFromBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t writeToBme(I2C_Handle_t *pToI2CHandle, uint8_t reg, uint8_t *buf, uint8_t len);

// Initialize for forced-mode single shots: verify chip ID, read the
// compensation parameters into RAM, set osrs_h and the idle config.
// deviceAddress is the raw byte (BME280_ADDR_76 or BME280_ADDR_77); the
// driver remembers it for subsequent bme280Measure calls.
// Returns CORE_OK, an I2C error, or BME280_INIT_CFG_ERR (wrong chip ID).
uint8_t bme280Init(I2C_Handle_t *pToI2CHandle, uint8_t deviceAddress);

// Trigger one forced-mode conversion (ctrl_meas mode=01), poll
// status.measuring with a bounded timeout, read press/temp/hum and convert
// with the datasheet integer compensation formulas.
// Returns CORE_OK or an I2C error / BME280_INIT_CFG_ERR (meas timeout).
uint8_t bme280Measure(I2C_Handle_t *pToI2CHandle, bme280_data_t *data);

#endif //OPENCASIO_BME280_H