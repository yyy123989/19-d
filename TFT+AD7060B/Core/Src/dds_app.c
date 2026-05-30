#include "dds_app.h"

#include "ad9910.h"
#include "ad9910_ram_tables.h"
#include "lcd.h"

#include <stdio.h>
#include <string.h>

#define DDS_ARRAY_COUNT(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))
#define DDS_MAX_FREQ_HZ 420000000UL
#define DDS_OSK_TOGGLE_MS 500U
#define DDS_DRAW_LINE_CHARS 40U
#define DDS_DRAW_CACHE_LINES 18U
#define DDS_DRAW_TEXT_X 4U
#define DDS_DRAW_CHAR_STEP_PIXELS 12U
#define DDS_DRAW_LINE_HEIGHT 20U

typedef enum {
  DDS_PAGE_HOME = 0,
  DDS_PAGE_SINGLE,
  DDS_PAGE_RAM,
  DDS_PAGE_DRG,
  DDS_PAGE_OSK,
  DDS_PAGE_CAL,
  DDS_PAGE_AD_VIEW
} DDS_Page;

typedef enum {
  DDS_MODE_SINGLE = 0,
  DDS_MODE_RAM,
  DDS_MODE_DRG,
  DDS_MODE_OSK,
  DDS_MODE_CAL,
  DDS_MODE_AD_VIEW,
  DDS_MODE_COUNT
} DDS_Mode;

typedef enum {
  DDS_OSK_OFF = 0,
  DDS_OSK_MANUAL,
  DDS_OSK_AUTO,
  DDS_OSK_COUNT
} DDS_OskMode;

typedef enum {
  DDS_RAM_TABLE_AMP = 0,
  DDS_RAM_TABLE_FRE,
  DDS_RAM_TABLE_PHA,
  DDS_RAM_TABLE_TRI,
  DDS_RAM_TABLE_SQR,
  DDS_RAM_TABLE_SINC,
  DDS_RAM_TABLE_COUNT
} DDS_RamTable;

typedef struct {
  DDS_Page page;
  DDS_Mode home_mode;
  DDS_Mode active_mode;
  uint8_t field;
  uint8_t single_freq;
  uint8_t single_amp;
  uint8_t single_phase;
  uint8_t ram_mode;
  uint8_t ram_table;
  uint8_t drg_lower;
  uint8_t drg_upper;
  uint8_t drg_step;
  uint8_t drg_rate;
  uint8_t drg_dir;
  uint8_t drg_rising;
  uint8_t osk_mode;
  uint8_t osk_output;
  uint32_t osk_tick_ms;
  uint8_t cal_offset;
  uint8_t cal_gain;
  uint8_t cal_phase;
} DDS_AppState;

typedef struct {
  uint16_t y;
  uint16_t color;
  char text[DDS_DRAW_LINE_CHARS];
  uint8_t valid;
} DDS_DrawCacheLine;

static const char *dds_mode_names[] = {
  "SINGLE", "RAM", "DRG", "OSK", "CAL", "AD VIEW"
};

static const uint32_t dds_freq_table[] = {
  0UL, 1UL, 10UL, 100UL, 500UL, 1000UL, 5000UL, 10000UL, 50000UL,
  100000UL, 500000UL, 1000000UL, 5000000UL, 10000000UL, 50000000UL,
  100000000UL, 200000000UL, 300000000UL, 380000000UL, 400000000UL, 420000000UL
};

static const uint16_t dds_amp_table[] = {
  0U, 1023U, 2047U, 4095U, 8191U, 9215U, 10239U, 11263U, 12287U,
  13311U, 14335U, 15359U, 16383U
};

static const uint16_t dds_phase_table[] = {0U, 90U, 180U, 270U, 360U};
static const uint32_t dds_offset_table[] = {0UL, 1000UL, 2000UL, 4000UL, 8000UL, 10000UL, 20000UL, 40000UL, 60000UL};
static const uint32_t dds_drg_step_table[] = {1UL, 10UL, 50UL, 100UL, 500UL, 1000UL, 5000UL, 10000UL, 50000UL};
static const uint16_t dds_drg_rate_table[] = {1U, 10U, 100U, 1000U, 5000U, 10000U, 20000U, 50000U};

