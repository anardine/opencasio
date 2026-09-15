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
   `MAG_EN`) and each sensor must be treated as cold-start-on-enable. On the
   current board, keep both rails enabled while the shared I2C bus is active:
   unpowered sensors clamp SCL/SDA. Do not restore idle rail gating until bus
   isolation is redesigned, and avoid busy-wait delays where a low-power sleep
   would do.
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
    gpio.c               validated GPIO_Init and atomic BSRR writes (toggle remains a later-phase declaration)
    i2c.c                I2C master init/transmit/receive (blocking)
    lcd.c                segment-LCD controller (RM0434 ch. 22)
  auxiliary/             board-level glue + external device drivers
    gpio-pins-setup.c    Board_GPIO_Init + per-function local pin handles
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
  `REFERENCE.md` §3–4. Use fully initialized local handles; the old global
  `pToGPIOx<N>` pointer array and mixed-function pin table have been removed.
- External I2C devices keep their raw-byte address convention:
  `RTC_ADDR = 0xAC`, `MAG_ADDR = 0x60` are pre-shifted write bytes. Keep new
  device drivers consistent and note the convention (see REFERENCE.md §6).
- `GPIO_Init`, `Board_GPIO_Init` and `Btn_GPIO_Init` return `CORE_OK` (0) or
  `GPIO_CFG_ERR`; do not test them as Boolean success. `initRCC` retains its
  1-success / 0-timeout contract. GPIO outputs start LOW on every init.
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

- Final connected image builds at 19,980 bytes flash / 712 bytes RAM. The
  V2J17S4 ST-Link requires OpenOCD HLA (`interface/stlink-hla.cfg`);
  programming and ELF verification pass.
- GPIO, I2C1, LCD, RTC, BME280, LED/buzzer outputs, EXTI superloop, and all
  UI modes are integrated. The firmware sleeps in `WFI` and uses the
  RV-3129-C3 timer as its recurring one-second event source.
- The F-91W glass remap is physically verified with a deterministic pattern.
  Stable halted-target decoding reports that every lit LCD RAM bit maps to a
  known cell. Live decoder captures can race frame updates.
- RTC PON cold-start handling, timer sequencing, TIME-SET/date persistence,
  alarm editing/arming, countdown pause/resume, and off-screen stopwatch and
  countdown progression are verified on target through real superloop events.
- The complete-frame LCD fix is physically accepted: clock seconds advance,
  and the alarm, stopwatch, and countdown screens operate correctly. Saleae
  D0=SCL/D1=SDA captures showed healthy one-second RTC reads; ST-LINK confirmed
  LCD RAM changes after rendering date and time before one UDR request.
- The mode order after countdown is BME temperature, pressure, humidity, then
  MAG. The three BME screens reuse one forced-mode measurement for a consistent
  environmental sample.
- ALARM toggles the temperature screen between Celsius and Fahrenheit. The
  choice persists across mode changes for the current powered session. The
  temperature includes tenths; the glass colon is used as its decimal marker,
  and the degree symbol is omitted.
- LED follows the debounced button level outside edit modes: on while held and
  off on release. ALARM, STW, and TMR retain one-shot secondary actions. In
  edit modes LED increments once per short press, then the current
  implementation repeats on the 1 Hz RTC tick after a hold longer than three
  seconds; the intended approximately 5 Hz behavior remains unverified.
- MAG samples once on entry and again only when ALARM is pressed. Its left two
  positions show the nearest eight-point compass direction; outside MAG the
  device is explicitly returned to its ~1 µA on-demand power-down state.
- BME280 at 0x76 passes initialization and forced measurements. Latest sample:
  20.43 °C, 93881.6 Pa, 49.52 %RH.
- Shared-bus hardware constraint: unpowered sensors clamp SCL/SDA. PB0/PB1
  stay HIGH after rail startup; do not gate either sensor while RTC/I2C access
  remains possible without first redesigning bus isolation.
- **Open blocker:** the current board does not ACK the MMC5603NJ at fixed
  address 0x30. A full valid 7-bit scan finds only RTC 0x56 and BME280 0x76.
  PB1/MAG_EN is configured as an output and reads HIGH in IDR/ODR; both tested
  enable polarities still NACK. Compass behavior is not accepted as verified.
- `tests/opencasio_test_script.csv` is the release matrix: 88 unique scenarios.
  Connected build/upload and LCD preflight pass; peripheral preflight fails on
  the missing magnetometer. Physical in-case and battery-only execution remains.

## 7. Next checks — in order

1. Measure U13.B1 while PB1/MAG_EN is HIGH. Inspect U12 output, U13 supply
   continuity, orientation/assembly, and shorts until address 0x30 ACKs and
   product ID register 0x39 returns 0x10.
2. If the supply/assembly path is sound but U13 still NACKs, capture SCL/SDA at
   the actual PB8/PB9-connected probe points during an address and product-ID
   transaction. Logic analyzer D0 is SCL and D1 is SDA.
3. Re-run MAG init/measurement and confirm changing plausible heading data;
   then change PREFLIGHT-03 to PASS.
4. Install the board and execute all 88 CSV scenarios with physical buttons,
   LED, buzzer, battery removal, real alarm firing, clock rollover, stopwatch,
   countdown, all three BME screens, and MAG.
5. Measure operating/sleep current and long-duration RTC accuracy. Sensor
   power-gating needs a hardware bus-isolation solution before optimization.

Every step ends with hardware evidence; `DEVELOPMENT_PLAN.md` records accepted
and outstanding checks, and `REFERENCE.md` records electrical facts.

## 8. Verification expectations

- Firmware changes compile clean with `pio run` (or the IDE equivalent).
- Behavior claims need hardware evidence (debugger state, SWO/UART trace, or
  measured pin behavior). No "should work" hand-offs.
- When a claim can't be verified without the physical watch board, say so
  explicitly and list exactly what to probe.
