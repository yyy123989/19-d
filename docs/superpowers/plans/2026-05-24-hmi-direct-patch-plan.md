# Serial HMI Direct Patch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Safely improve `D:\Users\港城泽人\Desktop\串口屏\1.HMI` so it matches the STM32 USART2 serial-screen protocol.

**Architecture:** Keep `1.HMI` as the binary source of truth, create a timestamped backup before touching it, and patch only verified text/script sections whose location and size are known. Generate a companion protocol/readme so the editor-side page objects and STM32 commands stay aligned.

**Tech Stack:** Windows PowerShell, Python binary inspection scripts, USART HMI binary project, STM32 USART2 command protocol.

---

### Task 1: Inspect and Back Up the Binary Project

**Files:**
- Read: `D:\Users\港城泽人\Desktop\串口屏\1.HMI`
- Create: `D:\Users\港城泽人\Desktop\串口屏\backup\1.<timestamp>.HMI`

- [ ] Verify `1.HMI` exists and record its size.
- [ ] Create the `backup` folder.
- [ ] Copy `1.HMI` to the backup folder before modifications.
- [ ] Parse the archive header enough to locate page and program sections.

### Task 2: Patch Only Verifiable Program Text

**Files:**
- Modify: `D:\Users\港城泽人\Desktop\串口屏\1.HMI`
- Create/Modify: `D:\Users\港城泽人\Desktop\串口屏\HMI_Project\Program.s`

- [ ] Build a short `Program.s` that keeps `baud=115200`, `dim=100`, `recmod=0`, `page 0`, and page tracking.
- [ ] Confirm the replacement script fits inside the existing `Program.s` binary section.
- [ ] Patch the script section and zero-pad unused bytes.
- [ ] Re-read the section and confirm the expected script is present.

### Task 3: Generate Final HMI Protocol and Layout Guide

**Files:**
- Modify: `D:\Users\港城泽人\Desktop\串口屏\HMI_Project\HMI创建指南.md`
- Create: `D:\Users\港城泽人\Desktop\串口屏\HMI_Project\STM32串口协议.md`

- [ ] Document page 0 controls `t0`, `va0` to `va7`, and `t1`.
- [ ] Document page 1 waveform control `w0` and page 2 system controls.
- [ ] Document STM32 commands exactly: `vaN.txt="..."` and `t1.txt="..."`, terminated by `FF FF FF`.
- [ ] Document that `1.HMI` must be opened/downloaded with USART HMI editor.

### Task 4: Verify

**Files:**
- Read: `D:\Users\港城泽人\Desktop\串口屏\1.HMI`
- Read: `D:\Users\港城泽人\Desktop\串口屏\HMI_Project\*.md`

- [ ] Check backup exists and original-size copy was made.
- [ ] Search the binary for `baud=115200`, `page 0`, and `page_now`.
- [ ] Check docs mention `va0` through `va7`, `t1`, `115200`, and `FF FF FF`.
- [ ] Do not claim editor download or hardware behavior unless the editor/hardware is actually used.
