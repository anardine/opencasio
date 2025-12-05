//
// Created by Alessandro Nardinelli on 14/11/25.
//

// import section
#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"


GPIO_Handle_t pToGPIO1;

int main() {

      // set clock to 16MHz
      if (!initRCC()) return RCC_CFG_ERR;             //if clock is unable to be set to HSI, return RCC_CFG_ERR and terminate


      // init peripherals section

      //GPIO
      pToGPIO1.GPIO_PinConfig.GPIO_PinAltFunMode = GPIO_AFH_AF2;



      //main loop

      while (1)
      {

      }

}