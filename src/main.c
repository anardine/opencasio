//
// Created by Alessandro Nardinelli on 14/11/25.
//
// OPENCASIO firmware — classic F-91W behaviour first.
//
// Modes (cycled with the MODE key, like the original MODE button):
//   TIME → ALARM → STW → TMR → TEMP → PRESSURE → HUMIDITY → MAG → TIME
//
// Button mapping (3 keys vs. the original's 4 — documented in the plan):
//   BTN_MODE  (PC3)  cycle mode / next field in edit modes  (F-91W "MODE")
//   BTN_ALARM (PE4)  context action                        (F-91W "adjust")
//       TIME   : enter TIME-SET (edit mode, blinking field)
//       TIMESET: exit + save
//       ALARM  : enter ALARM-SET (edit mode, blinking field)
//       ALRMSET: exit + save + arm
//       STW    : start / stop
//       TMR    : start / stop
//   BTN_LED   (PC13) light + key beep                      (F-91W "light")
//       Edit modes: increment the blinking field.
//       Every non-edit mode lights the LED. STW also resets while stopped,
//       TMR also adds 1 min while stopped, and ALARM also arms/disarms.
//
// Editing modes blink the active field at 1 Hz (the RV-3129-C3 timer is
// the blink source while TE runs — the same tick engine STW/TMR use).
//
// One RV-3129-C3 countdown timer at 1 Hz (auto-reload, TIE) is the single
// tick engine: STW seconds, TMR countdown, and edit-blink all dispatch off
// the same TF flag through RTC_INT (PA0, active-LOW).
//
// Sensors need no user configuration. One BME sample is shown across three
// consecutive screens after the classic watch functions; MAG follows them.
//

#include "../include/etc/error.h"
#include "driver/rcc.h"
#include "driver/gpio.h"
#include "auxiliary/gpio-pins-setup.h"
#include "driver/i2c.h"
#include "driver/lcd.h"
#include "auxiliary/rv-3129-c3.h"
#include "auxiliary/mmc5603nj.h"
#include "auxiliary/bme280.h"

I2C_Handle_t pToI2C = {
    .pI2Cx = I2C,
    .I2C_PinConfig = {
        .I2C_ClockSpeed = 100000,
        .I2C_Mode       = I2C_MODE_STANDARD,
    },
};

// Bus-scan results (boot diagnostics + future sensor overlay).
volatile uint8_t i2cScanAck[128];
volatile uint8_t i2cScanRtc, i2cScanMag, i2cScanBme76, i2cScanBme77;

// RTC read results.
rtc_time_t rtcTime;
rtc_date_t rtcDate;
volatile uint8_t rtcReadStatus;

// Sensor results retained for debugger preflight and the sensor UI modes.
mag_data_t   magData;
bme280_data_t bmeData;
volatile uint8_t bmeInitStatus, bmeMeasStatus;
volatile uint8_t lcdInitStatus;
volatile uint8_t magInitStatus, magMeasStatus;
static uint8_t magReady;
static volatile uint8_t editMonth = 1, editDay = 1;  // date edit shadows (month 1-12, day 1-31)

// Edit-mode state (declared early: the INT handler blinks these fields).
static rtc_time_t editTime;
static rtc_alarm_t editAlarm;

// BME280 address resolved from the boot bus scan.
static uint8_t bme280Addr;
static uint8_t bmeDataValid;
static uint8_t bmeTempFahrenheit;

// --- Event flags set by EXTI ISR callbacks (consumed by the superloop) ---
#define BUTTON_LED_MASK   (1U << 0)
#define BUTTON_MODE_MASK  (1U << 1)
#define BUTTON_ALARM_MASK (1U << 2)

volatile uint8_t buttonEventFlags;
static volatile uint8_t buttonHeldFlags;
static void showAlarm(void);
static void handleModeButton(void);
static void handleAlarmButton(void);
static void handleLedButton(void);
static void updateMagDisplay(void);
static void incrementTimeField(void);
static void incrementAlarmField(void);
static void autoRepeatEditField(void);
volatile uint8_t rtcIntFlag;    // PA0 falling (timer tick / alarm fire)

// --- UI state machine ---
typedef enum {
    MODE_TIME = 0, MODE_TIME_SET, MODE_ALARM, MODE_ALARM_SET,
    MODE_STW, MODE_TMR, MODE_BME_TEMP, MODE_BME_PRESSURE,
    MODE_BME_HUMIDITY, MODE_MAG,
} ui_mode_t;
static volatile ui_mode_t uiMode = MODE_TIME;

// Stopwatch: seconds accumulated by 1 s timer ticks. 1 s resolution for v1
// (centiseconds arrive with the TIM2 work). Keeps ticking across mode
// switches, like the original.
static volatile uint32_t stwSeconds;
static volatile uint8_t  stwRunning;

