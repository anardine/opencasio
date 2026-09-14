# Documentation Index (extracted from ~/Downloads PDFs)

Text extracted with `pdftotext -layout`:
- `rm0434.txt` — Reference manual RM0434 Rev 16, March 2026, 1530 pp. Covers STM32WB55xx/STM32WB35xx.
- `ds_stm32wb55re.txt` — Datasheet DS STM32WB55xx/STM32WB35xx, 193 pp.

Line numbers below refer to offsets in the extracted `.txt` files (grep-able).

## Device datasheets (external ICs)

- `rv3129_ds.txt` — Micro Crystal RV-3129-C3 datasheet (2 pp): pinout, I²C timing.
- `rv3129_appman.txt` — RV-3129-C3 Application Manual (68 pp): registers, alarm/timer functions, I²C protocol (§6.7 device addresses: WRITE ACh / READ ADh).
- `mmc5603nj_ds.txt` — MEMSIC MMC5603NJ Rev.B (18 pp): registers, on-demand and continuous modes, set/reset, product ID at 0x39.
- `bme280_ds.txt` — Bosch BME280 datasheet: reset/startup, calibration, forced-mode measurements, and compensation formulas.

## RM0434 chapters → rm0434.txt line ranges

| Ch | Peripheral | Line |
|----|------------|------|
| 2  | System and memory overview | 2913 |
| 5  | Power control (PWR) | 7815 |
| 6  | Reset and clock control (RCC) | 11728 |
| 7  | Clock recovery system (CRS) | 18450 |
| 10 | General-purpose I/Os (GPIO) | 21205 |
| 11 | SYSCFG | 24087 |
| 13 | DMA | 26051 |
| 15 | NVIC | 30532 |
| 16 | EXTI | 31002 |
| 19 | ADC | 34816 |
| 22 | Liquid crystal display controller (LCD) | 41928 |
| 27 | TIM1 | 51529 |
| 28 | TIM2 | 58604 |
| 29 | TIM16/TIM17 | 63378 |
| 30 | LPTIM | 66838 |
| 34 | Real-time clock (RTC, internal) | 69540 |
| 35 | Inter-integrated circuit interface (I2C) | 72485 |
| 38 | SPI | 85639 |

## Datasheet facts relevant to OPENCASIO

- LCD controller on WB55RE: up to **4 COM x 44 SEG** (or 8x40); F-91W glass uses fewer.
- I2C1 and I2C3 are available. OPENCASIO uses I2C1 for the external RTC (RV-3129-C3 at 7-bit 0x56 / raw write byte 0xAC), magnetometer (MMC5603NJ at 7-bit 0x30 / raw write byte 0x60), and BME280 at 0x76.
- LCD segment/common alternate-function mappings per pin are in the datasheet pinout tables (~line 3635+).
