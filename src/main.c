//
// Created by Alessandro Nardinelli on 14/11/25.
//

// import section
#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"
#include "driver/i2c.h"
#include "auxiliary/mmc5603nj.h"
#include "auxiliary/rv-3129-c3.h"


GPIO_Handle_t pToGPIO1;
I2C_Handle_t pToI2C;
magDataArray_t magRawDataArray; //array used to storage the results of the raw magnetic fields measurements
magDataArray_t magHeadings; //array used to store the heading data ready to be served to the LCD

int main() {

      // set clock to 16MHz
      if (!initRCC()) return RCC_CFG_ERR;             //if clock is unable to be set to HSI, return RCC_CFG_ERR and terminate
      

      uint8_t status = Board_GPIO_Init();
      if (status != CORE_OK) return status;

      // GPIO bring-up checkpoint: inspect IDR/ODR through ST-Link.
      // EXTI wake sources (3 buttons rising, RTC_INT falling) are armed by
      // Board_GPIO_Init; WFI returns on each event until Phase 7 adds the
      // event-driven superloop.
      while (1)
      {
            __asm volatile ("wfi");

      }

}