// Countdown timer: remaining seconds + settable duration in seconds
// (light key adds 1 min while stopped; default 10:00).
static volatile uint16_t tmrRemaining;
static volatile uint16_t tmrDuration = 600;
static volatile uint8_t  tmrRunning;
static volatile uint8_t  tmrPaused;

// Alarm settings (binary, 24 h). Written to the RTC alarm when arming.
static volatile uint8_t alarmHours = 6, alarmMinutes = 30;
static volatile uint8_t alarmArmed;

// --- Edit-mode state ---
// TIME-SET fields sequence: seconds, minutes, hours, day, month.
enum { EDF_SEC = 0, EDF_MIN, EDF_HOUR, EDF_DAY, EDF_MONTH, EDF_TIME_COUNT };
// ALARM-SET fields: hours, minutes.
enum { EDA_HOUR = 0, EDF_ALARM_HOUR = 0, EDF_ALARM_MIN, EDF_ALARM_COUNT };
static volatile uint8_t editField;
static volatile uint8_t editBlinkOn = 1;   // toggled by 1 Hz ticks while editing
static volatile uint8_t editLedHoldTicks;

// Alarm edit shadow values.
static volatile uint8_t alarmSetHours, alarmSetMinutes;

// EXTI ISR callback — overrides the weak default in gpio.c.
void GPIO_IRQCallback(uint8_t pinNumber) {
    switch (pinNumber) {
        case 13: buttonEventFlags |= BUTTON_LED_MASK; break;   // PC13 = BTN_LED
        case 3:  buttonEventFlags |= BUTTON_MODE_MASK; break;  // PC3  = BTN_MODE
        case 4:  buttonEventFlags |= BUTTON_ALARM_MASK; break; // PE4  = BTN_ALARM
        case 0:  rtcIntFlag   = 1; break;  // PA0  = RTC_INT
        default: break;
    }
}

// --- Display helpers -------------------------------------------------------

static void displayTimeOnLcd(void) {
    char buf[7];
    buf[0] = '0' + (rtcTime.hours / 10);
    buf[1] = '0' + (rtcTime.hours % 10);
    buf[2] = '0' + (rtcTime.minutes / 10);
    buf[3] = '0' + (rtcTime.minutes % 10);
    buf[4] = '0' + (rtcTime.seconds / 10);
    buf[5] = '0' + (rtcTime.seconds % 10);
    buf[6] = 0;
    lcdDisplayString(buf, 4);
    lcdSetColon();
}

static void displayDateOnLcd(void) {
    char buf[3];
    buf[0] = '0' + (rtcDate.month / 10);
    buf[1] = '0' + (rtcDate.month % 10);
    buf[2] = 0;
    lcdDisplayString(buf, 0);
    buf[0] = '0' + (rtcDate.day / 10);
    buf[1] = '0' + (rtcDate.day % 10);
    buf[2] = 0;
    lcdDisplayString(buf, 2);
}

static void displayClock(void) {
    rtcReadStatus = getTime(&pToI2C, &rtcTime);
    if (rtcReadStatus == CORE_OK) {
        rtcReadStatus = getDate(&pToI2C, &rtcDate);
        displayDateOnLcd();
        displayTimeOnLcd();
        lcdDisplayUpdate();
    }
}

static void displayTimeSetScreen(void) {
    char dateBuf[5];
    dateBuf[0] = '0' + (editMonth / 10);
    dateBuf[1] = '0' + (editMonth % 10);
    dateBuf[2] = '0' + (editDay / 10);
    dateBuf[3] = '0' + (editDay % 10);
    dateBuf[4] = 0;

    char timeBuf[7];
    timeBuf[0] = '0' + (editTime.hours / 10);
    timeBuf[1] = '0' + (editTime.hours % 10);
    timeBuf[2] = '0' + (editTime.minutes / 10);
    timeBuf[3] = '0' + (editTime.minutes % 10);
    timeBuf[4] = '0' + (editTime.seconds / 10);
    timeBuf[5] = '0' + (editTime.seconds % 10);
    timeBuf[6] = 0;

    if (!editBlinkOn) {
        if (editField == EDF_MONTH) dateBuf[0] = dateBuf[1] = ' ';
        if (editField == EDF_DAY)   dateBuf[2] = dateBuf[3] = ' ';
        if (editField == EDF_HOUR)  timeBuf[0] = timeBuf[1] = ' ';
        if (editField == EDF_MIN)   timeBuf[2] = timeBuf[3] = ' ';
        if (editField == EDF_SEC)   timeBuf[4] = timeBuf[5] = ' ';
    }

    lcdDisplayString(dateBuf, 0);
    lcdDisplayString(timeBuf, 4);
    if (editBlinkOn) lcdSetColon(); else lcdClearColon();
    lcdDisplayUpdate();
}

