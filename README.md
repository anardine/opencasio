<img width="1150" height="506" alt="Screenshot 2026-01-13 at 13 11 23" src="https://github.com/user-attachments/assets/a15cd5e8-34a2-47d6-af68-6591e4a638c1" />

# OPENCASIO - Mag and Weather on Casio F-91W For Outdoor Use

> [!IMPORTANT]
> This page contains only the software implementation. For hardware details, BOOM, gerber and assembly files please refer to: https://oshwlab.com/anardine.ef/opencasio-casio-outdoor-replacement-board

## What is this?
The OPENCASIO is a replacement board for the Casio F-91W, one of the most used wristwatches in the world.

The board integrates a sealed external RTC, a BME280 temperature/pressure/humidity sensor, and an MMC5603NJ magnetometer intended to provide a compact compass. The current assembled board passes RTC and BME280 checks but does not detect the magnetometer; see the firmware status below.

## What is it for?
OPENCASIO targets outdoor users who want useful environmental and navigation functions in the small F-91W case. The current firmware implements a 24-hour clock/calendar, alarm, stopwatch, countdown timer, upgraded light control, and BME readings. Compass support is implemented in firmware but remains blocked by the missing hardware response; 12-hour display selection is not currently implemented.

## Architecture

OPENCASIO uses an STM32WB55RE microcontroller. Its wireless subsystem is not used by the current firmware; the focus is direct-register control and low power.

The MCU runs from its internal HSI16 oscillator. The sealed RV-3129-C3 supplies independent timekeeping on the same battery rail.

The STM32WB55 includes an LCD controller, and the F-91W glass commons and segment lines are connected to its LCD alternate-function pins.

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
 │           │   I2C     │ STM32WB55REV6  │  ┌───┐  │           │ 
 │   MAG     ◄───────────►                ├─►│FET├──►    LED    │ 
 │           │           │                │  └───┘  │           │ 
 └───────────┘           │                │         └───────────┘ 
                         │                │                       
                    ┌────►                │                       
 ┌───────────┐      │    │                ├────┐    ┌───────────┐ 
 │   T,P,H   │      │    └───────▲┌───────┘    │    │           │ 
 │  SENSOR   ◄──────┘            ││            └────►   LCD     │ 
 │           │   I2C       ┌─────┼▼─────┐           │           │ 
 └───────────┘             │            │           └───────────┘ 
                           │  ST-LINK   │                         
                           │            │                         
                           └────────────┘                         
                                                                  
```

### Firmware

The firmware uses direct STM32WB55 register access; it does not use Arduino or HAL APIs. Build the final image with:

```sh
pio run -e nucleo_wb55rg_p
```

The connected V2J17S4 ST-Link requires deprecated OpenOCD HLA transport. PlatformIO's default upload transport does not support this adapter. Program the built ELF with:

```sh
~/.platformio/packages/tool-openocd/bin/openocd \
  -f interface/stlink-hla.cfg -f target/stm32wbx.cfg \
  -c "program .pio/build/nucleo_wb55rg_p/firmware.elf verify reset exit"
```

Current connected build: 18,708 bytes flash and 712 bytes RAM. OpenOCD programming and flash verification pass.

Implemented and verified on the board:

- Correct F-91W LCD mapping, characters, colon, and indicators.
- RV-3129-C3 clock/date, full-power-loss detection, 1 Hz UI tick, and alarm configuration.
- TIME/TIME-SET, ALARM/ALARM-SET, stopwatch, countdown, MAG, and BME mode flow.
- Alarm editing/arming, countdown pause/resume, and stopwatch/countdown progression while another screen is visible.
- BME280 temperature, pressure, and humidity measurement. Latest connected sample: 20.43 °C, 93881.6 Pa, 49.52 %RH.

> [!WARNING]
> The current board does not ACK the MMC5603NJ at its fixed 7-bit address 0x30. A full scan finds only the RTC at 0x56 and BME280 at 0x76 even while PB1/MAG_EN is driven HIGH. Compass operation is not verified; inspect U12, the switched rail at U13.B1, and U13 assembly before treating the watch as complete.

The release/standalone checklist is `tests/opencasio_test_script.csv`. It contains 88 scenarios for battery boot, LCD, buttons, clock editing, alarm, stopwatch, countdown, sensors, power behavior, and recovery. Connected build/upload and LCD preflight pass; peripheral preflight remains failed on the missing magnetometer response. Detailed evidence and unresolved checks are in `DEVELOPMENT_PLAN.md` and `REFERENCE.md`.

### How to Contribute

This is a tough project to assembly by hand. All the parts were sourced and assembled using JLCPCB amazing factory. Since it's around \$50 per board (completely ready and assembled, around \$250 for five), feel free to order to split. You're welcome to develop this using bigger parts or take the challenge to solder them. Since this is a one-sided board, you can also use a solder plate.
