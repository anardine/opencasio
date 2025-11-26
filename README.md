<img width="1140" height="517" alt="image" src="https://github.com/user-attachments/assets/1cbba678-d872-486f-8f7a-6ca3b4069be7" />

# OPENCAS - New Features for the Classic Casio F-91W

> [!IMPORTANT]
> This page contains only the software implementation. For hardware details, BOOM, gerber and assembly files please refer to: https://oshwlab.com/anardine.ef/opencas-new-gen-casio-f-91w

## What is this?
The OPENCAS is a replacement board for the Casio F-91W, one of the most used wristwatches in the world.

The board integrates a better, sealed RTC module (where the time drift is substantially smaller), as well as a temperature, pressure, and humidity sensor together with a gyro module with a stepcounter, so you can use to track the steps through the day, week, and months.

All of the native features are untouched, such as alarm, 12/24 time adjustments, and the stopwatch.

The visor LED also got upgraded and is now present in both sides of the clock, which helps during nighttime date/hour checking.

## Architecture

The OPENCAS runs on a STM32WB55REV6 microcontroller. Despite this microcontroller having BLE/WIFI capabilities, this is not used as of this verison. Instead, I focused on guaranteeing that the watch would be controlled as much as possible with low current consumption so that the battery could last a long time.

Since there's not much space, the internal oscillator has been used instead of an external one (HSI). The RTC itself has it's own for the LSE.

This microcontroller is also LCD capable, so all LCD drivers are directly mapped to the alternative LCD functions on most microcontroller GPIO ports.

The following diagram below gives an overview of all the features and how they relate to the microcontroller themselves:


```
                                                                  
                            ┌──────────┐                          
 ┌───────────┐              │  BUZZER  │            ┌───────────┐ 
 │           │   I2C        └────▲─────┘            │           │ 
 │   RTC     ◄──────┐            │            ┌─────►   BUTTONS │ 
 │           │      │    ┌───────┼────────┐   │     │           │ 
 └───────────┘      │    │                │   │     └───────────┘ 
                    └────►                │◄──┘                   
                         │                │                       
 ┌───────────┐           │                │         ┌───────────┐ 
 │           │   SPI     │ STM32WB55REV6  │  ┌───┐  │           │ 
 │  GYRO     ◄───────────►                ├─►│FET├──►    LED    │ 
 │           │           │                │  └───┘  │           │ 
 └───────────┘           │                │         └───────────┘ 
                         │                │                       
                    ┌────►                │                       
 ┌───────────┐      │    │                ├────┐    ┌───────────┐ 
 │   TEMP    │      │    └───────▲┌───────┘    │    │           │ 
 │  SENSOR   ◄──────┘            ││            └────►   LCD     │ 
 │           │   I2C       ┌─────┼▼─────┐           │           │ 
 └───────────┘             │            │           └───────────┘ 
                           │  ST-LINK   │                         
                           │            │                         
                           └────────────┘                         
                                                                  
```

### Firmware

The code runs using `platformio` and the `ST-Link` interface for uploading and debugging.

As of now, the code is still under development.


### How to Contribute

This is a tough project to assembly by hand. All the parts were sourced and assembled using JLCPCB amazing factory. Since it's around \$40 per board (completely ready and assembled, around \$200 for five), feel free to order to split. You're welcome to develop this using bigger parts or take the challenge to solder them. Since this is a one-sided board, you can also use a solder plate.