static void displayAlarmSetScreen(void) {
    char buf[5];
    buf[0] = '0' + (alarmSetHours / 10);
    buf[1] = '0' + (alarmSetHours % 10);
    buf[2] = '0' + (alarmSetMinutes / 10);
    buf[3] = '0' + (alarmSetMinutes % 10);
    buf[4] = 0;
    if (!editBlinkOn) {
        if (editField == EDF_ALARM_HOUR)      { buf[0] = buf[1] = ' '; }
        else /* EDF_ALARM_MIN */              { buf[2] = buf[3] = ' '; }
    }
    lcdDisplayString(buf, 4);
    lcdSetIndicator(LCD_INDICATOR_BELL);
    if (editBlinkOn) lcdSetColon(); else lcdClearColon();
    lcdDisplayUpdate();
}

// MM:SS on positions 4-7 (+ colon), positions 8-9 blanked.
static void displayMmss(uint8_t mm, uint8_t ss, uint8_t colonOn) {
    char buf[7];
    buf[0] = '0' + (mm / 10);
    buf[1] = '0' + (mm % 10);
    buf[2] = '0' + (ss / 10);
    buf[3] = '0' + (ss % 10);
    buf[4] = ' ';
    buf[5] = ' ';
    buf[6] = 0;
    lcdDisplayString(buf, 4);
    if (colonOn) lcdSetColon(); else lcdClearColon();
    lcdDisplayUpdate();
}

// --- Tick engine (RV-3129-C3 timer, exact 1 s auto-reload) ------------------

// Update every active software clock regardless of the visible mode. This is
// what lets stopwatch and countdown continue while another screen is shown.
static void handleTimerTick(void) {
    if (uiMode == MODE_TIME_SET || uiMode == MODE_ALARM_SET) {
        editBlinkOn = !editBlinkOn;
        if (buttonHeldFlags & BUTTON_LED_MASK) {
            if (editLedHoldTicks < 4U) editLedHoldTicks++;
            if (editLedHoldTicks >= 4U) autoRepeatEditField();
        }
    }

    if (uiMode == MODE_TIME) {
        displayClock();
    }

    if (stwRunning) {
        stwSeconds++;
        if (uiMode == MODE_STW)
            displayMmss((uint8_t)((stwSeconds / 60) % 100),
                        (uint8_t)(stwSeconds % 60), 1);
    }

    if (tmrRunning) {
        if (tmrRemaining > 0) tmrRemaining--;
        if (uiMode == MODE_TMR)
            displayMmss((uint8_t)(tmrRemaining / 60),
                        (uint8_t)(tmrRemaining % 60), 1);
        if (tmrRemaining == 0) {
            tmrRunning = 0;
            tmrPaused = 0;
            timerStop(&pToI2C);
            buzzerBeep(60);
        }
    }
}

// Alarm fired (AF): ring. Alarm re-arms next midnight? No — the RV-3129-C3
// alarm matches every day at the set time while AIE stays on.
static void handleAlarmFire(void) {
    lcdSetIndicator(LCD_INDICATOR_BELL);
    lcdDisplayUpdate();
    for (uint8_t i = 0; i < 3; i++) {
        buzzerBeep(80);
        for (volatile uint32_t d = 0; d < 30000; d++) __asm volatile ("nop");
    }
}

static void handleRtcInterrupt(void) {
    // Read the INT flag register once; dispatch each set flag, then clear
    // both (write-0-to-clear semantics, Control_INT Flag §3.2.3).
    uint8_t flags = 0;
    if (readFromRTC(&pToI2C, RTC_REG_CONTROL_INT_FLAG, &flags, 1) != CORE_OK)
        return;

    if (flags & RTC_FLAG_TF) {
        timerClear(&pToI2C);
        handleTimerTick();
    }
    if (flags & RTC_FLAG_AF) {
        handleAlarmFire();
    }
    uint8_t clearVal = 0xFFU & ~(RTC_FLAG_AF | RTC_FLAG_TF);
    writeToRTC(&pToI2C, RTC_REG_CONTROL_INT_FLAG, &clearVal, 1);

    if (uiMode == MODE_TIME_SET) {
        displayTimeSetScreen();
    } else if (uiMode == MODE_ALARM_SET) {
        displayAlarmSetScreen();
    }
}

// --- Tick engine control -----------------------------------------------------

// The RV-3129 runs a stable one-second auto-reload period. Retry after rail
// churn because this interrupt is the TIME screen's only periodic wakeup.
static void tickStart(void) {
    for (uint8_t attempt = 0; attempt < 10; attempt++) {
        if (timerStart1Hz(&pToI2C) == CORE_OK) return;
        for (volatile uint32_t d = 0; d < 320000; d++) __asm volatile ("nop");
    }
}

