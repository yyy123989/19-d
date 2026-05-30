#ifndef SERIAL_WAVE_H
#define SERIAL_WAVE_H

#include "ad7606b.h"
#include "usart.h"

#include <stdint.h>

#define SERIAL_WAVE_OUTPUT_INTERVAL_MS 20U

void SerialWave_Init(void);
void SerialWave_Tick(uint32_t now_ms, const int16_t samples[AD7606B_CHANNEL_COUNT]);

#endif
