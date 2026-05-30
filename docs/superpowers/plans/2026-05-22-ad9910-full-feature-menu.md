# AD9910 Full Feature Menu Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the AD9910 single-tone, RAM playback, DRG sweep, OSK, and software calibration functions into the current STM32F407VET6 + TFT + AD7606B project with a two-key TFT menu.

**Architecture:** Keep `ad9910.c` as the low-level register driver, add `dds_app.c` as the DDS mode/parameter state machine, and add `ui_menu.c` as the two-key short/long press scanner. `main.c` remains the hardware orchestration file for LCD refresh, TIM3 AD7606B sampling, and TIM4 DRG direction ticks.

**Tech Stack:** STM32F407VET6, STM32 HAL, Keil MDK project `TFT_AD7606B.uvprojx`, ARMCLANG through UV4, existing FSMC TFT driver, existing serial AD7606B driver, existing bit-banged AD9910 SPI.

---

## Scope Notes

- Do not flash the board in this plan. The user asked for code checks only after writing code.
- The workspace root `D:\Users\港城泽人\Desktop\TFT+AD7606b` is not a Git repository, so commit steps are skipped.
- PROFILE1 and PROFILE2 are physically tied to GND. The implementation must not depend on software control of those two pins.
- RAM submodes must use Profile 0 only. Do not port the reference demo's `Set_Profile(0~4)` cycling behavior because PROFILE1/PROFILE2 are not wired for this project.
- The current project wiring uses DDS SCLK on PB13 and DDS SDIO on PB15. Some AD9910 module/reference files use PB3/PB5; code comments must call out that the current project wiring is authoritative.
- Port the original AD9910 V1.2 RAM tables `RAM_AMP[]`, `RAM_Fre[]`, and `RAM_PHA[]` from `D:\Users\港城泽人\Desktop\模块\ad9910\AD9910模块资料\测试程序\AD9910 V1.2\User\AD9910.c`.
- AD9910 calibration from the reference project uses unavailable parallel hardware. This plan implements software frequency offset, amplitude gain, and phase offset.
- AD7606B TIM3 sampling stays at 40 kSPS. DRG direction switching stays on TIM4.

## File Structure

- Modify `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910.h`
  - Add RAM mode/wave enums, OSK API, RAM playback API, and hold/direction helpers.
- Modify `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c`
  - Add RAM playback using the imported tables, OSK control, pin wiring note, and safer parameter clamping.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910_ram_tables.h`
  - Expose the imported RAM table declarations and lengths.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910_ram_tables.c`
  - Store `RAM_AMP[]`, `RAM_Fre[]`, and `RAM_PHA[]` as `const uint32_t` arrays copied from the AD9910 V1.2 reference.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ui_menu.h`
  - Expose two-key event enum and scanner API.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c`
  - Detect KEY1/KEY0 short and long presses with debounce.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\dds_app.h`
  - Expose DDS app lifecycle, key handling, drawing, and TIM4 direction hook.
- Create `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c`
  - Hold mode/page/parameter state, apply AD9910 settings, and render menu/status lines.
- Modify `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c`
  - Remove local one-key DDS mode code and call `ui_menu` + `dds_app`.
- Modify `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx`
  - Add `ad9910_ram_tables.c`, `dds_app.c`, and `ui_menu.c` to `Application/User/Core`.

---

### Task 1: Baseline Guards

**Files:**
- Read: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\main.h`
- Read: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\gpio.c`
- Read: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c`
- Read: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c`

- [ ] **Step 1: Run the current Keil build**

Run:

```powershell
& 'D:\keil5\UV4\UV4.exe' -b 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx' -j0 -o 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\codex_pre_ad9910_full.log'
```

Expected: build completes with `0 Error(s)`. Existing warnings from the current toolchain are acceptable if they match the prior warning style.

- [ ] **Step 2: Confirm pin ownership before edits**

Run:

```powershell
rg -n "KEY1_Pin|KEY0_Pin|DDS_PROFILE0_Pin|DDS_OSK_Pin|DDS_DRCTL_Pin|DDS_DRHOLD_Pin|DDS_CS_Pin|DDS_SCLK_Pin|DDS_SDIO_Pin|DDS_IO_UPDATE_Pin|DDS_RESET_Pin|DDS_PWRDN_Pin" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\main.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\gpio.c'
```

Expected: KEY1 is PE3, KEY0 is PE4, PROFILE0 is PE2, OSK is PE5, DRCTL is PE6, DRHOLD is PB8, AD9910 serial pins are PB10/PB12/PB13/PB15/PA8/PB9.

- [ ] **Step 3: Confirm implementation starts from the old one-key DDS code**

Run:

```powershell
rg -n "APP_KeyPressed|APP_ServiceDdsMode|APP_SetDdsMode|APP_DDS_MODE" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c'
```

Expected: current `main.c` contains the old local mode-switching functions. These will be replaced in Task 5.

---

### Task 2: Add Two-Key Event Scanner

**Files:**
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ui_menu.h`
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c`

- [ ] **Step 1: Add a failing static check for the scanner API**

Run:

```powershell
Test-Path 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ui_menu.h'
Test-Path 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c'
```

Expected before implementation: both return `False`.

- [ ] **Step 2: Create `ui_menu.h`**

Write this interface:

```c
#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>