static void tickStopIfIdle(void) {
    if (!stwRunning && !tmrRunning &&
        uiMode != MODE_TIME_SET && uiMode != MODE_ALARM_SET)
        timerStop(&pToI2C);
}

static void buttonDebounceDelay(void) {
    for (volatile uint32_t delay = 0; delay < 320000; delay++)
        __asm volatile ("nop");
}

static uint8_t buttonInputsHigh(void) {
    uint8_t levels = 0;
    if (GPIOC->idr & (1U << 13)) levels |= BUTTON_LED_MASK;
    if (GPIOC->idr & (1U << 3))  levels |= BUTTON_MODE_MASK;
    if (GPIOE->idr & (1U << 4))  levels |= BUTTON_ALARM_MASK;
    return levels;
}

static void handleButtonEvents(void) {
    __asm volatile ("cpsid i" ::: "memory");
    uint8_t pending = buttonEventFlags;
    buttonEventFlags &= (uint8_t)~pending;
    __asm volatile ("cpsie i" ::: "memory");

    if (pending == 0) return;

    buttonDebounceDelay();

    uint8_t levels = buttonInputsHigh();
    uint8_t pressed = pending & levels;
    uint8_t released = pending & (uint8_t)~levels;

    buttonHeldFlags &= (uint8_t)~released;
    if (released & BUTTON_LED_MASK) editLedHoldTicks = 0;
    if (released & BUTTON_LED_MASK) ledOff();

    if (pressed & BUTTON_MODE_MASK) {
        if (!(buttonHeldFlags & BUTTON_MODE_MASK)) {
            buttonHeldFlags |= BUTTON_MODE_MASK;
            handleModeButton();
        }
    }
    if (pressed & BUTTON_ALARM_MASK) {
        if (!(buttonHeldFlags & BUTTON_ALARM_MASK)) {
            buttonHeldFlags |= BUTTON_ALARM_MASK;
            handleAlarmButton();
        }
    }
    if (pressed & BUTTON_LED_MASK) {
        if (!(buttonHeldFlags & BUTTON_LED_MASK)) {
            buttonHeldFlags |= BUTTON_LED_MASK;
            editLedHoldTicks = 0;
            if (uiMode != MODE_TIME_SET && uiMode != MODE_ALARM_SET)
                ledOn();
            handleLedButton();
        }
    }
}

// --- Edit modes ---------------------------------------------------------------

// TIME-SET: blinking seconds field, MODE = next field, LED = increment,
// ALARM = exit + save. Classic F-91W set order (sec → hour → min → …).
static void enterTimeSet(void) {
    editTime = rtcTime;
    editMonth = rtcDate.month;
    if (editMonth == 0 || editMonth > 12) editMonth = 1;
    editDay = rtcDate.day;
    if (editDay == 0 || editDay > 31) editDay = 1;
    editField = EDF_SEC;
    editBlinkOn = 1;
    uiMode = MODE_TIME_SET;
    tickStart();
    displayTimeSetScreen();
}

static void incrementTimeField(void) {
    switch (editField) {
        case EDF_SEC:   editTime.seconds = (editTime.seconds + 1) % 60; break;
        case EDF_MIN:   editTime.minutes = (editTime.minutes + 1) % 60; break;
        case EDF_HOUR:  editTime.hours   = (editTime.hours + 1) % 24;   break;
        case EDF_DAY:   editDay          = (editDay % 31) + 1;          break;
        case EDF_MONTH: editMonth        = (editMonth % 12) + 1;        break;
        default: break;
    }
    editBlinkOn = 1;
    displayTimeSetScreen();
    buzzerBeep(10);
}

static void exitTimeSet(void) {
    setTime(&pToI2C, &editTime);
    rtcDate.month = editMonth;
    rtcDate.day = editDay;
    setDate(&pToI2C, &rtcDate);
    uiMode = MODE_TIME;
    tickStart();
    displayClock();
    buzzerBeep(20);
}

static void enterAlarmSet(void) {
    alarmSetHours = alarmHours;
    alarmSetMinutes = alarmMinutes;
    editField = EDF_ALARM_HOUR;
    editBlinkOn = 1;
    uiMode = MODE_ALARM_SET;
    tickStart();
    displayAlarmSetScreen();
}

static void incrementAlarmField(void) {
    if (editField == EDF_ALARM_HOUR)
        alarmSetHours = (alarmSetHours + 1) % 24;
    else
        alarmSetMinutes = (alarmSetMinutes + 1) % 60;
    editBlinkOn = 1;
    displayAlarmSetScreen();
    buzzerBeep(10);
}

