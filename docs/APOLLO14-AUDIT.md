# Apollo 14 (R18) — Stage 0 audit and presentation contract

**Baseline**: tag `a13-r17` (R17 Apollo 13). **Branch**: `r18-apollo14-graphics`. **Opt-in target**:
`make qdos386-r18-pc` (`-DQDOS_R18` on top of `-DQDOS_R17`). Every R18 change is inside `#ifdef QDOS_R18`, so the
R17, R16B, R16A and default kernels rebuild byte-identical.

## What R17 has (audited)

| Area | R17 state | Source |
| --- | --- | --- |
| Display presenter | The only code that touches the VGA. It runs in the SYSTEM job every tick, with interrupts on, and switches between three physical states: text mode 3, mode 13h (320×200×256, chain-4) and mode 12h (640×480×16, planar, rendered text consoles only). Mode registers come from fixed IBM VGA tables, written by `write_regs`; the BIOS text state is saved at boot. | `platform/pc/vgapres13a.c` |
| Surfaces | INDEX8 surfaces (`struct gsurf`), at most **512×256** logically (`GS_MAX_W/H`), 256-entry RGB palette (0..255 per channel), dirty-row range, text stream, one surface per session, at most 4 at once, a 400 KiB pixel budget on the kernel heap. | `kernel/gsurf.[ch]`, `kernel/surface.c` |
| Presentation of a surface | Mode 13h only. 1:1 when it fits 320×200. Otherwise `GS_VIEW_FIT` (scaled nearest neighbour, aspect kept, centred) or `GS_VIEW_CROP` (a 320×200 window at vx, vy). **A 256×256 surface is FIT to 200×200: 56 of its 256 rows are dropped.** That is the EliteQL "missing lines". | `vgapres13a.c` `layout/src_row` |
| Mode 12h | Used for rendered text consoles (`VSESS_CM_GFX`). `put_line()` writes one 640-pixel row as four bitplanes (map mask 1 << p, write mode 0, bit mask 0xFF). The DAC is loaded with the 16 EGA text colours. | `vgapres13a.c` `to_gtext` |
| Session model | 20 sessions (0–3 legacy consoles, 4–19 private). The foreground session is presented. The Task Manager overlay forces text mode and the previous mode is redrawn when it closes. Focus changes only set a flag (`pres_focus_changed`); the presenter acts on the next tick. | `kernel/vcon.c`, `kernel/taskmgr.c` |
| Ring 3 API | `QT_SURFACE` (trap 0x3B) ops 0–36. User pointers are checked with `chan_user_range_ok`. The surface belongs to the job that opened it, and only the owner draws. Applications never touch VGA ports (no IOPL at CPL3). | `kernel/surface.c`, `sdk/include/qdos386.h` |
| SMP / BKL | The presenter runs in the SYSTEM job on CPU 0 (device CPU). Traps are serialised by the BKL; frame copies run with interrupts on and without the BKL held across the copy (the R16B rule). | R16B/R17 |

EliteQL's adapter (private fixture, `src/platform_qdos.s`) opens a 256×256 INDEX8 surface with the foreground flag,
sets `SURF_OP_VIEW` FIT, loads 8 palette entries, enables key events, and blits its whole 256×256 buffer each frame.

## Contract for R18

| Item | Rule |
| --- | --- |
| Input | The session's INDEX8 surface (any size up to 512×256), unchanged. Applications keep their pixels and palette. |
| Target | VGA mode 12h, 640×480, 16 colours, planar (the existing `mode12` register table). |
| Placement | `GS_VIEW_VGA12`: 1:1, centred: x0 = (640 − w) / 2, y0 = (480 − h) / 2. For 256×256 that is **(192, 112)**: every row and column is shown. `GS_VIEW_VGA12W`: each pixel twice as wide when 2w ≤ 640 (256×256 → 512×256 at (64, 112), the QL's own aspect); otherwise 1:1. No vertical scaling: 512×512 would exceed 480 lines. |
| Border | Colour 0 (DAC register 0 = palette entry 0). |
| Colour | Pixel value & 15 selects DAC register 0..15, loaded with palette entries 0..15 (RGB >> 2). Values 16..255 are shown as their low four bits (documented, deterministic, no invented colours). EliteQL uses entries 0..7. |
| Updates | Dirty rows only; a palette generation change reloads the 16 DAC registers; a full redraw on any focus, session, view or mode change. |
| Mode restoration | Leaving for text mode, mode 13h or a rendered text console uses the existing paths (`to_text`, `to_gfx`, `to_gtext`). Each reloads its own register table and DAC. |
| API | `SURF_OP_VIEW` b = 2 (VGA12) / 3 (VGA12W), c = 0. `SURF_OP_CAPS` bit `SURF_CAP_VGA12` (0x40). Older kernels refuse b > 1 with SURF_ERR_ARG, so a program can fall back to FIT. |
| User override | The user can choose how a session is presented without changing the program: Task Manager **F3** cycles program's choice → 640×480 1:1 → 640×480 2× wide → 320×200 fit; `/BIN/VIEW con APP\|FIT\|VGA12\|VGA12W`, and `SURF_OP_SESS_VIEW` (37) underneath. Pixels are never read or written by an override. |

## Gate 0 baseline (R17 kernel)

* `tests/r13a/run-gate.sh 2` (rendered text consoles, mode 12h): 26 checks, 0 failed.
* `tests/r13a/run-gate.sh 3` (logical 512×256 and 320×200 surfaces, FIT/CROP pixel-exact, palette restoration): 22
  checks, 0 failed.

Both logs are in `logs/a14/p0`.
