# Apollo 14 (R18) — known limitations

## Modes and hardware

* **Only standard VGA modes are used:** text mode 3, 320×200×256 (13h) and 640×480×16 (12h). There is no VBE or SVGA,
  so no packed 8-bit mode above 320×200. That is why a 256-colour surface larger than 320×200 is shown either scaled in
  mode 13h or in 16 colours in mode 12h.
* **16 colours in mode 12h:** a pixel is shown as its value & 15, through palette entries 0..15. Programs with more
  than 16 colours, or that use indices ≥ 16, look different in VGA12/VGA12W than in mode 13h; the pixels are kept
  unchanged. EliteQL (8 colours) is exact.
* **No vertical scaling:**
  * 512×512 (EliteQL's 256 rows doubled) does not fit 480 lines and is not offered.
  * VGA12W doubles only horizontally (512×256 for EliteQL, the QL aspect).
  * Logical surfaces stay limited to 512×256 (R13A).
* **Tested only in QEMU** (`-vga std`, SeaBIOS, TCG). Real VGA cards, other emulators (Tiny386 has no VGA output in this
  build), and monitors' handling of the 640×480 timing were not tested. No firmware setting is touched.
* **No vsync:**
  * The presenter copies dirty rows at the 100 Hz tick, without waiting for vertical retrace, so a frame can tear.
  * A snapshot of a moving picture can catch the presenter mid-copy: in the Elite run the animated title differed in a
    few dozen pixels, while every paused or static frame was exact.

## Performance

* **Planar conversion is done in C, row by row** (one gathering pass, then four plane writes of 20 dwords).
  * A full 640×480 redraw writes 480 rows, while an EliteQL frame writes only its dirty rows (up to 256), at most 64
    per call.
  * The presenter runs in the SYSTEM job with the BKL held and interrupts off (the R16 rule for the SYSTEM poll).
  * Its cost is measured (`SURF_OP_PRES_TIME`, `PLANAR TIME`) and reported in the runtime report. It is TCG time, not
    hardware time.
  * Write mode 2/3 or latch copies could reduce it further, but were not attempted.
* **Every changed row is redrawn whole (640 pixels),** even if only a few pixels changed.
* **Bounded update rate:** to bound the interrupts-off time, the presenter draws at most 64 rows per 10 ms tick
  (`G12_ROWS`). A program that changes all 256 rows every frame (EliteQL blits its whole screen) is therefore shown at
  up to 25 full updates per second, and a switch to a 640×480 surface takes 8 ticks to complete.

## Interface

* **The user override is per session and lasts until the session ends.** It is not remembered across runs, and there
  is no START option to set it at launch. Use Task Manager F3 or `VIEW con …` after the program has opened its
  surface.
* **A program that calls `SURF_OP_VIEW` again is still overridden** while a user override is set. Its own view
  returns when the override is cleared (APP).
* **The override can be set by:** a job on the session, its creator, the parent of a job on it, or a child of any of
  those. Two programs started from the same shell can therefore change each other's presentation. Pixels and palette
  are never exposed by this.
* **The Task Manager rule:** after the selected job ends, the first key selects the first row (R12C). Scripts that end
  several jobs press End twice.
* **`START prog /NEW` from a test harness:** the R12 harness waits for the shell prompt, which a full-screen
  graphics job hides until it ends. Harness sequences start such programs with `!START` (not a kernel issue).

## Not in scope (as instructed)

VBE/SVGA, accelerated graphics, mouse, overlapping windows, 3D, sound, BKL splitting, SMP, memory, filesystem and
compiler changes.

EliteQL is a separate private application. Its frozen `ELITEP.AOU` is used unchanged, through the user override, and
it is not part of the public package.
