//
// Created by Alessandro Nardinelli on 01/12/25.
//


#include "driver/gpio.h"

uint8_t GPIO_Init(GPIO_Handle_t *pToGPIOHandle) {

    // Identify the mode to set up
    uint8_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
    uint8_t mode = pToGPIOHandle->GPIO_PinConfig.GPIO_PinMode;
    uint8_t otype = pToGPIOHandle->GPIO_PinConfig.GPIO_PinOPType;
    uint8_t speed = pToGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed;
    uint8_t pupd = pToGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl;
    uint8_t af = pToGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode;

    // Clear previous mode for the pin (2 bits per pin)
    pToGPIOHandle->pGPIOx->moder &= ~(3U << (pin * 2));
    pToGPIOHandle->pGPIOx->moder |= (mode << (pin * 2));

    // Set the output type
    pToGPIOHandle->pGPIOx->otyper &= ~(1U << pin); // Clear
    pToGPIOHandle->pGPIOx->otyper |= (otype << pin);

    // Set the speed
    pToGPIOHandle->pGPIOx->ospeedr &= ~(3U << (pin * 2)); // Clear
    pToGPIOHandle->pGPIOx->ospeedr |= (speed << (pin * 2));

    // Set pull-up/pull-down configuration
    pToGPIOHandle->pGPIOx->pupdr &= ~(3U << (pin * 2)); // Clear
    pToGPIOHandle->pGPIOx->pupdr |= (pupd << (pin * 2));

    // Set alternate function, if required
    if(mode == GPIO_MODE_AF) {
        if(pin < 8) {
            pToGPIOHandle->pGPIOx->afrl &= ~(0xFU << (pin * 4)); // Clear
            pToGPIOHandle->pGPIOx->afrl |= ((af & 0xF) << (pin * 4));
        } else {
            pToGPIOHandle->pGPIOx->afrh &= ~(0xFU << ((pin - 8) * 4)); // Clear
            pToGPIOHandle->pGPIOx->afrh |= ((af & 0xF) << ((pin - 8) * 4));
        }
    }

    // Success
    return 1;

}


uint8_t GPIO_ReadFromInputPin(GPIO_Handle_t *pToGPIOHandle) {
    // Read the input data register 'idr', shift right by pinNumber, and mask LSB
    uint8_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;

    uint8_t value = (uint8_t)((pToGPIOHandle->pGPIOx->idr >> pin) & 0x1U);

    return value;
}


uint32_t GPIO_ReadFromInputPort(GPIOx_RegTypeDef *pGPIOx) {
    // Read the entire input data register 'idr'
    uint32_t value = (uint32_t)pGPIOx->idr;

    return value;
}


void GPIO_WriteToOutputPin(GPIO_Handle_t *pToGPIOHandle, uint8_t dataToWrite) {

    uint8_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
    // write data
    (pToGPIOHandle->pGPIOx->odr |= (dataToWrite << pin));

}

void GPIO_WriteToOutputPort(GPIOx_RegTypeDef *pGPIOx, uint32_t dataToWrite) {
    // write the entire data to the GPIO port
    pGPIOx->odr = dataToWrite;

}
