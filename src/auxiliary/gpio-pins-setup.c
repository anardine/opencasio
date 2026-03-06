#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"

//setup the characteristics for the GPIO setup for all the LCD pins
void LCD_GPIO_Init(void) {

    GPIO_PinConfig_t defaultCfg;
    defaultCfg.GPIO_PinMode = GPIO_MODE_AF;
    defaultCfg.GPIO_PinAltFunMode = 11; //All LCD pins are AF11 accordingly to the DS

    // Array of handles for GPIO pins
    GPIO_Handle_t *pins[] = {
        pToGPIOA1,  pToGPIOA2,  pToGPIOA3,  pToGPIOA4,  pToGPIOA6,  pToGPIOA7, pToGPIOA8,  pToGPIOA9,  pToGPIOA10,  pToGPIOA15, //A
        pToGPIOB2,  pToGPIOB3,  pToGPIOB4,  pToGPIOB5,  pToGPIOB6,  pToGPIOB7, pToGPIOB10, pToGPIOB11, pToGPIOB12, //B
        pToGPIOC0,  pToGPIOC1,  pToGPIOC2,  pToGPIOC4,  pToGPIOC5,  pToGPIOC6,  pToGPIOC10, pToGPIOC11, pToGPIOC12, //C
    };

    // Single loop initialization
    for (uint8_t i = 0; i < 28; i++) {
        pins[i]->pGPIOx = pinDefinitions[i].pGPIOx;
        pins[i]->GPIO_PinConfig = defaultCfg;
        pins[i]->GPIO_PinConfig.GPIO_PinNumber = pinDefinitions[i].pinNumber;
        GPIO_Init(pins[i]);  // If you have an init function
    }
}

void I2C_GPIO_Init(void) {

    GPIO_PinConfig_t defaultCfg;

    GPIO_Handle_t *pins[] = {
        pToGPIOB8, pToGPIOB9
    };

    defaultCfg.GPIO_PinMode = GPIO_MODE_AF;
    defaultCfg.GPIO_PinAltFunMode = 4;

    pins[0]->pGPIOx = GPIOB;
    pins[0]->GPIO_PinConfig = defaultCfg;
    pins[0]->GPIO_PinConfig.GPIO_PinNumber = 8;
    pins[1]->pGPIOx = GPIOB;
    pins[1]->GPIO_PinConfig = defaultCfg;
    pins[1]->GPIO_PinConfig.GPIO_PinNumber = 9;

    GPIO_Init(pins[0]);
    GPIO_Init(pins[1]);

}

void Buzzer_GPIO_Init(void) {

    pToGPIOA5->GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUTPUT;
    pToGPIOA5->GPIO_PinConfig.GPIO_PinNumber = 5;
    pToGPIOA5->pGPIOx = GPIOA;

    GPIO_Init(pToGPIOA5);

}

void Btn_GPIO_Init(void) {

    pToGPIOC13->pGPIOx = GPIOC;
    pToGPIOC13->GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pToGPIOC13);

    pToGPIOC3->pGPIOx = GPIOC;
    pToGPIOC3->GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pToGPIOC3);

    pToGPIOE4->pGPIOx = GPIOE;
    pToGPIOE4->GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pToGPIOE4);

}

