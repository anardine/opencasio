# AGENT.md — OPENCASIO firmware

Guidance for AI agents (and humans) working in this repository. Read together with
`README.md` (product intro) and `REFERENCE.md` (hardware ground truth: pin map,
netlist, LCD wiring, open questions).

## 1. What this is

Firmware for the OPENCASIO: a replacement PCB for the Casio F-91W wristwatch.
STM32WB55RE (Cortex-M4+FPU, QFN68) driving the original watch glass through the
MCU's internal segment-LCD controller, with an external sealed RTC
(RV-3129-C3), a magnetometer (MMC5603NJ), and a BME280 for temperature /
pressure / humidity. Design goal: **months of battery life**, classic-watch UX.

## 2. Non-negotiable engineering decisions

1. **No HAL, no CubeMX-generated code, no Arduino API calls.** All peripheral
   drivers are written directly against registers described in RM0434.
   `platformio.ini` selects `framework = arduino` / `board = nucleo_wb55rg_p`
   purely as a build/upload scaffold (startup files, linker script, stlink);
   the real target is the custom QFN68 board in `REFERENCE.md`. Do not add
   `#include <Arduino.h>` or use framework APIs. If the scaffold gets in the
   way, prefer fixing build flags over importing framework code.
2. **HSI16 only** — no external crystal on the MCU side. `initRCC()` sets
   SYSCLK = 16 MHz from HSI16; LSE exists only inside the RTC module domain
   (the RV-3129-C3 has its own oscillator). Never write code that assumes
   HSE/PLL availability without first checking RM0434 ch. 6.
3. **Blocking, polled I2C.** No DMA, no interrupts on the data path yet.
   Simple > clever until power measurements say otherwise.
4. **Power discipline is a feature.** Sensor rails are switched (`TEMP_EN`,
   `MAG_EN`); code must treat sensor init as cold-start-every-use, gate rails
   off when idle, and avoid busy-wait delays where a low-power sleep would do.
5. **Error codes, not silent failures.** Peripheral init functions return
   `uint8_t`; failure codes live in `include/etc/error.h`
   (`CORE_ERROR_CODE`, `PER_ERROR_CODE`). Extend these enums instead of
   inventing ad-hoc magic numbers.

## 3. Repository layout

```
src/
  main.c                 entry point: init sequence + superloop
  driver/                MCU register-level drivers (one file per peripheral)
    rcc.c                clock tree: initRCC(), enableRCC()/diableRCC()
    gpio.c               GPIO_Init, read/write helpers (toggle declared in gpio.h, not yet implemented)
    i2c.c                I2C master init/transmit/receive (blocking)
    lcd.c                segment-LCD controller (RM0434 ch. 22)
  auxiliary/             board-level glue + external device drivers
    gpio-pins-setup.c    per-function pin setup (LCD/I2C/buzzer/buttons)
    rv-3129-c3.c         external RTC: time/date/alarm/timer over I2C
    mmc5603nj.c          magnetometer: init/calibrate/read → heading
include/                 mirrors src/ one header per .c
  driver/stm32wb55xx.h   hand-written register & struct definitions (CMSIS-lite)
  etc/error.h            error enums shared across drivers
docs/
  rm0434.txt             extracted reference manual (grep it!)
  ds_stm32wb55re.txt     extracted datasheet (pinout tables live here)
  rv3129_ds.txt          RV-3129-C3 datasheet (RTC)
  rv3129_appman.txt      RV-3129-C3 application manual: registers, alarm/timer, I2C protocol
  mmc5603nj_ds.txt       MMC5603NJ Rev.B datasheet: registers, measurement modes
  INDEX.md               chapter → line-offset map for the ST documents
REFERENCE.md             hardware ground truth: nets, pins, LCD map, gotchas
debug/STM32WB55_CM4.svd  SVD for debugger register views
```

## 4. Driver conventions

- Handle structs follow the existing pattern:
  `{ <periph>_RegTypeDef *p<Periph>x; <Periph>_PinConfig_t config; }`
  e.g. `GPIO_Handle_t`, `I2C_Handle_t`. Register blocks are `volatile`
  structs in `stm32wb55xx.h` at absolute base addresses taken from RM0434
  ch. 2 memory map — never magic numbers at callsites.
