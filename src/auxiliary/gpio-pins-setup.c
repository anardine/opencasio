#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"

void LCD_GPIO_Init(void) {
    // REFERENCE.md section 4: only the 28 wired LCD/VLCD pins, not the
    // interleaved buzzer, button and I2C pins from the old shared table.
    static const struct {
        GPIOx_RegTypeDef *port;
        uint8_t pin;
    } pins[] = {
        {GPIOA, 1}, {GPIOA, 2}, {GPIOA, 3}, {GPIOA, 4}, {GPIOA, 6},
        {GPIOA, 7}, {GPIOA, 8}, {GPIOA, 9}, {GPIOA, 10}, {GPIOA, 15},
        {GPIOB, 2}, {GPIOB, 3}, {GPIOB, 4}, {GPIOB, 5}, {GPIOB, 6},
        {GPIOB, 7}, {GPIOB, 10}, {GPIOB, 11}, {GPIOB, 12},
        {GPIOC, 0}, {GPIOC, 1}, {GPIOC, 2}, {GPIOC, 4}, {GPIOC, 5},
        {GPIOC, 6}, {GPIOC, 10}, {GPIOC, 11}, {GPIOC, 12},
    };

    for (uint8_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        const GPIO_Handle_t pin = {
            .pGPIOx = pins[i].port,
            .GPIO_PinConfig = {
                .GPIO_PinNumber = pins[i].pin,
                .GPIO_PinMode = GPIO_MODE_AF,
                .GPIO_PinAltFunMode = GPIO_AFL_AF11,
            },
        };
        GPIO_Init(&pin);
    }
}

void I2C_GPIO_Init(void) {
    // DS11929 table 18: PB8/PB9 AF4. R7/R8 supply the external pulls;
    // RM0434 section 10.4.2 requires open-drain peripheral outputs.
    for (uint8_t number = 8; number <= 9; number++) {
        const GPIO_Handle_t pin = {
            .pGPIOx = GPIOB,
            .GPIO_PinConfig = {
                .GPIO_PinNumber = number,
                .GPIO_PinMode = GPIO_MODE_AF,
                .GPIO_PinOPType = GPIO_OTYPE_OD,
                .GPIO_PinAltFunMode = GPIO_AFH_AF4,
            },
        };
        GPIO_Init(&pin);
    }
}

void Buzzer_GPIO_Init(void) {
    const GPIO_Handle_t pin = {
        .pGPIOx = GPIOA,
        .GPIO_PinConfig = {
            .GPIO_PinNumber = 5,
            .GPIO_PinMode = GPIO_MODE_OUTPUT,
        },
    };
    GPIO_Init(&pin);
}

// PB0 = TEMP_EN, PB1 = MAG_EN. Both start LOW (Board_GPIO_Init).
static const GPIO_Handle_t tempEnPin = {
    .pGPIOx = GPIOB, .GPIO_PinConfig = { .GPIO_PinNumber = 0 },
};
static const GPIO_Handle_t magEnPin = {
    .pGPIOx = GPIOB, .GPIO_PinConfig = { .GPIO_PinNumber = 1 },
};

void railOn(void) {
    GPIO_WriteToOutputPin(&tempEnPin, 1);
    GPIO_WriteToOutputPin(&magEnPin, 1);
}

void railOff(void) {
    GPIO_WriteToOutputPin(&tempEnPin, 0);
    GPIO_WriteToOutputPin(&magEnPin, 0);
}

// U11/U12 rise time unknown (REFERENCE.md §7.3). ~1 ms at 16 MHz is
// conservative; tighten after measuring on the board.
void railSettleDelay(void) {
    for (volatile uint32_t i = 0; i < 4000; i++) __asm volatile ("nop");
}

uint8_t Btn_GPIO_Init(void) {
    // Schematic R12/R13/R14 provide external pulldowns. Leave internal
    // pulls off; GPIO_ReadFromInputPin returns 1 while a button is pressed.
    // Buttons are active-HIGH: the press edge is rising. Release is handled
    // by polling in the UI task later, not by a second interrupt edge.
    static const GPIO_Handle_t pins[] = {
        {.pGPIOx = GPIOC, .GPIO_PinConfig = {
            .GPIO_PinNumber = 13, .GPIO_PinMode = GPIO_MODE_INPUT,
            .GPIO_isInterrupt = GPIO_IRQ_RISING}}, // BTN_LED
        {.pGPIOx = GPIOC, .GPIO_PinConfig = {
            .GPIO_PinNumber = 3, .GPIO_PinMode = GPIO_MODE_INPUT,
            .GPIO_isInterrupt = GPIO_IRQ_RISING}}, // BTN_MODE
        {.pGPIOx = GPIOE, .GPIO_PinConfig = {
            .GPIO_PinNumber = 4, .GPIO_PinMode = GPIO_MODE_INPUT,
            .GPIO_isInterrupt = GPIO_IRQ_RISING}}, // BTN_ALARM
    };

    for (uint8_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        uint8_t status = GPIO_Init(&pins[i]);
        if (status != CORE_OK) return status;
    }
    return CORE_OK;
}

uint8_t Board_GPIO_Init(void) {
    // Zero-initialized fields select PP, low speed, no pulls and no EXTI.
    // GPIO_Init preloads each output LOW before enabling its driver.
    // TEMP_EN/MAG_EN polarity still needs the load-switch part number;
    // do not enable either sensor until that hardware question is resolved.
    static const GPIO_Handle_t pins[] = {
        {.pGPIOx = GPIOB, .GPIO_PinConfig = {
            .GPIO_PinNumber = 0, .GPIO_PinMode = GPIO_MODE_OUTPUT}}, // TEMP_EN
        {.pGPIOx = GPIOB, .GPIO_PinConfig = {
            .GPIO_PinNumber = 1, .GPIO_PinMode = GPIO_MODE_OUTPUT}}, // MAG_EN
        {.pGPIOx = GPIOB, .GPIO_PinConfig = {
            .GPIO_PinNumber = 13, .GPIO_PinMode = GPIO_MODE_OUTPUT}}, // LED_EN
        // RTC_INT is active-LOW, pulled up externally by R11: the assertion
        // edge is falling.
        {.pGPIOx = GPIOA, .GPIO_PinConfig = {
            .GPIO_PinNumber = 0, .GPIO_PinMode = GPIO_MODE_INPUT,
            .GPIO_isInterrupt = GPIO_IRQ_FALLING}},
    };

    for (uint8_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        uint8_t status = GPIO_Init(&pins[i]);
        if (status != CORE_OK) return status;
    }
    return Btn_GPIO_Init();
}

