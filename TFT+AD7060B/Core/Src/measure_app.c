#include "measure_app.h"

#include "lcd.h"
#include "stm32f4xx_hal.h"

#include <stdio.h>
#include <string.h>

#define MEASURE_LINE_CHARS 40U
#define MEASURE_LINE_HEIGHT 24U
#define MEASURE_TEXT_X 4U
#define MEASURE_GAIN_INPUT_CH 1U
#define MEASURE_GAIN_OUTPUT_CH 2U

typedef struct {
  uint32_t count;
  uint32_t vpp_mv[AD7606B_CHANNEL_COUNT];
  uint8_t valid;
} Measure_Snapshot;

static volatile int16_t measure_min_raw[AD7606B_CHANNEL_COUNT];
static volatile int16_t measure_max_raw[AD7606B_CHANNEL_COUNT];
static volatile uint32_t measure_count;
static Measure_Snapshot measure_last;
static uint32_t measure_open_vpp_mv;
static uint32_t measure_load_vpp_mv;
static uint8_t measure_open_valid;
static uint8_t measure_load_valid;
static uint8_t measure_capture_load_next;

static void Measure_ResetAccumLocked(void)
{
  uint8_t i;

  for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
    measure_min_raw[i] = INT16_MAX;
    measure_max_raw[i] = INT16_MIN;
  }
  measure_count = 0U;
}

static void Measure_FormatMilli(char *buffer, uint32_t size, const char *label,
                                uint32_t value_milli, uint8_t valid)
{
  if (valid == 0U) {
    (void)snprintf(buffer, size, "%s--", label);
    return;
  }

  (void)snprintf(buffer, size, "%s%lu.%03lu", label,
                 (unsigned long)(value_milli / 1000UL),
                 (unsigned long)(value_milli % 1000UL));
}

static void Measure_FormatOhm(char *buffer, uint32_t size, const char *label,
                              uint32_t ohm, uint8_t valid)
{
  if (valid == 0U) {
    (void)snprintf(buffer, size, "%s--", label);
  } else if (ohm >= 1000000UL) {
    (void)snprintf(buffer, size, "%s%lu.%03luM", label,
                   (unsigned long)(ohm / 1000000UL),
                   (unsigned long)((ohm % 1000000UL) / 1000UL));
  } else if (ohm >= 1000UL) {
    (void)snprintf(buffer, size, "%s%lu.%03luk", label,
                   (unsigned long)(ohm / 1000UL),
                   (unsigned long)(ohm % 1000UL));
  } else {
    (void)snprintf(buffer, size, "%s%luR", label, (unsigned long)ohm);
  }
}

static uint32_t Measure_RawVppMv(int16_t min_raw, int16_t max_raw)
{
  int32_t min_mv = AD7606B_RawToMillivolts(min_raw);
  int32_t max_mv = AD7606B_RawToMillivolts(max_raw);

  return (max_mv > min_mv) ? (uint32_t)(max_mv - min_mv) : 0UL;
}

static void Measure_UpdateSnapshot(void)
{
  int16_t min_copy[AD7606B_CHANNEL_COUNT];
  int16_t max_copy[AD7606B_CHANNEL_COUNT];
  uint32_t count_copy;
  uint8_t i;

  __disable_irq();
  count_copy = measure_count;
  if (count_copy != 0U) {
    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
      min_copy[i] = measure_min_raw[i];
      max_copy[i] = measure_max_raw[i];
    }
    Measure_ResetAccumLocked();
  }
  __enable_irq();

  if (count_copy == 0U) {
    return;
  }

  measure_last.count = count_copy;
  measure_last.valid = 1U;
  for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
    measure_last.vpp_mv[i] = Measure_RawVppMv(min_copy[i], max_copy[i]);
  }
}

