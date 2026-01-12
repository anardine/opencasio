//
// Created by Alessandro Nardinelli on 12/01/26.
//

#ifndef OPENCASIO_MMC5603NJ_H
#define OPENCASIO_MMC5603NJ_H
#pragma once


#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"

//define the i2c addr of the MMC5603NJ
uint8_t static const MAG_ADDR =  0x60;

uint8_t magInit(I2C_Handle_t *pToI2CHandle);
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle);
uint8_t magFetchStatus(I2C_Handle_t *pToI2CHandle);
uint8_t magFetchData(I2C_Handle_t *pToI2CHandle);
uint8_t magTranformToHeading(uint8_t *magRawDataArray, uint8_t *magHeadings);

#endif //OPENCASIO_MMC5603NJ_H