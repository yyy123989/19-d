#ifndef AD7606B_H
#define AD7606B_H

#include <stdint.h>

#define AD7606B_CHANNEL_COUNT 8U
#define AD7606B_RANGE_MV 5000L

void AD7606B_Init(void);
void AD7606B_StartConversion(void);
uint8_t AD7606B_ReadChannels(int16_t samples[AD7606B_CHANNEL_COUNT]);
uint8_t AD7606B_ReadChannelsReady(int16_t samples[AD7606B_CHANNEL_COUNT]);

int32_t AD7606B_RawToMillivolts(int16_t raw);
uint32_t AD7606B_FormatVoltage(char *buffer, uint32_t size, int16_t raw, uint8_t append_unit);

#endif
