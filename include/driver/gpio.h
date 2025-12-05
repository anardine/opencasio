//
// Created by Alessandro Nardinelli on 24/11/25.
//

#ifndef OPENCAS_GPIO_H
#define OPENCAS_GPIO_H
#pragma once

#include "driver/stm32wb55xx.h"

typedef struct {
      uint8_t GPIO_PinNumber;
      uint8_t GPIO_PinMode;
      uint8_t GPIO_PinSpeed;
      uint8_t GPIO_PinPuPdControl;
      uint8_t GPIO_PinOPType;
      uint8_t GPIO_PinAltFunMode;
      uint8_t GPIO_isInterrupt;

}GPIO_PinConfig_t;


typedef struct {
      GPIOx_RegTypeDef *pGPIOx; //holds the base addr of GPIO port to witch the pin belongs
      GPIO_PinConfig_t GPIO_PinConfig; //this holds gpio pin config settings

}GPIO_Handle_t;

uint8_t GPIO_Init(GPIO_Handle_t *pToGPIOHandle); // initialize all the characteristics of the GPIO port
uint8_t GPIO_ReadFromInputPin(GPIO_Handle_t *pToGPIOHandle); // reads the value on the input pin defined
uint32_t GPIO_ReadFromInputPort(GPIOx_RegTypeDef *pGPIOx); // read the entire value of all GPIOx pins
void GPIO_WriteToOutputPin(GPIO_Handle_t *pToGPIOHandle, uint8_t dataToWrite); // write the to the output pin
void GPIO_WriteToOutputPort(GPIOx_RegTypeDef *pGPIOx, uint32_t dataToWrite); // write to the output port of that GPIO entirelly
void GPIO_ToggleOutputPin(GPIOx_RegTypeDef *pGPIOx, uint8_t pinNumber);
//void GPIO_IRQInit(GPIO_Handle_t *pToGPIOHandle,  IRQn_Handler_t *IRQ_GPIO_h);


#endif //OPENCAS_GPIO_H