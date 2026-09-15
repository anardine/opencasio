//
// Created by Alessandro Nardinelli on 05/12/25.
//

#include "driver/i2c.h"
#include "driver/rcc.h"
#include "auxiliary/gpio-pins-setup.h"

// RM0434 Table 209, fI2CCLK = 16 MHz, Standard-mode 100 kHz:
// PRESC=3, SCLDEL=4, SDADEL=2, SCLH=0x0F, SCLL=0x13. Validated by ST —
// preferred over recomputing with float math.
#define I2C_TIMINGR_16MHZ_100KHZ   0x30420F13UL

// RM0434 35.9.1 note: PE must stay low for at least 3 APB cycles after
// clearing. At 16 MHz that is ~188 ns; 16 loop iterations is far above that.
#define I2C_PE_RESET_CYCLES        16U

// Status-poll bound. At 100 kHz one byte takes ~90 µs on the wire;
// 100k iterations of a 2-instruction poll at 16 MHz is ~12 ms — an order of
// magnitude above the slowest legal single transfer, so hitting the bound
// means the bus is stuck, not slow.
#define I2C_POLL_LIMIT             100000U

// Waits for an ISR flag with the error breakouts RM0434 35.9.7 defines.
// Returns CORE_OK, I2C_NACK_ERR, or I2C_BUS_ERR; leaves the flag set for
// the caller to act on / clear.
static uint8_t I2C_WaitFlag(I2Cx_Reg_TypeDef *pI2Cx, uint32_t flag) {
    uint32_t remaining = I2C_POLL_LIMIT;
    while (!(pI2Cx->isr & flag)) {
        // NACKF (bit 4) means the addressed target rejected the byte.
        if (pI2Cx->isr & (1U << 4)) return I2C_NACK_ERR;
        // BERR (bit 8) / ARLO (bit 9) mean the transaction is unrecoverable.
        if (pI2Cx->isr & ((1U << 8) | (1U << 9))) return I2C_BUS_ERR;
        if (--remaining == 0U) return I2C_BUS_ERR;
    }
    return CORE_OK;
}

// Clears the transaction-end flags the RM says are software-cleared (35.9.8).
static void I2C_ClearEndFlags(I2Cx_Reg_TypeDef *pI2Cx, uint8_t outcome) {
    pI2Cx->icr = (1U << 5);                    // STOPCF
    if (outcome == I2C_NACK_ERR) pI2Cx->icr |= (1U << 4);  // NACKCF
    if (outcome == I2C_BUS_ERR)  pI2Cx->icr |= (1U << 9) | (1U << 8); // ARLOCF | BERRCF
}

uint8_t I2C_Init(I2C_Handle_t *pToI2CHandle) {
    if (!pToI2CHandle || !pToI2CHandle->pI2Cx) return I2C_CFG_ERR;
    // Only the 16 MHz I2CCLK / 100 kHz Sm row of RM Table 209 is validated.
    if (pToI2CHandle->I2C_PinConfig.I2C_Mode != I2C_MODE_STANDARD ||
        pToI2CHandle->I2C_PinConfig.I2C_ClockSpeed != 100000U) {
        return I2C_CFG_ERR;
    }

    // PB8/PB9 as AF4 open-drain; R7/R8 on the board provide the pull-ups.
    I2C_GPIO_Init();
    enableRCC(I2C_PER);
    (void)RCC->apb1enr1; // clock-enable readback before register access

    I2Cx_Reg_TypeDef *i2c = pToI2CHandle->pI2Cx;

    // TIMINGR and CR1 filter fields may only be written with PE = 0.
    i2c->cr1 = 0;
    for (uint32_t i = 0; i < I2C_PE_RESET_CYCLES; i++) __asm volatile ("nop");

    // I2CCLK stays on the reset default I2C1SEL=00 (PCLK): with SYSCLK=HSI16
    // and reset APB1 prescaler 1, PCLK = 16 MHz, matching Table 209. Do NOT
    // switch to HSI16 here — that is a Stop-mode decision for Phase 7.
    i2c->timingr = I2C_TIMINGR_16MHZ_100KHZ;

    // CR1 reset state is already what we want: no DMA, no interrupts, analog
    // filter on, digital filter off, NOSTRETCH=0 (required in controller mode).
    i2c->cr1 |= 1U; // PE
    return CORE_OK;
}

