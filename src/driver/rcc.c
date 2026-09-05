//
// Created by Alessandro Nardinelli on 24/11/25.
//

#include "driver/rcc.h"

// Startup iteration guard, not a calibrated timebase (SYSCLK is changing).
#define RCC_STARTUP_POLL_LIMIT 100000U


uint8_t initRCC(void) {
    // Wait for HSI16, switch SYSCLK, then wait for SWS=01 before retiring MSI.
    SET_HSI_ON;
    uint32_t remaining = RCC_STARTUP_POLL_LIMIT;
    while (!HSI_RDY()) {
        if (--remaining == 0U) return 0;
    }

    SET_CLK_TO_HSI;
    remaining = RCC_STARTUP_POLL_LIMIT;
    while (!HSI_CLK_SELECTED()) {
        if (--remaining == 0U) return 0;
    }

    // Retain the existing LSI1 startup for the later LCD phase (6.4.31).
    SET_LSI1_ON;
    remaining = RCC_STARTUP_POLL_LIMIT;
    while (!LSI1_RDY()) {
        if (--remaining == 0U) return 0;
    }

    SET_MSI_OFF;
    return 1;
}


void enableRCC(enum RCC_PERIPHERAL rcc_per) {
    //Enable the clock on the designated peripheral

    switch (rcc_per) {
        case GPIO_A_PER:
             GPIOA_CLK_EN;
            break;
        case GPIO_B_PER:
            GPIOB_CLK_EN;
            break;
        case GPIO_C_PER:
            GPIOC_CLK_EN;
            break;
        case GPIO_D_PER:
            GPIOD_CLK_EN;
            break;
        case GPIO_E_PER:
            GPIOE_CLK_EN;
            break;
        case I2C_PER:
            I2C_CLK_EN;
            break;
        case SPI_PER:
            SPI_CLK_EN;
            break;
        case TIM1_PER:
            TIM1_CLK_EN;
            break;
        case LCD_PER:
            LCD_CLK_EN;
            break;
        default:
            // Handle unknown value
            break;
    }

}

void diableRCC(enum RCC_PERIPHERAL rcc_per) {
    //Disable the clock on the designated peripheral

    switch (rcc_per) {
        case GPIO_A_PER:
            GPIOA_CLK_DIS;
            break;
        case GPIO_B_PER:
            GPIOB_CLK_DIS;
            break;
        case GPIO_C_PER:
            GPIOC_CLK_DIS;
            break;
        case GPIO_D_PER:
            GPIOD_CLK_DIS;
            break;
        case GPIO_E_PER:
            GPIOE_CLK_DIS;
            break;
        case I2C_PER:
            I2C_CLK_DIS;
            break;
        case SPI_PER:
            SPI_CLK_DIS;
            break;
        case TIM1_PER:
            TIM1_CLK_DIS;
            break;
        default:
            // Handle unknown value
            break;
    }

}