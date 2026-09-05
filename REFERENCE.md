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

- Single battery rail `VBAT` feeds: MCU (all VDD/VDDA/VDDRF pins), RTC U2 (always on), both LED anodes, I2C pull-ups, switch inputs.
- **Sensor power gating**: BME280 hangs on U11-switched rail, MMC5603NJ on U12-switched rail. Firmware MUST gate these rails off when sensors are idle; after enabling a rail, re-run the sensor init sequence (cold start every time).
- Leakage caution: while a sensor rail is off, its I2C pins are still pulled to VBAT by R7/R8 → back-powering through the sensor's ESD diodes. Expect small but nonzero drain; consider reconfiguring bus pins or accept the tradeoff.
- `VLCD` rail = MCU pin 27 (**PB2**, alternate function `LCD_VLCD`, i.e. the internal LCD step-up output) + Cext1. There is NO dedicated VLCD supply pin on this package; the step-up converter drives VLCD out through PB2. LCD contrast/voltage selection happens inside the LCD peripheral (RM0434 ch. 22).

## 3. MCU pin map (VFQFPN68)

### Digital I/O

| QFN68 pin | Port/pin | Net | Direction / notes |
|-----------|----------|-----|--------------------|
| 2 | PC13 | BTN_LED | Button input, 10 kΩ pulldown to GND → **active-HIGH** (watch button shorts pad toward battery rail). PC13 is also WKUP2. |
| 12 | PC3 | BTN_MODE | Button input, active-HIGH, 10 kΩ pulldown. PC3 also has `LCD_VLCD` AF (not used for that here). |
| 40 | PE4 | BTN_ALARM | Button input, active-HIGH, 10 kΩ pulldown. |
| 15 | PA0 | RTC_INT | Input, RV-3129-C3 interrupt, 10 kΩ pull-up to VBAT → **active-LOW**. |
| 38 | PB0 | TEMP_EN | Output, BME280 rail switch enable. Polarity of the switch (active-high EN assumed) to be confirmed against switch part datasheet. |
| 39 | PB1 | MAG_EN | Output, magnetometer rail switch enable. |
| 47 | PB13 | LED_EN | Output → R18 47 Ω → Q2 gate (R21 10 kΩ pulldown keeps LED off at reset). HIGH = LED on. |
| 20 | PA5 | BUZZER_DIN | Output to buzzer amp U1. Candidate for TIM2_CH1 PWM (PA5 = TIM2_CH1 AF1) instead of plain GPIO beeps. |

### I2C1 bus (`BME_SCL` / `BME_SDA`, shared by all three slaves)

| QFN68 pin | Port/pin | AF | Net | Slaves on this bus |
|-----------|----------|----|-----|--------------------|
| 6 | PB8 | AF4 I2C1_SCL | BME_SCL | RTC U2 addr raw byte 0xAC (7-bit 0x56), MAG U13 addr 0x60, BME280 U4 addr 0x76 |
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
| SEG22 | 60 | PC12 | LCD_SEG30 / LCD_SEG42 * |
| SEG23 | 59 | PC11 | LCD_SEG29 / LCD_SEG41 * |
| SEG24 | 58 | PC10 | LCD_SEG28 / LCD_SEG40 * |
| SEG25 | 57 | PA15 | LCD_SEG17 |
| SEG26 | 50 | PC6 | LCD_SEG24 |
| SEG27 | 46 | PB12 | LCD_SEG12 |

\* PC10/PC11/PC12 carry dual SEG labels in datasheet Table 18 because the SEG[43:40] lines are redirected to COM[7:4] by the internal SEG/COM mux depending on the configured duty (RM0434 §22 block diagram). Which index applies for our 1/3-duty configuration must be confirmed against RM0434 during LCD driver work.

Notes:
- 3 commons only → glass operates in **1/3 duty, 1/3 bias** mode (RM0434 LCD chapter).
- All AF assignments above verified against datasheet Table 18 (coordinate-extracted); glass pad 13 = PC4 = `LCD_SEG22` is a normal segment line (earlier "GPIO-driven" claim was a parse error).
- Unused MCU LCD-capable pins stay GPIO so the controller doesn't drive phantom segments.
- Datasheet package capability: VFQFPN68 supports up to 4 COM × 28 SEG; we use 3 × 24.

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

## 6. Sensor addresses & quick facts

All confirmed against the device datasheets (`docs/rv3129_appman.txt`, `docs/mmc5603nj_ds.txt`).

| Device | 7-bit addr | Raw byte (driver convention) | Bus position |
|--------|-----------|------------------------------|--------------|
| RV-3129-C3 RTC | 0x56 | `0xAC` write / `0xAD` read (`RTC_ADDR`) | always powered |
| MMC5603NJ mag | 0x30 | `0x60` write / `0x61` read (`MAG_ADDR`) | gated rail U12 |

- **RV-3129-C3** (app manual): INT pin (pin 7) is open-drain, active-LOW → matches the R11 pull-up + PA0 input. Pin 7 doubles as CLKOUT selectable via Control_1 bit7 (`Clk/Int`). I²C protocol, register pages and alarm/timer functions in app-manual ch. 4–6.
- **MMC5603NJ** (Rev.B): factory-set 7-bit address `[0110000]`; product-ID register at `0x39` — read it at bring-up as presence check. On-demand and continuous modes; continuous mode needs non-zero ODR in `0x2A` plus `Cmm_freq_en`; automatic set/reset via `Auto_SR_en`. Full register map in the datasheet.
- BME280: fixed 7-bit address 0x76 (SDO = GND) or 0x77 (SDO = VDDIO) — strap not visible in netlist, verify at bring-up.

## 7. Open questions (resolve during development)

1. Which SEG index (30/42, 29/41, 28/40) applies on PC12/PC11/PC10 in 1/3 duty — resolve via RM0434 SEG/COM mux rules during LCD driver work.
2. F-91W glass truth table: pad ↔ digit-segment map. Needed before any digit rendering works.
3. Load-switch polarity (EN active-high?) and startup rise time → delay after enabling rail before I2C access. Switch part numbers not in netlist.
4. Buzzer amp U1 identity/gain: determines whether PA5 should be PWM (tone) or square-wave GPIO.
5. BME280 SDO strap (addr 0x76 vs 0x77).