typedef enum {
  UI_KEY_EVENT_NONE = 0,
  UI_KEY_EVENT_KEY1_SHORT,
  UI_KEY_EVENT_KEY0_SHORT,
  UI_KEY_EVENT_KEY1_LONG,
  UI_KEY_EVENT_KEY0_LONG
} UI_KeyEvent;

void UI_MenuKeysInit(void);
UI_KeyEvent UI_MenuScanKeys(uint32_t now_ms);

#endif
```

- [ ] **Step 3: Create `ui_menu.c`**

Implement with these rules:

```c
#include "ui_menu.h"

#include "main.h"

#define UI_KEY_DEBOUNCE_MS 30U
#define UI_KEY_LONG_MS     500U

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState last_raw;
  GPIO_PinState stable;
  uint32_t changed_ms;
  uint32_t pressed_ms;
  uint8_t pressed;
  uint8_t long_sent;
  UI_KeyEvent short_event;
  UI_KeyEvent long_event;
} UI_KeyState;
```

Use two `UI_KeyState` instances:

```c
static UI_KeyState ui_keys[] = {
  {KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0U, 0U, 0U, 0U,
   UI_KEY_EVENT_KEY1_SHORT, UI_KEY_EVENT_KEY1_LONG},
  {KEY0_GPIO_Port, KEY0_Pin, GPIO_PIN_SET, GPIO_PIN_SET, 0U, 0U, 0U, 0U,
   UI_KEY_EVENT_KEY0_SHORT, UI_KEY_EVENT_KEY0_LONG},
};
```

`UI_MenuKeysInit()` must configure `KEY1_Pin | KEY0_Pin` as input pull-up on GPIOE. `UI_MenuScanKeys(now_ms)` must return at most one event per call, with active-low press detection. A long event fires once after 500 ms; a short event fires on release if long was not sent.

- [ ] **Step 4: Verify scanner files exist and contain both keys**

Run:

```powershell
rg -n "UI_KEY_EVENT_KEY1_SHORT|UI_KEY_EVENT_KEY0_SHORT|UI_KEY_EVENT_KEY1_LONG|UI_KEY_EVENT_KEY0_LONG|KEY1_Pin|KEY0_Pin" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ui_menu.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c'
```

Expected: all event names and both pin names appear.

---

### Task 3: Extend AD9910 Driver

**Files:**
- Modify: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910.h`
- Modify: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c`
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910_ram_tables.h`
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910_ram_tables.c`

- [ ] **Step 1: Add a failing static check for the new AD9910 API**

Run:

```powershell
rg -n "AD9910_SetOsk|AD9910_SetRamPlayback|AD9910_SetRamTablePlayback|AD9910_SetDigitalRampHold|AD9910_RamMode|AD9910_RamTarget|AD9910_RamWave" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910.h'
rg -n "RAM_AMP|RAM_Fre|RAM_PHA|AD9910_RAM_AMP|AD9910_RAM_FRE|AD9910_RAM_PHA" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core'
```

Expected before implementation: no matches.

- [ ] **Step 2: Update `ad9910.h` with the public API**

Add these declarations after the existing constants:

```c
typedef enum {
  AD9910_RAM_WAVE_TRIANGLE = 0,
  AD9910_RAM_WAVE_SQUARE,
  AD9910_RAM_WAVE_SINE,
  AD9910_RAM_WAVE_SINC
} AD9910_RamWave;

