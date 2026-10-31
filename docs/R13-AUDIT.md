# R13 Phase 0 audit — display, input, sessions

Base: frozen R12C `7abb678b24764541845ef936adc45e9110f1a52b`, branch `r13-virtual-displays`. Baseline smoke
(`tests/r13/seq-p0-smoke.txt`, `logs/r13/phase0/`): the R12C kernels boot on QEMU and Tiny386 with the example volume,
`TASKS /L` 3 jobs, Tiny386 0x8900 shutdown clean, image clean.

## 1. What draws the screen today (R12C)

* `platform/pc/vga.c` (`QDOS_CON_VGA` builds only) writes character/attribute pairs to physical `0xB8000` in VGA
  text mode 3, which the BIOS set before the kernel started. It never calls the BIOS. Three entry points serve the
  console layer: `vdisp_put(row, col, cell)`, `vdisp_load(cells)` (whole screen) and `vdisp_cursor(row, col, on)`
  (CRTC 0x0A/0x0E/0x0F). The headless build (Tiny386, `build/r12c/pc/r12c.img`) has no display: `platform.c`
  provides empty `vdisp_*`, and the console text reaches the harness through the diagnostic stream.
* `kernel/vcon.c` (`QDOS_VCON`): **four static consoles** (`VCON_MAX 4`), each 80x25 cells + attribute, cursor,
  scroll region, input queue (128), cooked line, one input waiter, user count. `fg` is the console shown and fed by
  the keyboard. Output to a background console only updates its cells (`SHOWN(v)`); a switch reloads the whole
  screen (`vdisp_load`). Everything runs with interrupts off (traps, the keyboard IRQ, boot).
* R12C task selector (`kernel/taskmgr.c`): `vcon_overlay(1)` stops console drawing; the SYSTEM job's loop
  (`taskmgr_poll`, every tick) composes an 80x25 cell screen and loads it with `vdisp_load`; `vcon_overlay(0)`
  reloads the foreground console. It needs physical text mode.

## 2. Input and hotkeys

i8042 IRQ1 → `kbd_feed` (`kernel/kbd.c`): result 1 = a byte, 2 = Alt+F1..F4 (console number), 3 = Ctrl+Esc
(R12C). `platform/pc/i8042.c`: byte → `cin_put` → `vcon_input` (the open selector takes every key first; otherwise
the foreground console's queue, echo/cooked per mode, waiter woken); Alt+Fn → `taskmgr_console_key` /
`vcon_switch` **inside the IRQ** (a 4000-byte `vdisp_load`); Ctrl+Esc → `taskmgr_hotkey`. Hotkeys never reach a
program. Mouse: none.

## 3. Ownership

* `job_con[slot]`: every job belongs to one console; a new job inherits its creator's console unless the creator
  chose another (`vcon_set_child`, shell `START … /CON:n`, `qd_spawn` console argument). `users` counts live jobs;
  when the last job of the foreground console ends, the focus returns to console 0.
* CON channels record the opener's console at open (`ch->con`, `kernel/channel.c`); controls (`CHAN_CON_*`: goto,
  clear, attribute, query, mode, cursor, region, focus, child, snapshot, diag, caps) act on it.
* Consequence for R13: four surfaces, shared by console number, are the whole model. A per-job surface needs a
  session table larger than four, sessions created on demand, CON channels retargeted when a job moves to its own
  session, and teardown when the last user ends.

## 4. Jobs, processes, memory

`JOB_MAX` 16 (3 boot jobs: SYSTEM, the shell, IDLE; SYSTASK runs inside SYSTEM). Every job end (exit, kill, fault)
goes through `jobs.c` (lines 201 and 512) → `vcon_job_gone(slot)`. Process windows are private (R12A paging); the
kernel template identity-maps 0..16 MiB supervisor-only, and the kernel heap (`HEAP_END` = 0x200000) is mapped the
same in every address space — the only memory the kernel can address directly from any process context without
mapping work. Frame-pool pages in 0x200000..0x380000 are remapped in process spaces, so they are not safe to use
as kernel framebuffers. Decision: surface memory comes from the kernel heap, with explicit limits
(`docs/R13-MEMORY-BUDGET.md`).

## 5. VGA modes and the mode-switch path

* There is no real-mode BIOS path: the kernel runs in protected mode with paging, there is no V86 monitor and no
  BIOS thunk. `INT 10h` cannot be used. The only honest path is **direct VGA register programming**.
* Mode 13h (320x200, 256 colours, chain-4 linear at `0xA0000`) is the one indexed mode R13 will claim, and only
  after QEMU screenshots show it (Stage 2). 640x400/480 indexed modes need unchained/planar or VBE and are not
  attempted.
* What a switch must save and restore, because the BIOS state is not reproducible without the BIOS:
  1. the mode-3 register set (misc output, sequencer, CRTC, graphics controller, attribute controller), read back
     from the hardware at boot while the BIOS's mode 3 is still active;
  2. the 256-entry DAC palette (text mode uses entries 0..63 through the attribute palette);
  3. the text font in VGA plane 2 (8 KiB, 32 bytes × 256): mode 13h chain-4 writes land in all four planes and
     overwrite it;
  4. the text cells (already kept by `vcon`).
* Switching must not happen in the keyboard IRQ (Alt+Fn, Ctrl+Esc currently act there): the IRQ will only record
  the request; the SYSTEM job's tick loop (where SYSTASK already runs) does the register work (a short
  interrupts-off section for the register sequence only) and copies frames with interrupts on.
* Tiny386: the headless build has no display (`QDOS_CON_VGA` undefined). Graphics surfaces still work as memory;
  presentation is **NA** there.

## 6. Font

At Phase 0 no Minerva archive was attached and the session could not download one (hosts blocked), so the original
fallback `QDOS5x9` (5x9 glyphs in a 6x10 cell) was written first. During Stage 1 the user attached `min198.zip`
and the Minerva 1.98 GAS source; the Minerva QL fonts (`tb_f0`, `tb_f1`: also 5x9 in bits 6..2, QL character set,
GPL-2.0) were extracted with a source-vs-ROM byte check and are now the default; QDOS5x9 remains as a build option.
Details: `docs/R13-FONT-PROVENANCE.md`.

## 7. Baseline hashes to preserve

R12C `r12c-vga.img` `50ab9a6e…`, `r12c.img` `fc4a1368…`, `shell-c` `93b87d14…`; R12B `d22fd9f1…`/`01ebca11…`; R12A
`257460b9…`/`0b23ab9d…`; R12 `1e00386b…`/`dd11fe87…`; shells `dc50e084…`, `9ed34a73…`, `36720c5b…`; r11l default check
6 IDENTICAL. All R13 code is behind `-DQDOS_SURFACES` (kernel) and `R13_SHELL` (shell); `make r13-pc` builds the R13
images.
