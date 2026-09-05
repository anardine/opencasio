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
        config->GPIO_isInterrupt > GPIO_IRQ_BOTH ||
        // EXTI samples the input stage; IRQ on any other mode has no source
        (config->GPIO_isInterrupt != GPIO_IRQ_OFF && config->GPIO_PinMode != GPIO_MODE_INPUT)) {
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

    if (config->GPIO_isInterrupt != GPIO_IRQ_OFF) {
        return GPIO_IRQInit(pToGPIOHandle);
    }
    return CORE_OK;
}


// Maps a GPIO port pointer to the 3-bit source code used by SYSCFG_EXTICRn. 000=PA, 001=PB, 010=PC, 011=PD, 100=PE).
static uint8_t GPIO_PortToExtiSource(GPIOx_RegTypeDef *pGPIOx) {
    if (pGPIOx == GPIOA) return 0;
    if (pGPIOx == GPIOB) return 1;
    if (pGPIOx == GPIOC) return 2;
    if (pGPIOx == GPIOD) return 3;
    if (pGPIOx == GPIOE) return 4;
    return 0xFF;
}

// Maps an EXTI line (same number as the GPIO pin) to its NVIC IRQ number
static uint8_t GPIO_PinToIRQn(uint8_t pin) {
    if (pin <= 4U)  return (uint8_t)(EXTI0_IRQ + pin);
    if (pin <= 9U)  return EXTI9_5_IRQ;
    return EXTI15_10_IRQ;
}

uint8_t GPIO_IRQInit(const GPIO_Handle_t *pToGPIOHandle) {
    if (!pToGPIOHandle) return GPIO_CFG_ERR;

    const uint32_t pin = pToGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
    const uint8_t edge = pToGPIOHandle->GPIO_PinConfig.GPIO_isInterrupt;
    const uint8_t source = GPIO_PortToExtiSource(pToGPIOHandle->pGPIOx);
    if (source == 0xFFU || pin > 15U || edge > GPIO_IRQ_BOTH) return GPIO_CFG_ERR;

    // SYSCFG and EXTI have no clock gate on the WB55 (RCC_APB2ENR bits 10:0
    // are reserved, RM0434 section 6.4.22), so no enableRCC() is needed here.

    // 1. Route the port to the EXTI line: EXTICR(pin/4 + 1), 3-bit field per
    //    pin at offset (pin%4)*4; bit 3 of each nibble is reserved.
    volatile uint32_t *exticr = &SYSCFG->exticr1 + (pin / 4U);
    const uint32_t extiShift = (pin % 4U) * 4U;
    *exticr = (*exticr & ~(7U << extiShift)) | ((uint32_t)source << extiShift);

    // 2. Select the trigger edge(s) (RM0434 section 16.6.1/16.6.2).
    const uint32_t line = 1U << pin;
    EXTI->rtsr1 = (EXTI->rtsr1 & ~line) | ((edge & GPIO_IRQ_RISING) ? line : 0U);
    EXTI->ftsr1 = (EXTI->ftsr1 & ~line) | ((edge & GPIO_IRQ_FALLING) ? line : 0U);

    // 3. Drop any stale pending request, then unmask the line for CPU1
    //    (PR1 is rc_w1: writing 1 clears; IMR1 reset value masks GPIO lines).
    EXTI->pr1 = line;
    EXTI->imr1 |= line;

    // 4. Enable the NVIC line. Priorities stay at reset default (0): with
    //    four equal-priority wake sources there is nothing to arbitrate yet.
    const uint8_t irqn = GPIO_PinToIRQn((uint8_t)pin);
    NVIC_ISER->iser[irqn / 32U] |= 1U << (irqn % 32U);
    return CORE_OK;
}

uint8_t GPIO_IRQPending(uint8_t pinNumber) {
    return (uint8_t)((EXTI->pr1 >> pinNumber) & 1U);
}

void GPIO_IRQClear(uint8_t pinNumber) {
    // RM0434 section 16.6.4: PR1 bits are rc_w1 — write 1 to clear.
    EXTI->pr1 = 1U << pinNumber;
}

// Weak default: application code overrides this to react to button/RTC events.
__attribute__((weak)) void GPIO_IRQCallback(uint8_t pinNumber) {
    (void)pinNumber;
}

void GPIO_IRQHandling(uint8_t pinNumber) {
    if (GPIO_IRQPending(pinNumber)) {
        // Clear before the callback so an edge arriving during the callback
        // is not lost (PR1 re-sets even while the handler runs).
        GPIO_IRQClear(pinNumber);
        GPIO_IRQCallback(pinNumber);
    }
}

// Vector names match the weak aliases in the CMSIS startup file; these
// strong definitions take over the slots (EXTI lines 5-9 and 10-15 share
// one vector each, so they poll their pending bits).
void EXTI0_IRQHandler(void)  { GPIO_IRQHandling(0); }
void EXTI1_IRQHandler(void)  { GPIO_IRQHandling(1); }
void EXTI2_IRQHandler(void)  { GPIO_IRQHandling(2); }
void EXTI3_IRQHandler(void)  { GPIO_IRQHandling(3); }
void EXTI4_IRQHandler(void)  { GPIO_IRQHandling(4); }

void EXTI9_5_IRQHandler(void) {
    for (uint8_t pin = 5; pin <= 9; pin++) GPIO_IRQHandling(pin);
}

void EXTI15_10_IRQHandler(void) {
    for (uint8_t pin = 10; pin <= 15; pin++) GPIO_IRQHandling(pin);
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
