# DEVELOPMENT_PLAN.md

This project aims to implement custom firmware for the OPENCASIO (STM32WB55REV6-based replacement board for Casio F-91W). The development process must strictly adhere to the guidelines set in `AGENT.md` if an AI is being used.

## Project Goal
Develop robust, low-power firmware for a wristwatch replacement board, integrating RTC, temperature/pressure/humidity sensor, magnetometer, and LCD display, all within the constraints of a custom STM32WB55-based PCB.

## Engineering Principles
- **No HAL/Arduino API:** Direct register manipulation (based on RM0434).
- **Core Constraints:** HSI16-only, blocking I2C, explicit errors, and sensor rails held on until the shared-bus clamp is redesigned.
- **Verification:** Every implementation step requires physical hardware verification.

## Development Phases

1. **GPIO bring-up — implemented:** board outputs, buttons, RTC interrupt, and EXTI are configured. The connected UI check exercised physical mode changes successfully with the debounced button path; complete in-case mechanical checks remain.
2. **I2C1 bring-up — partially accepted:** RTC 0x56 and BME280 0x76 ACK. The MMC5603NJ at 0x30 is absent from the final full-address scan.
3. **RTC integration — implemented and verified:** binary/BCD time and date, PON cold-start handling, alarm, and recurring 1 Hz timer interrupts.
4. **Sensor integration — BME accepted, MAG blocked:** BME280 measurements are plausible; the magnetometer driver is implemented but cannot pass presence detection on the connected board.
5. **LCD driver — implemented and verified:** F-91W glass mapping and indicators match the deterministic target pattern with no unknown RAM cells. The complete-frame update fix is built, flashed, and physically verified with the clock, alarm, stopwatch, and countdown displays.
6. **Buzzer/LED — partially accepted:** GPIO output paths and LED press/release behavior run, but long-press edit acceleration still needs correction/verification; final brightness, audibility, and case fit remain standalone checks.
7. **Superloop/UI — core watch modes verified:** the clock, alarm, stopwatch, and countdown are working on the connected board with visible one-second updates. EXTI-driven mode changes, alarm state, pause/resume, and off-screen stopwatch/countdown progression are accepted.
8. **Standalone validation — pending:** execute the 88-scenario matrix after the magnetometer preflight failure is resolved or explicitly accepted.

## Current Status (2026-09-15 connected validation)

### Final image

- `.pio/build/nucleo_wb55rg_p/firmware.elf` builds successfully: 19,980 bytes flash and 712 bytes RAM.
- The connected V2J17S4 ST-Link cannot use PlatformIO's default modern transport. The same ELF was programmed through OpenOCD `interface/stlink-hla.cfg`; flash verification returned `Verified OK`, followed by target reset.
- `tests/opencasio_test_script.csv` contains 88 unique six-column scenarios. PREFLIGHT-01 and PREFLIGHT-02 are PASS. PREFLIGHT-03 is FAIL because U13 does not ACK.

### Connected LCD/UI fix validation (2026-09-15)

- Mode rendering fix applied in `src/driver/lcd.c`, `include/driver/lcd.h`, and `src/main.c`: `lcdDisplayClear()` now clears RAM without requesting an intermediate frame; callers submit the frame after writing its complete contents. LCD initialization still submits its initial clear frame explicitly.
- `displayClock()` now renders date and time into LCD RAM before making one update request. Previously the date update set UDR and write-protected LCD RAM, so the following time writes were discarded.
- Defensive button handling applied in `src/auxiliary/gpio-pins-setup.c` and `src/main.c`: BTN_MODE, BTN_ALARM, and BTN_LED use both-edge EXTI wakeups, approximately 20 ms stable-level debounce, press/release latching, and a short interrupt-critical snapshot so bounce and held buttons do not repeat actions.
- The Saleae capture on D0/SCL and D1/SDA showed the RTC timer flag every second, fully ACKed time/date reads, and advancing seconds. ST-LINK inspection confirmed LCD RAM then changed with the RTC value after the single-frame fix.
- The updated image was built, programmed, verified, and reset through OpenOCD. Physical operation of the clock, alarm, stopwatch, and countdown was accepted.

### Accepted on physical hardware

