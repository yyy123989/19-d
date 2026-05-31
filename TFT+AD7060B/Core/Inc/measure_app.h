#ifndef MEASURE_APP_H
#define MEASURE_APP_H

#include "ad7606b.h"
#include "ui_menu.h"

#include <stdint.h>

#define MEASURE_INPUT_SERIES_OHM 10000UL
#define MEASURE_OUTPUT_LOAD_OHM 1000UL

void Measure_AppInit(void);
void Measure_AppAccumulate(const int16_t samples[AD7606B_CHANNEL_COUNT]);
void Measure_AppHandleKey(UI_KeyEvent event);
void Measure_AppInvalidate(void);
void Measure_AppDraw(uint32_t sample_rate_hz, uint32_t fail_count);

#endif
