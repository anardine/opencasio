//
// Created by Alessandro Nardinelli on 12/01/26.
//

#ifndef OPENCASIO_MMC5603NJ_H
#define OPENCASIO_MMC5603NJ_H
#pragma once

#include <math.h>
#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"


uint8_t static const MAG_ADDR =  0x60; //define the i2c addr of the MMC5603NJ
typedef uint8_t magDataArray_t[10]; // array of uint8_t data to hold the data from Mag. 9 for mag readings, 1 for temperature if enabled

uint8_t readFromMag(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length);
uint8_t writeToMag(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length);
uint8_t magInit(I2C_Handle_t *pToI2CHandle);
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle);
// uint8_t magGetStatus(I2C_Handle_t *pToI2CHandle);
uint8_t magGetData(I2C_Handle_t *pToI2CHandle, uint8_t *magRawDataArray);
uint8_t magTransformToHeading(uint8_t *magRawDataArray);
uint8_t displayHeadings(I2C_Handle_t *pToI2CHandle, uint8_t *magHeadings);

#endif //OPENCASIO_MMC5603NJ_H