//
// Created by Alessandro Nardinelli on 20/12/25.
//

#ifndef OPENCASIO_RV_3129_C3_H
#define OPENCASIO_RV_3129_C3_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/i2c.h"


//define Macros for settting and clearing bit
#define ENABLE          0x01U
#define DISABLE         0x00U

//define the i2c addr of the RV-3129-C3
uint8_t static const RTC_ADDR =  0xAC;

uint8_t readFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length);
uint8_t writeToRTC(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length);


#endif //OPENCASIO_RV_3129_C3_H