static const AD9910_RamMode dds_ram_mode_table[] = {
  AD9910_RAM_MODE_DIRECT,
  AD9910_RAM_MODE_RAMP_UP,
  AD9910_RAM_MODE_BIDIRECTION,
  AD9910_RAM_MODE_CONT_BIDIRECTION,
  AD9910_RAM_MODE_CONT_RECIRC,
  AD9910_RAM_MODE_TRIANGLE_SQUARE
};

static const char *dds_ram_mode_names[] = {
  "DIRECT", "RAMP UP", "BIDIR", "CONT BI", "RECIRC", "TRI/SQR"
};

static const char *dds_ram_table_names[] = {
  "AMP", "FRE", "PHA", "TRI", "SQR", "SINC"
};

static const char *dds_drg_dir_names[] = {"UP", "DOWN", "BOTH"};
static const char *dds_osk_names[] = {"OFF", "MANUAL", "AUTO"};
static DDS_AppState dds_app;
static char line[DDS_DRAW_LINE_CHARS];
static DDS_DrawCacheLine draw_cache[DDS_DRAW_CACHE_LINES];
static DDS_Page draw_cache_page = DDS_PAGE_HOME;
static uint8_t draw_cache_valid;

static void DDS_DrawCacheInvalidate(void)
{
  uint8_t i;

  for (i = 0U; i < DDS_DRAW_CACHE_LINES; i++) {
    draw_cache[i].valid = 0U;
  }
}

static uint8_t DDS_DrawCacheChanged(uint16_t y, const char *text, uint16_t color)
{
  uint8_t i;
  uint8_t empty = DDS_DRAW_CACHE_LINES;

  for (i = 0U; i < DDS_DRAW_CACHE_LINES; i++) {
    if (draw_cache[i].valid == 0U) {
      if (empty == DDS_DRAW_CACHE_LINES) {
        empty = i;
      }
      continue;
    }

    if (draw_cache[i].y == y) {
      if ((draw_cache[i].color == color) && (strcmp(draw_cache[i].text, text) == 0)) {
        return 0U;
      }
      draw_cache[i].color = color;
      (void)snprintf(draw_cache[i].text, sizeof(draw_cache[i].text), "%s", text);
      return 1U;
    }
  }

  if (empty >= DDS_DRAW_CACHE_LINES) {
    empty = 0U;
  }

  draw_cache[empty].valid = 1U;
  draw_cache[empty].y = y;
  draw_cache[empty].color = color;
  (void)snprintf(draw_cache[empty].text, sizeof(draw_cache[empty].text), "%s", text);
  return 1U;
}

static uint32_t DDS_AppCalibratedFreq(uint32_t base_hz)
{
  uint32_t offset = dds_offset_table[dds_app.cal_offset];

  if (base_hz > (DDS_MAX_FREQ_HZ - offset)) {
    return DDS_MAX_FREQ_HZ;
  }
  return base_hz + offset;
}

static uint16_t DDS_AppCalibratedAmp(uint16_t base_amp)
{
  uint32_t amp = ((uint32_t)base_amp * (uint32_t)(8U + dds_app.cal_gain)) / 16UL;

  if (amp > AD9910_MAX_AMPLITUDE) {
    amp = AD9910_MAX_AMPLITUDE;
  }
  return (uint16_t)amp;
}

static uint16_t DDS_AppCalibratedPhase(uint16_t base_phase)
{
  return (uint16_t)((base_phase + dds_phase_table[dds_app.cal_phase]) % 360U);
}

static uint8_t DDS_AppFieldCount(void)
{
  switch (dds_app.page) {
    case DDS_PAGE_SINGLE:
      return 3U;
    case DDS_PAGE_RAM:
      return 2U;
    case DDS_PAGE_DRG:
      return 5U;
    case DDS_PAGE_OSK:
      return 1U;
    case DDS_PAGE_CAL:
      return 3U;
    default:
      return 1U;
  }
}