static void autoRepeatEditField(void) {
    while (GPIOC->idr & (1U << 13)) {
        // Existing 320k-loop debounce is ~20 ms at HSI16; 3.2M gives
        // approximately 200 ms between repeats, or five increments/second.
        for (volatile uint32_t delay = 0; delay < 3200000U; delay++)
            __asm volatile ("nop");
        if (!(GPIOC->idr & (1U << 13))) break;

        if (uiMode == MODE_TIME_SET) {
            switch (editField) {
                case EDF_SEC:   editTime.seconds = (editTime.seconds + 1) % 60; break;
                case EDF_MIN:   editTime.minutes = (editTime.minutes + 1) % 60; break;
                case EDF_HOUR:  editTime.hours   = (editTime.hours + 1) % 24;   break;
                case EDF_DAY:   editDay          = (editDay % 31) + 1;          break;
                case EDF_MONTH: editMonth        = (editMonth % 12) + 1;        break;
                default: break;
            }
            editBlinkOn = 1;
            displayTimeSetScreen();
        } else if (uiMode == MODE_ALARM_SET) {
            if (editField == EDF_ALARM_HOUR)
                alarmSetHours = (alarmSetHours + 1) % 24;
            else
                alarmSetMinutes = (alarmSetMinutes + 1) % 60;
            editBlinkOn = 1;
            displayAlarmSetScreen();
        } else {
            break;
        }
    }
    editLedHoldTicks = 0;
}

static void exitAlarmSet(void) {
    alarmHours = alarmSetHours;
    alarmMinutes = alarmSetMinutes;
    rtc_alarm_t a = { .hours = alarmHours, .minutes = alarmMinutes, .seconds = 0 };
    alarmArmed = (alarmSet(&pToI2C, &a) == CORE_OK);
    uiMode = MODE_ALARM;
    tickStopIfIdle();
    showAlarm();
    buzzerBeep(15);
}

// --- Mode screens ---------------------------------------------------------------

static void showAlarm(void) {
    lcdDisplayClear();
    if (alarmArmed) lcdSetIndicator(LCD_INDICATOR_BELL);
    char buf[5];
    buf[0] = '0' + (alarmArmed ? 1 : 0);
    buf[1] = '-';
    buf[2] = '0' + (alarmHours / 10);
    buf[3] = '0' + (alarmHours % 10);
    buf[4] = 0;
    lcdDisplayString(buf, 4);
    lcdSetColon();
    buf[0] = '0' + (alarmMinutes / 10);
    buf[1] = '0' + (alarmMinutes % 10);
    buf[2] = 0;
    lcdDisplayString(buf, 8);
    lcdDisplayUpdate();
}

static void showStw(void) {
    lcdDisplayClear();
    lcdSetIndicator(LCD_INDICATOR_LAP);
    displayMmss((uint8_t)((stwSeconds / 60) % 100),
                (uint8_t)(stwSeconds % 60), stwRunning);
}

static void showTmr(void) {
    lcdDisplayClear();
    uint16_t r = (tmrRunning || tmrPaused) ? tmrRemaining : tmrDuration;
    displayMmss((uint8_t)(r / 60), (uint8_t)(r % 60), 1);
}

static void showMagError(void) {
    lcdDisplayClear();
    lcdDisplayString("--", 0);
    lcdDisplayString("E-MAG", 4);
    lcdDisplayUpdate();
}

static void updateMagDisplay(void) {
    if (!magReady) {
        magInitStatus = magInit(&pToI2C);
        if (magInitStatus != CORE_OK) {
            showMagError();
            return;
        }
        magReady = 1;
    }

    magMeasStatus = magGetData(&pToI2C, &magData);
    if (magMeasStatus != CORE_OK) {
        // The bus occasionally NACKs a measurement request for longer than
        // a single immediate retry covers (logic-analyzer capture showed
        // back-to-back NACKs on address write with no settling gap).
        // Back off briefly and retry a few times before forcing a full
        // re-init/E-MAG, so a transient condition doesn't throw away an
        // already-working session.
        uint8_t attempt;
        for (attempt = 0; attempt < 3; attempt++) {
            for (volatile uint32_t d = 0; d < 40000; d++) __asm volatile ("nop");
            magMeasStatus = magGetData(&pToI2C, &magData);
            if (magMeasStatus == CORE_OK) break;
        }
        if (magMeasStatus != CORE_OK) {
            magReady = 0;
            showMagError();
            return;
        }
    }

    uint16_t heading = magTransformToHeading(&magData);
    static const char directions[8][3] = {
        "N ", "NE", "E ", "SE", "S ", "SW", "W ", "NW",
    };
    uint8_t direction = (uint8_t)(((heading + 225U) / 450U) % 8U);
    char buf[7] = {
        'H',
        '0' + (heading / 1000),
        '0' + ((heading / 100) % 10),
        '0' + ((heading / 10) % 10),
        '0' + (heading % 10),
        ' ',
        0,
    };
    lcdDisplayClear();
    lcdDisplayString(directions[direction], 0);
    lcdDisplayString(buf, 4);
    lcdDisplayUpdate();
}

