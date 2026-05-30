#ifndef DDS_APP_H
#define DDS_APP_H

#include "ad7606b.h"
#include "ui_menu.h"

#include <stdint.h>

void DDS_AppInit(void);
void DDS_AppHandleKey(UI_KeyEvent event);
void DDS_AppTick(uint32_t now_ms);
void DDS_AppDraw(const int16_t samples[AD7606B_CHANNEL_COUNT],
                 uint8_t sample_valid,
                 uint32_t sample_rate_hz,
                 uint32_t fail_count);
uint8_t DDS_AppIsDrgMode(void);
void DDS_AppDrgTimerTick(void);

#endif