static DDS_Page DDS_AppPageFromMode(DDS_Mode mode)
{
  switch (mode) {
    case DDS_MODE_RAM:
      return DDS_PAGE_RAM;
    case DDS_MODE_DRG:
      return DDS_PAGE_DRG;
    case DDS_MODE_OSK:
      return DDS_PAGE_OSK;
    case DDS_MODE_CAL:
      return DDS_PAGE_CAL;
    case DDS_MODE_AD_VIEW:
      return DDS_PAGE_AD_VIEW;
    case DDS_MODE_SINGLE:
    default:
      return DDS_PAGE_SINGLE;
  }
}

static void DDS_AppNextField(void)
{
  uint8_t count = DDS_AppFieldCount();

  if (count == 0U) {
    dds_app.field = 0U;
  } else {
    dds_app.field = (uint8_t)((dds_app.field + 1U) % count);
  }
}

static void DDS_AppIncrementSelected(void)
{
  switch (dds_app.page) {
    case DDS_PAGE_SINGLE:
      if (dds_app.field == 0U) {
        dds_app.single_freq = (uint8_t)((dds_app.single_freq + 1U) % DDS_ARRAY_COUNT(dds_freq_table));
      } else if (dds_app.field == 1U) {
        dds_app.single_amp = (uint8_t)((dds_app.single_amp + 1U) % DDS_ARRAY_COUNT(dds_amp_table));
      } else {
        dds_app.single_phase = (uint8_t)((dds_app.single_phase + 1U) % DDS_ARRAY_COUNT(dds_phase_table));
      }
      break;

    case DDS_PAGE_RAM:
      if (dds_app.field == 0U) {
        dds_app.ram_mode = (uint8_t)((dds_app.ram_mode + 1U) % DDS_ARRAY_COUNT(dds_ram_mode_table));
      } else {
        dds_app.ram_table = (uint8_t)((dds_app.ram_table + 1U) % DDS_RAM_TABLE_COUNT);
      }
      break;

    case DDS_PAGE_DRG:
      if (dds_app.field == 0U) {
        dds_app.drg_lower = (uint8_t)((dds_app.drg_lower + 1U) % DDS_ARRAY_COUNT(dds_freq_table));
      } else if (dds_app.field == 1U) {
        dds_app.drg_upper = (uint8_t)((dds_app.drg_upper + 1U) % DDS_ARRAY_COUNT(dds_freq_table));
      } else if (dds_app.field == 2U) {
        dds_app.drg_step = (uint8_t)((dds_app.drg_step + 1U) % DDS_ARRAY_COUNT(dds_drg_step_table));
      } else if (dds_app.field == 3U) {
        dds_app.drg_rate = (uint8_t)((dds_app.drg_rate + 1U) % DDS_ARRAY_COUNT(dds_drg_rate_table));
      } else {
        dds_app.drg_dir = (uint8_t)((dds_app.drg_dir + 1U) % DDS_ARRAY_COUNT(dds_drg_dir_names));
      }
      break;

    case DDS_PAGE_OSK:
      dds_app.osk_mode = (uint8_t)((dds_app.osk_mode + 1U) % DDS_OSK_COUNT);
      break;

    case DDS_PAGE_CAL:
      if (dds_app.field == 0U) {
        dds_app.cal_offset = (uint8_t)((dds_app.cal_offset + 1U) % DDS_ARRAY_COUNT(dds_offset_table));
      } else if (dds_app.field == 1U) {
        dds_app.cal_gain = (uint8_t)((dds_app.cal_gain + 1U) % 16U);
      } else {
        dds_app.cal_phase = (uint8_t)((dds_app.cal_phase + 1U) % DDS_ARRAY_COUNT(dds_phase_table));
      }
      break;

    default:
      break;
  }
}

static void DDS_AppApplySingle(void)
{
  uint32_t freq = DDS_AppCalibratedFreq(dds_freq_table[dds_app.single_freq]);
  uint16_t amp = DDS_AppCalibratedAmp(dds_amp_table[dds_app.single_amp]);
  uint16_t phase = DDS_AppCalibratedPhase(dds_phase_table[dds_app.single_phase]);

  AD9910_SetOsk(0U);
  AD9910_SetSingleTone(freq, amp, phase);
  AD9910_SelectProfile0();
  dds_app.active_mode = DDS_MODE_SINGLE;
}