static void enterMode(ui_mode_t m) {
    uiMode = m;
    switch (m) {
        case MODE_TIME:
            lcdDisplayClear();
            lcdClearAllIndicators();
            if (alarmArmed) lcdSetIndicator(LCD_INDICATOR_BELL);
            tickStart();   // 1 Hz ticks keep the TIME-mode clock ticking
            displayClock();
            break;
        case MODE_ALARM:
            showAlarm();
            break;
        case MODE_STW:
            showStw();
            break;
        case MODE_TMR:
            showTmr();
            break;
        default:
            uiMode = MODE_TIME;
            break;
    }
}
static void enterMagMode(void) {
    uiMode = MODE_MAG;
    magReady = 0;
    // Don't submit a blank frame here: updateMagDisplay() does its own
    // clear + single lcdDisplayUpdate(). Submitting one here first would
    // write-protect LCD RAM before the heading digits are rendered,
    // leaving the screen stuck blank (same root cause as the earlier
    // stuck-clock bug).
    updateMagDisplay();
}

static void exitMagMode(void) {
    if (magReady) magMeasStatus = magStandby(&pToI2C);
    magReady = 0;
    enterMode(MODE_TIME);
}

static uint8_t refreshBmeData(void) {
    bmeDataValid = 0;
    bmeInitStatus = bme280Init(&pToI2C, bme280Addr);
    if (bmeInitStatus != CORE_OK) return bmeInitStatus;

    bmeMeasStatus = bme280Measure(&pToI2C, &bmeData);
    if (bmeMeasStatus != CORE_OK) return bmeMeasStatus;

    bmeDataValid = 1;
    return CORE_OK;
}

static void showBmeError(void) {
    lcdDisplayClear();
    lcdDisplayString("E-BME", 4);
    lcdDisplayUpdate();
}

// The sensor sits inside the case against the wearer's wrist, so on-wrist
// readings run hot vs. ambient (body heat + no airflow, tested exposed on
// the bench). Subtract a fixed offset to approximate ambient/skin-adjacent
// temperature. This is a rough compensation, not a calibrated model.
#define BME_WRIST_OFFSET_C  -2.0f

static void showBmeTemperature(void) {
    float temperature = bmeData.temp_C + BME_WRIST_OFFSET_C;
    char unit = 'C';
    if (bmeTempFahrenheit) {
        temperature = temperature * 9.0f / 5.0f + 32.0f;
        unit = 'F';
    }

    int32_t tenths = (int32_t)(temperature * 10.0f +
                               (temperature >= 0 ? 0.5f : -0.5f));
    if (tenths < -999) tenths = -999;
    if (tenths > 9999) tenths = 9999;
    int32_t magnitude = tenths < 0 ? -tenths : tenths;
    int32_t whole = magnitude / 10;
    int32_t tenthDigit = magnitude % 10;

    // Whole-degree field: two digits (or sign + digit) rendered at
    // positions 4-5 (the "hours" field on the F-91W glass, no colon shown).
    char main_text[3] = {
        ' ',
        '0' + (whole % 10),
        0,
    };
    if (tenths < 0) {
        main_text[0] = (whole >= 10) ? '-' : ' ';
        if (whole < 10) main_text[1] = '-';
    } else if (whole >= 10) {
        main_text[0] = '0' + ((whole / 10) % 10);
    }
    lcdDisplayClear();
    lcdDisplayString(main_text, 6);
    // The tenths digit and unit letter are placed on the smaller
    // seconds-sized segment (positions 8-9), to the right of the whole
    // number, with no colon shown, e.g. whole=22, tenths=1, unit=C reads
    // as "22" followed by a small "1C".
    char frac_text[3] = { '0' + tenthDigit, unit, 0 };
    lcdDisplayString(frac_text, 8);
    lcdDisplayUpdate();
}

static void enterBmeTemperatureMode(void) {
    uiMode = MODE_BME_TEMP;
    if (refreshBmeData() != CORE_OK) {
        showBmeError();
        return;
    }
    showBmeTemperature();
}

