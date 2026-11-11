# Display (canonical R18/R20 PC baseline)

Files: `kernel/vcon.c|h` (virtual consoles and sessions), `kernel/surface.c|h` (surfaces, `QT_SURFACE`), `kernel/gsurf.c|h`
(graphics surfaces), `kernel/fontreg.c|h`, `kernel/font*.c` (fonts), `kernel/kev.c|h` (key events), `kernel/kbd.c|h`
(keyboard), `platform/pc/vga*.c` (VGA output), `platform/pc/platform.c` (console output: debug port 0x402, and VGA when
built with `QDOS_CON_VGA`). Full ABI notes: `docs/R13-DISPLAY-ABI.md`, `docs/R13A-GRAPHICS-ABI.md`,
`docs/APOLLO14-DISPLAY-ABI.md`.

## Model

- **Virtual consoles** (`QDOS_VCON`): 80×25 text consoles with their own cells, cursor, attribute, scroll region and input
  queue. Exactly one is foreground: the display shows it and keyboard input goes to it. Output to a background console
  updates only its buffer.
- **Sessions** (`VSESS_MAX` = 20): slots 0–3 are the legacy consoles, 4–19 private sessions. A session owns a key queue
  (`KEV_QSIZE` 64; oldest kept, newest dropped and counted).
- **Surfaces** (`QT_SURFACE`, version `SURF_VERSION` = 1): `TEXT80x25` is the session's cells (the console API); `INDEX8` is
  an optional framebuffer the session can show instead (at most `SURF_GFX_MAX` = 4 at once). Only the foreground session is
  presented and receives keys.
- **Presentation**: the platform shows the foreground session. VGA text is used by the VGA build; the debug port carries
  the diagnostic text in both builds.

## Interrupt discipline

Every display function is called with interrupts off: from a trap, from an interrupt handler (keyboard), or from boot. The
keyboard handler may switch the foreground console and queue input; traps write output and read input. They never interleave,
so the display code takes no lock (`docs/LOCKING.md`).

## Limits

See `docs/R20C-LIMITS-INVENTORY.md` (sessions, surfaces, fonts, key-queue size).
