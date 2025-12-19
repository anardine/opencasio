//
// Created by Alessandro Nardinelli on 05/12/25.
//

#ifndef OPENCAS_I2C_H
#define OPENCAS_I2C_H
#pragma once

#include "stm32wb55xx.h"


typedef struct
{
      volatile uint8_t I2C_ClockSpeed;
      volatile uint8_t I2C_DeviceAddress;
      volatile uint8_t I2C_AckControl;
      volatile uint8_t I2C_DutyCycleForFastMode;
      volatile uint8_t I2C_Mode;

}I2C_PinConfig_t;


typedef struct stm32f411xx_i2c
{
      I2Cx_Reg_TypeDef *pI2Cx;
      I2C_PinConfig_t I2C_PinConfig;

}I2C_Handle_t;

uint8_t I2C_Init(I2C_Handle_t *pToI2CHandle); // initialize all the characteristics of the I2C port with the default internal clock config
void I2C_DeInit(I2Cx_Reg_TypeDef *pI2Cx); // resets all data from a specific I2C port
uint8_t I2C_Transmit(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t length, uint8_t address);
uint8_t I2C_Receive(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t address);




#endif //OPENCAS_I2C_H