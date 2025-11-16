//
// Created by Alessandro Nardinelli on 14/11/25.
//

#ifndef OPENCAS_STM32WB55XX_H
#define OPENCAS_STM32WB55XX_H

#include <stdint.h>

// Base address definitions for STM32WB55xx

//Buses
#define AHB1_BASE_ADDR                    (0x40020000UL)
#define AHB2_BASE_ADDR                    (0x48000000UL)
#define AHB4_BASE_ADDR                    (0x58000000UL)
#define APB1_BASE_ADDR                    (0x40000000UL)
#define APB2_BASE_ADDR                    (0x40010000UL)

//GPIOs
#define GPIOA_BASE_ADDR                   (AHB2_BASE_ADDR + 0x0000UL)
#define GPIOB_BASE_ADDR                   (AHB2_BASE_ADDR + 0x0400UL)
#define GPIOC_BASE_ADDR                   (AHB2_BASE_ADDR + 0x0800UL)
#define GPIOD_BASE_ADDR                   (AHB2_BASE_ADDR + 0x0C00UL)
#define GPIOE_BASE_ADDR                   (AHB2_BASE_ADDR + 0x1000UL)

//SYS Config and RCC controll
#define RCC_BASE_ADDR                     (AHB4_BASE_ADDR + 0x0000UL)
#define SYSCFG1_BASE_ADDR                 (APB2_BASE_ADDR + 0x0000UL)
#define SYSCFG2_BASE_ADDR                 (APB2_BASE_ADDR + 0x0100UL)

//LCD
#define LCD_BASE_ADDR                     (APB1_BASE_ADDR + 0x2400UL)

//Comm peripherals
#define TIM1_BASE_ADDR                    (APB2_BASE_ADDR + 0x2C00UL)
#define SPI2_BASE_ADDR                    (APB1_BASE_ADDR + 0x3800UL)
#define I2C1_BASE_ADDR                    (APB1_BASE_ADDR + 0x5400UL)


// Pointer structures

//GPIO
typedef struct
{
    uint32_t moder;    // GPIO port mode register
    uint32_t otyper;   // GPIO port output type register
    uint32_t ospeedr;  // GPIO port output speed register
    uint32_t pupdr;    // GPIO port pull-up/pull-down register
    uint32_t idr;      // GPIO port input data register
    uint32_t odr;      // GPIO port output data register
    uint32_t bsrr;     // GPIO port bit set/reset register
    uint32_t lckr;     // GPIO port configuration lock register
    uint32_t afrl;     // GPIO alternate function low register
    uint32_t afrh;     // GPIO alternate function high register
    uint32_t brr;      // GPIO port bit reset register

} GPIOx_TypeDef;        // Structure representing a GPIO peripheral