static uint8_t Measure_CalcGainMilli(uint32_t *gain_milli)
{
  uint32_t vin = measure_last.vpp_mv[MEASURE_GAIN_INPUT_CH];
  uint32_t vout = measure_last.vpp_mv[MEASURE_GAIN_OUTPUT_CH];

  if ((measure_last.valid == 0U) || (vin == 0U)) {
    return 0U;
  }

  *gain_milli = (uint32_t)((((uint64_t)vout) * 1000ULL + (vin / 2UL)) / vin);
  return 1U;
}

static uint8_t Measure_CalcInputOhm(uint32_t *rin_ohm)
{
  uint32_t source_vpp = measure_last.vpp_mv[0];
  uint32_t input_vpp = measure_last.vpp_mv[1];
  uint32_t drop_vpp;

  if ((measure_last.valid == 0U) || (source_vpp <= input_vpp) || (input_vpp == 0U)) {
    return 0U;
  }

  drop_vpp = source_vpp - input_vpp;
  *rin_ohm = (uint32_t)((((uint64_t)MEASURE_INPUT_SERIES_OHM) * input_vpp +
                         (drop_vpp / 2UL)) / drop_vpp);
  return 1U;
}

static uint8_t Measure_CalcOutputOhm(uint32_t *rout_ohm)
{
  if ((measure_open_valid == 0U) || (measure_load_valid == 0U) ||
      (measure_load_vpp_mv == 0U) || (measure_open_vpp_mv <= measure_load_vpp_mv)) {
    return 0U;
  }

  *rout_ohm = (uint32_t)((((uint64_t)MEASURE_OUTPUT_LOAD_OHM) *
                          (measure_open_vpp_mv - measure_load_vpp_mv) +
                          (measure_load_vpp_mv / 2UL)) / measure_load_vpp_mv);
  return 1U;
}

static void Measure_DrawLine(uint16_t y, const char *text, uint16_t color)
{
  uint32_t tail_x;

  LCD_DrawString(MEASURE_TEXT_X, y, text, color, LCD_COLOR_BLACK, 2U);
  tail_x = (uint32_t)MEASURE_TEXT_X + ((uint32_t)strlen(text) * 12UL);
  if ((tail_x + 1U) < LCD_WIDTH) {
    LCD_FillRect((uint16_t)tail_x, y,
                 (uint16_t)((uint32_t)LCD_WIDTH - tail_x),
                 MEASURE_LINE_HEIGHT, LCD_COLOR_BLACK);
  }
}

void Measure_AppInit(void)
{
  uint8_t i;

  __disable_irq();
  Measure_ResetAccumLocked();
  __enable_irq();

  measure_last.count = 0U;
  measure_last.valid = 0U;
  for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
    measure_last.vpp_mv[i] = 0UL;
  }
  measure_open_vpp_mv = 0UL;
  measure_load_vpp_mv = 0UL;
  measure_open_valid = 0U;
  measure_load_valid = 0U;
  measure_capture_load_next = 0U;
}

void Measure_AppAccumulate(const int16_t samples[AD7606B_CHANNEL_COUNT])
{
  uint8_t i;

  if (samples == NULL) {
    return;
  }

  if (measure_count == 0U) {
    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
      measure_min_raw[i] = samples[i];
      measure_max_raw[i] = samples[i];
    }
  } else {
    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
      if (samples[i] < measure_min_raw[i]) {
        measure_min_raw[i] = samples[i];
      }
      if (samples[i] > measure_max_raw[i]) {
        measure_max_raw[i] = samples[i];
      }
    }
  }

  if (measure_count < UINT32_MAX) {
    measure_count++;
  }
}

