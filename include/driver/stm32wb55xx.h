//
// Created by Alessandro Nardinelli on 14/11/25.
//

#ifndef OPENCAS_STM32WB55XX_H
#define OPENCAS_STM32WB55XX_H

#include <stdint.h>

// Base address definitions for STM32WB55xx

//Buses
#define AHB1_BASE_ADDR                  (0x40020000UL)
#define AHB2_BASE_ADDR                  (0x48000000UL)
#define AHB4_BASE_ADDR                  (0x58000000UL)
#define APB1_BASE_ADDR                  (0x40000000UL)
#define APB2_BASE_ADDR                  (0x40010000UL)

//System Wide Configs and RCC control
#define RCC_BASE_ADDR                   (AHB4_BASE_ADDR + 0x0000UL)
#define SYSCFG_BASE_ADDR                (APB2_BASE_ADDR + 0x0000UL)
#define EXTI_BASE_ADDR                  (AHB4_BASE_ADDR + 0X0800UL)

//GPIOs
#define GPIOA_BASE_ADDR                 (AHB2_BASE_ADDR + 0x0000UL)
#define GPIOB_BASE_ADDR                 (AHB2_BASE_ADDR + 0x0400UL)
#define GPIOC_BASE_ADDR                 (AHB2_BASE_ADDR + 0x0800UL)
#define GPIOD_BASE_ADDR                 (AHB2_BASE_ADDR + 0x0C00UL)
#define GPIOE_BASE_ADDR                 (AHB2_BASE_ADDR + 0x1000UL)

//LCD
#define LCD_BASE_ADDR                   (APB1_BASE_ADDR + 0x2400UL)

//Comm peripherals
#define TIM1_BASE_ADDR                  (APB2_BASE_ADDR + 0x2C00UL)
#define SPI2_BASE_ADDR                  (APB1_BASE_ADDR + 0x3800UL)
#define I2C1_BASE_ADDR                  (APB1_BASE_ADDR + 0x5400UL)


// Pointer structures

//RCC - This does not include CPU2 registers since there's no use of CPU2 on this project
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


//SYSCFG
typedef struct
{
    uint32_t memrmp;            // 0x00: memory remap register
    uint32_t cfgr1;             // 0x04: configuration register 1
    uint32_t exticr1;           // 0x08: external interrupt configuration register 1
    uint32_t exticr2;           // 0x0C: external interrupt configuration register 2
    uint32_t exticr3;           // 0x10: external interrupt configuration register 3
    uint32_t exticr4;           // 0x14: external interrupt configuration register 4
    uint32_t scsr;              // 0x18: system configuration security control register
    uint32_t cfgr2;             // 0x1C: configuration register 2
    uint32_t swpr;              // 0x20: SRAM2 write protection register
    uint32_t skr;               // 0x24: SRAM2 key register
    uint32_t swpr2;             // 0x28: SRAM2 write protection register 2
    uint32_t reserved[53];      // 0x2C-0xFF: reserved (53 words to align with 0x100)
    uint32_t imr1;              // 0x100: CPU1 interrupt mask register 1
    uint32_t imr2;              // 0x104: CPU1 interrupt mask register 2
    uint32_t c2imr1;            // 0x108: CPU2 interrupt mask register 1
    uint32_t c2imr2;            // 0x10C: CPU2 interrupt mask register 2
    uint32_t sipcr;             // 0x110: secure IP control register
    
}SYSCFG_RegTypeDef;

#define SYSCFG                          ((SYSCFG_RegTypeDef*) SYSCFG_BASE_ADDR)


//EXTI - This does not include CPU2 registers since there's no use of CPU2 on this project
typedef struct
{
    uint32_t rtsr1;             // 0x000: Rising trigger selection register 1
    uint32_t ftsr1;             // 0x004: Falling trigger selection register 1
    uint32_t swier1;            // 0x008: Software interrupt event register 1
    uint32_t pr1;               // 0x00C: Pending register 1
    uint32_t reserved0[4];      // 0x010-0x01F: reserved
    uint32_t rtsr2;             // 0x020: Rising trigger selection register 2
    uint32_t ftsr2;             // 0x024: Falling trigger selection register 2
    uint32_t swier2;            // 0x028: Software interrupt event register 2
    uint32_t pr2;               // 0x02C: Pending register 2
    uint32_t reserved1[20];     // 0x030-0x07F: reserved
    uint32_t imr1;              // 0x080: Interrupt mask register 1
    uint32_t emr1;              // 0x084: Event mask register 1
    uint32_t reserved2[2];      // 0x088-0x08F: reserved
    uint32_t imr2;              // 0x090: Interrupt mask register 2
    uint32_t emr2;              // 0x094: Event mask register 2

} EXTI_RegTypeDef;

#define EXTI                            ((EXTI_RegTypeDef*) EXTI_BASE_ADDR)


//GPIO
typedef struct
{
    uint32_t moder;             // 0x00: GPIO port mode register
    uint32_t otyper;            // 0x04: GPIO port output type register
    uint32_t ospeedr;           // 0x08: GPIO port output speed register
    uint32_t pupdr;             // 0x0C: GPIO port pull-up/pull-down register
    uint32_t idr;               // 0x10: GPIO port input data register
    uint32_t odr;               // 0x14: GPIO port output data register
    uint32_t bsrr;              // 0x18: GPIO port bit set/reset register
    uint32_t lckr;              // 0x1C: GPIO port configuration lock register
    uint32_t afrl;              // 0x20: GPIO alternate function low register
    uint32_t afrh;              // 0x24: GPIO alternate function high register
    uint32_t brr;               // 0x28: GPIO port bit reset register

} GPIOx_RegTypeDef;

