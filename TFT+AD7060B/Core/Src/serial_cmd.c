#include "serial_cmd.h"

#include "ad9910.h"
#include "dds_app.h"
#include "usart.h"

#include <stdio.h>
#include <string.h>

#define SERIAL_CMD_LINE_CHARS 80U
#define SERIAL_CMD_MAX_FREQ_HZ 420000000UL

typedef enum {
  SERIAL_CMD_PARSE_OK = 0,
  SERIAL_CMD_PARSE_EMPTY,
  SERIAL_CMD_PARSE_QUERY,
  SERIAL_CMD_PARSE_FORMAT,
  SERIAL_CMD_PARSE_RANGE
} SerialCommand_ParseResult;

static char serial_cmd_line[SERIAL_CMD_LINE_CHARS];
static uint8_t serial_cmd_len;

static char SerialCommand_Upper(char ch)
{
  if ((ch >= 'a') && (ch <= 'z')) {
    return (char)(ch - ('a' - 'A'));
  }
  return ch;
}

static uint8_t SerialCommand_KeyEquals(const char *key, const char *literal)
{
  while ((*key != '\0') && (*literal != '\0')) {
    if (SerialCommand_Upper(*key) != *literal) {
      return 0U;
    }
    key++;
    literal++;
  }
  return ((*key == '\0') && (*literal == '\0')) ? 1U : 0U;
}

static uint8_t SerialCommand_IsPrefix(const char *token)
{
  return (uint8_t)(SerialCommand_KeyEquals(token, "DDS") ||
                   SerialCommand_KeyEquals(token, "SINGLE") ||
                   SerialCommand_KeyEquals(token, "S"));
}

static uint8_t SerialCommand_IsQuery(const char *line)
{
  while ((*line == ' ') || (*line == '\t')) {
    line++;
  }
  return (uint8_t)(SerialCommand_KeyEquals(line, "DDS?") ||
                   SerialCommand_KeyEquals(line, "SINGLE?"));
}

static SerialCommand_ParseResult SerialCommand_ParseUint(const char *text,
                                                         uint32_t max_value,
                                                         uint32_t *value)
{
  uint32_t parsed = 0U;
  uint8_t digits = 0U;

  if ((text == 0) || (value == 0)) {
    return SERIAL_CMD_PARSE_FORMAT;
  }

  while (*text != '\0') {
    if ((*text < '0') || (*text > '9')) {
      return SERIAL_CMD_PARSE_FORMAT;
    }
    parsed = (parsed * 10UL) + (uint32_t)(*text - '0');
    if (parsed > max_value) {
      return SERIAL_CMD_PARSE_RANGE;
    }
    digits = 1U;
    text++;
  }

  if (digits == 0U) {
    return SERIAL_CMD_PARSE_FORMAT;
  }

  *value = parsed;
  return SERIAL_CMD_PARSE_OK;
}

static char *SerialCommand_NextToken(char **cursor)
{
  char *start;

  if ((cursor == 0) || (*cursor == 0)) {
    return 0;
  }

  while ((**cursor == ' ') || (**cursor == '\t') || (**cursor == ',')) {
    (*cursor)++;
  }

  if (**cursor == '\0') {
    return 0;
  }

  start = *cursor;
  while ((**cursor != '\0') && (**cursor != ' ') && (**cursor != '\t') && (**cursor != ',')) {
    (*cursor)++;
  }

  if (**cursor != '\0') {
    **cursor = '\0';
    (*cursor)++;
  }

  return start;
}