static void enterBmePressureMode(void) {
    uiMode = MODE_BME_PRESSURE;
    if (!bmeDataValid && refreshBmeData() != CORE_OK) {
        showBmeError();
        return;
    }

    lcdDisplayClear();
    int32_t pHpa = (int32_t)(bmeData.pressure_Pa + 50.0f) / 100;
    if (pHpa < 0) pHpa = 0;
    if (pHpa > 9999) pHpa = 9999;

    char pressure[7] = {
        'P',
        ' ',
        '0' + ((pHpa / 1000) % 10),
        '0' + ((pHpa / 100) % 10),
        '0' + ((pHpa / 10) % 10),
        '0' + (pHpa % 10),
        0,
    };
    lcdDisplayString(pressure, 4);
    lcdDisplayUpdate();
}

static void enterBmeHumidityMode(void) {
    uiMode = MODE_BME_HUMIDITY;
    if (!bmeDataValid && refreshBmeData() != CORE_OK) {
        showBmeError();
        return;
    }

    lcdDisplayClear();
    int32_t humidity = (int32_t)(bmeData.humidity_pct + 0.5f);
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;

    char humidityText[7] = {
        'r',
        'H',
        ' ',
        '0' + ((humidity / 100) % 10),
        '0' + ((humidity / 10) % 10),
        '0' + (humidity % 10),
        0,
    };
    lcdDisplayString(humidityText, 4);
    lcdDisplayUpdate();
}

// --- Button handlers ---------------------------------------------------------------


static void cycleMode(void) {
    switch (uiMode) {
        case MODE_TIME:      enterMode(MODE_ALARM); break;
        case MODE_TIME_SET:  exitTimeSet();         break;   // MODE = save+exit
        case MODE_ALARM:     enterMode(MODE_STW);   break;
        case MODE_ALARM_SET: exitAlarmSet();        break;
        case MODE_STW:       enterMode(MODE_TMR);   break;
        case MODE_TMR:       enterBmeTemperatureMode(); break;
        case MODE_BME_TEMP:  enterBmePressureMode();    break;
        case MODE_BME_PRESSURE: enterBmeHumidityMode(); break;
        case MODE_BME_HUMIDITY: enterMagMode();         break;
        case MODE_MAG:       exitMagMode();             break;
        default:             enterMode(MODE_TIME);  break;
    }
    buzzerBeep(15);
}

static void handleModeButton(void) {
    if (uiMode == MODE_TIME_SET) {
        // next edit field (sec -> min -> hour -> day -> month -> sec)
        editField = (editField + 1) % EDF_TIME_COUNT;
        editBlinkOn = 1;
        displayTimeSetScreen();
        buzzerBeep(10);
        return;
    }
    if (uiMode == MODE_ALARM_SET) {
        editField = (editField + 1) % 2;
        editBlinkOn = 1;
        displayAlarmSetScreen();
        buzzerBeep(10);
        return;
    }
    cycleMode();
}

static void handleAlarmButton(void) {
    switch (uiMode) {
        case MODE_TIME:
            enterTimeSet();
            buzzerBeep(15);
            break;
        case MODE_TIME_SET:
            exitTimeSet();
            break;
        case MODE_ALARM:
            enterAlarmSet();
            buzzerBeep(15);
            break;
        case MODE_ALARM_SET:
            exitAlarmSet();
            break;
        case MODE_STW:
            stwRunning = !stwRunning;
            if (stwRunning) tickStart();
            else tickStopIfIdle();
            showStw();
            buzzerBeep(15);
            break;
        case MODE_TMR:
            if (tmrRunning) {
                tmrRunning = 0;
                tmrPaused = 1;
                tickStopIfIdle();
            } else {
                if (!tmrPaused) tmrRemaining = tmrDuration;
                tmrRunning = 1;
                tmrPaused = 0;
                tickStart();
            }
            showTmr();
            buzzerBeep(15);
            break;
        case MODE_BME_TEMP:
            bmeTempFahrenheit = !bmeTempFahrenheit;
            if (!bmeDataValid && refreshBmeData() != CORE_OK)
                showBmeError();
            else
                showBmeTemperature();
            buzzerBeep(15);
            break;
        case MODE_MAG:
            updateMagDisplay();
            buzzerBeep(15);
            break;
        default:
            break;
    }
}

static void handleLedButton(void) {
    if (uiMode == MODE_TIME_SET) {
        incrementTimeField();
        return;
    }
    if (uiMode == MODE_ALARM_SET) {
        incrementAlarmField();
        return;
    }

    switch (uiMode) {
        case MODE_STW:
            if (!stwRunning) {
                stwSeconds = 0;
                showStw();
            }
            break;
        case MODE_TMR:
            if (!tmrRunning) {
                tmrDuration += 60;
                if (tmrDuration > 5940) tmrDuration = 600;
                tmrRemaining = 0;
                tmrPaused = 0;
                showTmr();
            }
            break;
        case MODE_ALARM:
            if (alarmArmed) {
                uint8_t intEn;
                if (readFromRTC(&pToI2C, RTC_REG_CONTROL_INT, &intEn, 1) == CORE_OK) {
                    intEn &= ~RTC_INT_AIE;
                    writeToRTC(&pToI2C, RTC_REG_CONTROL_INT, &intEn, 1);
                }
                alarmClear(&pToI2C);
                alarmArmed = 0;
            } else {
                rtc_alarm_t a = { .hours = alarmHours, .minutes = alarmMinutes, .seconds = 0 };
                alarmArmed = (alarmSet(&pToI2C, &a) == CORE_OK);
            }
            showAlarm();
            break;
        default:
            break;
    }
    buzzerBeep(20);
}