static void DDS_AppApplyRam(void)
{
  AD9910_RamTarget target = AD9910_RAM_TARGET_AMPLITUDE;
  const uint32_t *table = AD9910_RAM_AMP;
  uint32_t ram_freq = DDS_AppCalibratedFreq(1000UL);
  uint16_t ram_amp = DDS_AppCalibratedAmp(AD9910_MAX_AMPLITUDE);

  switch ((DDS_RamTable)dds_app.ram_table) {
    case DDS_RAM_TABLE_FRE:
      target = AD9910_RAM_TARGET_FREQUENCY;
      table = AD9910_RAM_FRE;
      break;
    case DDS_RAM_TABLE_PHA:
      target = AD9910_RAM_TARGET_PHASE;
      table = AD9910_RAM_PHA;
      break;
    case DDS_RAM_TABLE_TRI:
      AD9910_SetOsk(0U);
      AD9910_SetRamPlayback(dds_ram_mode_table[dds_app.ram_mode], AD9910_RAM_WAVE_TRIANGLE,
                            ram_freq, ram_amp, 500U);
      dds_app.active_mode = DDS_MODE_RAM;
      return;
    case DDS_RAM_TABLE_SQR:
      AD9910_SetOsk(0U);
      AD9910_SetRamPlayback(dds_ram_mode_table[dds_app.ram_mode], AD9910_RAM_WAVE_SQUARE,
                            ram_freq, ram_amp, 500U);
      dds_app.active_mode = DDS_MODE_RAM;
      return;
    case DDS_RAM_TABLE_SINC:
      AD9910_SetOsk(0U);
      AD9910_SetRamPlayback(dds_ram_mode_table[dds_app.ram_mode], AD9910_RAM_WAVE_SINC,
                            ram_freq, ram_amp, 500U);
      dds_app.active_mode = DDS_MODE_RAM;
      return;
    case DDS_RAM_TABLE_AMP:
    default:
      target = AD9910_RAM_TARGET_AMPLITUDE;
      table = AD9910_RAM_AMP;
      break;
  }

  AD9910_SetOsk(0U);
  AD9910_SetRamTablePlayback(dds_ram_mode_table[dds_app.ram_mode], target, table, AD9910_RAM_TABLE_POINTS);
  dds_app.active_mode = DDS_MODE_RAM;
}

static void DDS_AppApplyDrg(void)
{
  uint32_t lower = DDS_AppCalibratedFreq(dds_freq_table[dds_app.drg_lower]);
  uint32_t upper = DDS_AppCalibratedFreq(dds_freq_table[dds_app.drg_upper]);
  uint32_t step = dds_drg_step_table[dds_app.drg_step];
  uint16_t rate = dds_drg_rate_table[dds_app.drg_rate];

  AD9910_SetOsk(0U);
  AD9910_SetDigitalRampHold(0U);
  AD9910_SetDigitalRampFrequency(lower, upper, step, step, rate, rate);
  if (dds_app.drg_dir == 1U) {   /* UI "DOWN": DRCTL=0 → negative sweep */
    dds_app.drg_rising = 0U;
  } else {                       /* UI "UP"(0) or "BOTH"(2): DRCTL=1 → positive sweep */
    dds_app.drg_rising = 1U;
  }
  AD9910_SetDigitalRampDirection(dds_app.drg_rising);
  dds_app.active_mode = DDS_MODE_DRG;
}

static void DDS_AppApplyOsk(void)
{
  AD9910_SetSingleTone(1000UL, AD9910_MAX_AMPLITUDE, 0U);
  AD9910_SelectProfile0();
  dds_app.osk_output = (dds_app.osk_mode == DDS_OSK_OFF) ? 0U : 1U;
  AD9910_SetOsk(dds_app.osk_output);
  dds_app.osk_tick_ms = 0U;
  dds_app.active_mode = DDS_MODE_OSK;
}

