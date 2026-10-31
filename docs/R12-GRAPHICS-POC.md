# R12 Stage 5: graphical surface proof — DEFERRED (design only)

**Status: not attempted in code. Gate 5 is not claimed.** The plan makes Stage 5 optional ("never block the text-mode release", "if a safe implementation is not readily available, document the design and defer"). Stage 5 was deferred because the text release took the available time and because a safe implementation needs kernel work that the audit below shows is not small. Nothing in this package changes the text-only images; default images stay byte-identical to R11M-FIX1 (`scripts/r11l-default-check.sh`, six hashes).

## Audit (what exists today)

| Item | State in the tree |
|---|---|
| Display | `platform/pc/vga.c` writes 16-bit character/attribute cells to physical `0xB8000` (80x25 text). Nothing else touches the VGA registers; the BIOS (SeaBIOS under QEMU) leaves text mode 3 active. |
| Virtual consoles | `kernel/vcon.c` keeps four 80x25 cell arrays in kernel memory and copies the visible one to `0xB8000`. Applications never see video memory (R11M/R12 console API). |
| VBE / PCI | None. No PCI probing, no Bochs VBE (`0x1CE/0x1CF`) code, no linear framebuffer mapping. QEMU's `-vga std` would offer one at a PCI BAR. |
| Paging | A paged job sees only its private 1 MiB window; the frame pool and the framebuffer are not mapped in job directories. A surface would need a window mapping or a copy-in API. |
| Tiny386 | Headless in the release smoke (no display); the surface would be QEMU-only. |

## Design (for a later round)

1. **Surface object, backend-neutral.** `struct qd_surface { int w, h, stride, fmt; owner job; unsigned char *pix; }` in the kernel heap (or a reserved region, since a 320x200x8 surface is 64,000 bytes — the shared heap holds one such block, but not several; see the stack finding in `R12-SUPERBASIC-PORT.md`). One bounded surface per system (320x200, 8-bit palette) for the first proof.
2. **API (libqdos, new console control channel ops).** `qd_gfx_create(w,h)`, `qd_gfx_destroy()`, `qd_gfx_pixel(x,y,c)`, `qd_gfx_rect(x,y,w,h,c)`, `qd_gfx_present()`, `qd_gfx_restore()`. Draw calls write into the kernel-side surface (no direct framebuffer access from applications).
3. **Present / restore.** `present` switches the display to graphics (VGA mode 13h by direct register programming, since there is no BIOS in protected mode, or Bochs VBE 320x200x8 via `0x1CE/0x1CF` and the LFB) and blits the surface to `0xA0000`/LFB; `restore` reprograms mode 3 **and reloads the 8x16 font into plane 2** (mode 13h's chain-4 writes destroy it), the palette and the CRTC text state, then asks `vcon` to repaint the visible console. The font data (4 KiB) has to be kept in the kernel image.
4. **Ownership.** The surface belongs to the job that created it; while presented, console switching (Alt+Fn) first restores text mode. Job exit or kill restores text mode in the same reclaim path that already returns the focus to console 0. All four text consoles keep working because text state is never discarded.
5. **Build option.** `-DQDOS_GFX`, off in default and R11M images, so those stay byte-identical.
6. **Demonstration (Gate 5).** QEMU graphical run: colour bars, then a moving rectangle for a bounded time, then return to the shell prompt; the proof is a screen capture in graphics mode (monitor `screendump`) and a VGA text-buffer capture after the return.

## Why it was not done safely in this round

* Restoring text mode correctly (font plane, palette, sequencer/CRTC/graphics-controller registers) is a hardware-register change that cannot be validated on the CPU-only harnesses and only partly in QEMU; a wrong restore leaves the machine with an unreadable display after a job is killed, which would endanger the text release's Alt+Fn and `KILL` guarantees.
* The kernel-heap budget (one ~0.9 MB heap shared by job stacks, the BASIC interpreter stack and channels) leaves little room for a framebuffer-sized surface next to three paged jobs.
* The plan forbids VBE window management, SDL, mouse or game ports in this milestone, so only the single-surface design above is in scope, and it still needs the kernel pieces listed.

Stage 5 is therefore a documented deferral; the text-mode release does not depend on it.
