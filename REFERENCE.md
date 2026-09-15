# OPENCASIO — Hardware Reference

Ground truth for firmware development. Sources (all extracted under `docs/`):
- Schematic netlist: `docs/schematic.tel` (component packages and board connectivity).
- Datasheet DS11929 Rev 18 (`docs/ds_stm32wb55re.txt`) — pinout table 16.
- Reference manual RM0434 Rev 16 (`docs/rm0434.txt`) — see `docs/INDEX.md` for chapter offsets.
- RV-3129-C3 datasheet + application manual (`docs/rv3129_ds.txt`, `docs/rv3129_appman.txt`).
- MMC5603NJ datasheet Rev.B (`docs/mmc5603nj_ds.txt`).
- BME280 datasheet (`docs/bme280_ds.txt`)

## 1. Bill of materials (functional view)

| Ref | Part | Package | Role |
|-----|------|---------|------|
| U6 | STM32WB55RE (README's "STM32WB55REV6"; R = VFQFPN68 package) | QFN-68 8×8 mm | Main MCU (Cortex-M4 @16 MHz HSI16, LCD controller, I2C) |
| U2 | Micro Crystal RV-3129-C3 | Hermetically sealed ceramic 3.7×2.5×0.9 mm, 10-term (datasheet; footprint `OSC-SMD_10P-L3.7-W2.5-P0.80`) | External sealed RTC, always-on from VBAT |
| U4 | Bosch BME280 | LGA-8 | Temperature / pressure / humidity |
| U13 | MEMSIC MMC5603NJ | FlipChip-4 | 3-axis magnetometer (compass) |
| U1 | Buzzer driver amp | W-QFN2020-12 | Drives the piezo from MCU `BUZZER_DIN`; OUT on pin 6, piezo net `BUZZER` on pin 7 |
| U11 | Load switch (XFBGA-4) | — | Switched power rail for BME280, controlled by `TEMP_EN` |
| U12 | Load switch (XFBGA-4) | — | Switched power rail for MMC5603NJ, controlled by `MAG_EN` |
| Q2 | N-FET SOT-323 | — | Low-side switch for LED3+LED4 (upgraded backlight/light LED) |
| LCD1 | Casio F-91W glass | `f91w-lcd-pad-footprint` | 27 glass pins: 3 commons (COM1–COM3) + 24 segment pads |
| R7, R8 | 4.7 kΩ | 0201 | I2C pull-ups (SCL/SDA) to VBAT |
| R11–R14, R21 | 10 kΩ | 0201 | R11: RTC_INT pull-up; R12–R14: button pulldowns; R21: FET gate pulldown |
| R18–R20 | 47 Ω | 0402 | R18: FET gate series; R19/R20: LED ballast |
| Cext1 | 1 µF | 0201 | VLCD reservoir cap |

## 2. Power architecture

- Single battery rail `VBAT` feeds the MCU, RTC U2, both LED anodes, I2C pull-ups, and both sensor-switch inputs.
- **Shared-bus constraint:** unpowered sensors clamp SCL/SDA despite the 4.7 kΩ pull-ups. Current firmware raises active-HIGH `TEMP_EN` and `MAG_EN` before the boot scan and keeps both rails enabled while any RTC or sensor traffic is possible. Do not restore idle power-gating without bus isolation or a separate pull-up domain.
- If a future hardware revision isolates the bus, each sensor must be initialized after its switched rail rises because every enable is a cold start.
- `VLCD` rail = MCU pin 27 (**PB2**, alternate function `LCD_VLCD`, i.e. the internal LCD step-up output) + Cext1. There is NO dedicated VLCD supply pin on this package; the step-up converter drives VLCD out through PB2. LCD contrast/voltage selection happens inside the LCD peripheral (RM0434 ch. 22).

## 3. MCU pin map (VFQFPN68)

### Digital I/O

| QFN68 pin | Port/pin | Net | Direction / notes |
|-----------|----------|-----|--------------------|
| 2 | PC13 | BTN_LED | Button input, 10 kΩ pulldown to GND → **active-HIGH** (watch button shorts pad toward battery rail). PC13 is also WKUP2. |
| 12 | PC3 | BTN_MODE | Button input, active-HIGH, 10 kΩ pulldown. PC3 also has `LCD_VLCD` AF (not used for that here). |
| 40 | PE4 | BTN_ALARM | Button input, active-HIGH, 10 kΩ pulldown. |
| 15 | PA0 | RTC_INT | Input, RV-3129-C3 interrupt, 10 kΩ pull-up to VBAT → **active-LOW**. |
| 38 | PB0 | TEMP_EN | Active-HIGH output for U11/BME280 rail. Final preflight: HIGH in IDR/ODR and BME280 responds at 0x76. |
| 39 | PB1 | MAG_EN | Active-HIGH output for U12/MMC5603NJ rail. Final preflight: HIGH in IDR/ODR, but U13 does not ACK at 0x30; see §7. |
| 47 | PB13 | LED_EN | Output → R18 47 Ω → Q2 gate (R21 10 kΩ pulldown keeps LED off at reset). HIGH = LED on. |
| 20 | PA5 | BUZZER_DIN | Output to buzzer amp U1. Candidate for TIM2_CH1 PWM (PA5 = TIM2_CH1 AF1) instead of plain GPIO beeps. |

### I2C1 bus (`BME_SCL` / `BME_SDA`, shared by all three slaves)

| QFN68 pin | Port/pin | AF | Net | Slaves on this bus |
|-----------|----------|----|-----|--------------------|
| 6 | PB8 | AF4 I2C1_SCL | BME_SCL | RTC U2 7-bit 0x56 (raw write 0xAC), MMC5603NJ U13 0x30 (raw write 0x60), BME280 U4 0x76 |
| 7 | PB9 | AF4 I2C1_SDA | BME_SDA | (PB9's LCD_COM3 AF is unused — it is I2C SDA here) |

### Debug

| QFN68 pin | Port/pin | Net |
|-----------|----------|-----|
| 54 | PA13 | SWDIO |
| 56 | PA14 | SWCLK |
| 8 | NRST | NRST + C48 (RC on reset line) |

## 4. LCD wiring (glass pad ↔ MCU segment line)

Glass pads named by the netlist `LCD_COM1..3` (glass pads 7–9) and `LCD_SEG*` (glass pads 1–6 and 10–27 → 24 segment nets; pad numbers 7/8/9 are skipped in the segment net naming). MCU SEG-line indices come from datasheet Table 18 (AF11); they do NOT match glass pad numbers.

| Glass pad | QFN68 pin | Port/pin | MCU LCD function |
|-----------|-----------|----------|------------------|
| COM1 | 23 | PA8 | LCD_COM0 |
| COM2 | 24 | PA9 | LCD_COM1 |
| COM3 | 51 | PA10 | LCD_COM2 |
| SEG1 | 17 | PA2 | LCD_SEG1 |
| SEG2 | 16 | PA1 | LCD_SEG0 |
| SEG3 | 11 | PC2 | LCD_SEG20 |
| SEG4 | 10 | PC1 | LCD_SEG19 |
| SEG5 | 9 | PC0 | LCD_SEG18 |
| SEG6 | 18 | PA3 | LCD_SEG2 |
| SEG10 | 19 | PA4 | LCD_SEG5 |
| SEG11 | 21 | PA6 | LCD_SEG3 |
| SEG12 | 22 | PA7 | LCD_SEG4 |
| SEG13 | 25 | PC4 | LCD_SEG22 |
| SEG14 | 26 | PC5 | LCD_SEG23 |
| SEG15 | 28 | PB10 | LCD_SEG10 |
| SEG16 | 29 | PB11 | LCD_SEG11 |
| SEG17 | 67 | PB7 | LCD_SEG21 |
| SEG18 | 66 | PB6 | LCD_SEG6 |
| SEG19 | 65 | PB5 | LCD_SEG9 |
| SEG20 | 64 | PB4 | LCD_SEG8 |
| SEG21 | 63 | PB3 | LCD_SEG7 |
| SEG22 | 60 | PC12 | LCD_SEG42 * |
| SEG23 | 59 | PC11 | LCD_SEG41 * |
| SEG24 | 58 | PC10 | LCD_SEG40 * |
| SEG25 | 57 | PA15 | LCD_SEG17 |
| SEG26 | 50 | PC6 | LCD_SEG24 |
| SEG27 | 46 | PB12 | LCD_SEG12 |
\* PC10/PC11/PC12 carry dual SEG labels in datasheet Table 18. RESOLVED via
RM0434 Table 117 (Remapping capability, VFQFPN68, 1/3 duty, MUX_SEG=0):
the shared pins act as **SEG[42:40]**, so glass pads 22/23/24 = SEG42/41/40.
Notes:
- 3 commons only → glass operates in **1/3 duty, 1/3 bias** mode (RM0434 LCD chapter).
- All AF assignments above verified against datasheet Table 18 (coordinate-extracted); glass pad 13 = PC4 = `LCD_SEG22` is a normal segment line (earlier "GPIO-driven" claim was a parse error).
- Unused MCU LCD-capable pins stay GPIO so the controller doesn't drive phantom segments.
- Datasheet package capability: VFQFPN68 supports up to 4 COM × 28 SEG; we use 3 × 24.

## 4a. Glass pad → digit mapping (LCD bring-up root cause)

The original "random segments" bug: the Sensor-Watch tables use *their*
SAM L22 SLCD segment lines, which are 1:1 with the **glass pad** each net
reaches (SW SEG18-23 = glass pads 1-6, SW SEG17-0 = pads 10-27; SW
COM0-2 = pads 7-9). Writing those indices straight into STM32 LCD_RAM lit
the wrong physical lines. `SegLineRemap[24]` in `src/driver/lcd.c`
translates SW seg index → our STM32 SEG line; every pixel write goes
through it. Derived from: Sensor-Watch's OSO-SWAT-A1-05 board files +
ATSAML22 symbol (glass pad → SW SEG net), pluto-fw's `segmap.map`
(carrotIndustries, same glass on MSP430 — used to cross-check the digit
topology), our ODB++ netlist (glass pad → MCU pin), and RM0434 Table 117
for the PC10/11/12 duty mux. `test/lcd/verify.py` decodes LCD RAM back to
characters to confirm the mapping headlessly.

Connected LCD proof: the deterministic `SU 12 12:34:56` pattern plus SIGNAL,
BELL, PM, 24H, LAP, and colon was visually confirmed on the physical glass.
With the target halted for an atomic capture, `python3 test/lcd/verify.py`
reported that every lit LCD RAM bit maps to a known glass cell. Live captures
can race LCD updates and must not be treated as mapping failures.

## 4b. F-91W glass truth table

Adapted from the [Sensor-Watch](https://github.com/joeycastillo/Sensor-Watch) project (MIT license, © 2020 Joey Castillo), which uses the same Casio F-91W glass. The tables below map each digit position's 7 segments to COM/SEG pairs, and define the 7-segment bit patterns for each ASCII character. Source: `src/driver/lcd.c` (`Segment_Map[]`, `Character_Set[]`).

### Segment map (COM/SEG per digit position)

The F-91W has 10 digit positions. Each position has 7 segments (A-G) mapped to a (COM, SEG) pair. The LCD controller uses 1/3 duty (COM0-COM2). Segments listed as "none" don't exist for that position.

Standard 7-segment layout:
```
   A
 F   B
   G
 E   C
   D
```

| Position | Function | Seg A | Seg B | Seg C | Seg D | Seg E | Seg F | Seg G | Extra |
|----------|----------|-------|-------|-------|-------|-------|-------|-------|-------|
| 0 | Day of week | COM0 SEG13 | COM1 SEG13 | COM2 SEG13 | COM2 SEG15 | COM2 SEG14 | COM0 SEG14 | COM1 SEG15 | COM1 SEG14 |
| 1 | Day of week | COM0 SEG11 | COM1 SEG11 | COM1 SEG11 | COM2 SEG11 | COM1 SEG12 | COM1 SEG12 | COM2 SEG12 | COM0 SEG12 |
| 2 | Day of month | COM1 SEG9 | COM0 SEG9 | COM2 SEG9 | COM1 SEG9 | COM0 SEG10 | none | COM1 SEG9 | none |
| 3 | Day of month | COM0 SEG7 | COM1 SEG7 | COM2 SEG7 | COM2 SEG6 | COM2 SEG8 | COM0 SEG8 | COM1 SEG8 | none |
| 4 | Clock hours | COM1 SEG18 | COM2 SEG19 | COM0 SEG19 | COM1 SEG18 | COM0 SEG18 | COM2 SEG18 | COM1 SEG19 | none |
| 5 | Clock hours | COM2 SEG20 | COM2 SEG21 | COM1 SEG21 | COM0 SEG21 | COM0 SEG20 | COM1 SEG17 | COM1 SEG20 | none |
| 6 | Clock minutes | COM0 SEG22 | COM2 SEG23 | COM0 SEG23 | COM0 SEG22 | COM1 SEG22 | COM2 SEG22 | COM1 SEG23 | none |
| 7 | Clock minutes | COM2 SEG1 | COM2 SEG10 | COM0 SEG1 | COM0 SEG0 | COM1 SEG0 | COM2 SEG0 | COM1 SEG1 | none |
| 8 | Clock seconds | COM2 SEG2 | COM2 SEG3 | COM0 SEG4 | COM0 SEG3 | COM0 SEG2 | COM1 SEG2 | COM1 SEG3 | none |
| 9 | Clock seconds | COM2 SEG4 | COM2 SEG5 | COM1 SEG6 | COM0 SEG6 | COM0 SEG5 | COM1 SEG4 | COM1 SEG5 | none |

Notes:
- Positions 0 and 1 share segments B/C and E/F (the F-91W day-of-week digits are narrow).
- Position 0 has an extra segment (COM1 SEG14) used for characters B, D, and @.
- Position 2 is missing segment F (limited character set).
- Segments A/D are shared in positions 1, 4, and 6 (both halves of the digit share one COM/SEG pair).

### Character set (7-segment bit patterns)

Bits [6:0] correspond to segments G-F-E-D-C-B-A (LSB = segment A). `1` = on, `-` = off.

| Char | A | B | C | D | E | F | G | Notes |
|------|---|---|---|---|---|---|---|-------|
| (space) | - | - | - | - | - | - | - | |
| 0 | 1 | 1 | 1 | 1 | 1 | 1 | - | |
| 1 | - | 1 | 1 | - | - | - | - | |
| 2 | 1 | 1 | - | 1 | 1 | - | 1 | |
| 3 | 1 | 1 | 1 | 1 | - | - | 1 | |
| 4 | - | 1 | 1 | - | - | 1 | 1 | |
| 5 | 1 | - | 1 | 1 | - | 1 | 1 | |
| 6 | 1 | - | 1 | 1 | 1 | 1 | 1 | |
| 7 | 1 | 1 | 1 | - | - | - | - | |
| 8 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | |
| 9 | 1 | 1 | 1 | 1 | - | 1 | 1 | |
| A | 1 | 1 | 1 | - | 1 | 1 | 1 | |
| B | 1 | 1 | 1 | 1 | 1 | 1 | 1 | = 8 pattern |
| C | 1 | - | - | 1 | 1 | 1 | - | |
| D | 1 | 1 | 1 | 1 | 1 | 1 | - | = 0 pattern |
| E | 1 | - | - | 1 | 1 | 1 | 1 | |
| F | 1 | - | - | - | 1 | 1 | 1 | |
| G | 1 | - | 1 | 1 | 1 | 1 | - | |
| H | - | 1 | 1 | - | 1 | 1 | 1 | |
| I | 1 | - | - | 1 | - | - | - | position 0 only |
| J | - | 1 | 1 | 1 | - | - | - | |
| L | - | - | - | 1 | 1 | 1 | - | |
| N | 1 | 1 | 1 | - | 1 | 1 | - | |
| O | 1 | 1 | 1 | 1 | 1 | 1 | - | = 0 pattern |
| P | 1 | 1 | - | - | 1 | 1 | 1 | |
| R | 1 | 1 | 1 | - | 1 | 1 | 1 | position 1 only |
| S | 1 | - | 1 | 1 | - | 1 | 1 | = 5 pattern |
| T | 1 | - | - | - | - | - | - | position 0 only |
| U | - | 1 | 1 | 1 | 1 | 1 | - | |
| Y | - | 1 | 1 | 1 | - | 1 | 1 | |
| a | 1 | 1 | 1 | 1 | 1 | - | 1 | |
| b | - | - | 1 | 1 | 1 | 1 | 1 | |
| c | - | - | - | 1 | 1 | - | 1 | |
| d | - | 1 | 1 | 1 | 1 | - | 1 | |
| e | 1 | 1 | - | 1 | 1 | 1 | 1 | |
| h | - | - | 1 | - | 1 | 1 | 1 | |
| i | - | - | - | - | 1 | - | - | |
| l | - | - | - | - | 1 | 1 | - | |
| n | - | - | 1 | - | 1 | - | 1 | |
| o | - | - | 1 | 1 | 1 | - | 1 | |
| r | - | - | - | - | 1 | - | 1 | |
| s | 1 | - | 1 | 1 | - | 1 | 1 | = 5 pattern |
| t | - | - | - | 1 | 1 | 1 | 1 | |
| u | - | 1 | - | - | - | 1 | 1 | upper half |
| - | - | - | - | - | - | - | 1 | minus |

Full table (95 entries, ASCII 0x20-0x7E) in `src/driver/lcd.c` `Character_Set[]`.

### Indicator and colon segments

| Indicator | COM | SEG | Icon |
|-----------|-----|-----|------|
| SIGNAL | 0 | 17 | hourly signal / sensor-on |
| BELL | 0 | 16 | alarm set |
| PM | 2 | 17 | afternoon (PM) |
| 24H | 2 | 16 | 24-hour mode |
| LAP | 1 | 10 | stopwatch lap |
| Colon | 1 | 16 | between hours and minutes |

## 5. Full netlist (verbatim, schematic version)

```
$NETS
'$1N3553' ; LED3.1 R19.2 
'$1N3554' ; LED4.1 R20.2 
'$1N3555' ; Q2.3 R19.1 R20.1 
'$1N3556' ; C57.1 C58.2 U12.A1 U13.B1            // switched MAG rail
'$1N3557' ; C28.2 C29.2 U4.2 U4.6 U4.8 U11.A1    // switched TEMP rail
'$1N3692' ; Q2.1 R18.2 R21.2                      // FET gate node
'$1N3693' ; R18.1 U6.47                           // LED_EN from PB13
'$1N3709' ; C1.2 U1.10 
'$1N3711' ; C3.2 U1.8 
'$1N3712' ; C3.1 U1.11 
'$1N3714' ; C4.2 U1.4 
'$1N3715' ; C4.1 U1.9 
'BME_SCL' ; R7.1 U2.4 U4.4 U6.6 U13.A2 
'BME_SDA' ; R8.1 U2.5 U4.3 U6.7 U13.B2 
'BTN_ALARM' ; R12.2 U6.40                         // PE4
'BTN_LED' ; R14.2 U6.2                            // PC13
'BTN_MODE' ; R13.2 U6.12                          // PC3
'BUZZER_DIN' ; U1.3 U6.20                         // PA5
'BUZZER_OUT' ; U1.6 
'LCD_COM1' ; LCD1.7 U6.23 
'LCD_COM2' ; LCD1.8 U6.24 
'LCD_COM3' ; LCD1.9 U6.51 
'LCD_SEG1..27' ; see section 4 for full pin split
'MAG_EN' ; U6.39 U12.B2 
'RTC_INT' ; R11.1 U2.7 U6.15                      // PA0
'TEMP_EN' ; U6.38 U11.B2 
BUZZER ; U1.7                                     // piezo drive
GND ; C1.1 C2.1 C28.1 C29.1 C31.1 C32.1 C33.1 C34.1 C37.1 C39.2 C40.2 C41.1
      C42.1 C44.1 C45.1 C46.1 C47.1 C48.2 C55.1 C56.1 C57.2 C58.1 Cext1.2
      GND1.1 Q2.2 R12.1 R13.1 R14.1 R21.1 U1.5 U2.3 U2.6 U2.9 U4.1 U4.5 U4.7
      U6.5 U6.32 U6.42 U6.69 U11.B1 U12.B1 U13.A1
NRST ; C48.1 RST1.1 U6.8 
SWCLK ; CLK1.1 U6.56 
SWDIO ; DIO1.1 U6.54 
VBAT ; C2.2 C31.2 C32.2 C33.2 C34.2 C37.2 C41.2 C42.2 C44.2 C45.2 C46.2 C47.2
       C55.2 C56.2 LED3.2 LED4.2 R7.2 R8.2 R11.2 U1.1 U1.2 U1.12 U2.2 U6.1
       U6.14 U6.30 U6.33 U6.41 U6.43 U6.44 U6.45 U6.55 U6.68 U11.A2 U12.A2
       VDD1.1 
VLCD ; Cext1.1 U6.27                              // PB2 / LCD_VLCD
VREF ; C39.1 C40.1 U6.13                          // VREF+
$END
```

## 6. Sensor addresses and observed state

| Device | Documented 7-bit address | Raw byte convention | Final connected preflight |
|--------|--------------------------|---------------------|---------------------------|
| RV-3129-C3 RTC | 0x56 | `0xAC` write / `0xAD` read (`RTC_ADDR`) | ACK; time/date and 1 Hz interrupt timer verified |
| MMC5603NJ magnetometer | 0x30 | `0x60` write / `0x61` read (`MAG_ADDR`) | **No ACK**; absent from full 0x01-0x7F scan |
| BME280 | 0x76 | `0xEC` write / `0xED` read | ACK; chip initialization and forced measurement verified |

- **RV-3129-C3:** INT pin 7 is open-drain active-LOW, matching R11 and PA0. Register reads require STOP between the register-address write and read. `Control_Status.PON` is bit 5 and indicates a full RTC power-on reset; bit 7 is EEbusy, not a validity-loss flag.
- **RTC timer:** firmware uses the 32 Hz source with reload count 31. TE/TAR are disabled before loading the count, TF is cleared, then TAR/TE/TIE are enabled. Recurring one-second delivery was verified on target.
- **MMC5603NJ:** fixed address 0x30 and product-ID register 0x39 should return 0x10. CTRL0 at 0x1B is write-only and is shadowed by the driver. These driver rules are implemented, but the current board NACKs before any register can be read.
- **Compass orientation:** the SWDIO/SWCLK edge is watch north/forward. U13's 90° PCB rotation maps sensor +X to watch-forward and sensor -Y to watch-right; heading is computed clockwise with `atan2(-Y, X)`.
- **Compass UI/power:** MAG takes one on-demand sample on entry, then remains static until ALARM requests another sample. The left two date positions show the nearest cardinal/intercardinal direction (`N`, `NE`, `E`, `SE`, `S`, `SW`, `W`, `NW`). Outside MAG, ODR and continuous mode are cleared for the datasheet's ~1 µA power-down state. `MAG_EN` cannot be lowered on this board because an unpowered U13 clamps the shared RTC I2C lines.
- **BME280:** SDO strap is confirmed at 0x76. Soft-reset settling, calibration reads, forced-mode retry, Bosch pressure Q24.8 conversion, and humidity compensation are implemented. Latest connected sample: 20.43 °C, 93881.6 Pa, 49.52 %RH.

## 7. Remaining hardware and standalone checks

1. **Magnetometer blocker:** measure the switched rail at U13.B1 while PB1/MAG_EN is HIGH. Then check U12 output, U13 orientation/assembly, supply continuity, and shorts. Firmware scans all valid 7-bit addresses; only 0x56 and 0x76 ACK. Reversing the tested MAG_EN polarity did not expose another device.
2. Exercise the three physical watch buttons in the assembled case. Software EXTI injection verified the handlers and state machine, but not button mechanics, pad contact, or case alignment.
3. Confirm LED brightness and buzzer audibility after installation. GPIO paths execute on target; final in-case output quality remains unchecked.
4. Run all 88 standalone scenarios in `tests/opencasio_test_script.csv`, including battery removal, alarm firing, mode cycling, stopwatch/countdown persistence, and long clock rollover.
5. Measure operating and sleep current. The present hardware requires both sensor rails to remain enabled for shared-I2C operation, so the intended power-gating design is not yet available.
6. Identify buzzer amplifier U1 and decide whether GPIO square-wave drive is sufficient or TIM2 PWM is required.

## 8. Bring-up findings (on-silicon through 2026-09-15)

1. **Boot ROM:** a virgin STM32WB55 may boot the system loader after flash programming because `FLASH_ACR.EMPTY` is re-evaluated at option-byte load. A power-on reset, or explicitly clearing EMPTY, is required once.
2. **Sensor rails:** unpowered BME280/MMC5603NJ devices clamp the shared I2C bus. Active-HIGH U11/U12 operation and a conservative ~20 ms settle are established. Both rails currently stay enabled.
3. **BME280:** final driver passes initialization and measurement on silicon. Earlier reset-default samples came from insufficient post-reset settling; pressure and humidity compensation defects are fixed.
4. **MMC5603NJ:** write-only CTRL0 shadowing and Status1 bit-6 completion handling are implemented. MAG measurements are manually requested with ALARM and return to on-demand power-down afterward. Earlier one-second Saleae testing confirmed the scheduler and persistent `0x30` NACK, but compass heading behavior is not accepted as verified.
5. **LCD:** STM32 segment-line remapping, high-word COM handling, and PC10/PC11/PC12 SEG42/41/40 selection are resolved. The complete deterministic pattern was physically accepted and stable RAM decoding reports no unknown cells. The clock now renders date and time before one UDR request; this avoids LCD RAM write protection discarding the time update.
6. **RTC:** PON bit handling, legal timer sequencing, exact recurring one-second ticks, TIME-SET blinking, and month/day persistence are verified on target. A Saleae capture with D0=SCL and D1=SDA showed TF=0x02, fully ACKed reads, and advancing BCD seconds.
7. **UI clocks:** physical clock, alarm, stopwatch, and countdown operation is accepted. The clock visibly advances, alarm state renders correctly, stopwatch start/stop/reset works, and countdown pause/resume works. Off-screen progression remains verified from the earlier target checks.
8. **Programming:** the V2J17S4 ST-Link requires deprecated OpenOCD HLA transport. The final ELF builds at 19,980 bytes flash / 712 bytes RAM and was programmed with `stlink-hla.cfg`; OpenOCD reported `Verified OK`.
9. **Preflight disposition:** `PREFLIGHT-01` build/upload and `PREFLIGHT-02` LCD pass. `PREFLIGHT-03` remains FAIL solely because the magnetometer does not ACK. The CSV intentionally gates disconnection on resolving or explicitly accepting that hardware failure.
10. **LED edit acceleration:** debounced short presses increment the selected field, and a held LED button currently repeats on the 1 Hz RTC tick after more than three seconds. The intended approximately 5 Hz behavior remains outstanding.
