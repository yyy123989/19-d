# Serial DDS Single Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the PC send USART1 text commands to update AD9910 single-tone frequency, amplitude, and phase.

**Architecture:** USART1 remains the ADC waveform output port and gains a small RX ring buffer. A serial command task drains received bytes, parses line-based commands, and calls a DDS app API that applies clamped custom single-tone values.

**Tech Stack:** STM32F407 HAL/register code, Keil MDK ARMCC/ARMCLANG, Python unittest for host-side command protocol checks.

---

### Task 1: Command Protocol Test

**Files:**
- Create: `tests/test_serial_cmd_protocol.py`
- Create: `tools/serial_cmd_protocol.py`

- [ ] Write failing unittest cases for `DDS F=1000 A=8192 P=90`, `FREQ=2500`, `AMP=12000 PHASE=180`, invalid range, and query command.
- [ ] Run `python -m unittest tests.test_serial_cmd_protocol` and verify it fails because the parser does not exist.
- [ ] Implement the tiny Python protocol helper used by the test.
- [ ] Re-run the unittest and verify it passes.

### Task 2: Firmware USART1 RX

**Files:**
- Modify: `TFT+AD7060B/Core/Src/usart.c`
- Modify: `TFT+AD7060B/Core/Inc/usart.h`

- [ ] Add a USART1 RX ring buffer and `USART1_ReadByte`.
- [ ] Enable `USART_CR1_RXNEIE` during USART1 init.
- [ ] Update `USART1_TxIRQHandler` to service RXNE/ORE before TXE.

### Task 3: DDS Custom Single API

**Files:**
- Modify: `TFT+AD7060B/Core/Src/dds_app.c`
- Modify: `TFT+AD7060B/Core/Inc/dds_app.h`

- [ ] Store actual `single_freq_hz`, `single_amp`, and `single_phase_deg` values in the DDS state.
- [ ] Keep button stepping by copying table values into those actual fields.
- [ ] Add `DDS_AppSetSingleCustom(freq, amp, phase, mask)` that clamps ranges, switches to single mode, applies AD9910, and invalidates LCD cache.
- [ ] Draw and top bar display actual custom values instead of table-only values.

### Task 4: Firmware Serial Command Task

**Files:**
- Create: `TFT+AD7060B/Core/Src/serial_cmd.c`
- Create: `TFT+AD7060B/Core/Inc/serial_cmd.h`
- Modify: `TFT+AD7060B/Core/Src/main.c`
- Modify: `TFT+AD7060B/MDK-ARM/TFT_AD7606B.uvprojx`

- [ ] Add a line buffer that consumes `USART1_ReadByte`.
- [ ] Parse `DDS F=<hz> A=<0..16383> P=<deg>`, `FREQ=`, `AMP=`, `PHASE=`, and `DDS?`.
- [ ] Send compact `OK` or `ERR` responses on USART1.
- [ ] Call the task every main-loop iteration, before display/serial output work.
- [ ] Add `serial_cmd.c` to Keil project.

### Task 5: Verification

**Files:**
- No source changes.

- [ ] Run `python -m unittest tests.test_serial_cmd_protocol`.
- [ ] Run Keil build for `TFT_AD7606B`.
- [ ] Inspect git diff and report command examples to the user.