void Measure_AppHandleKey(UI_KeyEvent event)
{
  if (event == UI_KEY_EVENT_KEY0_SHORT) {
    Measure_UpdateSnapshot();
    if (measure_last.valid == 0U) {
      return;
    }
    if (measure_capture_load_next == 0U) {
      measure_open_vpp_mv = measure_last.vpp_mv[MEASURE_GAIN_OUTPUT_CH];
      measure_open_valid = 1U;
      measure_load_valid = 0U;
      measure_capture_load_next = 1U;
    } else {
      measure_load_vpp_mv = measure_last.vpp_mv[MEASURE_GAIN_OUTPUT_CH];
      measure_load_valid = 1U;
      measure_capture_load_next = 0U;
    }
  } else if (event == UI_KEY_EVENT_KEY1_SHORT) {
    measure_open_vpp_mv = 0UL;
    measure_load_vpp_mv = 0UL;
    measure_open_valid = 0U;
    measure_load_valid = 0U;
    measure_capture_load_next = 0U;
  }
}

void Measure_AppDraw(uint32_t sample_rate_hz, uint32_t fail_count)
{
  char text[MEASURE_LINE_CHARS];
  char value[18];
  uint32_t gain_milli;
  uint32_t rin_ohm;
  uint32_t rout_ohm;
  uint8_t gain_valid;
  uint8_t rin_valid;
  uint8_t rout_valid;

  Measure_UpdateSnapshot();

  (void)snprintf(text, sizeof(text), "MEAS SPS:%lu",
                 (unsigned long)sample_rate_hz);
  Measure_DrawLine(78U, text, LCD_COLOR_GREEN);
  (void)snprintf(text, sizeof(text), "C1:VS C2:VIN C3:OUT");
  Measure_DrawLine(102U, text, LCD_COLOR_WHITE);

  (void)snprintf(text, sizeof(text), "V1:%lu V2:%lu mVpp",
                 (unsigned long)measure_last.vpp_mv[0],
                 (unsigned long)measure_last.vpp_mv[1]);
  Measure_DrawLine(126U, text, LCD_COLOR_WHITE);
  (void)snprintf(text, sizeof(text), "V3:%lu N:%lu E:%lu",
                 (unsigned long)measure_last.vpp_mv[2],
                 (unsigned long)measure_last.count,
                 (unsigned long)fail_count);
  Measure_DrawLine(150U, text, LCD_COLOR_WHITE);

  gain_valid = Measure_CalcGainMilli(&gain_milli);
  Measure_FormatMilli(value, sizeof(value), "GAIN:", gain_milli, gain_valid);
  (void)snprintf(text, sizeof(text), "%s", value);
  Measure_DrawLine(174U, text, gain_valid ? LCD_COLOR_WHITE : LCD_COLOR_RED);

  rin_valid = Measure_CalcInputOhm(&rin_ohm);
  Measure_FormatOhm(value, sizeof(value), "RIN:", rin_ohm, rin_valid);
  (void)snprintf(text, sizeof(text), "%s RS:%luk", value,
                 (unsigned long)(MEASURE_INPUT_SERIES_OHM / 1000UL));
  Measure_DrawLine(198U, text, rin_valid ? LCD_COLOR_WHITE : LCD_COLOR_RED);

  (void)snprintf(text, sizeof(text), "K0:%s K1:CLR",
                 (measure_capture_load_next != 0U) ? "LOAD" : "OPEN");
  Measure_DrawLine(222U, text, LCD_COLOR_GREEN);
  (void)snprintf(text, sizeof(text), "V0:%lu VL:%lu",
                 (unsigned long)measure_open_vpp_mv,
                 (unsigned long)measure_load_vpp_mv);
  Measure_DrawLine(246U, text, LCD_COLOR_WHITE);

  rout_valid = Measure_CalcOutputOhm(&rout_ohm);
  Measure_FormatOhm(value, sizeof(value), "ROUT:", rout_ohm, rout_valid);
  (void)snprintf(text, sizeof(text), "%s RL:%luR", value,
                 (unsigned long)MEASURE_OUTPUT_LOAD_OHM);
  Measure_DrawLine(270U, text, rout_valid ? LCD_COLOR_WHITE : LCD_COLOR_RED);
}
