#include "driver/gpio.h"

// GPIOA handles
GPIO_Handle_t pToGPIOA0;
GPIO_Handle_t pToGPIOA1;
GPIO_Handle_t pToGPIOA2;
GPIO_Handle_t pToGPIOA3;
GPIO_Handle_t pToGPIOA4;
GPIO_Handle_t pToGPIOA5;
GPIO_Handle_t pToGPIOA6;
GPIO_Handle_t pToGPIOA7;
GPIO_Handle_t pToGPIOA8;
GPIO_Handle_t pToGPIOA9;
GPIO_Handle_t pToGPIOA10;
GPIO_Handle_t pToGPIOA11;
GPIO_Handle_t pToGPIOA12;
GPIO_Handle_t pToGPIOA13;
GPIO_Handle_t pToGPIOA14;
GPIO_Handle_t pToGPIOA15;

// GPIOB handles
GPIO_Handle_t pToGPIOB0;
GPIO_Handle_t pToGPIOB1;
GPIO_Handle_t pToGPIOB2;
GPIO_Handle_t pToGPIOB3;
GPIO_Handle_t pToGPIOB4;
GPIO_Handle_t pToGPIOB5;
GPIO_Handle_t pToGPIOB6;
GPIO_Handle_t pToGPIOB7;
GPIO_Handle_t pToGPIOB8;
GPIO_Handle_t pToGPIOB9;
GPIO_Handle_t pToGPIOB10;
GPIO_Handle_t pToGPIOB11;
GPIO_Handle_t pToGPIOB12;
GPIO_Handle_t pToGPIOB13;
GPIO_Handle_t pToGPIOB14;
GPIO_Handle_t pToGPIOB15;

// GPIOC handles
GPIO_Handle_t pToGPIOC0;
GPIO_Handle_t pToGPIOC1;
GPIO_Handle_t pToGPIOC2;
GPIO_Handle_t pToGPIOC3;
GPIO_Handle_t pToGPIOC4;
GPIO_Handle_t pToGPIOC5;
GPIO_Handle_t pToGPIOC6;
GPIO_Handle_t pToGPIOC7;
GPIO_Handle_t pToGPIOC8;
GPIO_Handle_t pToGPIOC9;
GPIO_Handle_t pToGPIOC10;
GPIO_Handle_t pToGPIOC11;
GPIO_Handle_t pToGPIOC12;
GPIO_Handle_t pToGPIOC13;
GPIO_Handle_t pToGPIOC14;
GPIO_Handle_t pToGPIOC15;

// GPIOD handles
GPIO_Handle_t pToGPIOD0;
GPIO_Handle_t pToGPIOD1;

// GPIOE handles
GPIO_Handle_t pToGPIOE4;



//setup the characteristics for the GPIO setup for all the LCD pins
void LCD_GPIO_Init(void) {

    GPIO_PinConfig_t defaultCfg;
    defaultCfg.GPIO_PinMode = GPIO_MODE_AF;
    defaultCfg.GPIO_PinAltFunMode = 11; //All LCD pins are AF11 accordingly to the DS

    // Array of handles for GPIO pins
    GPIO_Handle_t *pins[] = {
        &pToGPIOA1,  &pToGPIOA2,  &pToGPIOA3,  &pToGPIOA4,  &pToGPIOA6,  &pToGPIOA7, &pToGPIOA8,  &pToGPIOA9,  &pToGPIOA10,  &pToGPIOA15, //A
        &pToGPIOB3,  &pToGPIOB4,  &pToGPIOB5,  &pToGPIOB6,  &pToGPIOB7, &pToGPIOB10, &pToGPIOB11, &pToGPIOB12, //B
        &pToGPIOC0,  &pToGPIOC1,  &pToGPIOC2,  &pToGPIOC4,  &pToGPIOC5,  &pToGPIOC6,  &pToGPIOC10, &pToGPIOC11, &pToGPIOC12, //C
    };

    for (int i = 0; i < 10; i++) {
        pins[i]->pGPIOx = GPIOA;
        pins[i]->GPIO_PinConfig = defaultCfg;
        pins[i]->GPIO_PinConfig.GPIO_PinNumber = i; 
        GPIO_Init(pins[i]);
    }

    for (int i = 10; i < 18; i++) {
        pins[i]->pGPIOx = GPIOB;
        pins[i]->GPIO_PinConfig = defaultCfg;
        pins[i]->GPIO_PinConfig.GPIO_PinNumber = i; 
        GPIO_Init(pins[i]);
    }

    for (int i = 21; i < 27; i++) {
        pins[i]->pGPIOx = GPIOC;
        pins[i]->GPIO_PinConfig = defaultCfg;
        pins[i]->GPIO_PinConfig.GPIO_PinNumber = i; 
        GPIO_Init(pins[i]);
    }

}

void I2C_GPIO_Init(void) {

    GPIO_PinConfig_t defaultCfg;

    GPIO_Handle_t *pins[] = {
        &pToGPIOB8, &pToGPIOB9
    };

    defaultCfg.GPIO_PinMode = GPIO_MODE_AF;
    defaultCfg.GPIO_PinAltFunMode = 4;

    pins[0]->pGPIOx = GPIOB;
    pins[0]->GPIO_PinConfig = defaultCfg;
    pins[1]->pGPIOx = GPIOB;
    pins[1]->GPIO_PinConfig = defaultCfg;

}