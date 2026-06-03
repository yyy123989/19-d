#include "serial_wave.h"

#define SERIAL_WAVE_LINE_CHARS 112U

static uint32_t serial_wave_last_tick;

void SerialWave_Init(void)
{
  serial_wave_last_tick = 0U;
}

void SerialWave_Tick(uint32_t now_ms, const int16_t samples[AD7606B_CHANNEL_COUNT])
{
  char line[SERIAL_WAVE_LINE_CHARS];
  uint32_t offset = 0U;
  uint8_t i;

  if (samples == NULL) {
    return;
  }

  /* USART1 输出给 SerialPlot，每行 8 个电压值，用逗号分隔。 */
  if ((uint32_t)(now_ms - serial_wave_last_tick) < SERIAL_WAVE_OUTPUT_INTERVAL_MS) {
    return;
  }

  for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
    if ((offset + 12U) >= sizeof(line)) {
      return;
    }

    offset += AD7606B_FormatVoltage(&line[offset], (uint32_t)sizeof(line) - offset, samples[i], 0U);

    if (i < (AD7606B_CHANNEL_COUNT - 1U)) {
      line[offset++] = ',';
    }
  }

  if ((offset + 2U) >= sizeof(line)) {
    return;
  }

  line[offset++] = '\r';
  line[offset++] = '\n';

  if (USART1_Write((const uint8_t *)line, (uint16_t)offset) != 0U) {
    serial_wave_last_tick = now_ms;
  }
}