static void DDS_AppApplyCurrent(void)
{
  switch (dds_app.page) {
    case DDS_PAGE_RAM:
      DDS_AppApplyRam();
      break;
    case DDS_PAGE_DRG:
      DDS_AppApplyDrg();
      break;
    case DDS_PAGE_OSK:
      DDS_AppApplyOsk();
      break;
    case DDS_PAGE_CAL:
    case DDS_PAGE_SINGLE:
    default:
      DDS_AppApplySingle();
      break;
  }
}

static void DDS_DrawLine(uint16_t y, const char *text, uint16_t color)
{
  uint32_t tail_x;

  if (DDS_DrawCacheChanged(y, text, color) == 0U) {
    return;
  }

  LCD_DrawString(DDS_DRAW_TEXT_X, y, text, color, LCD_COLOR_BLACK, 2U);
  tail_x = (uint32_t)DDS_DRAW_TEXT_X + ((uint32_t)strlen(text) * DDS_DRAW_CHAR_STEP_PIXELS);
  if ((tail_x + 1U) < LCD_WIDTH) {
    LCD_FillRect((uint16_t)tail_x,
                 y,
                 (uint16_t)((uint32_t)LCD_WIDTH - tail_x),
                 DDS_DRAW_LINE_HEIGHT,
                 LCD_COLOR_BLACK);
  }
}

static void DDS_DrawTopBar(const int16_t samples[AD7606B_CHANNEL_COUNT],
                           uint8_t sample_valid,
                           uint32_t sample_rate_hz,
                           uint32_t fail_count)
{
  const char *ad_state = (sample_valid != 0U) ? "OK" : "ERR";
  char voltage[12];

  (void)snprintf(line, sizeof(line), "M:%s F:%lu", dds_mode_names[dds_app.home_mode],
                 (unsigned long)dds_freq_table[dds_app.single_freq]);
  DDS_DrawLine(0U, line, LCD_COLOR_GREEN);
  (void)snprintf(line, sizeof(line), "AD:%s SPS:%lu", ad_state, (unsigned long)sample_rate_hz);
  DDS_DrawLine(22U, line, (sample_valid != 0U) ? LCD_COLOR_WHITE : LCD_COLOR_RED);
  (void)AD7606B_FormatVoltage(voltage, sizeof(voltage), samples[0], 1U);
  (void)snprintf(line, sizeof(line), "E:%lu CH1:%s", (unsigned long)fail_count, voltage);
  DDS_DrawLine(44U, line, LCD_COLOR_WHITE);
}

void DDS_AppInit(void)
{
  dds_app.page = DDS_PAGE_HOME;
  dds_app.home_mode = DDS_MODE_SINGLE;
  dds_app.active_mode = DDS_MODE_SINGLE;
  dds_app.field = 0U;
  dds_app.single_freq = 5U;
  dds_app.single_amp = (uint8_t)(DDS_ARRAY_COUNT(dds_amp_table) - 1U);
  dds_app.single_phase = 0U;
  dds_app.ram_mode = 4U;
  dds_app.ram_table = DDS_RAM_TABLE_TRI;
  dds_app.drg_lower = 5U;
  dds_app.drg_upper = 7U;
  dds_app.drg_step = 1U;
  dds_app.drg_rate = 5U;
  dds_app.drg_dir = 2U;
  dds_app.drg_rising = 1U;
  dds_app.osk_mode = DDS_OSK_OFF;
  dds_app.osk_output = 0U;
  dds_app.osk_tick_ms = 0U;
  dds_app.cal_offset = 0U;
  dds_app.cal_gain = 8U;
  dds_app.cal_phase = 0U;
  AD9910_Init();
  DDS_AppApplySingle();
}