typedef enum {
  AD9910_RAM_MODE_DIRECT = 0,
  AD9910_RAM_MODE_RAMP_UP,
  AD9910_RAM_MODE_BIDIRECTION,
  AD9910_RAM_MODE_CONT_BIDIRECTION,
  AD9910_RAM_MODE_CONT_RECIRC,
  AD9910_RAM_MODE_TRIANGLE_SQUARE
} AD9910_RamMode;

typedef enum {
  AD9910_RAM_TARGET_FREQUENCY = 0,
  AD9910_RAM_TARGET_AMPLITUDE,
  AD9910_RAM_TARGET_PHASE
} AD9910_RamTarget;

void AD9910_SetDigitalRampHold(uint8_t hold);
void AD9910_SetOsk(uint8_t enabled);
void AD9910_SetRamPlayback(AD9910_RamMode mode, AD9910_RamWave wave,
                           uint32_t freq_hz, uint16_t amplitude,
                           uint16_t duty_permille);
void AD9910_SetRamTablePlayback(AD9910_RamMode mode, AD9910_RamTarget target,
                                const uint32_t *words, uint16_t word_count);
```

- [ ] **Step 3: Create imported RAM table files**

Create `ad9910_ram_tables.h`:

```c
#ifndef AD9910_RAM_TABLES_H
#define AD9910_RAM_TABLES_H

#include <stdint.h>

extern const uint32_t AD9910_RAM_AMP[1024];
extern const uint32_t AD9910_RAM_FRE[1024];
extern const uint32_t AD9910_RAM_PHA[1024];

#define AD9910_RAM_TABLE_POINTS 1024U

#endif
```

Create `ad9910_ram_tables.c` and copy the original 1024-point 32-bit tables from:

`D:\Users\港城泽人\Desktop\模块\ad9910\AD9910模块资料\测试程序\AD9910 V1.2\User\AD9910.c`

Rename the arrays while preserving data order:

- `RAM_AMP[]` -> `AD9910_RAM_AMP[]`
- `RAM_Fre[]` -> `AD9910_RAM_FRE[]`
- `RAM_PHA[]` -> `AD9910_RAM_PHA[]`

Do not hand-generate replacement sine/triangle/sinc tables for these three imported tables.

- [ ] **Step 4: Add AD9910 helper functions in `ad9910.c`**

Add private clamp helpers and a wiring note near the bit-banged SPI helpers:

```c
/*
 * Wiring note: the AD9910 V1.2 reference code and some module docs use
 * PB3/PB5 for SCLK/SDIO. This project is actually wired through main.h as
 * DDS_SCLK=PB13 and DDS_SDIO=PB15, so the project pin definitions win.
 */
static uint32_t AD9910_ClampFrequency(uint32_t freq_hz)
{
    if (freq_hz > 420000000UL) {
        return 420000000UL;
    }
    return freq_hz;
}

