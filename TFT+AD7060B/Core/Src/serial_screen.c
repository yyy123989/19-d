#include "serial_screen.h"

#include "usart.h"

#include <stdio.h>
#include <string.h>

#define SERIAL_SCREEN_COMMAND_CHARS 48U
#define SERIAL_SCREEN_FIELD_COUNT 9U
#define SERIAL_SCREEN_FIELDS_PER_TICK 3U
#define SERIAL_SCREEN_CACHE_TEXT_CHARS 32U

static uint32_t serial_screen_last_tick;
static uint8_t serial_screen_field;
static char serial_screen_cache[SERIAL_SCREEN_FIELD_COUNT][SERIAL_SCREEN_CACHE_TEXT_CHARS];
static uint8_t serial_screen_cache_valid[SERIAL_SCREEN_FIELD_COUNT];

static uint8_t SerialScreen_SendRaw(const char *command)
{
  uint8_t packet[SERIAL_SCREEN_COMMAND_CHARS + 3U];
  uint16_t len = 0U;

  if (command == NULL) {
    return 0U;
  }

  while ((command[len] != '\0') && (len < (SERIAL_SCREEN_COMMAND_CHARS - 1U))) {
    len++;
  }

  /* 串口屏指令必须以 0xFF 0xFF 0xFF 结束。 */
  (void)memcpy(packet, command, len);
  packet[len++] = 0xFFU;
  packet[len++] = 0xFFU;
  packet[len++] = 0xFFU;
  return USART2_Write(packet, len);
}

static uint8_t SerialScreen_SetText(const char *obj, const char *text)
{
  char command[SERIAL_SCREEN_COMMAND_CHARS];

  if ((obj == NULL) || (text == NULL)) {
    return 0U;
  }

  (void)snprintf(command, sizeof(command), "%s.txt=\"%s\"", obj, text);
  return SerialScreen_SendRaw(command);
}

static uint8_t SerialScreen_TextSame(uint8_t cache_index, const char *text)
{
  if ((cache_index >= SERIAL_SCREEN_FIELD_COUNT) || (text == NULL)) {
    return 0U;
  }
  if ((serial_screen_cache_valid[cache_index] != 0U) &&
      (strncmp(serial_screen_cache[cache_index], text, SERIAL_SCREEN_CACHE_TEXT_CHARS) == 0)) {
    return 1U;
  }
  return 0U;
}

static void SerialScreen_StoreText(uint8_t cache_index, const char *text)
{
  if ((cache_index >= SERIAL_SCREEN_FIELD_COUNT) || (text == NULL)) {
    return;
  }
  (void)snprintf(serial_screen_cache[cache_index], SERIAL_SCREEN_CACHE_TEXT_CHARS, "%s", text);
  serial_screen_cache_valid[cache_index] = 1U;
}

static void SerialScreen_SetChannel(uint8_t channel, int16_t raw)
{
  char obj[5];
  char value[18];
  char text[28];

  if (channel >= AD7606B_CHANNEL_COUNT) {
    return;
  }

  (void)snprintf(obj, sizeof(obj), "va%u", (unsigned int)channel);
  (void)AD7606B_FormatVoltage(value, sizeof(value), raw, 1U);
  (void)snprintf(text, sizeof(text), "CH%u:%s", (unsigned int)(channel + 1U), value);
  if (SerialScreen_TextSame(channel, text) == 0U) {
    if (SerialScreen_SetText(obj, text) != 0U) {
      SerialScreen_StoreText(channel, text);
    }
  }
}

void SerialScreen_Init(void)
{
  serial_screen_last_tick = 0U;
  serial_screen_field = 0U;
  (void)memset(serial_screen_cache_valid, 0, sizeof(serial_screen_cache_valid));

  SerialScreen_SendRaw("bkcmd=0");
  SerialScreen_SetText("t0", "DDS AD7606B");
}

void SerialScreen_Tick(uint32_t now_ms,
                       const int16_t samples[AD7606B_CHANNEL_COUNT],
                       uint8_t sample_valid,
                       uint32_t sample_rate_hz,
                       uint32_t fail_count)
{
  char text[32];
  uint8_t update;

  if (samples == NULL) {
    return;
  }
  if ((uint32_t)(now_ms - serial_screen_last_tick) < SERIAL_SCREEN_UPDATE_INTERVAL_MS) {
    return;
  }
  serial_screen_last_tick = now_ms;

  /* 每个 tick 只更新几个字段，避免 USART2 队列占满后拖慢主循环。 */
  for (update = 0U; update < SERIAL_SCREEN_FIELDS_PER_TICK; update++) {
    if (serial_screen_field < AD7606B_CHANNEL_COUNT) {
      SerialScreen_SetChannel(serial_screen_field, samples[serial_screen_field]);
    } else {
      (void)snprintf(text, sizeof(text), "AD:%s SPS:%lu E:%lu",
                     (sample_valid != 0U) ? "OK" : "ERR",
                     (unsigned long)sample_rate_hz,
                     (unsigned long)fail_count);
      if (SerialScreen_TextSame(AD7606B_CHANNEL_COUNT, text) == 0U) {
        if (SerialScreen_SetText("t1", text) != 0U) {
          SerialScreen_StoreText(AD7606B_CHANNEL_COUNT, text);
        }
      }
    }

    serial_screen_field++;
    if (serial_screen_field >= SERIAL_SCREEN_FIELD_COUNT) {
      serial_screen_field = 0U;
    }
  }
}
