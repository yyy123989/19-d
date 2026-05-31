# Rin Rout Gain Measurement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a MEASURE page that calculates gain, input resistance, and output resistance from AD7606B channel peak-to-peak measurements.

**Architecture:** Add a focused `measure_app` module that accumulates AD7606B min/max samples in the BUSY interrupt and computes Vpp-based measurements in the main/UI path. Integrate the page into the existing DDS menu without changing the DDS signal-generation modes.

**Tech Stack:** STM32F407VET6, STM32 HAL, Keil MDK ARMCC, existing AD7606B serial driver, existing TFT LCD drawing layer.

---

### Task 1: Measurement Module

**Files:**
- Create: `TFT+AD7060B/Core/Inc/measure_app.h`
- Create: `TFT+AD7060B/Core/Src/measure_app.c`
- Test: `tests/test_measure_feature.py`

- [ ] Add default known resistor constants for `Rs=10000 ohm` and `RL=1000 ohm`.
- [ ] Accumulate channel min/max from each valid AD7606B sample frame.
- [ ] Compute `Gain = CH3_Vpp / CH2_Vpp`.
- [ ] Compute `Rin = Rs * CH2_Vpp / (CH1_Vpp - CH2_Vpp)`.
- [ ] Capture output no-load/load Vpp from CH3 and compute `Rout = RL * (V0 - VL) / VL`.

### Task 2: UI Integration

**Files:**
- Modify: `TFT+AD7060B/Core/Src/dds_app.c`
- Modify: `TFT+AD7060B/Core/Src/main.c`
- Modify: `TFT+AD7060B/MDK-ARM/2019_D.uvprojx`

- [ ] Add `MEASURE` menu mode.
- [ ] Draw channel mapping and computed values on TFT.
- [ ] Use KEY0 short to capture output resistance open/load stages.
- [ ] Use KEY1 short to reset output resistance captures.

### Task 3: Verification

- [ ] Run `python -m unittest tests.test_measure_feature tests.test_serial_cmd_protocol`.
- [ ] Build Keil target `2019_D`.
- [ ] Confirm `0 Error(s), 0 Warning(s)`.
