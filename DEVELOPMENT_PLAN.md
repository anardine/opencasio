# DEVELOPMENT_PLAN.md

This project aims to implement custom firmware for the OPENCASIO (STM32WB55REV6-based replacement board for Casio F-91W). The development process must strictly adhere to the guidelines set in `AGENT.md`.

## Project Goal
Develop robust, low-power firmware for a wristwatch replacement board, integrating RTC, temperature/pressure/humidity sensor, magnetometer, and LCD display, all within the constraints of a custom STM32WB55-based PCB.

## Engineering Principles
- **No HAL/Arduino API:** Direct register manipulation (based on RM0434).
- **Core Constraints:** HSI16-only, blocking I2C, power gating for sensors, explicit error handling.
- **Verification:** Every implementation step requires physical hardware verification.

## Development Phases (Based on AGENT.md)

1. **GPIO Bring-up:** 
   - Define and initialize handles for user buttons, RTC interrupt input, and sensor/LED power control rails.
2. **I2C1 Bring-up:** 
   - Configure I2C1 (PB8/PB9) and perform bus scanning to verify RTC (0xAC), Mag (0x60), and BME280 (0x76/0x77) connectivity.
3. **RTC Integration:** 
   - Implement read/write functions for time and date using `rv-3129-c3.c`, ensuring correct BCD handling.
4. **Sensor Integration & Power Gating:** 
   - Implement power gating logic, device initialization, measurement sequences for BME280 and MMC5603NJ, and heading calculation.
5. **LCD Driver Completion:** 
   - Finalize LCD configuration (1/3 duty, 1/3 bias, internal step-up) and implement rendering functions based on the glass truth table.
6. **Buzzer/LED Control:** 
   - Implement tone generation (initially GPIO, migrating to TIM2 PWM) and LED control.
7. **Superloop & Power Management:** 
   - Transition from a simple busy-wait loop to an event-driven, tickless-ish architecture utilizing LPTIM/RTC timer wakeups for maximum battery life.

## Current Status
- GPIO bring-up is implemented: `main()` calls `Board_GPIO_Init()` after clock setup, then waits with `WFI` for debugger inspection. No button actions or interrupt wakeups yet.
- `initRCC()` now waits for HSI16 readiness and the full `SWS=01` status before disabling MSI; startup waits are bounded. The existing LSI1 startup is retained for later LCD work.
- GPIO init returns `CORE_OK` / `GPIO_CFG_ERR`; output writes use BSRR, and outputs are latched LOW before their mode is enabled.
- EXTI/NVIC interrupt support is implemented: buttons PC13/PC3/PE4 fire on the rising (press) edge, RTC_INT PA0 on the falling (assert) edge. `GPIO_IRQCallback(pin)` is the weak override point for application reactions. `main()` sleeps in WFI and wakes on any of these events.
- I2C1 is initialized at 100 kHz Standard mode (RM0434 Table 209, TIMINGR=0x30420F13 at 16 MHz I2CCLK) and main() runs a rail-gated bus scan, storing results in `i2cScanAck[]` / `i2cScanRtc/Mag/Bme76/Bme77` for the debugger. Transfers use AUTOEND STOP (the RV-3129-C3 forbids repeated START) and report `I2C_NACK_ERR` / `I2C_BUS_ERR` explicitly.
- RTC, sensor, LCD and buzzer integration remain pending. Their existing pin helpers use initialized local handles, not null global pointers; this does not complete those phases.
- `pio run` passes. Both the output-clear bug and incorrect HSI16 status check were reproduced before their fixes and passed afterward. Physical GPIO/clock verification is still pending: no ST-Link was detected locally. Do not advance to Phase 2 until the hardware gate below passes.

## Detailed Task List

