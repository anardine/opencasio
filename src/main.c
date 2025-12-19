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

      //GPIOs

      //main loop

      while (1)
      {

      }

}