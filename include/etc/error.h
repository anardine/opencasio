//
// Created by Alessandro Nardinelli on 27/11/25.
//

#ifndef OPENCAS_ERROR_H
#define OPENCAS_ERROR_H

enum CORE_ERROR_CODE {
      RCC_CFG_ERR             = 1, // threw under the condition of failure in enabling the clock of the STM
      GPIO_CFG_ERR            = 2, // threw under the condition of a GPIO init failure
      I2C_CFG_ERR             = 3, // threw under the condition of a I2C init failure
};

enum PER_ERROR_CODE {
      RTC_CFG_ERR             = 1, // threw under an init failure condition
      RTC_ALARM_CFG_ERR       = 2, // threw under a failure to configure the alarm
      RTC_TIMER_CFG_ERR       = 3, // threw under a failure to configure the timer
      MAG_INIT_CFG_ERR        = 4 // threw under a failure to configure the mag
};

#endif //OPENCAS_ERROR_H