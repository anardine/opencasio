#include "driver/gpio.h"

// GPIOA handles
GPIO_Handle_t *pToGPIOA0;
GPIO_Handle_t *pToGPIOA1;
GPIO_Handle_t *pToGPIOA2;
GPIO_Handle_t *pToGPIOA3;
GPIO_Handle_t *pToGPIOA4;
GPIO_Handle_t *pToGPIOA5;
GPIO_Handle_t *pToGPIOA6;
GPIO_Handle_t *pToGPIOA7;
GPIO_Handle_t *pToGPIOA8;
GPIO_Handle_t *pToGPIOA9;
GPIO_Handle_t *pToGPIOA10;
GPIO_Handle_t *pToGPIOA11;
GPIO_Handle_t *pToGPIOA12;
GPIO_Handle_t *pToGPIOA13;
GPIO_Handle_t *pToGPIOA14;
GPIO_Handle_t *pToGPIOA15;

// GPIOB handles
GPIO_Handle_t *pToGPIOB0;
GPIO_Handle_t *pToGPIOB1;
GPIO_Handle_t *pToGPIOB2;
GPIO_Handle_t *pToGPIOB3;
GPIO_Handle_t *pToGPIOB4;
GPIO_Handle_t *pToGPIOB5;
GPIO_Handle_t *pToGPIOB6;
GPIO_Handle_t *pToGPIOB7;
GPIO_Handle_t *pToGPIOB8;
GPIO_Handle_t *pToGPIOB9;
GPIO_Handle_t *pToGPIOB10;
GPIO_Handle_t *pToGPIOB11;
GPIO_Handle_t *pToGPIOB12;
GPIO_Handle_t *pToGPIOB13;
GPIO_Handle_t *pToGPIOB14;
GPIO_Handle_t *pToGPIOB15;

// GPIOC handles
GPIO_Handle_t *pToGPIOC0;
GPIO_Handle_t *pToGPIOC1;
GPIO_Handle_t *pToGPIOC2;
GPIO_Handle_t *pToGPIOC3;
GPIO_Handle_t *pToGPIOC4;
GPIO_Handle_t *pToGPIOC5;
GPIO_Handle_t *pToGPIOC6;
GPIO_Handle_t *pToGPIOC7;
GPIO_Handle_t *pToGPIOC8;
GPIO_Handle_t *pToGPIOC9;
GPIO_Handle_t *pToGPIOC10;
GPIO_Handle_t *pToGPIOC11;
GPIO_Handle_t *pToGPIOC12;
GPIO_Handle_t *pToGPIOC13;
GPIO_Handle_t *pToGPIOC14;
GPIO_Handle_t *pToGPIOC15;

// GPIOD handles
GPIO_Handle_t *pToGPIOD0;
GPIO_Handle_t *pToGPIOD1;

// GPIOE handles
GPIO_Handle_t *pToGPIOE4;


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

// TODO: Defensive programming with return of codes. For now the functions return void just for simplicity of first implementation
void LCD_GPIO_Init(void);
void I2C_GPIO_Init(void);
void Buzzer_GPIO_Init(void);
void Btn_GPIO_Init(void);