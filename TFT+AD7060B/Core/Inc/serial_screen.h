#ifndef SERIAL_SCREEN_H
#define SERIAL_SCREEN_H

#include "ad7606b.h"

#include <stdint.h>

#define SERIAL_SCREEN_UPDATE_INTERVAL_MS 50U

void SerialScreen_Init(void);
void SerialScreen_Tick(uint32_t now_ms,
                       const int16_t samples[AD7606B_CHANNEL_COUNT],
                       uint8_t sample_valid,
                       uint32_t sample_rate_hz,
                       uint32_t fail_count);

#endif
