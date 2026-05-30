#ifndef DDS_APP_H
#define DDS_APP_H

#include "ad7606b.h"
#include "ui_menu.h"

#include <stdint.h>

#define DDS_SINGLE_FIELD_FREQ  (1U << 0)
#define DDS_SINGLE_FIELD_AMP   (1U << 1)
#define DDS_SINGLE_FIELD_PHASE (1U << 2)

void DDS_AppInit(void);
void DDS_AppHandleKey(UI_KeyEvent event);
void DDS_AppTick(uint32_t now_ms);
void DDS_AppDraw(const int16_t samples[AD7606B_CHANNEL_COUNT],
                 uint8_t sample_valid,
                 uint32_t sample_rate_hz,
                 uint32_t fail_count);
uint8_t DDS_AppIsDrgMode(void);
void DDS_AppDrgTimerTick(void);
uint8_t DDS_AppSetSingleCustom(uint32_t freq_hz, uint16_t amplitude, uint16_t phase_deg, uint8_t field_mask);
void DDS_AppGetSingle(uint32_t *freq_hz, uint16_t *amplitude, uint16_t *phase_deg);

#endif