static SerialCommand_ParseResult SerialCommand_ParseLine(char *line,
                                                         uint32_t *freq_hz,
                                                         uint16_t *amplitude,
                                                         uint16_t *phase_deg,
                                                         uint8_t *field_mask)
{
  char *cursor = line;
  char *token;
  char *equals;
  uint32_t value;
  SerialCommand_ParseResult result;

  if ((line == 0) || (freq_hz == 0) || (amplitude == 0) ||
      (phase_deg == 0) || (field_mask == 0)) {
    return SERIAL_CMD_PARSE_FORMAT;
  }

  if (SerialCommand_IsQuery(line) != 0U) {
    return SERIAL_CMD_PARSE_QUERY;
  }

  *field_mask = 0U;

  token = SerialCommand_NextToken(&cursor);
  if (token == 0) {
    return SERIAL_CMD_PARSE_EMPTY;
  }
  if (SerialCommand_IsPrefix(token) != 0U) {
    token = SerialCommand_NextToken(&cursor);
    if (token == 0) {
      return SERIAL_CMD_PARSE_FORMAT;
    }
  }

  while (token != 0) {
    equals = strchr(token, '=');
    if (equals == 0) {
      return SERIAL_CMD_PARSE_FORMAT;
    }
    *equals = '\0';
    equals++;

    if (SerialCommand_KeyEquals(token, "F") ||
        SerialCommand_KeyEquals(token, "FREQ") ||
        SerialCommand_KeyEquals(token, "FREQUENCY")) {
      result = SerialCommand_ParseUint(equals, SERIAL_CMD_MAX_FREQ_HZ, &value);
      if (result != SERIAL_CMD_PARSE_OK) {
        return result;
      }
      *freq_hz = value;
      *field_mask |= DDS_SINGLE_FIELD_FREQ;
    } else if (SerialCommand_KeyEquals(token, "A") ||
               SerialCommand_KeyEquals(token, "AMP") ||
               SerialCommand_KeyEquals(token, "AMPLITUDE")) {
      result = SerialCommand_ParseUint(equals, AD9910_MAX_AMPLITUDE, &value);
      if (result != SERIAL_CMD_PARSE_OK) {
        return result;
      }
      *amplitude = (uint16_t)value;
      *field_mask |= DDS_SINGLE_FIELD_AMP;
    } else if (SerialCommand_KeyEquals(token, "P") ||
               SerialCommand_KeyEquals(token, "PH") ||
               SerialCommand_KeyEquals(token, "PHA") ||
               SerialCommand_KeyEquals(token, "PHASE")) {
      result = SerialCommand_ParseUint(equals, 360UL, &value);
      if (result != SERIAL_CMD_PARSE_OK) {
        return result;
      }
      *phase_deg = (uint16_t)value;
      *field_mask |= DDS_SINGLE_FIELD_PHASE;
    } else {
      return SERIAL_CMD_PARSE_FORMAT;
    }

    token = SerialCommand_NextToken(&cursor);
  }

  return (*field_mask == 0U) ? SERIAL_CMD_PARSE_FORMAT : SERIAL_CMD_PARSE_OK;
}

static uint8_t SerialCommand_WriteText(const char *text, uint16_t len)
{
  if ((text == 0) || (len == 0U)) {
    return 0U;
  }
  return USART1_Write((const uint8_t *)text, len);
}

static void SerialCommand_SendState(const char *prefix)
{
  char response[48];
  uint32_t freq_hz;
  uint16_t amplitude;
  uint16_t phase_deg;
  int len;

  DDS_AppGetSingle(&freq_hz, &amplitude, &phase_deg);
  len = snprintf(response, sizeof(response), "%s F=%lu A=%u P=%u\r\n",
                 prefix,
                 (unsigned long)freq_hz,
                 (unsigned int)amplitude,
                 (unsigned int)phase_deg);
  if ((len > 0) && ((uint32_t)len < sizeof(response))) {
    if (SerialCommand_WriteText(response, (uint16_t)len) == 0U) {
      return;
    }
  }
}

static void SerialCommand_SendError(SerialCommand_ParseResult result)
{
  const char *message = "ERR DDS CMD\r\n";

  if (result == SERIAL_CMD_PARSE_RANGE) {
    message = "ERR DDS RANGE\r\n";
  }

  if (SerialCommand_WriteText(message, (uint16_t)strlen(message)) == 0U) {
    return;
  }
}

static void SerialCommand_ProcessLine(void)
{
  uint32_t freq_hz = 0U;
  uint16_t amplitude = 0U;
  uint16_t phase_deg = 0U;
  uint8_t field_mask = 0U;
  SerialCommand_ParseResult result;

  serial_cmd_line[serial_cmd_len] = '\0';
  result = SerialCommand_ParseLine(serial_cmd_line, &freq_hz, &amplitude, &phase_deg, &field_mask);

  if (result == SERIAL_CMD_PARSE_EMPTY) {
    return;
  }
  if (result == SERIAL_CMD_PARSE_QUERY) {
    SerialCommand_SendState("DDS");
    return;
  }
  if (result != SERIAL_CMD_PARSE_OK) {
    SerialCommand_SendError(result);
    return;
  }

  if (DDS_AppSetSingleCustom(freq_hz, amplitude, phase_deg, field_mask) != 0U) {
    SerialCommand_SendState("OK");
  } else {
    SerialCommand_SendError(SERIAL_CMD_PARSE_FORMAT);
  }
}

void SerialCommand_Init(void)
{
  serial_cmd_len = 0U;
}

void SerialCommand_Tick(void)
{
  uint8_t data;

  while (USART1_ReadByte(&data) != 0U) {
    if ((data == '\r') || (data == '\n')) {
      if (serial_cmd_len != 0U) {
        SerialCommand_ProcessLine();
        serial_cmd_len = 0U;
      }
    } else if (serial_cmd_len < (SERIAL_CMD_LINE_CHARS - 1U)) {
      serial_cmd_line[serial_cmd_len] = (char)data;
      serial_cmd_len++;
    } else {
      serial_cmd_len = 0U;
      SerialCommand_SendError(SERIAL_CMD_PARSE_FORMAT);
    }
  }
}
