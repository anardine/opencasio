#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"

//Define the ENUM

enum PeripheralCode {
    LCD_PER,
    I2C_PER,
    BUZ_PER,
    BTN_PER,
};

// Define a structure to hold pin configuration data
  typedef struct {
      GPIOx_RegTypeDef *pGPIOx;
      uint8_t pinNumber;
      enum PeripheralCode perCode;
  } LCD_GPIO_PinDef_t;

// Define all LCD pins with their GPIO port and pin number
const LCD_GPIO_PinDef_t pinDefinitions[] = {
    // GPIO A
    {GPIOA, 1, LCD_PER},   {GPIOA, 2, LCD_PER},   {GPIOA, 3, LCD_PER},
    {GPIOA, 4, LCD_PER},   {GPIOA, 5, BUZ_PER},{GPIOA, 6, LCD_PER},   {GPIOA, 7, LCD_PER},
    {GPIOA, 8, LCD_PER},   {GPIOA, 9, LCD_PER},   {GPIOA, 10, LCD_PER},
    {GPIOA, 15, LCD_PER},
    // GPIO B
    {GPIOB, 3, LCD_PER},   {GPIOB, 4, LCD_PER},   {GPIOB, 5, LCD_PER},
    {GPIOB, 6, LCD_PER},   {GPIOB, 7, LCD_PER},   {GPIOB, 8, I2C_PER},
    {GPIOB, 9, I2C_PER},   {GPIOB, 10, LCD_PER},   {GPIOB, 11, LCD_PER},
    {GPIOB, 12, LCD_PER},
    // GPIO C
    {GPIOC, 0, LCD_PER},   {GPIOC, 1, LCD_PER},   {GPIOC, 2, LCD_PER},
    {GPIOC, 3, BTN_PER},   {GPIOC, 4, LCD_PER},   {GPIOC, 5, LCD_PER},   {GPIOC, 6, LCD_PER},
    {GPIOC, 10, LCD_PER},  {GPIOC, 11, LCD_PER},  {GPIOC, 12, LCD_PER},  {GPIOC, 13, BTN_PER},
    // GPIO E
    {GPIOE, 4, BTN_PER}
};

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