void I2C_DeInit(I2Cx_Reg_TypeDef *pI2Cx) {
    // PE=0 releases SCL/SDA and resets the internal state machine (35.9.1).
    pI2Cx->cr1 = 0;
    diableRCC(I2C_PER);
}

uint8_t I2C_Transmit(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t memAddr,
                     uint8_t length, uint8_t deviceAddress) {
    if (!pToI2CHandle || !pToI2CHandle->pI2Cx) return I2C_CFG_ERR;
    I2Cx_Reg_TypeDef *i2c = pToI2CHandle->pI2Cx;

    // NBYTES = memAddr byte + payload. The RV-3129-C3 forbids repeated START,
    // so AUTOEND=1 always: STOP terminates every transaction (35.9.2).
    const uint32_t nbytes = (uint32_t)length + 1U;
    // SADD[7:1] takes the 7-bit address: the raw convention byte's bit0 is R/W.
    i2c->cr2 = (((uint32_t)deviceAddress >> 1) << 1) |
               (nbytes << 16) | (1U << 25) | (1U << 13); // AUTOEND | START

    uint8_t status = I2C_WaitFlag(i2c, 1U << 1); // TXIS
    if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }
    i2c->txdr = memAddr;

    for (uint8_t i = 0; i < length; i++) {
        status = I2C_WaitFlag(i2c, 1U << 1);
        if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }
        i2c->txdr = data[i];
    }

    status = I2C_WaitFlag(i2c, 1U << 5); // STOPF: AUTOEND sent the STOP
    I2C_ClearEndFlags(i2c, status);
    return status;
}

uint8_t I2C_Receive(I2C_Handle_t *pToI2CHandle, uint8_t *data,
                    uint8_t length, uint8_t deviceAddress) {
    if (!pToI2CHandle || !pToI2CHandle->pI2Cx || (!data && length)) return I2C_CFG_ERR;
    I2Cx_Reg_TypeDef *i2c = pToI2CHandle->pI2Cx;

    // RD_WRN=1 (bit 10); controller generates NACK+STOP after the last byte.
    i2c->cr2 = (((uint32_t)deviceAddress >> 1) << 1) | (1U << 10) |
               ((uint32_t)length << 16) | (1U << 25) | (1U << 13);

    for (uint8_t i = 0; i < length; i++) {
        uint8_t status = I2C_WaitFlag(i2c, 1U << 2); // RXNE
        if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }
        data[i] = (uint8_t)i2c->rxdr;
    }

    uint8_t status = I2C_WaitFlag(i2c, 1U << 5); // STOPF
    I2C_ClearEndFlags(i2c, status);
    return status;
}

uint8_t I2C_MemRead(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data,
                     uint8_t length, uint8_t deviceAddress) {
    if (!pToI2CHandle || !pToI2CHandle->pI2Cx || (!data && length)) return I2C_CFG_ERR;
    I2Cx_Reg_TypeDef *i2c = pToI2CHandle->pI2Cx;

    // Write the register pointer with AUTOEND=0: no STOP is issued, so the
    // target's internal read pointer/latch is not disturbed before the
    // read phase begins (some parts reset/re-latch output regs on STOP).
    i2c->cr2 = (((uint32_t)deviceAddress >> 1) << 1) |
               (1U << 16) | (1U << 13); // NBYTES=1 | START, AUTOEND=0

    uint8_t status = I2C_WaitFlag(i2c, 1U << 1); // TXIS
    if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }
    i2c->txdr = memAddr;

    status = I2C_WaitFlag(i2c, 1U << 6); // TC: transfer complete, ready for repeated START
    if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }

    // Repeated START directly into the read phase, AUTOEND=1 so STOP is
    // generated automatically after the last byte.
    i2c->cr2 = (((uint32_t)deviceAddress >> 1) << 1) | (1U << 10) |
               ((uint32_t)length << 16) | (1U << 25) | (1U << 13); // RD_WRN | NBYTES | AUTOEND | START

    for (uint8_t i = 0; i < length; i++) {
        status = I2C_WaitFlag(i2c, 1U << 2); // RXNE
        if (status != CORE_OK) { I2C_ClearEndFlags(i2c, status); return status; }
        data[i] = (uint8_t)i2c->rxdr;
    }

    status = I2C_WaitFlag(i2c, 1U << 5); // STOPF
    I2C_ClearEndFlags(i2c, status);
    return status;
}
