//
// Created by Alessandro Nardinelli on 14/11/25.
//

#ifndef OPENCAS_STM32WB55XX_H
#define OPENCAS_STM32WB55XX_H

#include <stdint.h>
#include <sys/types.h>

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

//SYS Config and RCC control
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
    uint32_t moder;             // GPIO port mode register
    uint32_t otyper;            // GPIO port output type register
    uint32_t ospeedr;           // GPIO port output speed register
    uint32_t pupdr;             // GPIO port pull-up/pull-down register
    uint32_t idr;               // GPIO port input data register
    uint32_t odr;               // GPIO port output data register
    uint32_t bsrr;              // GPIO port bit set/reset register
    uint32_t lckr;              // GPIO port configuration lock register
    uint32_t afrl;              // GPIO alternate function low register
    uint32_t afrh;              // GPIO alternate function high register
    uint32_t brr;               // GPIO port bit reset register

} GPIOx_RegTypeDef;        // Structure representing a GPIO peripheral

#define GPIOA                           ((GPIOx_RegTypeDef *) GPIOA_BASE_ADDR)
#define GPIOB                           ((GPIOx_RegTypeDef *) GPIOB_BASE_ADDR)
#define GPIOC                           ((GPIOx_RegTypeDef *) GPIOC_BASE_ADDR)
#define GPIOD                           ((GPIOx_RegTypeDef *) GPIOD_BASE_ADDR)
#define GPIOE                           ((GPIOx_RegTypeDef *) GPIOE_BASE_ADDR)



typedef struct
{
    uint32_t cr;                // clock control register
    uint32_t icscr;             // internal clock sources calibration register
    uint32_t cfgr;              // clock configuration register
    uint32_t pllcfgr;           // PLL configuration register
    uint32_t pllsai1cfgr;       // PLLSAI1 configuration register
    uint32_t reserved1;         // reserved
    uint32_t cier;              // clock interrupt enable register
    uint32_t cifr;              // clock interrupt flag register
    uint32_t cicr;              // clock interrupt clear register
    uint32_t reserved2;         // reserved
    uint32_t ahb1rstr;          // AHB1 peripheral reset register
    uint32_t ahb2rstr;          // AHB2 peripheral reset register
    uint32_t ahb3rstr;          // AHB3 peripheral reset register
    uint32_t reserved3;         // reserved
    uint32_t apb1rstr1;         // APB1 peripheral reset register 1
    uint32_t apb1rstr2;         // APB1 peripheral reset register 2
    uint32_t apb2rstr;          // APB2 peripheral reset register
    uint32_t apb3rstr;          // APB3 peripheral reset register
    uint32_t ahb1enr;           // AHB1 peripheral clock enable register
    uint32_t ahb2enr;           // AHB2 peripheral clock enable register
    uint32_t ahb3enr;           // AHB3 peripheral clock enable register
    uint32_t reserved4;         // reserved
    uint32_t apb1enr1;          // APB1 peripheral clock enable register 1
    uint32_t apb1enr2;          // APB1 peripheral clock enable register 2
    uint32_t apb2enr;           // APB2 peripheral clock enable register
    uint32_t apb3enr;           // APB3 peripheral clock enable register
    uint32_t ahb1smenr;         // AHB1 peripheral clocks enable in Sleep and Stop modes register
    uint32_t ahb2smenr;         // AHB2 peripheral clocks enable in Sleep and Stop modes register
    uint32_t ahb3smenr;         // AHB3 peripheral clocks enable in Sleep and Stop modes register
    uint32_t reserved5;         // reserved
    uint32_t apb1smenr1;        // APB1 peripheral clocks enable in Sleep and Stop modes register 1
    uint32_t apb1smenr2;        // APB1 peripheral clocks enable in Sleep and Stop modes register 2
    uint32_t apb2smenr;         // APB2 peripheral clocks enable in Sleep and Stop modes register
    uint32_t apb3smenr;         // APB3 peripheral clocks enable in Sleep and Stop modes register
    uint32_t ccipr;             // peripherals independent clock configuration register
    uint32_t reserved6;         // reserved
    uint32_t bdcr;              // backup domain control register
    uint32_t csr;               // control/status register
    uint32_t crrcr;             // clock recovery RC register
    uint32_t hsecr;             // HSE clock control register

} RCC_RegTypeDef;

#define RCC                             ((RCC_RegTypeDef *) RCC_BASE_ADDR)







#endif
