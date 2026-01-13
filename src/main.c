//
// Created by Alessandro Nardinelli on 14/11/25.
//

// import section
#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
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


      // Init peripherals section

      //GPIOs

      //I2C



      //main loop
      while (1)
      {

      }

}