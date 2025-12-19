//
// Created by Alessandro Nardinelli on 05/12/25.
//

#include "driver/i2c.h"

uint8_t I2C_Init(I2C_Handle_t *pToI2CHandle) {

    if (pToI2CHandle->I2C_PinConfig.I2C_Mode == I2C_MODE_FMP) {
        //set Fast-Mode Plus if enabled
      (SYSCFG->cfgr1 |= (1 << 20));
    }

      uint8_t freq = pToI2CHandle->I2C_PinConfig.I2C_ClockSpeed;
      uint8_t freqMode = pToI2CHandle->I2C_PinConfig.I2C_Mode;

    // freq_mode: 0 = Standard (<=100kHz), 1 = Fast (<=400kHz), 2 = Fast Mode Plus (<=1MHz)
    uint32_t presc=0, scll=0, sclh=0, sdadel=0, scldel=0;
    uint32_t t_rise_ns, t_fall_ns;
    float t_scl, t_presc;

    //TODO: Check the implementation details using the reference manual

    // Select I2C mode, adjust timing formula based on mode
    if(freqMode == I2C_MODE_STANDARD) {
        // Standard Mode (max 100kHz), typical tr=1000ns tf=300ns
        t_rise_ns = 1000;
        t_fall_ns = 300;
    } else if(freqMode == I2C_MODE_FM) {
        // Fast Mode (max 400kHz), typical tr=300ns tf=300ns
        t_rise_ns = 300;
        t_fall_ns = 300;
    } else if (freqMode == I2C_MODE_FMP){
        // Fast Mode Plus (max 1MHz), typical tr=120ns tf=120ns
        t_rise_ns = 120;
        t_fall_ns = 120;
    } else {
        return 1;
    }

    // See STM32WB Reference: SCLL+1/CLk, SCLH+1/Clk, DEL, SDADEL, PRESC, etc
    // Calculate timings based on AN4235 and ref man
    // tSCL = 1/SCL = tSCLL + tSCLH + tRise + tFall

    t_scl = 1e9f / (float)freq; // in ns

    // Try multiple prescalers (0-15), select a valid result
    for (presc = 0; presc < 16; presc++) {
        t_presc = ((float)(presc+1)) * 1000000000.0f / (float) F_CPU;

        // SCLL: SCL low period = (SCLL+1) * tPRESC
        // SCLH: SCL high period = (SCLH+1) * tPRESC
        float desired_scll = ((t_scl/2.0f) - t_fall_ns) / t_presc - 1.0f;
        float desired_sclh = ((t_scl/2.0f) - t_rise_ns) / t_presc - 1.0f;

        scll = (uint32_t)desired_scll;
        sclh = (uint32_t)desired_sclh;

        if (scll < 1) scll = 1;
        if (sclh < 1) sclh = 1;
        if(scll > 255 || sclh > 255)
            continue; // try another presc

        // Minimum setup and data hold times (typ values)
        if(freqMode == 0) {
            // standard: tSCLDEL>400ns, tSDADEL>300ns
            scldel = (uint32_t)((400.0f/t_presc)+1)-1;
            sdadel = (uint32_t)((300.0f/t_presc));
        } else {
            // fast/plus: tSCLDEL>100ns, tSDADEL>50ns
            scldel = (uint32_t)((100.0f/t_presc)+1)-1;
            sdadel = (uint32_t)((50.0f/t_presc));
        }
        if(scldel>15) scldel=15;
        if(sdadel>15) sdadel=15;

        // Compose the timing register as [31:28]=PRESC [27:24]=SCLDEL [23:20]=SDADEL
        // [19:16]=SCLH [7:0]=SCLL
        uint32_t timingr = (presc << 28) | (scldel << 20) | (sdadel << 16) | (sclh << 8) | scll;

        // Apply timing configuration to I2C peripheral
        pToI2CHandle->pI2Cx->timingr = timingr;

        //enable the peripheral
        pToI2CHandle->pI2Cx->cr1 |= (I2C_ENABLE << 0);

        return 0; // configured!
    }

    // Return 1 if not configured
    return 1;

}


