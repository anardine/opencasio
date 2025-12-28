//
// Created by Alessandro Nardinelli on 20/12/25.
//


#include "auxiliary/rv-3129-c3.h"

uint8_t readFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length) {

      //Any serial communication with the RV-3129-C3 starts with a “START condition” and terminates with the “STOP condition” No restart allowed on comms with the RV-3129. The RV-3129-C3 does not allow a repeated START. Therefore a STOP has to be released before the next START

      I2C_Transmit(pToI2CHandle, data, memAddr, 0, RTC_ADDR);
      I2C_Receive(pToI2CHandle, data, length, RTC_ADDR);

      return 0;
}


uint8_t writeToRTC(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length) {

      I2C_Transmit(pToI2CHandle, data, memAddr, length, RTC_ADDR);

      return 0;
}

uint8_t getTimeFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t *timePointer) {

      readFromRTC(pToI2CHandle,0x08, timePointer,1); //seconds
      readFromRTC(pToI2CHandle,0x09, (timePointer+1),1); //minutes
      readFromRTC(pToI2CHandle,0x0A, (timePointer+2),1); //hours

      return 0;
}

uint8_t setTimeFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t *timePointer) {

      writeToRTC(pToI2CHandle,0x08, timePointer,1); //seconds
      writeToRTC(pToI2CHandle,0x09, (timePointer+1),1); //minutes
      writeToRTC(pToI2CHandle,0x0A, (timePointer+2),1); //hours

      return 0;
}

uint8_t getDateFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t *datePointer) {

      readFromRTC(pToI2CHandle,0x0B, datePointer,1); //days
      readFromRTC(pToI2CHandle,0x0C, (datePointer+1),1); //weekday
      readFromRTC(pToI2CHandle,0x0D, (datePointer+2),1); //Months
      readFromRTC(pToI2CHandle,0x0E, (datePointer+3),1); //Years

      return 0;
}

uint8_t setDateFromRTC(I2C_Handle_t *pToI2CHandle, uint8_t *datePointer) {

      writeToRTC(pToI2CHandle,0x0B, datePointer,1); //days
      writeToRTC(pToI2CHandle,0x0C, (datePointer+1),1); //weekday
      writeToRTC(pToI2CHandle,0x0D, (datePointer+2),1); //Months
      writeToRTC(pToI2CHandle,0x0E, (datePointer+3),1); //Years

      return 0;
}

uint8_t alarmInit(I2C_Handle_t *pToI2CHandle) {

      uint8_t flag = ENABLE;
      uint8_t alarmSet;
      //Enable the Alarm Interrupt in Control_INT.
      writeToRTC(pToI2CHandle,0x01, &flag,1); //write to Control_INT 1 to AIE bit
      readFromRTC(pToI2CHandle,0x01, &alarmSet,1); //read alarm value to check if alarm is enabled

      return alarmSet; //return true if alarm is set init
}

uint8_t alarmClear(I2C_Handle_t *pToI2CHandle) {

      uint8_t flag = DISABLE;

      //Enable the Alarm Interrupt in Control_INT.
      writeToRTC(pToI2CHandle,0x02, &flag,1); //write to Control_INT Flag the AF bit to clear Alarm INT

      return 0;
}

uint8_t alarmSet(I2C_Handle_t *pToI2CHandle) {

      return 0;
}

void displayTime(I2C_Handle_t *pToI2CHandle, uint8_t *timePointer) {

      getTimeFromRTC(pToI2CHandle, timePointer);

      uint8_t seconds = timePointer[0];
      uint8_t minutes = timePointer[1];
      uint8_t hours = timePointer[2];

      //TODO: implement display logic for time

}