static uint16_t AD9910_ClampAmplitude(uint16_t amplitude)
{
    if (amplitude > AD9910_MAX_AMPLITUDE) {
        return AD9910_MAX_AMPLITUDE;
    }
    return amplitude;
}
```

- [ ] **Step 5: Add OSK and DRHOLD functions in `ad9910.c`**

Add:

```c
void AD9910_SetDigitalRampHold(uint8_t hold)
{
    HAL_GPIO_WritePin(DDS_DRHOLD_GPIO_Port, DDS_DRHOLD_Pin,
                      (hold != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void AD9910_SetOsk(uint8_t enabled)
{
    HAL_GPIO_WritePin(DDS_OSK_GPIO_Port, DDS_OSK_Pin,
                      (enabled != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
```

- [ ] **Step 6: Add Profile-0-only RAM playback**

Implement `AD9910_SetRamTablePlayback()` by reusing the current RAM load/run sequence, but write only Profile 0 (`0x0E`) for playback setup. Do not recreate `Set_Profile()` and do not write Profile 1-4 (`0x0F` to `0x12`) in this project.

```c
void AD9910_SetRamTablePlayback(AD9910_RamMode mode, AD9910_RamTarget target,
                                const uint32_t *words, uint16_t word_count)
{
    uint16_t i;

    if ((words == 0) || (word_count == 0U) || (word_count > AD9910_RAM_POINTS)) {
        return;
    }

    /* Write CFR1/CFR2 target bits and Profile0 RAM limits according to mode/target. */
    AD9910_WriteRamControl(mode, target, word_count);

    AD9910_SetProfile(0U);
    AD9910_PulseIoUpdate();

    AD9910_PinClr(DDS_CS_GPIO_Port, DDS_CS_Pin);
    AD9910_WriteByte(AD9910_RAM_REG);
    for (i = 0U; i < word_count; i++) {
        AD9910_WriteRamWord(words[i]);
    }
    AD9910_PinSet(DDS_CS_GPIO_Port, DDS_CS_Pin);
    AD9910_PulseIoUpdate();
}
```

`AD9910_WriteRamControl()` must mirror the reference RAM mode register byte arrays for Profile 0, but compress any original Profile 1-4 demonstrations into a single Profile 0 range. The direct mode imported from reference uses these source functions as the starting point:

- `AD9910_RAM_Fre_W()` + `AD9910_RAM_DIR_Fre_R()`
- `AD9910_RAM_AMP_W()` + `AD9910_RAM_DIR_AMP_R()`
- `AD9910_RAM_Pha_W()` + `AD9910_RAM_DIR_PHA_R()`

Then change `AD9910_SetRamPlayback()` to pick the correct imported table and call `AD9910_SetRamTablePlayback()`. Change `AD9910_SetTriangleWave()` to use the RAM amplitude target and Profile 0 only.

```c
AD9910_SetRamPlayback(AD9910_RAM_MODE_TRIANGLE_SQUARE,
                      AD9910_RAM_WAVE_TRIANGLE,
                      freq_hz,
                      amplitude,
                      500U);
```

- [ ] **Step 7: Verify new AD9910 symbols, imported RAM tables, and no PROFILE1/2 dependency**

Run:

```powershell
rg -n "AD9910_SetOsk|AD9910_SetRamPlayback|AD9910_SetRamTablePlayback|AD9910_SetDigitalRampHold|AD9910_RAM_MODE_TRIANGLE_SQUARE|AD9910_RamTarget" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
rg -n "AD9910_RAM_AMP|AD9910_RAM_FRE|AD9910_RAM_PHA|RAM_AMP|RAM_Fre|RAM_PHA" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910_ram_tables.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910_ram_tables.c'
rg -n "PB3/PB5|DDS_SCLK=PB13|DDS_SDIO=PB15" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
rg -n "PROFILE1|PROFILE2|PF1|PF2" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910.h'
rg -n "Set_Profile|0x0F|0x10|0x11|0x12" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
```

Expected: the first three commands find the new API, imported tables, and wiring note. The `PROFILE1/PROFILE2/PF1/PF2` command finds no matches. The final command must not find `Set_Profile`; if it finds `0x0F` to `0x12`, inspect and reject the change unless those bytes are unrelated to AD9910 Profile 1-4 writes.

---

### Task 4: Add DDS Application State Machine

**Files:**
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\dds_app.h`
- Create: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c`

- [ ] **Step 1: Add a failing static check for DDS app files**

Run:

```powershell
Test-Path 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\dds_app.h'
Test-Path 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c'
```

Expected before implementation: both return `False`.

- [ ] **Step 2: Create `dds_app.h`**

Write this interface:

```c
#ifndef DDS_APP_H
#define DDS_APP_H

#include "ad7606b.h"
#include "ui_menu.h"

#include <stdint.h>

void DDS_AppInit(void);
void DDS_AppHandleKey(UI_KeyEvent event);
void DDS_AppTick(uint32_t now_ms);
void DDS_AppDraw(const int16_t samples[AD7606B_CHANNEL_COUNT],
                 uint8_t sample_valid,
                 uint32_t sample_rate_hz,
                 uint32_t fail_count);
uint8_t DDS_AppIsDrgMode(void);
void DDS_AppDrgTimerTick(void);

#endif
```

- [ ] **Step 3: Create `dds_app.c` state and tables**

Use these mode/page types and parameter tables:

```c
typedef enum {
  DDS_PAGE_HOME = 0,
  DDS_PAGE_SINGLE,
  DDS_PAGE_RAM,
  DDS_PAGE_DRG,
  DDS_PAGE_OSK,
  DDS_PAGE_CAL,
  DDS_PAGE_AD_VIEW
} DDS_Page;

typedef enum {
  DDS_MODE_SINGLE = 0,
  DDS_MODE_RAM,
  DDS_MODE_DRG,
  DDS_MODE_OSK,
  DDS_MODE_CAL,
  DDS_MODE_AD_VIEW,
  DDS_MODE_COUNT
} DDS_Mode;

static const uint32_t dds_freq_table[] = {
  0UL, 1UL, 10UL, 100UL, 500UL, 1000UL, 5000UL, 10000UL, 50000UL,
  100000UL, 500000UL, 1000000UL, 5000000UL, 10000000UL, 50000000UL,
  100000000UL, 200000000UL, 300000000UL, 380000000UL, 400000000UL, 420000000UL
};

static const uint16_t dds_amp_table[] = {
  0U, 1023U, 2047U, 4095U, 8191U, 9215U, 10239U, 11263U, 12287U,
  13311U, 14335U, 15359U, 16383U
};

static const uint16_t dds_phase_table[] = {0U, 90U, 180U, 270U, 360U};
static const uint32_t dds_offset_table[] = {0UL, 1000UL, 2000UL, 4000UL, 8000UL, 10000UL, 20000UL, 40000UL, 60000UL};
```

State fields must include current page, selected home mode, selected field index, single frequency/amplitude/phase indexes, RAM mode/wave indexes, DRG lower/upper/step/rate indexes, OSK mode, calibration offset/gain/phase indexes, and DRG direction.

- [ ] **Step 4: Implement parameter application helpers**

Implement helpers with these behaviors:

```c
static uint32_t DDS_AppCalibratedFreq(uint32_t base_hz);
static uint16_t DDS_AppCalibratedAmp(uint16_t base_amp);
static uint16_t DDS_AppCalibratedPhase(uint16_t base_phase);
static void DDS_AppApplySingle(void);
static void DDS_AppApplyRam(void);
static void DDS_AppApplyDrg(void);
static void DDS_AppApplyOsk(void);
static void DDS_AppApplyCurrent(void);
```

Required rules:

- `DDS_AppCalibratedFreq()` returns `min(base_hz + offset, 420000000)`.
- `DDS_AppCalibratedAmp()` maps `gain=8` to 1.00x using integer math: `amp * (8 + gain_index) / 16`, then clamps to 16383.
- `DDS_AppCalibratedPhase()` returns `(base_phase + phase_offset) % 360`.
- `DDS_AppApplySingle()` calls `AD9910_Init()`, `AD9910_SetSingleTone(0U, freq, amp, phase)`, then `AD9910_SetProfile(0U)`.
- `DDS_AppApplyRam()` calls `AD9910_Init()` and `AD9910_SetRamPlayback(...)`. The RAM path must select among imported `AD9910_RAM_AMP`, `AD9910_RAM_FRE`, and `AD9910_RAM_PHA` through the AD9910 driver; it must not use `Set_Profile(0~4)` behavior from the reference demo.
- `DDS_AppApplyDrg()` calls `AD9910_Init()`, `AD9910_SetDigitalRampHold(0U)`, and `AD9910_SetDigitalRampFrequency(...)`.
- `DDS_AppApplyOsk()` starts from single-tone 1 kHz and controls PE5 through `AD9910_SetOsk()`.

- [ ] **Step 5: Implement key handling**

Use this two-level menu behavior:

- Home page:
  - KEY1 short: move to next main mode.
  - KEY0 short: move to next main mode.
  - KEY0 long: enter selected mode page.
  - KEY1 long: enter `AD VIEW`.
- Mode page:
  - KEY1 short: move to next parameter field.
  - KEY0 short: increment selected parameter.
  - KEY1 long: return to home page.
  - KEY0 long: apply current output.
- AD VIEW page:
  - KEY1 long or KEY0 long: return to home page.

Implement `DDS_AppHandleKey(UI_KeyEvent event)` using a `switch` over page and event. Avoid blocking delays in this function.

- [ ] **Step 6: Implement OSK periodic tick and DRG timer hook**

`DDS_AppTick(now_ms)` must toggle OSK only when the OSK page is in auto mode. Use a 500 ms toggle period.

`DDS_AppDrgTimerTick()` must do:

```c
if (DDS_AppIsDrgMode() != 0U) {
  dds_drg_rising ^= 1U;
  AD9910_SetDigitalRampDirection(dds_drg_rising);
}
```

Do not write AD9910 registers from TIM4 except the DRCTL pin direction helper.

- [ ] **Step 7: Implement TFT drawing**

`DDS_AppDraw()` must clear and redraw compact text lines using existing `LCD_FillRect()` and `LCD_DrawString()` only. It must show:

- Mixed top bar on every non-AD-VIEW page: mode name, one key DDS parameter, `AD:OK/ERR`, `SPS:<rate>`, `E:<fail_count>`, and `CH1:<raw>`.
- Home: selected mode list.
- Single: frequency, amplitude, phase, calibration summary.
- RAM: submode and wave.
- DRG: lower, upper, step, rate, direction.
- OSK: off/manual/auto and current OSK output state.
- CAL: frequency offset, gain index, phase offset.
- AD VIEW: SPS, error count, and CH1 to CH8 raw values.

Use one static `char line[40]` buffer in `dds_app.c` so the mixed top bar does not truncate common values.

- [ ] **Step 8: Verify DDS app symbols**

Run:

```powershell
rg -n "DDS_AppInit|DDS_AppHandleKey|DDS_AppTick|DDS_AppDraw|DDS_AppDrgTimerTick|DDS_PAGE_HOME|DDS_MODE_COUNT" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\dds_app.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c'
```

Expected: all symbols are present.

---

### Task 5: Refactor `main.c` to Use the New App

**Files:**
- Modify: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c`

- [ ] **Step 1: Add a failing static check for old local menu code**

Run:

```powershell
rg -n "APP_KeyPressed|APP_KeyInit|APP_ServiceDdsMode|APP_SetDdsMode|APP_DdsModeTitle|APP_DrawAd7606bValues" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c'
```

Expected before this task: old functions are present.

- [ ] **Step 2: Replace includes**

In the user includes block, keep `ad7606b.h` and `lcd.h`, add:

```c
#include "dds_app.h"
#include "ui_menu.h"
```

Remove direct `#include "ad9910.h"` from `main.c`; AD9910 direct calls move into `dds_app.c`.

- [ ] **Step 3: Remove old local DDS mode state**

Remove these items from `main.c`:

```c
typedef enum {
  APP_DDS_MODE_SINGLE = 0,
  APP_DDS_MODE_DRG,
  APP_DDS_MODE_RAM,
  APP_DDS_MODE_COUNT
} APP_DdsMode;
```

Remove these variables:

```c
static volatile APP_DdsMode app_dds_mode = APP_DDS_MODE_SINGLE;
static volatile uint8_t app_drg_rising = 1U;
```

Remove these functions:

```c
static void APP_KeyInit(void);
static uint8_t APP_KeyPressed(void);
static const char *APP_DdsModeTitle(APP_DdsMode mode);
static void APP_SetDdsMode(APP_DdsMode mode);
static void APP_ServiceDdsMode(void);
static void APP_DrawAd7606bValues(uint8_t valid);
```

- [ ] **Step 4: Update TIM4 interrupt hook**

Replace the body of `APP_TIM4_IRQHandler()` with:

```c
void APP_TIM4_IRQHandler(void)
{
  DDS_AppDrgTimerTick();
}
```

- [ ] **Step 5: Update initialization in `main()`**

Replace the app init sequence after `MX_FSMC_Init()` with:

```c
  UI_MenuKeysInit();
  LCD_Init();
  LCD_Fill(LCD_COLOR_RED);
  HAL_Delay(500);
  LCD_Fill(LCD_COLOR_GREEN);
  HAL_Delay(500);
  LCD_Fill(LCD_COLOR_BLUE);
  HAL_Delay(500);
  LCD_Fill(LCD_COLOR_BLACK);

  ad_sample_rate_hz = APP_SAMPLE_RATE_HZ;
  AD7606B_Init();
  DDS_AppInit();
  DDS_AppDraw((const int16_t *)ad_samples, 0U, ad_sample_rate_hz, ad_sample_fail_count);
  APP_TIM3_Init();
  APP_TIM4_Init();
```

- [ ] **Step 6: Update the main loop**

Replace the loop body with:

```c
    UI_KeyEvent key_event = UI_MenuScanKeys(HAL_GetTick());
    uint8_t last_ok;
    uint32_t fail_count;
    int16_t display_samples[AD7606B_CHANNEL_COUNT];
    uint8_t i;

    DDS_AppHandleKey(key_event);
    DDS_AppTick(HAL_GetTick());

    if ((HAL_GetTick() - last_draw_tick) >= APP_LCD_REFRESH_MS) {
      __disable_irq();
      for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
        display_samples[i] = ad_samples[i];
      }
      fail_count = ad_sample_fail_count;
      last_ok = ad_last_read_ok;
      __enable_irq();

      DDS_AppDraw(display_samples, last_ok, ad_sample_rate_hz, fail_count);
      last_draw_tick = HAL_GetTick();
    }
```

If declaring arrays inside `while` causes style or compiler issues, move `display_samples`, `last_ok`, `fail_count`, and `i` before the `while`.

- [ ] **Step 7: Verify old local menu code is gone**

Run:

```powershell
rg -n "APP_KeyPressed|APP_KeyInit|APP_ServiceDdsMode|APP_SetDdsMode|APP_DdsModeTitle|APP_DrawAd7606bValues|APP_DDS_MODE" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c'
```

Expected: no matches.

---

### Task 6: Add New Source Files to Keil Project

**Files:**
- Modify: `D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx`

- [ ] **Step 1: Add a failing static check for project membership**

Run:

```powershell
rg -n "ad9910_ram_tables.c|dds_app.c|ui_menu.c" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx'
```

Expected before implementation: no matches.

- [ ] **Step 2: Insert the new files after `ad9910.c`**

Inside the `Application/User/Core` group, after the `ad9910.c` file block, add:

```xml
            <File>
              <FileName>ad9910_ram_tables.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Core/Src/ad9910_ram_tables.c</FilePath>
            </File>
            <File>
              <FileName>dds_app.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Core/Src/dds_app.c</FilePath>
            </File>
            <File>
              <FileName>ui_menu.c</FileName>
              <FileType>1</FileType>
              <FilePath>../Core/Src/ui_menu.c</FilePath>
            </File>
```

- [ ] **Step 3: Verify project membership**

Run:

```powershell
rg -n "ad9910_ram_tables.c|dds_app.c|ui_menu.c" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx'
```

Expected: all three new files appear once.

---

### Task 7: Build and Static Verification

**Files:**
- Read: all modified files from Tasks 2 to 6.

- [ ] **Step 1: Build with Keil UV4**

Run:

```powershell
& 'D:\keil5\UV4\UV4.exe' -b 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\TFT_AD7606B.uvprojx' -j0 -o 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\codex_ad9910_full_menu.log'
```

Expected: output log ends with `0 Error(s)`. Existing ARMCLANG warning style is acceptable.

- [ ] **Step 2: Verify no flash action was performed**

Run:

```powershell
Get-Content 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\MDK-ARM\codex_ad9910_full_menu.log' -Tail 40
```

Expected: the log shows build/link/hex creation only. It must not show download, flash, erase, or program messages.

- [ ] **Step 3: Verify PROFILE1/PROFILE2 are not used**

Run:

```powershell
rg -n "PROFILE1|PROFILE2|PF1|PF2" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core'
rg -n "Set_Profile|0x0F|0x10|0x11|0x12" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
```

Expected: no PROFILE1/PROFILE2/PF1/PF2 matches in the new application code. `Set_Profile` must not appear. If `0x0F` to `0x12` appears, inspect and reject AD9910 Profile 1-4 writes.

- [ ] **Step 4: Verify KEY0/KEY1 are inputs only**

Run:

```powershell
rg -n "KEY1_Pin|KEY0_Pin|GPIO_MODE_OUTPUT|GPIO_MODE_INPUT|GPIO_PULLUP" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\gpio.c' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c'
```

Expected: KEY1/KEY0 are configured as `GPIO_MODE_INPUT` with `GPIO_PULLUP`, never output.

- [ ] **Step 5: Verify TIM4 still owns DRG direction ticks**

Run:

```powershell
rg -n "APP_TIM4_IRQHandler|DDS_AppDrgTimerTick|AD9910_SetDigitalRampDirection" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\main.c' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
```

Expected: TIM4 calls `DDS_AppDrgTimerTick()`, and only the DDS app uses that tick to call `AD9910_SetDigitalRampDirection()`.

- [ ] **Step 6: Verify all requested user modes are reachable**

Run:

```powershell
rg -n "DDS_MODE_SINGLE|DDS_MODE_RAM|DDS_MODE_DRG|DDS_MODE_OSK|DDS_MODE_CAL|DDS_MODE_AD_VIEW|AD9910_SetSingleTone|AD9910_SetRamPlayback|AD9910_SetDigitalRampFrequency|AD9910_SetOsk|DDS_AppCalibrated|CH1|SPS|fail_count" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\dds_app.c'
```

Expected: every requested mode and driver call is present, and the mixed top bar has live AD7606B status text.

- [ ] **Step 7: Verify long press, wiring note, and RAM table imports**

Run:

```powershell
rg -n "UI_KEY_LONG_MS\\s+500U" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ui_menu.c'
rg -n "PB3/PB5|DDS_SCLK=PB13|DDS_SDIO=PB15" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910.c'
rg -n "AD9910_RAM_AMP|AD9910_RAM_FRE|AD9910_RAM_PHA" 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Inc\ad9910_ram_tables.h' 'D:\Users\港城泽人\Desktop\TFT+AD7606b\TFT+AD7060B\Core\Src\ad9910_ram_tables.c'
```

Expected: all three commands find the corrected implementation details.

---

## Handoff Checklist

- [ ] Keil build log reports `0 Error(s)`.
- [ ] New files are included in `.uvprojx`.
- [ ] No flash/download command was run.
- [ ] Two-key controls are KEY1 PE3 and KEY0 PE4 only.
- [ ] PROFILE1 and PROFILE2 are not software-controlled.
- [ ] DRG direction remains TIM4-driven.
- [ ] TFT still has `AD VIEW` for 8-channel AD7606B raw values, and DDS pages also show a mixed top bar with at least CH1 live value.
- [ ] Long press threshold is exactly 500 ms.
- [ ] AD9910 PB13/PB15 wiring note exists in code.
- [ ] `RAM_AMP[]`, `RAM_Fre[]`, and `RAM_PHA[]` have been ported from the AD9910 V1.2 source into the project.
