#ifndef TFT_FSMC_H
#define TFT_FSMC_H

#include <stdint.h>

void TFT_WriteCommand(uint16_t command);
void TFT_WriteData(uint16_t data);
uint16_t TFT_ReadData(void);

#endif

