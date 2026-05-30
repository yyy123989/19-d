#include "tft_fsmc.h"

/*
 * Board schematic: TFT RS/DC is wired to FSMC_A18.
 * If the board uses another address line, update TFT_RAM_ADDR accordingly.
 */
#define TFT_REG_ADDR ((volatile uint16_t *)0x60000000U)
#define TFT_RAM_ADDR ((volatile uint16_t *)0x60080000U)

void TFT_WriteCommand(uint16_t command)
{
    *TFT_REG_ADDR = command;
}

void TFT_WriteData(uint16_t data)
{
    *TFT_RAM_ADDR = data;
}

uint16_t TFT_ReadData(void)
{
    return *TFT_RAM_ADDR;
}
