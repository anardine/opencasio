//
// Created by Alessandro Nardinelli on 24/11/25.
//

#include "driver/rcc.h"


uint8_t initRCC() {
    //set internal clock source to 16MHz
    //when the MCU is powered on, the initial clock source selected is MSI on 4MHz. We need to check the RCC_CR register to understand if MSI is ready to then set the MSI to 16MHz. When it wakes from standby mode, the MCU instead runs on HSI instead (which is by default 16MHz)
    const uint32_t rccState = RCC->cr;

    switch (rccState) {

        case 0x61:                                  //Reset value: 0x0000 0061 (after POR reset)
            while (!MSI_RDY_FLAG()) {}                //wait for MSI flag to report ready
            RCC->cr &= ~(0xF << 7);                 //clear MSIs default of 4MHz
            SET_MSI_16();                             //set the MSI to 16MHz
            return 1;                               // return true if MSI was correctly set

        case 0x160:                                 //Reset value: 0x0000 0160 (after wake-up from Standby reset)

            return 2;                               // return 2 if clock is HSI-based

        default:
            RCC->cr = 0x160;                        //sets clock back to HSI
        return 3;                                   //returns 3 if clock had a different configuration than the default
    }

}


void enableRCC(const enum RCC_PERIPHERAL rcc_per) {
    //Enable the clock on the designated peripheral

    switch (rcc_per) {
        case GPIO_A_PER:
             GPIOA_CLK_EN();
            break;
        case GPIO_B_PER:
            GPIOB_CLK_EN();
            break;
        case GPIO_C_PER:
            GPIOC_CLK_EN();
            break;
        case GPIO_D_PER:
            GPIOD_CLK_EN();
            break;
        case GPIO_E_PER:
            GPIOE_CLK_EN();
            break;
        case I2C_PER:
            I2C_CLK_EN();
            break;
        case SPI_PER:
            SPI_CLK_EN();
            break;
        case TIM1_PER:
            TIM1_CLK_EN();
            break;
        case LCD_PER:
            LCD_CLK_EN();
            break;
        default:
            // Handle unknown value
            break;
    }

}

void diableRCC(const enum RCC_PERIPHERAL rcc_per) {
    //Disable the clock on the designated peripheral

    switch (rcc_per) {
        case GPIO_A_PER:
            GPIOA_CLK_DIS();
            break;
        case GPIO_B_PER:
            GPIOB_CLK_DIS();
            break;
        case GPIO_C_PER:
            GPIOC_CLK_DIS();
            break;
        case GPIO_D_PER:
            GPIOD_CLK_DIS();
            break;
        case GPIO_E_PER:
            GPIOE_CLK_DIS();
            break;
        case I2C_PER:
            I2C_CLK_DIS();
            break;
        case SPI_PER:
            SPI_CLK_DIS();
            break;
        case TIM1_PER:
            TIM1_CLK_DIS();
            break;
        default:
            // Handle unknown value
            break;
    }

}