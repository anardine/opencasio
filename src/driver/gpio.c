//
// Created by Alessandro Nardinelli on 01/12/25.
//


#include "driver/gpio.h"

uint8_t GPIO_Init(const GPIO_Handle_t *pToGPIOHandle) {
    if (!pToGPIOHandle) return GPIO_CFG_ERR;

    GPIOx_RegTypeDef *port = pToGPIOHandle->pGPIOx;
    const GPIO_PinConfig_t *config = &pToGPIOHandle->GPIO_PinConfig;
    const uint32_t pin = config->GPIO_PinNumber;
    enum RCC_PERIPHERAL clock;

    if (port == GPIOA) clock = GPIO_A_PER;
    else if (port == GPIOB) clock = GPIO_B_PER;
    else if (port == GPIOC) clock = GPIO_C_PER;
    else if (port == GPIOD) clock = GPIO_D_PER;
    else if (port == GPIOE) clock = GPIO_E_PER;
    else return GPIO_CFG_ERR;

    // Defensive implementation. Returns an error if out of bounds. Port E only implements pins 0 to 4 and Port H 0 to 1 and 3. All others are 0 to 15 on the STM32WB55xx
    if (pin > 15U || (port == GPIOE && pin > 4U) ||
        config->GPIO_PinMode > GPIO_MODE_ANALOG ||
        config->GPIO_PinOPType > GPIO_OTYPE_OD ||
        config->GPIO_PinSpeed > GPIO_OSPEED_HIGH ||
        config->GPIO_PinPuPdControl > GPIO_PUPD_PD ||
        (config->GPIO_PinMode == GPIO_MODE_AF && config->GPIO_PinAltFunMode > 15U) ||
        config->GPIO_isInterrupt != 0U) {
        return GPIO_CFG_ERR;
    }

    enableRCC(clock);

    //Forces the CPU to read the RCC clock-enable register immediately after enabling the GPIO port clock
    (void)RCC->ahb2enr;

    // Preload LOW before enabling the output driver, including when a previous run left the latch HIGH.
    if (config->GPIO_PinMode == GPIO_MODE_OUTPUT) {
        GPIO_WriteToOutputPin(pToGPIOHandle, 0U);
    }

    // OTYPER  (1 bit/pin): 0 = push-pull, 1 = open-drain
    // OSPEEDR (2 bits/pin): 00 low, 01 medium, 10 fast, 11 high
    // PUPDR   (2 bits/pin): 00 no pull, 01 pull-up, 10 pull-down

    const uint32_t shift = pin * 2U;
    port->otyper = (port->otyper & ~(1U << pin)) | ((uint32_t)config->GPIO_PinOPType << pin);
    port->ospeedr = (port->ospeedr & ~(3U << shift)) | ((uint32_t)config->GPIO_PinSpeed << shift);
    port->pupdr = (port->pupdr & ~(3U << shift)) | ((uint32_t)config->GPIO_PinPuPdControl << shift);


    if (config->GPIO_PinMode == GPIO_MODE_AF) {
        const uint32_t afShift = (pin % 8U) * 4U;
        volatile uint32_t *afr = pin < 8U ? &port->afrl : &port->afrh;
        *afr = (*afr & ~(0xFU << afShift)) | ((uint32_t)config->GPIO_PinAltFunMode << afShift);
    }

    // Select the mode after electrical/AF setup.
    port->moder = (port->moder & ~(3U << shift)) | ((uint32_t)config->GPIO_PinMode << shift);
    return CORE_OK;
}


uint8_t GPIO_ReadFromInputPin(const GPIO_Handle_t *pToGPIOHandle) {

    // Board inputs differ in polarity (buttons active-HIGH, RTC_INT active-LOW);
    // Leave assertion/debounce semantics to their consumers.
    uint8_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;

    uint8_t value = (uint8_t)((pToGPIOHandle->pGPIOx->idr >> pin) & 0x1U);

    return value;
}


uint32_t GPIO_ReadFromInputPort(GPIOx_RegTypeDef *pGPIOx) {
    // Read the entire input data register 'idr'
    uint32_t value = (uint32_t)pGPIOx->idr;

    return value;
}


void GPIO_WriteToOutputPin(const GPIO_Handle_t *pToGPIOHandle, uint8_t dataToWrite) {
    const uint32_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
    // RM0434 section 10.5.7: one BSRR write changes only this pin, without
    // an ODR read/modify/write that could lose another pin's update.
    pToGPIOHandle->pGPIOx->bsrr = dataToWrite ? (1U << pin) : (1U << (pin + 16U));
}

void GPIO_WriteToOutputPort(GPIOx_RegTypeDef *pGPIOx, uint32_t dataToWrite) {
    // write the entire data to the GPIO port
    pGPIOx->odr = dataToWrite;

}
