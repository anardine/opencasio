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
- `initRCC()` is implemented.
- Base driver skeletons for GPIO, I2C, and LCD exist but require integration and completion.
- `main.c` is the entry point for initialization and superloop.

## Detailed Task List

### Phase 1: GPIO Bring-up
- [ ] Define GPIO handles in `include/driver/gpio.h` and `src/driver/gpio.c`.
- [ ] Implement `GPIO_Init` function to configure input buttons and power control rails.
- [ ] Initialize GPIOs in `src/main.c`.

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
