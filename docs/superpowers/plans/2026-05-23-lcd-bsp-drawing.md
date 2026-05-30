# LCD BSP Drawing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the practical LCD drawing/text APIs from the 32F4 LCD BSP to the current TFT+AD7606B+AD9910 project without replacing the working TFT initialization.

**Architecture:** Keep the current FSMC access layer (`tft_fsmc.c`) and current `LCD_Init()` path because the screen already works. Extend `Core/Inc/lcd.h` and `Core/Src/lcd.c` with BSP-compatible drawing functions and lightweight text helpers. Avoid importing the full 12/16/24/32 font tables for now because the current ARMCC Lite image is close enough that a 12KB font import can re-trigger `L6047U`.

**Tech Stack:** STM32F407VET6, HAL/FSMC, Keil ARMCC 5, RGB565 TFT LCD.

---

### Task 1: Static API Red Check

**Files:**
- Read: `D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/Core/Inc/lcd.h`

- [ ] **Step 1: Run the missing API check**

```powershell
$header = Get-Content 'D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/Core/Inc/lcd.h' -Raw
$required = @(
  'LCD_DrawLine', 'LCD_DrawRectangle', 'LCD_DrawCircle', 'LCD_FillCircle',
  'LCD_DrawHLine', 'LCD_ColorFill', 'LCD_ShowChar', 'LCD_ShowNum',
  'LCD_ShowXNum', 'LCD_ShowString'
)
$missing = $required | Where-Object { $header -notmatch "$_\s*\(" }
if ($missing.Count -gt 0) { Write-Host "Missing: $($missing -join ', ')"; exit 1 }
```

Expected: fails before implementation and prints the missing API names.

### Task 2: Extend LCD Public API

**Files:**
- Modify: `D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/Core/Inc/lcd.h`

- [ ] **Step 1: Add RGB565 colors and drawing prototypes**

Add common BSP color names, width/height constants, shape drawing functions, compatible `LCD_Show*` functions, and lowercase `lcd_*` aliases for code imported from examples.

### Task 3: Implement Drawing Primitives

**Files:**
- Modify: `D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/Core/Src/lcd.c`

- [ ] **Step 1: Add line, rectangle, circle, fill-circle, horizontal line, and color-fill implementations**

Use the existing `LCD_DrawPixel`, `LCD_FillRect`, and internal window writer so the current controller setup remains untouched.

### Task 4: Implement Text Compatibility

**Files:**
- Modify: `D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/Core/Src/lcd.c`

- [ ] **Step 1: Expand the current 5x7 glyph support**

Add glyphs needed by the current UI and measurement strings: space, plus, dot, slash, percent, equals, angle brackets, parentheses, underscore.

- [ ] **Step 2: Add `LCD_ShowChar`, `LCD_ShowNum`, `LCD_ShowXNum`, and `LCD_ShowString` wrappers**

Map reference BSP sizes to the current scalable 5x7 font to keep image size small.

### Task 5: Verification

**Files:**
- Read: `D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/MDK-ARM/TFT_AD7606B.uvprojx`

- [ ] **Step 1: Re-run the static API check**

Expected: exits 0 with no missing API.

- [ ] **Step 2: Build with ARMCC 5**

```powershell
& 'D:/keil5/stm32/UV4/UV4.exe' -b 'D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/MDK-ARM/TFT_AD7606B.uvprojx' -o 'D:/Users/港城泽人/Desktop/TFT+AD7606b/TFT+AD7060B/MDK-ARM/codex_lcd_bsp.log'
```

Expected: `0 Error(s), 0 Warning(s)` and image size remains below the ARMCC Lite limit.
