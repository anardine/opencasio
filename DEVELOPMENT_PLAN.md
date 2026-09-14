# DEVELOPMENT_PLAN.md

This project aims to implement custom firmware for the OPENCASIO (STM32WB55REV6-based replacement board for Casio F-91W). The development process must strictly adhere to the guidelines set in `AGENT.md`.

## Project Goal
Develop robust, low-power firmware for a wristwatch replacement board, integrating RTC, temperature/pressure/humidity sensor, magnetometer, and LCD display, all within the constraints of a custom STM32WB55-based PCB.

## Engineering Principles
- **No HAL/Arduino API:** Direct register manipulation (based on RM0434).
- **Core Constraints:** HSI16-only, blocking I2C, explicit errors, and sensor rails held on until the shared-bus clamp is redesigned.
- **Verification:** Every implementation step requires physical hardware verification.

## Development Phases

1. **GPIO bring-up — implemented:** board outputs, buttons, RTC interrupt, and EXTI are configured; software-triggered EXTI paths work on target. Physical in-case button checks remain.
2. **I2C1 bring-up — partially accepted:** RTC 0x56 and BME280 0x76 ACK. The MMC5603NJ at 0x30 is absent from the final full-address scan.
3. **RTC integration — implemented and verified:** binary/BCD time and date, PON cold-start handling, alarm, and recurring 1 Hz timer interrupts.
4. **Sensor integration — BME accepted, MAG blocked:** BME280 measurements are plausible; the magnetometer driver is implemented but cannot pass presence detection on the connected board.
5. **LCD driver — implemented and physically verified:** F-91W glass mapping and indicators match the deterministic target pattern with no unknown RAM cells.
6. **Buzzer/LED — implemented:** GPIO output paths run; final brightness, audibility, and case fit remain standalone checks.
7. **Superloop/UI — implemented and target-verified:** EXTI-driven modes, edit flows, alarm state, stopwatch, and countdown behavior operate through the RTC tick engine.
8. **Standalone validation — pending:** execute the 88-scenario matrix after the magnetometer preflight failure is resolved or explicitly accepted.

## Current Status (2026-09-13 connected preflight)

### Final image

- `.pio/build/nucleo_wb55rg_p/firmware.elf` builds successfully: 18,708 bytes flash and 712 bytes RAM.
- The connected V2J17S4 ST-Link cannot use PlatformIO's default modern transport. The same ELF was programmed through OpenOCD `interface/stlink-hla.cfg`; flash verification returned `Verified OK`, followed by target reset.
- `tests/opencasio_test_script.csv` contains 88 unique six-column scenarios. PREFLIGHT-01 and PREFLIGHT-02 are PASS. PREFLIGHT-03 is FAIL because U13 does not ACK.

### Accepted on physical hardware

- F-91W LCD segment remap, PC10/11/12 SEG42/41/40 selection, indicators, colon, and stable RAM-to-glass decoding.
- RV-3129-C3 read/write, `Control_Status.PON` cold-start detection, exact recurring 1 Hz timer sequence, TIME-SET blink, and weekday/day persistence.
- ALARM-SET entry, hour/minute editing, save-and-arm, BELL truthfulness, and disarm/AIE clearing.
- Countdown pause/resume without duration reload and continued countdown outside the timer screen.
- Stopwatch progression outside the stopwatch screen.
- BME280 initialization, forced measurement, and complete display values. Latest debugger sample: 20.43 °C, 93881.6 Pa, 49.52 %RH; displayed pressure rounds to all four hPa digits.
- Sensor enable GPIOs: PB0/PB1 are outputs and read HIGH in both IDR and ODR while the shared bus is active.

### Blocking hardware finding

- A complete scan of 0x01-0x7F finds only 0x56 and 0x76. MMC5603NJ address 0x30 does not ACK; product-ID register 0x39 cannot be read.
- Both tested MAG_EN polarities produced the same NACK. PB1 drive is present at the MCU, so the next check is voltage at U13.B1, followed by U12 output, continuity, assembly orientation, and shorts.
- Compass display and heading correctness are therefore **not accepted**. Do not mark PREFLIGHT-03 PASS until product ID 0x10 and a plausible changing heading are observed.

### Remaining standalone checks

- Physical button contacts and case alignment.
- LED brightness and buzzer audibility in the assembled watch.
- Full battery-removal/PON boot flow, RTC rollover, real alarm firing, and the complete 88-scenario CSV run.
- Long-duration clock accuracy, sleep/operating current, and battery-life measurement.
- Any future sensor power-gating optimization requires bus isolation; unpowered sensors currently clamp shared SCL/SDA.

## Detailed Task List

### Phase 1: GPIO Bring-up
- [x] Configure PC13/PC3/PE4 buttons, PA0 RTC interrupt, PB0/PB1 sensor enables, PB13 LED, and PA5 buzzer.
- [x] Configure EXTI polarity and NVIC delivery; exercise each UI event path on target through EXTI software injection.
- [x] Confirm PB0/PB1 mode and HIGH output/readback during connected preflight.
- [ ] Verify idle/pressed electrical levels and reliable operation with the physical watch buttons installed.
- [ ] Verify LED and buzzer output quality in the case.