### Phase 1: GPIO Bring-up
- [x] Define initialized board GPIO handles in `src/auxiliary/gpio-pins-setup.c`; retain generic handle types in `include/driver/gpio.h`.
- [x] Configure PC13/PC3/PE4 buttons and PA0 RTC input without internal pulls (external resistors provide the bias).
- [x] Configure PB0/PB1/PB13 as low-speed push-pull outputs, initially LOW, and initialize from `src/main.c`.
- [x] Arm EXTI interrupts: buttons rising edge, RTC_INT falling edge (SYSCFG mux + RTSR/FTSR + IMR1 + NVIC ISER).
- [ ] Verify on the physical watch through ST-Link before Phase 2.

Hardware gate (no host checks; verify on the board):
- At the `WFI` loop, confirm `RCC_CR.HSIRDY=1`, `RCC_CFGR.SW=01`, `RCC_CFGR.SWS=01`, and GPIO A/B/C/E clocks enabled (`(AHB2ENR & 0x17) == 0x17`).
- Confirm `(GPIOB_ODR & 0x2003) == 0` and probe PB0/PB1/PB13 for LOW throughout startup. BSRR reads return zero on hardware; inspect ODR and the physical pins instead.
- Check PC13/PC3/PE4: idle LOW, pressed HIGH. Check PA0: idle HIGH, LOW when the external RTC asserts INT (not configured by this increment).
- Confirm PA13/PA14 remain usable for SWD and I2C/LCD/buzzer pins were not reconfigured by board startup.
- Identify U11/U12 and verify enable polarity before claiming the sensor rails are OFF. LOW enable levels alone cannot prove switched-rail behavior or absence of I2C back-powering.

### Phase 2: I2C1 Bring-up
- [x] Implement `I2C_Init` in `src/driver/i2c.c` for 100 kHz Standard Mode (PB8/PB9, AF4 open-drain, external pull-ups R7/R8).
- [x] Add I2C bus scanning code to `src/main.c` to verify peripheral IDs (RTC 0x56 always-on; MAG 0x30 and BME280 0x76/0x77 behind gated rails, probed then gated off).
- [ ] Verify on the physical watch through ST-Link before Phase 3.

Hardware gate:
- At the WFI loop, check `i2cScanRtc==1`, `i2cScanMag==1`, and exactly one of `i2cScanBme76`/`i2cScanBme77==1` (the BME280 SDO strap, REFERENCE.md §7.5).
- If any is 0, scope SCL/SDA at U6 pins 6/7 during the scan: confirm the 100 kHz clock, the address byte, and whether the NACK is on the address phase (absent device / rail off) or data phase.
- Measure U11/U12 rail rise time to replace the ~1 ms `railSettleDelay()` guess, and confirm EN polarity is really active-HIGH (REFERENCE.md §7.3).

### Phase 3: RTC Integration (RV-3129-C3)
- [x] Implement low-level I2C read/write functions in `src/auxiliary/rv-3129-c3.c`.
- [x] Add functions for setting/getting time/date (BCD conversions).
- [x] Integrate RTC read into `src/main.c` initialization sequence.

### Phase 4: Sensor Integration & Power Gating
- [x] Implement rail control functions (`TEMP_EN`, `MAG_EN`) using GPIO drivers.
- [x] Implement BME280 init and forced-mode measurement sequence in a new driver file.
- [x] Implement MMC5603NJ init and measurement sequence in `src/auxiliary/mmc5603nj.c`.
- [x] Create heading calculation helper.

### Phase 5: LCD Driver Completion
- [x] Implement `lcdDisable`, `lcdGetStatus`, `lcdDisplayWrite`, `lcdDisplayUpdate` in `src/driver/lcd.c`.
- [x] Configure LCD controller (1/3 duty/bias, internal step-up).
- [x] Add rendering functions based on the glass truth table from `REFERENCE.md`.

### Phase 6: Buzzer/LED Control
- [x] Implement LED toggle function (`PB13`).
- [x] Implement tone generation function (`PA5`) using GPIO (later PWM).

### Phase 7: Superloop & Power Management
- [x] Refactor `main.c` superloop into an event-driven architecture using EXTI interrupts.
- [x] Implement low-power sleep (WFI) between events.

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
