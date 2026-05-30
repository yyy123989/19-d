# STM32F407VET6 TFT + AD7606B Design

## Goal

Use an STM32F407VET6 development board with its reserved TFT LCD connector to drive an 8080 parallel TFT through FSMC, and connect an AD7606B through a separate 16-bit GPIO parallel bus.

## Board Constraint

The development board already exposes a TFT connector. The connector includes the LCD 8080 bus signals, LCD control signals, backlight/reset signals, and touch SPI signals. These pins are treated as fixed board resources and must not be reused for AD7606B wiring.

The TFT connector signals visible on the board are:

- Power: `3V3`, `GND`
- LCD control: `CS`, `RS`, `WR`, `RD`, `RST`
- LCD data: `D0` to `D15`
- Backlight: `BLK`
- Touch: `TCS`, `TSCK`, `TSI`, `TSO`, `IRQ`

## TFT Interface

The TFT is configured as an FSMC 8080-style 16-bit display.

Typical STM32F407 FSMC mapping:

| TFT Signal | STM32 Function |
| --- | --- |
| `D0-D15` | `FSMC_D0-D15` |
| `RD` | `FSMC_NOE` |
| `WR` | `FSMC_NWE` |
| `CS` | `FSMC_NE1` |
| `RS` | `FSMC_A18` |
| `RST` | GPIO output |
| `BLK` | GPIO output or PWM output |

The board schematic shows the TFT connector using `FSMC_A18` for LCD `RS`.

## AD7606B Interface

AD7606B uses 16-bit parallel mode on GPIOB so firmware can read one complete sample word with `GPIOB->IDR`.

| AD7606B Signal | STM32F407VET6 Pin | Direction | Notes |
| --- | --- | --- | --- |
| `DB0` | `PB0` | Input | Parallel data |
| `DB1` | `PB1` | Input | Parallel data |
| `DB2` | `PB2` | Input | Add BOOT1-safe pulldown |
| `DB3` | `PB3` | Input | Disable JTAG, keep SWD |
| `DB4` | `PB4` | Input | Disable JTAG, keep SWD |
| `DB5` | `PB5` | Input | Parallel data |
| `DB6` | `PB6` | Input | Parallel data |
| `DB7` | `PB7` | Input | Parallel data |
| `DB8` | `PB8` | Input | Parallel data |
| `DB9` | `PB9` | Input | Parallel data |
| `DB10` | `PB10` | Input | Parallel data |
| `DB11` | `PB11` | Input | Parallel data |
| `DB12` | `PB12` | Input | Parallel data |
| `DB13` | `PB13` | Input | Parallel data |
| `DB14` | `PB14` | Input | Parallel data |
| `DB15` | `PB15` | Input | Parallel data |
| `CONVST A/B` | `PA0` | Output | Start as GPIO pulse; timer pulse can replace it after basic readout works |
| `BUSY` | `PA1` | Input | Use EXTI if interrupt-driven conversion is needed |
| `RD` | `PA2` | Output | Read strobe |
| `CS` | `PA3` | Output | ADC chip select |
| `RESET` | `PC4` | Output | ADC reset |
| `RANGE` | `PC5` | Output | Input range select |
| `OS0` | `PC6` | Output | Oversampling select |
| `OS1` | `PC7` | Output | Oversampling select |
| `OS2` | `PC8` | Output | Oversampling select |
| `PAR/SER SEL` | `PC9` | Output or hardware strap | Select parallel mode |
| `STBY` | `PC10` | Output | Standby control |
| `FRSTDATA` | `PC11` | Input | Optional frame marker |
| `WR` | `PC12` | Output | Hold high for simple readout; use later for register writes |

## Reserved Resources

- `PA13` and `PA14` stay reserved for SWD.
- CubeMX debug mode must be set to Serial Wire so `PB3` and `PB4` are available.
- `PA9` and `PA10` should remain available for USART1 debug logs if possible.
- TFT touch SPI pins should be confirmed before assigning any SPI peripheral to other devices.

## CubeMX Configuration Direction

Configure the project for STM32F407VET6 with:

- RCC clock source according to the actual board crystal.
- SYS Debug set to Serial Wire.
- FSMC enabled as 16-bit asynchronous SRAM/NOR-style interface for the TFT.
- GPIOB `PB0-PB15` as input for AD7606B data.
- AD7606B control pins as GPIO outputs with safe default levels.
- `BUSY` as GPIO input, optionally with EXTI.
- USART1 optional for debug output.

## Success Criteria

- The TFT initialization can write commands and pixel data through FSMC.
- The backlight and reset pins can be controlled.
- AD7606B can be reset, conversion-triggered, and read through GPIOB.
- A first firmware test can display raw AD7606B channel values on the TFT.