- Every driver mirrors its RM chapter: if you add a register bit, name it
  after the RM bit name (`<REG>_<BIT>_Pos/_Msk` style is fine).
- Pin setup lives in `auxiliary/gpio-pins-setup.*`, organized by function
  (`LCD_GPIO_Init`, `I2C_GPIO_Init`, …), using the AF table in
  `REFERENCE.md` §3–4. The `pToGPIOx<N>` pointer array there predates the
  handle-based approach; prefer explicit local handles in new code and shrink
  the array when touched.
- External I2C devices keep their raw-byte address convention:
  `RTC_ADDR = 0xAC`, `MAG_ADDR = 0x60` are pre-shifted write bytes. Keep new
  device drivers consistent and note the convention (see REFERENCE.md §6).
- Comments explain *why* against the manual ("RM0434 §22.4.2: …"), not what.

## 5. Documentation rules
- External-device drivers cite their own datasheet (`rv3129_appman.txt`,
  `mmc5603nj_ds.txt`), not RM0434. RTC register pages/BCD formats and mag
  measurement/set-reset sequences come from those files.

- Before writing/changing any register-level code, pull the relevant section
  of `docs/rm0434.txt` (use `docs/INDEX.md` offsets) or the datasheet pinout.
  Do not write peripheral code from memory.
- Hardware facts belong in `REFERENCE.md`. If bring-up reveals a netlist
  discrepancy, update REFERENCE.md in the same change.
- Git: agents must NOT stage/commit/push. The owner handles all git
  operations explicitly.

## 6. Current state

- `initRCC()` works (HSI16 @ 16 MHz); GPIO and I2C drivers exist but are not
  wired into `main()` yet; LCD driver has `LCD_Init` implemented
  (RCC enable, GPIO AF setup via `LCD_GPIO_Init`, VSEL, PS/DIV, LCDEN) but
  `LCD_Blink` is empty and `lcdDisable`/`lcdGetStatus`/`lcdDisplayLow`/
  `lcdDisplayHigh` are declared without definitions. No display rendering, no
  sensor integration, no buttons, no buzzer, no LED control in `main.c`.

## 7. Next phase — integration plan (in order)

1. **GPIO bring-up**: instantiate handles for BTN_MODE/BTN_ALARM/BTN_LED
   inputs (active-HIGH, pulldown), RTC_INT input (active-LOW, external
   pull-up), LED_EN output (PB13), TEMP_EN/MAG_EN outputs (PB0/PB1).
2. **I2C1 bring-up**: PB8/PB9 AF4, 100 kHz standard mode to start; scan bus
   and confirm 0x56 (RTC), 0x30 (mag), 0x76/0x77 (BME280) ACK.
3. **RTC**: read/write time & date via `rv-3129-c3.c`; verify BCD handling;
   wire alarm interrupt to RTC_INT/PA0 later.
4. **Sensors with power gating**: enable rail → delay for rail rise +
   device boot → init → measure → gate off. BME280 forced-mode single shots;
   MMC5603NJ on-demand measurement + `magTransformToHeading()`.
5. **LCD**: configure LCD_GLASS in 1/3 duty / 1/3 bias, internal step-up via
   PB2/LCD_VLCD (VLCD source selection + contrast in LCD_FCR), map glass
   pads per REFERENCE.md §4, then implement digit rendering once the glass
   truth table (REFERENCE.md §7.2) is available.
6. **Buzzer/LED**: PA5 tone generation (start GPIO square wave; move to
   TIM2_CH1 PWM), PB13 LED control.
7. **Superloop design**: event-driven tickless-ish loop around LPTIM/RTC
   timer wakeups; no `while(1){}` busy spin except during debug.

Each step ends with hardware verification via ST-Link before moving on.

## 8. Verification expectations

- Firmware changes compile clean with `pio run` (or the IDE equivalent).
- Behavior claims need hardware evidence (debugger state, SWO/UART trace, or
  measured pin behavior). No "should work" hand-offs.
- When a claim can't be verified without the physical watch board, say so
  explicitly and list exactly what to probe.