void I2C_DeInit(I2Cx_Reg_TypeDef *pI2Cx) {
        //TODO: Implement de-init
}


uint8_t I2C_Transmit(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t length, uint8_t address) {
    
    //put device in controller mode and configure sending bits
    uint32_t volatile cr2 = 0;

    // add the address to CR2
    cr2 |= ((uint32_t)address << 1);

    //enable transfer direction to be TX
    cr2 &= ~(1 << 10);

    //set address mode to 7bit addressing mode
    cr2 &= ~(1 << 11);

    //define the number of bytes to be sent
    cr2 |= ((uint32_t)length << 16);

    //define that a stop condition should be sent when the bytes are transmitted
    cr2 |= (1 << 25);

    //send start bit. To launch the communication, set the START bit of the I2C_CR2 register. The controller then automatically sends a START condition followed by the target address, either immediately if the BUSY flag is low, or tBUF time after the BUSY flag transits from high to lowstate. The BUSY flag is set upon sending the START condition.
    cr2 |= (1 << 13);

    //set cr2
    pToI2CHandle->pI2Cx->cr2 = cr2;

    //send data
    for (int i = 0; i < length; i++) {
        // Wait until TXIS (Transmit interrupt status) flag is set or NACK received
        // TXIS is bit 1 in ISR
        while (!(pToI2CHandle->pI2Cx->isr & (1 << 1))){}

        // Write data to TXDR
        pToI2CHandle->pI2Cx->txdr = data[i];
    }

    // 4. Wait for the STOP condition to be detected
    // STOPF is bit 5 in ISR
    while (!(pToI2CHandle->pI2Cx->isr & (1 << 5))){}

    // Clear STOPF flag by writing to ICR (Interrupt Clear Register)
    pToI2CHandle->pI2Cx->icr |= (1 << 5);

    // Reset CR2 (optional, but good practice to clear NBYTES and ADDR)
    pToI2CHandle->pI2Cx->cr2 = 0;

    return 0;

}


uint8_t I2C_Receive(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t length, uint8_t address) {

    // 1. Configure CR2 for receive
    uint32_t volatile cr2 = 0;

    // Set target address (7-bit)
    cr2 |= ((uint32_t)address << 1);

    // Set transfer direction to Read (RD_WRN = 1)
    cr2 |= (1 << 10);

    // Set number of bytes to read
    cr2 |= ((uint32_t)length << 16);

    // Auto-stop generation: hardware sends STOP after NBYTES
    cr2 |= (1 << 25);

    // Start condition
    cr2 |= (1 << 13);

    pToI2CHandle->pI2Cx->cr2 = cr2;

    // 2. Read 'length' bytes
    for (uint8_t i = 0; i < length; i++) {
        // Wait for RXNE (Receive data register not empty) flag or NACK
        while (!(pToI2CHandle->pI2Cx->isr & (1 << 2))) {
            // Check for NACKF (bit 4) in case slave doesn't respond
            if (pToI2CHandle->pI2Cx->isr & (1 << 4)) {
                pToI2CHandle->pI2Cx->icr |= (1 << 4); // Clear NACK
                return 1;
            }
        }

        // Read data from RXDR
        data[i] = (uint8_t)pToI2CHandle->pI2Cx->rxdr;
    }

    // 3. Wait for the STOP condition to be detected
    // STOPF is bit 5 in ISR
    while (!(pToI2CHandle->pI2Cx->isr & (1 << 5))) {}

    // Clear STOPF flag by writing to ICR
    pToI2CHandle->pI2Cx->icr |= (1 << 5);

    // Reset CR2
    pToI2CHandle->pI2Cx->cr2 = 0;

    return 0;
}


