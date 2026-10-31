# QDOS/386 R10-VGA: interactive VGA text console

Branch `r10-vga`, started from the frozen R10 FPU tag `r10-fpu-frozen` (533ee57).

## What changed

- `platform/pc/vga.c`, `platform/pc/vga.h` (new): 80x25 colour text at 0xB8000.
  Character/attribute writes, CR, LF, BS, TAB, wrap, scroll, clear, default
  attribute 0x07. The cursor is moved through CRTC registers 0x0E/0x0F. No BIOS
  video calls. The whole file is compiled only with `QDOS_CON_VGA`.
- `platform/pc/platform.c`: `con_putc` still writes port 0x402 always, and
  optionally COM1 (`QDOS_CON_SERIAL`). With `QDOS_CON_VGA` it also calls `vga_putc`.
- `Makefile`: `vga.o` added to `PC_OBJS`.
- `launch/`: Tiny386 and QEMU launchers and configs for the interactive and
  diagnostic modes.

Kernel behaviour is otherwise unchanged. The default image (no VGA macro) is
byte-identical to the image before the VGA change (sha256 8b4c7b86…), so the
diagnostic path is unchanged by construction.

Keyboard input is the existing PS/2 path (i8042, IRQ1, kbd.c). No keyboard code changed.

## Boot modes

- Diagnostic: the original automated configuration. Serial, 0x402 output, scripted
  and regression paths. Image `r9shell-default.img` (8b4c7b…) or
  `r9shell-diag-serial.img` (51825e…, serial plus halt debug exit).
- Interactive VGA: image `r9shell-vga-interactive.img` (6bbfc1…), built with
  `QDOS_CON_VGA`. It boots to `QDOS/386 R8 shell. Type HELP.` and does not run the
  regression suite or shut down.

Disk layout for both emulators: boot image on IDE primary master, FAT16 data
volume `win1-vga-data.img` (758c5a…) on IDE primary slave. The data volume
contains SHELL.AOU, HELLO.AOU (slot-1 link), FPJOB.AOU, README.TXT, and CHAIN.AOU
(a deliberate bad-chain fixture placed by `make-fat16.py`).

## Evidence (labelled)

All keystrokes in this package were **scripted**: xdotool X-input injection for the
graphical windows, and `tests/interactive/session.py` for the diagnostic session.
No human-operated session was recorded. Please run the launchers yourself to
confirm the experience by hand.

| Check | QEMU (scripted xdotool) | Tiny386 windowed (scripted xdotool) |
|---|---|---|
| Prompt on VGA screen | yes | yes |
| HELP, DIR | yes | yes |
| TYPE README.TXT (63 lines, scrolls) | yes | yes |
| RUN HELLO.AOU, twice (exit status 7) | yes | yes |
| Backspace line editing | yes (DIRR, BS, BS, IR -> DIIR) | yes |
| Unknown command | yes | yes |
| FPJOB.AOU | yes (tw=ffff) | yes (tw=0000, see issues) |

Screenshots: `evidence/screenshots/`. Machine-readable logs: `evidence/logs/`
(QEMU 0x402 output captured with `isa-debugcon`; Tiny386 stdout).

Diagnostic configuration, default image, headless Tiny386, scripted PS/2 session:
DIR (R9 volume, 23 files), RUN HELLO.AOU (exit status 7). FPJOB is absent on
that volume, which is expected. Log: `evidence/logs/diagnostic-default-image-scripted.txt`.

## Not done in this milestone

- The full R2–R10 automated matrix (`scripts/r9-matrix.sh`, `run-all.sh`) was not
  re-run. It needs the historical i386VM harnesses (VMP/VMR9) and the headless build
  across several rounds. This milestone ran only the boot, diagnostic and scripted
  checks above. Kernel behaviour is shown unchanged by the byte-identical default image.
- No human keyboard session.

## Known issues

- Tiny386 window: the last text row is partly clipped in the 720x400 window. Cosmetic.
- FPJOB snapshot tag word reads 0000 on Tiny386 and ffff on QEMU. This is the
  known emulator behaviour from the R10 FPU milestone (FNSTENV tag word). The
  FPJOB result is still clean and exact on both.
- An earlier QEMU interactive run loaded the slot-0 HELLO.AOU, so RUN refused it
  ("not a slot 1 program"). The data volume was rebuilt with `hello1.aout`. The
  earlier volume hash 734a09… is superseded. All evidence here uses 758c5a….
- The QEMU boot stall at EIP 0x7c8a noted in the frozen R10 notes was not seen
  in this milestone.

## Emulator patches

None are required for the VGA milestone. The Tiny386 x87 rounding patch from R10
is kept separately in `emulator-patches/`. The Tiny386 binary used for the windowed
screenshots is a local build of the patched dev tree. It is not the user's Tiny386
and does not replace it.
