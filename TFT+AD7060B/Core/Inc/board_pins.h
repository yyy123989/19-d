#ifndef BOARD_PINS_H
#define BOARD_PINS_H

/*
 * Board policy:
 * - The board TFT connector owns the FSMC 8080 16-bit bus.
 * - AD7606B uses serial GPIO mode to avoid the PB0-PB15 parallel bus.
 * - PB2/PB4 are left unused by AD7606B, so BOOT1 and SWD-related pins stay safe.
 * - AD9910 is controlled with independent GPIO pins on PA8, PB8-PB10,
 *   PB12-PB15, and PE2-PE6. It does not share AD7606B or FSMC lines.
 */

#endif