### Phase 2: I2C1 Bring-up
- [x] Configure I2C1 on PB8/PB9 at 100 kHz, AF4 open-drain, using external R7/R8 pull-ups.
- [x] Confirm RTC at 0x56 and BME280 at 0x76.
- [x] Confirm no alternate magnetometer address exists with a full valid 7-bit scan.
- [ ] Restore MMC5603NJ presence at 0x30 and read product ID 0x10.
- [ ] Capture SCL/SDA at the correct PB8/PB9 probe points if rail/assembly inspection does not resolve the NACK.

### Phase 3: RTC Integration
- [x] Implement STOP-separated register reads and writes, BCD conversion, and time/date APIs.
- [x] Detect full RTC power loss with `Control_Status.PON` bit 5 and enter TIME-SET with defined defaults.
- [x] Implement legal RV-3129 timer sequencing using 32 Hz/count 31 for recurring one-second ticks.
- [x] Verify RTC ticks, field blinking, time/date save, alarm configuration, and interrupt clearing on target.
- [ ] Run battery-removal, minute rollover, long-duration accuracy, and real alarm-fire scenarios standalone.

### Phase 4: Sensors and Shared-Bus Power
- [x] Implement active-HIGH TEMP_EN/MAG_EN control and conservative rail settling.
- [x] Keep both rails powered during I2C access because unpowered devices clamp the shared bus.
- [x] Implement and verify BME280 reset, calibration, forced conversion, temperature, Q24.8 pressure, and Bosch humidity compensation.
- [x] Implement MMC5603NJ CTRL0 shadowing, set/reset flow, measurement polling, and heading calculation.
- [ ] Electrically diagnose U12/U13 and verify MMC5603NJ product ID, measurement data, and heading changes.
- [ ] Measure current draw; redesign bus isolation before claiming idle sensor power-gating.

### Phase 5: LCD Driver
- [x] Configure the controller for 1/3 duty, 1/3 bias, and internal step-up.
- [x] Map all 24 physical glass segment lines through `SegLineRemap` and handle both LCD RAM words per COM.
- [x] Resolve PC10/PC11/PC12 as SEG40/41/42 for this package/duty configuration.
- [x] Physically verify `SU 12 12:34:56`, colon, and every indicator; stable decoder reports only known cells.

### Phase 6: Buzzer and LED
- [x] Implement PB13 LED control and PA5 GPIO square-wave beep generation.
- [ ] Confirm brightness/audibility in the assembled watch and identify U1 before deciding whether TIM2 PWM is needed.

### Phase 7: Superloop and UI
- [x] Sleep in `WFI` and dispatch button/RTC events from flags set by EXTI callbacks.
- [x] Implement TIME, TIME-SET, ALARM, ALARM-SET, stopwatch, countdown, MAG, and BME modes.
- [x] Verify date-field blinking/save, alarm editing/arming, countdown pause/resume, and off-screen stopwatch/countdown progression on target.
- [x] Render signed rounded temperature and four pressure digits on the BME screen.
- [ ] Run every user-visible path using physical buttons after installation.

### Phase 8: Release Validation
- [x] Build, program, verify, and reset the final connected image.
- [x] Record deterministic LCD and peripheral preflight results in the CSV.
- [ ] Resolve or explicitly accept the magnetometer hardware failure.
- [ ] Execute and record all 88 disconnected scenarios.
- [ ] Measure battery current and evaluate long-term RTC accuracy.

## Simulator (Renode)

A local simulation of the firmware runs under Renode 1.16.1, installed in
`simulation/tool/`. Platform description, peripheral models, and run script
live in `simulation/model/` and `simulation/run.resc`.

Run:
```
simulation/tool/Renode.app/Contents/MacOS/renode --disable-xwt --console simulation/run.resc
```

What works:
- Platform boots: Cortex-M4, RCC (Python model returning HSIRDY/SWS/LSI1RDY),
  GPIO A-E, SYSCFG, EXTI, NVIC, I2C1, flash, SRAM.
- Firmware ELF loads and executes: `initRCC()`, `Board_GPIO_Init()`, EXTI
  setup, and the bus-scan sequence all run.
- Three I2C sensor stubs (RTC 0x56, MAG 0x30, BME280 0x76) are registered
  on the I2C bus and ACK their addresses.

What doesn't work yet:
- The `STM32F7_I2C` model (I2C v2) does not drive the polled status flags
  (`TXIS`, `NACKF`, `STOPF`) our driver waits for. The firmware hangs on
  the first `I2C_Transmit` and all scan results read `0x00`. This is a
  model-fidelity gap, not a firmware bug — the same driver is expected to
  work on real hardware.
- Fixing this requires either a custom I2C controller model in C# that
  sets the ISR flags on address/data phases, or switching to Renode's
  robot-test framework with scripted peripheral responses. Parked for now.

Files:
- `simulation/model/opencasio.repl` — platform description (peripheral map)
- `simulation/model/opencasio_rcc.py` — RCC PythonPeripheral (ready flags)
- `simulation/model/RegisterFileI2CSlave.cs` — generic I2C register-file slave
- `simulation/run.resc` — Renode script: loads platform, ELF, prints scan
