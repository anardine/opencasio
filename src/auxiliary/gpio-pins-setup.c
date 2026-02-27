#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"

//setup the characteristics for the GPIO setup for all the LCD pins
void LCD_GPIO_Init(void) {

    // Define a structure to hold pin configuration data
    typedef struct {
        GPIOx_RegTypeDef *pGPIOx;
        uint8_t pinNumber;
    } LCD_GPIO_PinDef_t;

    // Define all LCD pins with their GPIO port and pin number
    const LCD_GPIO_PinDef_t pinDefinitions[] = {
        // GPIO A
        {GPIOA, 1},   {GPIOA, 2},   {GPIOA, 3},   {GPIOA, 4},   {GPIOA, 6},
        {GPIOA, 7},   {GPIOA, 8},   {GPIOA, 9},   {GPIOA, 10},  {GPIOA, 15},
        // GPIO B
        {GPIOB, 3},   {GPIOB, 4},   {GPIOB, 5},   {GPIOB, 6},   {GPIOB, 7},
        {GPIOB, 10},  {GPIOB, 11},  {GPIOB, 12},
        // GPIO C
        {GPIOC, 0},   {GPIOC, 1},   {GPIOC, 2},   {GPIOC, 4},   {GPIOC, 5},
        {GPIOC, 6},   {GPIOC, 10},  {GPIOC, 11},  {GPIOC, 12},
    };

    GPIO_PinConfig_t defaultCfg;
    defaultCfg.GPIO_PinMode = GPIO_MODE_AF;
    defaultCfg.GPIO_PinAltFunMode = 11; //All LCD pins are AF11 accordingly to the DS

    // Array of handles for GPIO pins
    GPIO_Handle_t *pins[] = {
        &pToGPIOA1,  &pToGPIOA2,  &pToGPIOA3,  &pToGPIOA4,  &pToGPIOA6,  &pToGPIOA7, &pToGPIOA8,  &pToGPIOA9,  &pToGPIOA10,  &pToGPIOA15, //A
        &pToGPIOB3,  &pToGPIOB4,  &pToGPIOB5,  &pToGPIOB6,  &pToGPIOB7, &pToGPIOB10, &pToGPIOB11, &pToGPIOB12, //B
        &pToGPIOC0,  &pToGPIOC1,  &pToGPIOC2,  &pToGPIOC4,  &pToGPIOC5,  &pToGPIOC6,  &pToGPIOC10, &pToGPIOC11, &pToGPIOC12, //C
    };

    // Single loop initialization
    for (uint8_t i = 0; i < 27; i++) {
        pins[i]->pGPIOx = pinDefinitions[i].pGPIOx;
        pins[i]->GPIO_PinConfig = defaultCfg;
        pins[i]->GPIO_PinConfig.GPIO_PinNumber = pinDefinitions[i].pinNumber;
        GPIO_Init(pins[i]);  // If you have an init function
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
    pins[0]->GPIO_PinConfig.GPIO_PinNumber = 8;
    pins[1]->pGPIOx = GPIOB;
    pins[1]->GPIO_PinConfig = defaultCfg;
    pins[1]->GPIO_PinConfig.GPIO_PinNumber = 9;

    GPIO_Init(pins[0]);
    GPIO_Init(pins[1]);

}

void Buzzer_GPIO_Init(void) {

    GPIO_Handle_t pinPA5 = &pToGPIOA5;

    pinPA5.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUTPUT;
    pinPA5.GPIO_PinConfig.GPIO_PinNumber = 5;
    pinPA5->pGPIOx = GPIOA;

    GPIO_Init(pinPA5);

}

void Btn_GPIO_Init(void) {

    GPIO_Handle_t pinPC13 = &pToGPIOC13;    // BTN LED
    GPIO_Handle_t pinPC3 = &pToGPIOC3;      // BTN MODE
    GPIO_Handle_t pinPE4 = &pToGPIOE4;    // BTN ALARM

    pinPC13->pGPIOx = GPIOC;
    pinPC13.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pinPC13);

    pinPC3->pGPIOx = GPIOC;
    pinPC3.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pinPC3);

    pinPE4->pGPIOx = GPIOE;
    pinPE4.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_INPUT;
    GPIO_Init(pinPE4);

}

