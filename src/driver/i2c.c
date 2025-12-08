//
// Created by Alessandro Nardinelli on 05/12/25.
//

#include "driver/i2c.h"

void I2C_Init(I2C_Handle_t *pToI2CHandle) {

    if (pToI2CHandle->I2C_PinConfig.I2C_Mode == I2C_MODE_FMP) {
        //set Fast-Mode Plus if enabled
      (SYSCFG->cfgr1 |= (1 << 20));
    }
      //enable the peripheral
      pToI2CHandle->pI2Cx->cr1 |= (I2C_ENABLE << 0);

}

uint8_t I2C_Start (I2C_Handle_t *pToI2CHandle) {

      uint8_t ackControl = pToI2CHandle->I2C_PinConfig.I2C_AckControl;
      uint8_t freq = pToI2CHandle->I2C_PinConfig.I2C_ClockSpeed;
      uint8_t addr = pToI2CHandle->I2C_PinConfig.I2C_DeviceAddress;
      uint8_t dutyCycle = pToI2CHandle->I2C_PinConfig.I2C_DutyCycleForFastMode;
      uint8_t freqMode = pToI2CHandle->I2C_PinConfig.I2C_Mode;

    // freq_mode: 0 = Standard (<=100kHz), 1 = Fast (<=400kHz), 2 = Fast Mode Plus (<=1MHz)
    uint32_t presc=0, scll=0, sclh=0, sdadel=0, scldel=0;
    uint32_t t_rise_ns, t_fall_ns;
    float t_scl, t_presc;

    //TODO: Check the implementation details using the reference manual

    // Select I2C mode, adjust timing formula based on mode
    if(freqMode == 0) {
        // Standard Mode (max 100kHz), typical tr=1000ns tf=300ns
        t_rise_ns = 1000;
        t_fall_ns = 300;
    } else if(freqMode == 1) {
        // Fast Mode (max 400kHz), typical tr=300ns tf=300ns
        t_rise_ns = 300;
        t_fall_ns = 300;
    } else if (freqMode == 2){
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

        return 0; // configured!
    }

    // If we reach here, no config is possible for the given arguments.
    // You could assert(0), log an error, or fall back to a default config.

}


void I2C_DeInit(I2Cx_Reg_TypeDef *pI2Cx) {

}


void I2C_Transmit(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint32_t length, uint8_t address) {




}


void I2C_Receive(I2C_Handle_t *pToI2CHandle, uint8_t *data, uint8_t address) {


}


