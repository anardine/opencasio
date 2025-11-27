//
// Created by Alessandro Nardinelli on 14/11/25.
//

// import section
#include "aux/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"


int main() {

      // set clock to 16MHz
      if (!initRCC()) return RCC_CFG_ERR;             //if clock is unable to be set to HSI, return RCC_CFG_ERR and end application


      // init peripherals section


      //main loop

      while (1)
      {

      }

}