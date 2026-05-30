# CubeMX `main.c` User Snippet

These pieces have already been applied to `Core/Src/main.c`. Keep them inside `USER CODE` blocks so CubeMX regeneration preserves them.

## Includes

Add inside `/* USER CODE BEGIN Includes */`:

```c
#include "ad7606b.h"
#include "lcd.h"
```

## Private Variables

Add inside `/* USER CODE BEGIN PV */`:

```c
static int16_t ad_samples[AD7606B_CHANNEL_COUNT];
```

## Initialization

Add inside `/* USER CODE BEGIN 2 */`, after `MX_FSMC_Init()` has run:

```c
LCD_Init();
LCD_Fill(LCD_COLOR_RED);
HAL_Delay(500);
LCD_Fill(LCD_COLOR_GREEN);
HAL_Delay(500);
LCD_Fill(LCD_COLOR_BLUE);

AD7606B_Init();
```

## Main Loop

Add inside the generated `while (1)` loop:

```c
AD7606B_StartConversion();
if (AD7606B_ReadChannels(ad_samples) != 0U) {
    LCD_DrawPixel(10, 10, LCD_COLOR_WHITE);
} else {
    LCD_DrawPixel(10, 10, LCD_COLOR_RED);
}
HAL_Delay(10);
```