void DDS_AppHandleKey(UI_KeyEvent event)
{
  if (event == UI_KEY_EVENT_NONE) {
    return;
  }

  if (dds_app.page == DDS_PAGE_HOME) {
    if ((event == UI_KEY_EVENT_KEY1_SHORT) || (event == UI_KEY_EVENT_KEY0_SHORT)) {
      dds_app.home_mode = (DDS_Mode)((dds_app.home_mode + 1U) % DDS_MODE_COUNT);
    } else if (event == UI_KEY_EVENT_KEY0_LONG) {
      dds_app.page = DDS_AppPageFromMode(dds_app.home_mode);
      dds_app.field = 0U;
    } else if (event == UI_KEY_EVENT_KEY1_LONG) {
      dds_app.page = DDS_PAGE_AD_VIEW;
      dds_app.home_mode = DDS_MODE_AD_VIEW;
      dds_app.field = 0U;
    }
    return;
  }

  if (dds_app.page == DDS_PAGE_AD_VIEW) {
    if ((event == UI_KEY_EVENT_KEY1_LONG) || (event == UI_KEY_EVENT_KEY0_LONG)) {
      dds_app.page = DDS_PAGE_HOME;
      dds_app.home_mode = dds_app.active_mode;
      dds_app.field = 0U;
    }
    return;
  }

  if (event == UI_KEY_EVENT_KEY1_SHORT) {
    DDS_AppNextField();
  } else if (event == UI_KEY_EVENT_KEY0_SHORT) {
    DDS_AppIncrementSelected();
  } else if (event == UI_KEY_EVENT_KEY1_LONG) {
    dds_app.page = DDS_PAGE_HOME;
    dds_app.home_mode = dds_app.active_mode;
    dds_app.field = 0U;
  } else if (event == UI_KEY_EVENT_KEY0_LONG) {
    DDS_AppApplyCurrent();
  }
}

void DDS_AppTick(uint32_t now_ms)
{
  /* OSK AUTO toggles regardless of current page to avoid freezing output. */
  if ((dds_app.osk_mode == DDS_OSK_AUTO) &&
      ((uint32_t)(now_ms - dds_app.osk_tick_ms) >= DDS_OSK_TOGGLE_MS)) {
    dds_app.osk_tick_ms = now_ms;
    dds_app.osk_output ^= 1U;
    AD9910_SetOsk(dds_app.osk_output);
  }
}

uint8_t DDS_AppIsDrgMode(void)
{
  return ((dds_app.active_mode == DDS_MODE_DRG) && (dds_app.drg_dir == 2U)) ? 1U : 0U;
}

void DDS_AppDrgTimerTick(void)
{
  if (DDS_AppIsDrgMode() != 0U) {
    dds_app.drg_rising ^= 1U;
    AD9910_SetDigitalRampDirection(dds_app.drg_rising);
  }
}