#define GPIOA                           ((GPIOx_RegTypeDef *) GPIOA_BASE_ADDR)
#define GPIOB                           ((GPIOx_RegTypeDef *) GPIOB_BASE_ADDR)
#define GPIOC                           ((GPIOx_RegTypeDef *) GPIOC_BASE_ADDR)
#define GPIOD                           ((GPIOx_RegTypeDef *) GPIOD_BASE_ADDR)
#define GPIOE                           ((GPIOx_RegTypeDef *) GPIOE_BASE_ADDR)


typedef struct
{
    uint32_t cr;                // 0x000: LCD control register
    uint32_t fcr;               // 0x004: LCD frame control register
    uint32_t sr;                // 0x008: LCD status register
    uint32_t clr;               // 0x00C: LCD clear register
    uint32_t reserved;          // 0x010: Reserved
    uint32_t com0_l;            // 0x014: LCD display RAM COM0 low word
    uint32_t com0_h;            // 0x018: LCD display RAM COM0 high word
    uint32_t com1_l;            // 0x01C: LCD display RAM COM1 low word
    uint32_t com1_h;            // 0x020: LCD display RAM COM1 high word
    uint32_t com2_l;            // 0x024: LCD display RAM COM2 low word
    uint32_t com2_h;            // 0x028: LCD display RAM COM2 high word
    uint32_t com3_l;            // 0x02C: LCD display RAM COM3 low word
    uint32_t com3_h;            // 0x030: LCD display RAM COM3 high word
    uint32_t com4_l;            // 0x034: LCD display RAM COM4 low word
    uint32_t com4_h;            // 0x038: LCD display RAM COM4 high word
    uint32_t com5_l;            // 0x03C: LCD display RAM COM5 low word
    uint32_t com5_h;            // 0x040: LCD display RAM COM5 high word
    uint32_t com6_l;            // 0x044: LCD display RAM COM6 low word
    uint32_t com6_h;            // 0x048: LCD display RAM COM6 high word
    uint32_t com7_l;            // 0x04C: LCD display RAM COM7 low word
    uint32_t com7_h;            // 0x050: LCD display RAM COM7 high word

}LCD_RegTypeDef;

#define LCD                             ((LCD_RegTypeDef*) LCD_BASE_ADDR)


typedef struct
{
    uint32_t cr1;               // 0x00: control register 1
    uint32_t cr2;               // 0x04: control register 2
    uint32_t smcr;              // 0x08: slave mode control register
    uint32_t dier;              // 0x0C: DMA/interrupt enable register
    uint32_t sr;                // 0x10: status register
    uint32_t egr;               // 0x14: event generation register
    uint32_t ccmr1;             // 0x18: capture/compare mode register 1
    uint32_t ccmr2;             // 0x1C: capture/compare mode register 2
    uint32_t ccer;              // 0x20: capture/compare enable register
    uint32_t cnt;               // 0x24: counter
    uint32_t psc;               // 0x28: prescaler
    uint32_t arr;               // 0x2C: auto-reload register
    uint32_t rcr;               // 0x30: repetition counter register
    uint32_t ccr1;              // 0x34: capture/compare register 1
    uint32_t ccr2;              // 0x38: capture/compare register 2
    uint32_t ccr3;              // 0x3C: capture/compare register 3
    uint32_t ccr4;              // 0x40: capture/compare register 4
    uint32_t bdtr;              // 0x44: break and dead-time register
    uint32_t dcr;               // 0x48: DMA control register
    uint32_t dmar;              // 0x4C: DMA address for full transfer
    uint32_t or1;               // 0x50: option register 1
    uint32_t ccmr3;             // 0x54: capture/compare mode register 3
    uint32_t ccr5;              // 0x58: capture/compare register 5
    uint32_t ccr6;              // 0x5C: capture/compare register 6
    uint32_t af1;               // 0x60: TIM1 alternate function option register 1
    uint32_t af2;               // 0x64: TIM1 Alternate function register 2
    uint32_t tisel;             // 0x68: timer input selection register

}TIMx_RegTypeDef;

#define TIMER                          ((TIMx_RegTypeDef *) TIM1_BASE_ADDR)

typedef struct
{
    uint32_t cr1;               //0x00: Control register 1
    uint32_t cr2;               //0x04: Control register 2
    uint32_t sr;                //0x08: Status register
    uint32_t dr;                //0x0C: Data register
    uint32_t crcpr;             //0x10: CRC polynomial register
    uint32_t rxcrcr;            //0x14: RX CRC register
    uint32_t txcrcr;            //0x18: TX CRC register

}SPIx_RegTypeDef;

#define SPI                            ((SPIx_RegTypeDef *) SPI2_BASE_ADDR)

typedef struct
{
    uint32_t cr1;               // 0x00: control register 1
    uint32_t cr2;               // 0x04: control register 2
    uint32_t oar1;              // 0x08: own address register 1
    uint32_t oar2;              // 0x0C: own address register 2
    uint32_t timingr;           // 0x10: timing register
    uint32_t timeoutr;          // 0x14: timeout register
    uint32_t isr;               // 0x18: interrupt and status register
    uint32_t icr;               // 0x1C: interrupt clear register
    uint32_t pecr;              // 0x20: packet error checking register
    uint32_t rxdr;              // 0x24: receive data register
    uint32_t txdr;              // 0x28: transmit data register
}I2Cx_Reg_TypeDef;

#define I2C                         ((I2Cx_Reg_TypeDef *) I2C1_BASE_ADDR)




#endif