int main() {
      // Clock tree first: every peripheral init below assumes SYSCLK =
      // HSI16 16 MHz (I2C TIMINGR, rail settle delay, beep timing).
      if (!initRCC()) return RCC_CFG_ERR;

      uint8_t status = Board_GPIO_Init();
      if (status != CORE_OK) return status;

      status = I2C_Init(&pToI2C);
      if (status != CORE_OK) return status;

      // --- LCD ---
      lcdInitStatus = LCD_Init();
      if (lcdInitStatus != CORE_OK) return lcdInitStatus;

      // --- Boot bus scan: device presence + BME280 address resolution.
      // Hardware constraint (measured on silicon): the gated sensors clamp
      // BME_SCL/SDA to GND while their rails are off, defeating even the
      // internal pull-ups — the rails must stay on for ANY I2C access,
      // including the 1 Hz clock refresh. They therefore stay on until the
      // sensor overlay defines its own gating policy (board-revision
      // finding: the shared bus needs isolation or a separate pull-up
      // domain). ---
      railOn();
      railSettleDelay();

      i2cScanRtc = (I2C_Transmit(&pToI2C, 0, 0, 0, RTC_ADDR) == CORE_OK);
      i2cScanAck[RTC_ADDR >> 1] = i2cScanRtc;
      i2cScanMag   = (I2C_Transmit(&pToI2C, 0, 0, 0, MAG_ADDR) == CORE_OK);
      i2cScanBme76 = (I2C_Transmit(&pToI2C, 0, 0, 0, BME280_ADDR_76) == CORE_OK);
      i2cScanBme77 = (I2C_Transmit(&pToI2C, 0, 0, 0, BME280_ADDR_77) == CORE_OK);
      i2cScanAck[MAG_ADDR >> 1] = i2cScanMag;
      i2cScanAck[0x76]          = i2cScanBme76;
      i2cScanAck[0x77]          = i2cScanBme77;
      bme280Addr = i2cScanBme76 ? BME280_ADDR_76 : BME280_ADDR_77;
      (void)bme280Addr;


      // Reset / clear any stale RTC status flags and INT flags
      uint8_t rtcStatus = 0;
      readFromRTC(&pToI2C, RTC_REG_CONTROL_STATUS, &rtcStatus, 1);
      uint8_t clearPon = 0xFFU & ~RTC_STATUS_PON;
      writeToRTC(&pToI2C, RTC_REG_CONTROL_STATUS, &clearPon, 1);
      uint8_t clearFlags = 0x00U;
      writeToRTC(&pToI2C, RTC_REG_CONTROL_INT_FLAG, &clearFlags, 1);
      EXTI->pr1 = 1U << 0;

      // Read current time/date or seed defaults
      rtcReadStatus = getTime(&pToI2C, &rtcTime);
      if (rtcReadStatus != CORE_OK || (rtcStatus & RTC_STATUS_PON) ||
          rtcTime.seconds > 59 || rtcTime.minutes > 59 || rtcTime.hours > 23) {
            rtcTime = (rtc_time_t){ .seconds = 0, .minutes = 0, .hours = 0 };
            setTime(&pToI2C, &rtcTime);
      }
      rtcReadStatus = getDate(&pToI2C, &rtcDate);
      if (rtcReadStatus != CORE_OK || (rtcStatus & RTC_STATUS_PON) ||
          rtcDate.day == 0 || rtcDate.day > 31 || rtcDate.month == 0 || rtcDate.month > 12) {
            rtcDate = (rtc_date_t){ .day = 1, .weekday = 1, .month = 1, .year = 0 };
            setDate(&pToI2C, &rtcDate);
      }

    // Bring-up confirmation.
    ledOn();
    buzzerBeep(50);
    ledOff();

    // Start blinking in TIME-SET mode immediately upon battery insertion / boot
    enterTimeSet();

      // --- Event-driven superloop: WFI until an EXTI event. ---
      while (1) {
            __asm volatile ("wfi");

            handleButtonEvents();
            if (rtcIntFlag || !(GPIOA->idr & (1U << 0))) {
                  rtcIntFlag = 0;
                  handleRtcInterrupt();
            }
      }
}