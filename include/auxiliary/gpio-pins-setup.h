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
    PER_LCD,
    PER_I2C,
    PER_BUZ,
    PER_BTN,
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
    {GPIOA, 1, PER_LCD},   {GPIOA, 2, PER_LCD},   {GPIOA, 3, PER_LCD},
    {GPIOA, 4, PER_LCD},   {GPIOA, 5, PER_BUZ},{GPIOA, 6, PER_LCD},   {GPIOA, 7, PER_LCD},
    {GPIOA, 8, PER_LCD},   {GPIOA, 9, PER_LCD},   {GPIOA, 10, PER_LCD},
    {GPIOA, 15, PER_LCD},
    // GPIO B
    {GPIOB, 2, PER_LCD}, {GPIOB, 3, PER_LCD},   {GPIOB, 4, PER_LCD},
    {GPIOB, 5, PER_LCD}, {GPIOB, 6, PER_LCD},   {GPIOB, 7, PER_LCD},
    {GPIOB, 8, PER_I2C}, {GPIOB, 9, PER_I2C},   {GPIOB, 10, PER_LCD},
    {GPIOB, 11, PER_LCD},{GPIOB, 12, PER_LCD},
    // GPIO C
    {GPIOC, 0, PER_LCD},   {GPIOC, 1, PER_LCD},   {GPIOC, 2, PER_LCD},
    {GPIOC, 3, PER_BTN},   {GPIOC, 4, PER_LCD},   {GPIOC, 5, PER_LCD},   {GPIOC, 6, PER_LCD},
    {GPIOC, 10, PER_LCD},  {GPIOC, 11, PER_LCD},  {GPIOC, 12, PER_LCD},  {GPIOC, 13, PER_BTN},
    // GPIO E
    {GPIOE, 4, PER_BTN}
};

// TODO: Defensive programming with return of codes. For now the functions return void just for simplicity of first implementation
void LCD_GPIO_Init(void);
void I2C_GPIO_Init(void);
void Buzzer_GPIO_Init(void);
void Btn_GPIO_Init(void);