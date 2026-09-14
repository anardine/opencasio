//
// Created by Alessandro Nardinelli on 14/01/26.
//

#include "driver/lcd.h"

// --- F-91W glass truth table ---
// Adapted from Sensor-Watch (joeycastillo/Sensor-Watch, MIT license).
// Same physical glass, DIFFERENT controller wiring: their Segment_Map /
// IndicatorMap indices are SAM L22 SLCD lines (glass pad numbering), ours
// are the STM32WB55 lines from REFERENCE.md section 4. SegLineRemap below
// translates between them; every pixel write goes through it.
// 7-segment character patterns: bits [6:0] = segments A-G.
// Index = character - 0x20 (ASCII space through ~).
static const uint8_t Character_Set[] = {
    0b00000000, // (space)
    0b01100000, // ! (L in top half for positions 4/6)
    0b00100010, // "
    0b01100011, // # (degree symbol)
    0b00101101, // $ (S without center)
    0b00000000, // % (unused)
    0b01000100, // & (lowercase 7)
    0b00100000, // '
    0b00111001, // (
    0b00001111, // )
    0b11000000, // * (+ for position 0)
    0b01110000, // + (segments E,F,G)
    0b00000100, // ,
    0b01000000, // -
    0b01000000, // . (same as -)
    0b00010010, // /
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000, // : (unused)
    0b00000000, // ; (unused)
    0b01011000, // <
    0b01001000, // =
    0b01001100, // >
    0b01010011, // ?
    0b11111111, // @ (all segments)
    0b01110111, // A
    0b01111111, // B
    0b00111001, // C
    0b00111111, // D
    0b01111001, // E
    0b01110001, // F
    0b00111101, // G
    0b01110110, // H
    0b10001001, // I (position 0 only)
    0b00001110, // J
    0b01110101, // K
    0b00111000, // L
    0b10110111, // M (position 0 only)
    0b00110111, // N
    0b00111111, // O
    0b01110011, // P
    0b01100111, // Q
    0b11110111, // R (position 1 only)
    0b01101101, // S
    0b10000001, // T (position 0 only)
    0b00111110, // U
    0b00111110, // V
    0b10111110, // W (position 0 only)
    0b01111110, // X
    0b01101110, // Y
    0b00011011, // Z
    0b00111001, // [
    0b00100100, // backslash
    0b00001111, // ]
    0b00100011, // ^
    0b00001000, // _
    0b00000010, // `
    0b01011111, // a
    0b01111100, // b
    0b01011000, // c
    0b01011110, // d
    0b01111011, // e
    0b01110001, // f
    0b01101111, // g
    0b01110100, // h
    0b00010000, // i
    0b01000010, // j
    0b01110101, // k
    0b00110000, // l
    0b10110111, // m (position 0 only)
    0b01010100, // n
    0b01011100, // o
    0b01110011, // p
    0b01100111, // q
    0b01010000, // r
    0b01101101, // s
    0b01111000, // t
    0b01100010, // u (upper half)
    0b00011100, // v (lower half)
    0b10111110, // w (position 0 only)
    0b01111110, // x
    0b01101110, // y
    0b00011011, // z
    0b00010110, // { (il ligature)
    0b00110110, // | (ll ligature)
    0b00110100, // } (li ligature)
    0b00000001, // ~
};

// Segment map: each position has 8 bytes, each byte encodes
// COM[7:6] (2 bits) + SEG[5:0] (6 bits) for one of the 7 segments.
// COM > 2 means no segment exists for that position (skip).
static const uint64_t Segment_Map[] = {
    0x4e4f0e8e8f8d4d0dULL, // Position 0, day-of-week
    0xc8c4c4c8b4b4b0bULL,  // Position 1, day-of-week
    0xc049c00a49890949ULL, // Position 2, day-of-month
    0xc048088886874707ULL, // Position 3, day-of-month
    0xc053921252139352ULL, // Position 4, clock hours
    0xc054511415559594ULL, // Position 5, clock hours
    0xc057965616179716ULL, // Position 6, clock minutes
    0xc041804000018a81ULL, // Position 7, clock minutes
    0xc043420203048382ULL, // Position 8, clock seconds
    0xc045440506468584ULL, // Position 9, clock seconds
};

// Indicator segments: (COM, SEG) pairs from Sensor-Watch.
static const struct { uint8_t com; uint8_t seg; } IndicatorMap[] = {
    {0, 17},  // SIGNAL
    {0, 16},  // BELL
    {2, 17},  // PM
    {2, 16},  // 24H
    {1, 10},  // LAP
};

