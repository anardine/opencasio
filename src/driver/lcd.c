//
// Created by Alessandro Nardinelli on 14/01/26.
//

#include "driver/lcd.h"

// Bounded poll for a status flag (RM0434 §22.6.3).
#define LCD_POLL_LIMIT  100000U

// COM RAM register pairs: com0_l/com0_h at indices 0/1, etc.
// The struct fields com0_l..com7_h map to LCD_RAM registers at
// offset 0x14 + 4*x (RM0434 §22.6.5–22.6.7).
// For 1/3 duty only COM0-COM2 are active.
static volatile uint32_t *comRegL[3] = {
    &LCD->com0_l, &LCD->com1_l, &LCD->com2_l,
};
static volatile uint32_t *comRegH[3] = {
    &LCD->com0_h, &LCD->com1_h, &LCD->com2_h,
};

uint8_t LCD_Init(void) {
    // 1. Route LSI1 to RTCCLK (BDCR.RTCSEL=10) so LCDCLK is driven.
    //    BDCR writes require DBP=1 in PWR_CR1 (RM0434 §6.4.32, §6.6.1).
    PWR->cr1 |= PWR_CR1_DBP;
    // Clear RTCSEL then set to LSI1 (bits 9:8).
    RCC->bdcr = (RCC->bdcr & ~(3U << 8)) | RCC_BDCR_RTCSEL_LSI1;

    // 2. Enable LCD APB clock and configure GPIO pins.
    enableRCC(LCD_PER);
    LCD_GPIO_Init();

    // 3. Configure LCD_CR while LCD is disabled (VSEL/MUX_SEG/BIAS/DUTY/BUFEN
    //    are write-protected when ENS=1). VSEL=0 → internal step-up converter
    //    (PB2/VLCD + Cext1, REFERENCE.md §2). MUX_SEG=0 → SEG[42:40] for
    //    VFQFPN68 1/3 duty (Table 117). BUFEN=1 for stable intermediate voltages.
    LCD->cr = LCD_CR_DUTY_1_3 | LCD_CR_BIAS_1_3 | LCD_CR_BUFEN;

    // 4. Configure LCD_FCR: frame rate, contrast, high drive.
    //    PS=4, DIV=6 → fframe ≈ 31 Hz at 1/3 duty with LCDCLK=32kHz (Table 115).
    //    CC=VLCD3 (midrange contrast — adjust on hardware). HD=1 + PON=1 for
    //    permanent high drive (buffered mode ignores HD/PON, but set per RM).
    LCD->fcr = LCD_FCR_PS_4 | LCD_FCR_DIV_6 | LCD_FCR_CC_VLCD3 |
               LCD_FCR_HD | LCD_FCR_PON_1;

    // 5. Wait for FCR synchronization (FCRSF clears on FCR write, sets when
    //    the new value reaches the LCDCLK domain).
    uint32_t timeout = LCD_POLL_LIMIT;
    while (!(LCD->sr & LCD_SR_FCRSF)) {
        if (--timeout == 0U) return LCD_CFG_ERR;
    }

    // 6. Enable the LCD controller. RDY is set when the step-up converter
    //    has stabilized (RM0434 §22.3.5).
    LCD->cr |= LCD_CR_LCDEN;

    timeout = LCD_POLL_LIMIT;
    while (!(LCD->sr & LCD_SR_RDY)) {
        if (--timeout == 0U) return LCD_CFG_ERR;
    }

    // 7. Clear all RAM and trigger the first display update.
    lcdDisplayClear();

    return CORE_OK;
}

void lcdDisable(void) {
    LCD->cr &= ~LCD_CR_LCDEN;
    // ENS clears at the end of the last displayed frame (RM0434 §22.6.3).
    uint32_t timeout = LCD_POLL_LIMIT;
    while (LCD->sr & LCD_SR_ENS) {
        if (--timeout == 0U) break;
    }
    // Lock backup domain again.
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
    // High word: only bits 11:0 are SEG[43:32] (§22.6.6).
    *comRegH[com] = segHigh & 0x0FFFU;
    return CORE_OK;
}

void lcdDisplayClear(void) {
    for (uint8_t i = 0; i < 3; i++) {
        *comRegL[i] = 0;
        *comRegH[i] = 0;
    }
    lcdDisplayUpdate();
}

void lcdDisplayUpdate(void) {
    // Set UDR to request transfer from LCD_RAM to display buffer.
    // The update happens at the start of the next frame (§22.3.6).
    LCD->sr = LCD_SR_UDR;
}
