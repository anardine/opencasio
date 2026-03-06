//
// Created by Alessandro Nardinelli on 14/01/26.
//

#include "driver/lcd.h"


uint8_t LCD_Init(LCD_Handler_t *pToLCDHandler) {

//Configure the LCD clock source and frequency. Lower frequency prefered given lower power consumption

      //enable LCD clock using LSI1
      enableRCC(LCD_PER);

      //set the source voltage for the LCD from VLCD (on pin PB2)
      pToLCDHandler->pToLCD->cr |= (1 << 1);

      // define the framerate of the LCD to around 30.12 Hz for low power consumption
      pToLCDHandler->pToLCD->fcr |= (3 << 22); // PS
      pToLCDHandler->pToLCD->fcr |= (1 << 18); // DIV
      pToLCDHandler->pToLCD->cr |= (4 << 2);  //set duty cycle to 1/8

      //enable the LCD in LCD Control Register
      pToLCDHandler->pToLCD->cr |= (1 << 0);

      return 0;
}