// --- Glass pad remapping (the actual bring-up bug) ---
// Sensor-Watch's Segment_Map / IndicatorMap indices are *their* SAM L22
// SLCD segment lines, i.e. 1:1 with the F-91W glass pad each net reaches
// (glass pads 1-6 = SW SEG18-23, pads 10-27 = SW SEG17-0; COMs at pads
// 7-9 = SW COM0-2).  Our board wires the same glass pads to different
// STM32 LCD segment lines (REFERENCE.md section 4), so every seg index
// must be translated before it touches LCD_RAM.
//
// SegLineRemap[sw_seg] = STM32 SEG line:
//   sw 0->pad27->SEG12   sw 1->pad26->SEG24   sw 2->pad25->SEG17
//   sw 3->pad24->SEG40   sw 4->pad23->SEG41   sw 5->pad22->SEG42
//   sw 6->pad21->SEG7    sw 7->pad20->SEG8    sw 8->pad19->SEG9
//   sw 9->pad18->SEG6    sw 10->pad17->SEG21  sw 11->pad16->SEG11
//   sw 12->pad15->SEG10  sw 13->pad14->SEG23  sw 14->pad13->SEG22
//   sw 15->pad12->SEG4   sw 16->pad11->SEG3   sw 17->pad10->SEG5
//   sw 18->pad6->SEG2    sw 19->pad5->SEG18   sw 20->pad4->SEG19
//   sw 21->pad3->SEG20   sw 22->pad2->SEG0    sw 23->pad1->SEG1
// (PC10/11/12 carry SEG42/41/40 in 1/3 duty per RM0434 Table 117.)
// COM needs no translation: SW COM0/1/2 = glass pads 7/8/9 = our
// PA8/PA9/PA10 = STM32 COM0/1/2, same indices.
static const uint8_t SegLineRemap[24] = {
    12, 24, 17, 40, 41, 42, 7, 8, 9, 6, 21, 11,
    10, 23, 22, 4, 3, 5, 2, 18, 19, 20, 0, 1,
};

// RAM words for 1/3 duty (RM0434 22.6.5-22.6.7): LCD_RAM[2*com] holds
// SEG[31:0] of COM n, LCD_RAM[2*com+1] bits 11:0 hold SEG[43:32].
static volatile uint32_t *const comRegL[3] = {
    &LCD->com0_l, &LCD->com1_l, &LCD->com2_l,
};
static volatile uint32_t *const comRegH[3] = {
    &LCD->com0_h, &LCD->com1_h, &LCD->com2_h,
};

#define LCD_POLL_LIMIT  100000U

// --- Pixel-level operations ---
// com/seg inputs use Sensor-Watch numbering (see tables above); they are
// translated to STM32 SEG lines here so all callers (Segment_Map,
// IndicatorMap, colon, ninth segment) stay untouched.

void lcdSetPixel(uint8_t com, uint8_t seg) {
    if (com > 2 || seg > 23) return;
    uint8_t line = SegLineRemap[seg];
    if (line < 32)
        comRegL[com][0] |= (1U << line);
    else
        *comRegH[com] |= (1U << (line - 32));
}

void lcdClearPixel(uint8_t com, uint8_t seg) {
    if (com > 2 || seg > 23) return;
    uint8_t line = SegLineRemap[seg];
    if (line < 32)
        comRegL[com][0] &= ~(1U << line);
    else
        *comRegH[com] &= ~(1U << (line - 32));
}

// --- Character display ---

void lcdDisplayChar(char c, uint8_t position) {
    if (position >= LCD_NUM_POSITIONS) return;

    // Position 0 has a funky ninth segment — clear it first.
    if (position == 0) lcdClearPixel(0, 15);

    // Clamp to printable range.
    if (c < 0x20 || c > 0x7E) c = ' ';

    uint64_t segmap = Segment_Map[position];
    uint8_t segdata = Character_Set[(uint8_t)c - 0x20];

    for (int i = 0; i < 8; i++) {
        uint8_t com = (segmap & 0xFF) >> 6;
        if (com > 2) {
            segmap >>= 8;
            segdata >>= 1;
            continue;
        }
        uint8_t seg = segmap & 0x3F;
        if (segdata & 1)
            lcdSetPixel(com, seg);
        else
            lcdClearPixel(com, seg);
        segmap >>= 8;
        segdata >>= 1;
    }

    // Special: position 0/1 extra segments for B, D, @.
    if (position == 0 && (c == 'B' || c == 'D' || c == '@'))
        lcdSetPixel(0, 15);
}

