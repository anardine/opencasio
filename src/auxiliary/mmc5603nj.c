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


// uint8_t magGetStatus(I2C_Handle_t *pToI2CHandle) {
//       return 0;
// }

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

//Receives the raw readings from the mag and returns the calculated heading in degree
uint16_t magTransformToHeading(uint8_t *magRawDataArray) {
      
      // MMC5603NJ raw data format (from registers 0x00-0x08):
      // Bytes 0-1: X axis (MSB, LSB)
      // Bytes 2-3: Y axis (MSB, LSB)
      // Bytes 4-5: Z axis (MSB, LSB)
      // Byte 6: X lower 4 bits (bits 4-7, bits 0-3 unused)
      // Byte 7: Y lower 4 bits (bits 4-7, bits 0-3 unused)
      // Byte 8: Z lower 4 bits (bits 4-7, bits 0-3 unused)
      
      // Extract raw 20-bit X, Y, Z values
      // X: 8-bit MSB (byte 0) + 8-bit LSB (byte 1) + 4-bit lower (byte 6 bits 4-7, shifted right by 4)
      int32_t xRaw = ((int32_t)magRawDataArray[0] << 12) | ((int32_t)magRawDataArray[1] << 4) | (((int32_t)magRawDataArray[6] >> 4) & 0x0F);
      
      // Y: 8-bit MSB (byte 2) + 8-bit LSB (byte 3) + 4-bit lower (byte 7 bits 4-7, shifted right by 4)
      int32_t yRaw = ((int32_t)magRawDataArray[2] << 12) | ((int32_t)magRawDataArray[3] << 4) | (((int32_t)magRawDataArray[7] >> 4) & 0x0F);
      
      // Z: 8-bit MSB (byte 4) + 8-bit LSB (byte 5) + 4-bit lower (byte 8 bits 4-7, shifted right by 4)
      int32_t zRaw = ((int32_t)magRawDataArray[4] << 12) | ((int32_t)magRawDataArray[5] << 4) | (((int32_t)magRawDataArray[8] >> 4) & 0x0F);
      
      // Convert to signed values (offset binary encoding, subtract 2^19 to center at 0)
      int32_t xSigned = xRaw - 524288;
      int32_t ySigned = yRaw - 524288;
      
      // Calculate heading angle using arctangent
      // Positive X points in the direction we want to measure (user facing direction)
      // Positive Y points West (90° counter-clockwise from X)
      // We want: 0° = +X direction, 90° = +Y rotated (East), 180° = -X, 270° = -Y
      
      double headingRad = atan2((double)xSigned, (double)ySigned);
      double headingDeg = (headingRad * 180.0) / M_PI;
      
      // Normalize to 0-360 degrees
      if (headingDeg < 0) {
            headingDeg += 360.0;
      }
      
      // Return heading value with 0.1 degree resolution (0-3600 representing 0-360.0 degrees)
      uint16_t headingValue = (uint16_t)(headingDeg * 10.0);
      
      return headingValue;
}


uint8_t displayHeadings(I2C_Handle_t *pToI2CHandle, uint16_t headingValue) {

      //TODO: Implement the display function of the heading


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