void DDS_AppDraw(const int16_t samples[AD7606B_CHANNEL_COUNT],
                 uint8_t sample_valid,
                 uint32_t sample_rate_hz,
                 uint32_t fail_count)
{
  uint8_t i;
  char voltage[12];

  if ((draw_cache_valid == 0U) || (draw_cache_page != dds_app.page)) {
    LCD_FillRect(0U, 0U, 240U, 320U, LCD_COLOR_BLACK);
    DDS_DrawCacheInvalidate();
    draw_cache_page = dds_app.page;
    draw_cache_valid = 1U;
  }

  if (dds_app.page == DDS_PAGE_AD_VIEW) {
    (void)snprintf(line, sizeof(line), "AD VIEW SPS:%lu", (unsigned long)sample_rate_hz);
    DDS_DrawLine(0U, line, LCD_COLOR_GREEN);
    (void)snprintf(line, sizeof(line), "AD:%s ERR:%lu",
                   (sample_valid != 0U) ? "OK" : "FAIL", (unsigned long)fail_count);
    DDS_DrawLine(24U, line, (sample_valid != 0U) ? LCD_COLOR_WHITE : LCD_COLOR_RED);
    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
      (void)AD7606B_FormatVoltage(voltage, sizeof(voltage), samples[i], 1U);
      (void)snprintf(line, sizeof(line), "CH%u:%s", (unsigned int)(i + 1U), voltage);
      DDS_DrawLine((uint16_t)(54U + (i * 26U)), line, LCD_COLOR_WHITE);
    }
    return;
  }

  DDS_DrawTopBar(samples, sample_valid, sample_rate_hz, fail_count);

  if (dds_app.page == DDS_PAGE_HOME) {
    for (i = 0U; i < DDS_MODE_COUNT; i++) {
      (void)snprintf(line, sizeof(line), "%c M%u %s",
                     (i == (uint8_t)dds_app.home_mode) ? '>' : ' ',
                     (unsigned int)(i + 1U), dds_mode_names[i]);
      DDS_DrawLine((uint16_t)(78U + (i * 24U)), line,
                   (i == (uint8_t)dds_app.home_mode) ? LCD_COLOR_GREEN : LCD_COLOR_WHITE);
    }
  } else if (dds_app.page == DDS_PAGE_SINGLE) {
    (void)snprintf(line, sizeof(line), "%c F:%lu", (dds_app.field == 0U) ? '>' : ' ',
                   (unsigned long)dds_freq_table[dds_app.single_freq]);
    DDS_DrawLine(78U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c A:%u", (dds_app.field == 1U) ? '>' : ' ',
                   (unsigned int)dds_amp_table[dds_app.single_amp]);
    DDS_DrawLine(102U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c P:%u", (dds_app.field == 2U) ? '>' : ' ',
                   (unsigned int)dds_phase_table[dds_app.single_phase]);
    DDS_DrawLine(126U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "CAL O:%lu G:%u",
                   (unsigned long)dds_offset_table[dds_app.cal_offset],
                   (unsigned int)dds_app.cal_gain);
    DDS_DrawLine(154U, line, LCD_COLOR_GREEN);
  } else if (dds_app.page == DDS_PAGE_RAM) {
    (void)snprintf(line, sizeof(line), "%c MODE:%s", (dds_app.field == 0U) ? '>' : ' ',
                   dds_ram_mode_names[dds_app.ram_mode]);
    DDS_DrawLine(78U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c TABLE:%s", (dds_app.field == 1U) ? '>' : ' ',
                   dds_ram_table_names[dds_app.ram_table]);
    DDS_DrawLine(102U, line, LCD_COLOR_WHITE);
  } else if (dds_app.page == DDS_PAGE_DRG) {
    (void)snprintf(line, sizeof(line), "%c L:%lu", (dds_app.field == 0U) ? '>' : ' ',
                   (unsigned long)dds_freq_table[dds_app.drg_lower]);
    DDS_DrawLine(78U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c H:%lu", (dds_app.field == 1U) ? '>' : ' ',
                   (unsigned long)dds_freq_table[dds_app.drg_upper]);
    DDS_DrawLine(102U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c STEP:%lu", (dds_app.field == 2U) ? '>' : ' ',
                   (unsigned long)dds_drg_step_table[dds_app.drg_step]);
    DDS_DrawLine(126U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c RATE:%u", (dds_app.field == 3U) ? '>' : ' ',
                   (unsigned int)dds_drg_rate_table[dds_app.drg_rate]);
    DDS_DrawLine(150U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c DIR:%s", (dds_app.field == 4U) ? '>' : ' ',
                   dds_drg_dir_names[dds_app.drg_dir]);
    DDS_DrawLine(174U, line, LCD_COLOR_WHITE);
  } else if (dds_app.page == DDS_PAGE_OSK) {
    (void)snprintf(line, sizeof(line), "%c OSK:%s", '>', dds_osk_names[dds_app.osk_mode]);
    DDS_DrawLine(78U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "OUT:%u", (unsigned int)dds_app.osk_output);
    DDS_DrawLine(102U, line, LCD_COLOR_GREEN);
  } else if (dds_app.page == DDS_PAGE_CAL) {
    (void)snprintf(line, sizeof(line), "%c OFF:%lu", (dds_app.field == 0U) ? '>' : ' ',
                   (unsigned long)dds_offset_table[dds_app.cal_offset]);
    DDS_DrawLine(78U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c GAIN:%u", (dds_app.field == 1U) ? '>' : ' ',
                   (unsigned int)dds_app.cal_gain);
    DDS_DrawLine(102U, line, LCD_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "%c PH:%u", (dds_app.field == 2U) ? '>' : ' ',
                   (unsigned int)dds_phase_table[dds_app.cal_phase]);
    DDS_DrawLine(126U, line, LCD_COLOR_WHITE);
  }
}