void lcdDisplayString(const char *str, uint8_t position) {
    uint8_t i = 0;
    while (str[i] && position + i < LCD_NUM_POSITIONS) {
        lcdDisplayChar(str[i], position + i);
        i++;
    }
}

// --- Colon and indicators ---

void lcdSetColon(void)     { lcdSetPixel(1, 16); }
void lcdClearColon(void)   { lcdClearPixel(1, 16); }

void lcdSetIndicator(lcd_indicator_t ind) {
    if (ind > LCD_INDICATOR_LAP) return;
    lcdSetPixel(IndicatorMap[ind].com, IndicatorMap[ind].seg);
}

void lcdClearIndicator(lcd_indicator_t ind) {
    if (ind > LCD_INDICATOR_LAP) return;
    lcdClearPixel(IndicatorMap[ind].com, IndicatorMap[ind].seg);
}

void lcdClearAllIndicators(void) {
    for (uint8_t i = 0; i <= LCD_INDICATOR_LAP; i++)
        lcdClearPixel(IndicatorMap[i].com, IndicatorMap[i].seg);
}

// --- LCD controller init and management ---

uint8_t LCD_Init(void) {
    // 1. Route LSI1 to RTCCLK (BDCR.RTCSEL=10) for LCDCLK.
    PWR->cr1 |= PWR_CR1_DBP;
    RCC->bdcr = (RCC->bdcr & ~(3U << 8)) | RCC_BDCR_RTCSEL_LSI1;

    // 2. Enable LCD APB clock and configure GPIO pins.
    enableRCC(LCD_PER);
    LCD_GPIO_Init();

    // 3. Configure LCD_CR (write-protected when ENS=1).
    LCD->cr = LCD_CR_DUTY_1_3 | LCD_CR_BIAS_1_3 | LCD_CR_BUFEN;

    // 4. Frame rate ~31 Hz, contrast VLCD3, high drive.
    LCD->fcr = LCD_FCR_PS_4 | LCD_FCR_DIV_6 | LCD_FCR_CC_VLCD3 |
               LCD_FCR_HD | LCD_FCR_PON_1;

    // 5. Wait for FCR sync.
    uint32_t timeout = LCD_POLL_LIMIT;
    while (!(LCD->sr & LCD_SR_FCRSF)) {
        if (--timeout == 0U) return LCD_CFG_ERR;
    }

    // 6. Enable LCD, wait for step-up converter RDY.
    LCD->cr |= LCD_CR_LCDEN;
    timeout = LCD_POLL_LIMIT;
    while (!(LCD->sr & LCD_SR_RDY)) {
        if (--timeout == 0U) return LCD_CFG_ERR;
    }

    // 7. Clear all RAM and trigger first update.
    lcdDisplayClear();
    return CORE_OK;
}

void lcdDisable(void) {
    LCD->cr &= ~LCD_CR_LCDEN;
    uint32_t timeout = LCD_POLL_LIMIT;
    while (LCD->sr & LCD_SR_ENS) {
        if (--timeout == 0U) break;
    }
    PWR->cr1 &= ~PWR_CR1_DBP;
}

lcd_status_t lcdGetStatus(void) {
    uint32_t sr = LCD->sr;
    lcd_status_t s;
    s.enabled     = (sr & LCD_SR_ENS)   ? 1 : 0;
    s.ready       = (sr & LCD_SR_RDY)   ? 1 : 0;
    s.update_done = (sr & LCD_SR_UDD)   ? 1 : 0;
    s.sof         = (sr & LCD_SR_SOF)   ? 1 : 0;
    s.fcr_synced  = (sr & LCD_SR_FCRSF) ? 1 : 0;
    return s;
}

uint8_t lcdDisplayWrite(uint8_t com, uint32_t segLow, uint32_t segHigh) {
    if (com > 2) return LCD_CFG_ERR;
    *comRegL[com] = segLow;
    // High word: only bits 11:0 are SEG[43:32].
    if (com == 0) LCD->com0_h = segHigh & 0x0FFFU;
    else if (com == 1) LCD->com1_h = segHigh & 0x0FFFU;
    else LCD->com2_h = segHigh & 0x0FFFU;
    return CORE_OK;
}

void lcdDisplayClear(void) {
    for (uint8_t i = 0; i < 3; i++) {
        *comRegL[i] = 0;
    }
    LCD->com0_h = 0;
    LCD->com1_h = 0;
    LCD->com2_h = 0;
    lcdDisplayUpdate();
}

void lcdDisplayUpdate(void) {
    LCD->sr = LCD_SR_UDR;
}
