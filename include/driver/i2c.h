//
// Created by Alessandro Nardinelli on 05/12/25.
//

#ifndef OPENCAS_I2C_H
#define OPENCAS_I2C_H
#pragma once

#include "stm32wb55xx.h"


// Only the fields Phase 2 uses. Clock speed is in Hz (100000 = standard mode).
// Device addresses follow the project convention: pre-shifted raw bytes
// (RTC 0xAC, MAG 0x60, BME280 0xEC/0xEE) — I2C_Transmit/I2C_Receive take
// the same raw byte and derive the 7-bit SADD field themselves.
typedef struct
{
      uint32_t I2C_ClockSpeed;
      uint8_t  I2C_Mode;               // I2C_MODE_STANDARD / _FM / _FMP

}I2C_PinConfig_t;


typedef struct
{
      I2Cx_Reg_TypeDef *pI2Cx;
      I2C_PinConfig_t I2C_PinConfig;

}I2C_Handle_t;

// Returns CORE_OK / I2C_CFG_ERR. Assumes I2CCLK = 16 MHz (PCLK at reset
// prescalers, or HSI16): the only supported configuration for now.
uint8_t I2C_Init(I2C_Handle_t *pToI2CHandle);
void I2C_DeInit(I2Cx_Reg_TypeDef *pI2Cx); // gate the peripheral clock and release the bus pins

// Both take the project-convention RAW address byte (bit0 = R/W, ignored here).
// length == 0 on I2C_Transmit probes the address only (used by the bus scan).
// Returns CORE_OK, I2C_NACK_ERR (no device), or I2C_BUS_ERR (bus fault/timeout).
uint8_t I2C_Transmit(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t memAddr, uint8_t length, uint8_t deviceAddress);
uint8_t I2C_Receive(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t length, uint8_t deviceAddress);

// Register read using a repeated START (no STOP) between the pointer write
// and the read phase, instead of write-STOP-read. Some targets reset or
// re-latch their output registers on STOP, silently returning stale/zero
// data on a subsequent separate read transaction; repeated START avoids
// that window. Use for devices/registers suspected of this behavior
// (e.g. MMC5603NJ XOUT/TOUT bursts).
uint8_t I2C_MemRead(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length, uint8_t deviceAddress);




#endif //OPENCAS_I2C_H