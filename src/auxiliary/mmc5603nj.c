//
// Created by Alessandro Nardinelli on 12/01/26.
//

#include "auxiliary/mmc5603nj.h"

uint8_t readFromMag(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length) {

      //Any serial communication with the MMC5603NJ starts with a “START condition” and terminates with the “STOP condition”
      I2C_Transmit(pToI2CHandle, data, memAddr, 0, MAG_ADDR); // transmits the memAddr first,
      I2C_Receive(pToI2CHandle, data, length, MAG_ADDR); // read the details from the memAddr sent beforehand

      return 0;
}
uint8_t writeToMag(I2C_Handle_t *pToI2CHandle, uint8_t memAddr, uint8_t *data, uint8_t length) {

      I2C_Transmit(pToI2CHandle, data, memAddr, length, MAG_ADDR); // transmit the data

      return 0;
}


//Init the mag sensor. The sensor is initialized with the default mode of just doing a SET/RESET and sampling by request.
//For a continuous sampling rate of 25Hz with a SET/RESET every 25Hz, set MAG_CONTINUOUS_MODE on stm32wb55xx.h.
uint8_t magInit(I2C_Handle_t *pToI2CHandle) {
#if MAG_CONTINUOUS_MODE
      // Enable automatic degaussing with SET/RESET.
      uint8_t controlReg0 = 0;
      controlReg0 |= (1 << 5); //Automatic SET/RESET
      controlReg0 |= (1 << 7); //This bit should be set before continuous-mode measurements are started.

      if (writeToMag(pToI2CHandle, 0x1B, &controlReg0, 1) != 0) {
            return 1;  // I2C write error
      }

      // set ODR
      uint8_t measurementFreq = 25; //25Hz if BW is 00
      if (writeToMag(pToI2CHandle, 0x1A, &measurementFreq, 1) != 0) {
            return 1;  // I2C write error
      }

      // set the mag to continuous mode. 0x1D is the addr of controlReg2
      uint8_t controlReg2 = 0;
      controlReg2 |= (1 << 0); //automatic SET/RESET every 25 measures
      controlReg2 |= (1 << 4); //The device will enter continuous mode
      if (writeToMag(pToI2CHandle, 0x1D, &controlReg2, 1) != 0) {
            return 1;  // I2C write error
      }
#else

      magCalibrate(pToI2CHandle);

#endif
      return 0;  // Success
}

//Uses the Mag SET/RESET operation to remove any residual magnetic field that could have resulted in mag decalibration.
uint8_t magCalibrate(I2C_Handle_t *pToI2CHandle) {
      uint8_t controlReg0 = 0;

      //SET command
      controlReg0 |= (1 << 3); //SET
      if (writeToMag(pToI2CHandle, 0x1B, &controlReg0, 1) != 0) {
            return 1;  // SET calibration error
      }
      controlReg0 = 0;
      //RESET command
      controlReg0 |= (1 << 4); //RESET
      if (writeToMag(pToI2CHandle, 0x1B, &controlReg0, 1) != 0) {
            return 1;  // RESET calibration error
      }
      return 0;
}


uint8_t magGetStatus(I2C_Handle_t *pToI2CHandle) {
      return 0;
}

//Gets the raw data from the sensor
uint8_t magGetData(I2C_Handle_t *pToI2CHandle, uint8_t *magRawDataArray) {

      uint8_t controlReg0;
      uint8_t temp;

      readFromMag(pToI2CHandle, 0x1B, &controlReg0, 1);
      controlReg0 |= (1 << 0) | (1 << 5);
      writeToMag(pToI2CHandle, 0x1B, &controlReg0, 1);

       uint8_t statusCheck = 0;

      while (!((statusCheck >> 1) & 0x01)) {
            readFromMag(pToI2CHandle, 0x18, &statusCheck, 1);
      }

      readFromMag(pToI2CHandle, 0x00, magRawDataArray, 10); // stores the data at the raw data array for posterior use.

      return 0;
}


uint8_t magTranformToHeadings(uint8_t *magRawDataArray, uint8_t *magHeadings) {
      return 0;
}


uint8_t displayHeadings(I2C_Handle_t *pToI2CHandle, uint8_t *magHeadings) {
      return 0;
}

uint8_t magReset(I2C_Handle_t *pToI2CHandle) {

      // Reset the device.
      uint8_t controlReg1 = 0;
      controlReg1 |= (1 << 7);
      if (writeToMag(pToI2CHandle, 0x1C, &controlReg1, 1) != 0) {
            return 1;  // Communication error
      }

      return 0;
}