- F-91W LCD segment remap, PC10/11/12 SEG42/41/40 selection, indicators, colon, and stable RAM-to-glass decoding.
- RV-3129-C3 read/write, `Control_Status.PON` cold-start detection, exact recurring 1 Hz timer sequence, TIME-SET blink, and month/day persistence.
- Clock display refresh with advancing seconds after complete-frame LCD submission.
- ALARM-SET entry, hour/minute editing, save-and-arm, BELL truthfulness, and disarm/AIE clearing.
- Visible alarm, stopwatch, and countdown screens through physical mode operation.
- Countdown pause/resume without duration reload and continued countdown outside the timer screen.
- Stopwatch start/stop/reset and progression outside the stopwatch screen.
- BME280 initialization and forced measurement. The UI now presents one consistent sample across separate temperature, pressure, and humidity screens; hardware verification of the new sequence is pending. Latest debugger sample: 20.43 °C, 93881.6 Pa, 49.52 %RH.
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
- [x] Add both-edge button wakeups, stable-level debounce, and press/release latching in the source.
- [x] Build and verify the debounced physical mode path while exercising clock, alarm, stopwatch, and countdown.
- [ ] Complete dedicated fast-press, slow-press, held-button, and mechanical-bounce stress checks.
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
- [x] Add a MAG reading on entry, ALARM-triggered subsequent refresh, eight-point direction labels, SWD-edge-forward axis mapping, and explicit ~1 µA standby outside MAG.
- [x] Verify MMC5603NJ transactions are issued only when a MAG reading is requested.
- [ ] Physically verify heading orientation after the connected board's MMC5603NJ begins ACKing.
- [ ] Electrically diagnose U12/U13 and verify MMC5603NJ product ID, measurement data, and heading changes.
- [ ] Measure current draw; redesign bus isolation before claiming idle sensor power-gating.

### Phase 5: LCD Driver
- [x] Configure the controller for 1/3 duty, 1/3 bias, and internal step-up.
- [x] Map all 24 physical glass segment lines through `SegLineRemap` and handle both LCD RAM words per COM.
- [x] Resolve PC10/PC11/PC12 as SEG40/41/42 for this package/duty configuration.
- [x] Physically verify `SU 12 12:34:56`, colon, and every indicator; stable decoder reports only known cells.
- [x] Change mode rendering to submit one complete LCD frame after its RAM contents are written.
- [x] Build and physically verify advancing CLOCK plus visible ALARM, STOPWATCH, and TIMER screens after the frame-sequencing fix.

### Phase 6: Buzzer and LED
- [x] Implement PB13 LED control and PA5 GPIO square-wave beep generation.
- [x] Illuminate the LED continuously from the debounced press edge through release in every non-edit mode, while preserving one-shot secondary actions.
- [x] Implement edit-mode LED long-press acceleration after three seconds at approximately five increments per second; in-case verification remains.
- [ ] Confirm brightness/audibility in the assembled watch and identify U1 before deciding whether TIM2 PWM is needed.

### Phase 7: Superloop and UI
- [x] Sleep in `WFI` and dispatch button/RTC events from flags set by EXTI callbacks.
- [x] Implement TIME, TIME-SET, ALARM, ALARM-SET, stopwatch, countdown, separate BME temperature/pressure/humidity screens, and MAG mode.
- [x] Verify date-field blinking/save, alarm editing/arming, countdown pause/resume, and off-screen stopwatch/countdown progression on target.
- [x] Render signed temperature to one decimal place, four-digit hPa pressure, and relative humidity on consecutive BME screens from one measurement.
- [x] Toggle Celsius/Fahrenheit with ALARM and retain the unit across mode changes while powered.
- [ ] Build, flash, and physically verify the new BME screen sequence.
- [x] Verify the updated `TIME -> ALARM -> STOPWATCH -> TIMER` render and button path on physical hardware.
- [ ] Complete the full sequence through all BME screens and MAG after the magnetometer blocker is resolved or accepted.
- [ ] Run every user-visible path using physical buttons after installation.

### Phase 8: Release Validation
- [x] Build, program, verify, and reset the previous connected image.
- [x] Build, program, verify, and reset the image containing the LCD frame and button debounce changes.
- [x] Record deterministic LCD and peripheral preflight results in the CSV.
- [ ] Resolve or explicitly accept the magnetometer hardware failure.
- [ ] Execute and record all 88 disconnected scenarios.
- [ ] Measure battery current and evaluate long-term RTC accuracy.

### Phase 9: Bug bashing
- [x] Fix the date display ordering and unknown weekday characters; render weekday plus day-of-month and validate the weekday during RTC boot and TIME-SET.
- [x] Fix LED hold acceleration; edit fields now repeat at approximately five increments per second after the three-second hold threshold.
- [x] Fix the day-of-month boot default; invalid day, weekday, or month values now fall back to 01/01 with Sunday as the defined default.
- [x] Add in-session hard/soft-iron compass calibration from rotating samples; physical heading verification remains blocked until MMC5603NJ address 0x30 ACKs.
- [x] Keep the BME280 temperature reading uncorrected; remove the watch from the wrist when precise ambient measurement is required.




























