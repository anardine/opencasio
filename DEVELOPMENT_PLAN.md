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
- I2C, RTC, sensor, LCD and buzzer integration remain pending. Their existing pin helpers use initialized local handles, not null global pointers; this does not complete those phases.
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
- [ ] Implement `I2C_Init` in `src/driver/i2c.c` for 100 kHz Standard Mode (PB8/PB9).
- [ ] Add I2C bus scanning code to `src/main.c` to verify peripheral IDs.

### Phase 3: RTC Integration (RV-3129-C3)
- [ ] Implement low-level I2C read/write functions in `src/auxiliary/rv-3129-c3.c`.
- [ ] Add functions for setting/getting time/date (BCD conversions).
- [ ] Integrate RTC read into `src/main.c` initialization sequence.

### Phase 4: Sensor Integration & Power Gating
- [ ] Implement rail control functions (`TEMP_EN`, `MAG_EN`) using GPIO drivers.
- [ ] Implement BME280 init and forced-mode measurement sequence in a new driver file.
- [ ] Implement MMC5603NJ init and measurement sequence in `src/auxiliary/mmc5603nj.c`.
- [ ] Create heading calculation helper.

### Phase 5: LCD Driver Completion
- [ ] Implement `lcdDisable`, `lcdGetStatus`, `lcdDisplayLow`, `lcdDisplayHigh` in `src/driver/lcd.c`.
- [ ] Configure LCD controller (1/3 duty/bias, internal step-up).
- [ ] Add rendering functions based on the glass truth table from `REFERENCE.md`.

### Phase 6: Buzzer/LED Control
- [ ] Implement LED toggle function (`PB13`).
- [ ] Implement tone generation function (`PA5`) using GPIO (later PWM).

### Phase 7: Superloop & Power Management
- [ ] Refactor `main.c` superloop into an event-driven architecture using LPTIM/RTC interrupts.
- [ ] Implement low-power sleep modes between events.
