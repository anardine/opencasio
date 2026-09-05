//
// Created by Alessandro Nardinelli on 24/11/25.
//

#ifndef OPENCAS_GPIO_H
#define OPENCAS_GPIO_H
#pragma once

#include "driver/stm32wb55xx.h"
#include "driver/rcc.h"
#include "etc/error.h"

typedef struct {
      uint8_t GPIO_PinNumber;
      uint8_t GPIO_PinMode;
      uint8_t GPIO_PinSpeed;
      uint8_t GPIO_PinPuPdControl;
      uint8_t GPIO_PinOPType;
      uint8_t GPIO_PinAltFunMode;
      uint8_t GPIO_isInterrupt; // GPIO_IRQ_OFF/RISING/FALLING/BOTH; INPUT mode only

}GPIO_PinConfig_t;


typedef struct {
      GPIOx_RegTypeDef *pGPIOx; //holds the base addr of GPIO port to witch the pin belongs
      GPIO_PinConfig_t GPIO_PinConfig; //this holds gpio pin config settings

}GPIO_Handle_t;

uint8_t GPIO_Init(const GPIO_Handle_t *pToGPIOHandle);
uint8_t GPIO_ReadFromInputPin(const GPIO_Handle_t *pToGPIOHandle);
uint32_t GPIO_ReadFromInputPort(GPIOx_RegTypeDef *pGPIOx);
void GPIO_WriteToOutputPin(const GPIO_Handle_t *pToGPIOHandle, uint8_t dataToWrite);
void GPIO_WriteToOutputPort(GPIOx_RegTypeDef *pGPIOx, uint32_t dataToWrite); // write to the output port of that GPIO entirelly
void GPIO_ToggleOutputPin(GPIOx_RegTypeDef *pGPIOx, uint8_t pinNumber);

uint8_t GPIO_IRQInit(const GPIO_Handle_t *pToGPIOHandle);
uint8_t GPIO_IRQPending(uint8_t pinNumber); // EXTI_PR1 pending bit for this line
void GPIO_IRQClear(uint8_t pinNumber);      // acknowledge: write 1 to the pending bit (rc_w1)
void GPIO_IRQHandling(uint8_t pinNumber);   // ISR glue: clear pending, then invoke callback
void GPIO_IRQCallback(uint8_t pinNumber);   // weak no-op; override in application code


#endif //OPENCAS_GPIO_H