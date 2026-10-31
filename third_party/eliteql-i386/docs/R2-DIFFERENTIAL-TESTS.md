# R2 verification scope

`make check` runs 33 fixture suites. All eleven R1 suites remain. Added suites
cover flight, tidy, combat, tactics, navigation, missiles, docking, targeting,
market, planet movement, station/WARP, encounters/pool, compass, dials, scanner,
planet projection/altitude, presentation, screen RLE/sound priorities, explosion,
dust and solar initialization. Original fixture drivers and fixture headers
remain unchanged. Logs: R2-TEST.log. Passing a source-derived fixture suite does
not automatically qualify a routine as independently verified against m68k.

`make platform-check` uses recording test adapters for file operation order,
144/148-byte compatibility, optional EQA2 extension, short reads/writes, error
precedence, 260 keyboard-queue bounds and sound output decisions. Actual OS
services are NOT VERIFIED. Log: R2-PLATFORM-TEST.log.

## Independent original-68000 execution

`make oracle-check` builds the syntax-adapted unchanged JAS algorithms with
actual m68k GAS 1.38, executes M68000 mode in Unicorn, and compares them with
actual i386 GAS 1.38 bytes from the binutils 1.94 final a.out image. No native
game logic is recompiled from C. ABI sentinels check i386 callee saves and stack.
Buffers and state bytes are compared directly; numeric short-array outputs are
compared after the explicitly documented endianness conversion.

| Suite | Cases | Deterministic output SHA256 |
|---|---:|---|
| movement: own_rotate, move_ship, rotate_body, tidy_ship, motion_position, motion_matrix | 2,048 each, 12,288 total | `75dc3d3d297ebf18b9fe3870ec96f940244b6a87aaff58126de36fd092638fd6` |
| raster: clipped lines, full framebuffer, guards, XOR reversal | 2,048 | `0eb5c78c366698ac1755d8a847772a1ab29557b9bec27cf3c323b37eca932993` |
| render: projected vertices, visibility, beam state, cached lines, frame bytes and erase; clipped circles | 2,048 ships + 512 circles | `8e1d565cb7edcecf37b16653bfc08eb8e5715388ba32fe1c3a9ce87411b4250b` |
| effects: 168-byte dust workspace/RNG/frame/guards; cloud points/RNG; full galaxy charts | 1,024 dust × 3 steps, 1,024 clouds, 64 charts | `46f3a58898eeaedd68d98c2d50a99b1ccd457477a7044122d1a57febcf565ff5` |

Seeds and exact input construction are in `tools/diff-*.py`. Combined historical
link execution is recorded in R2-ORACLE-ALL.log. Earlier per-suite logs also
record ELF-host-container runs of the same assembled instruction bytes.

The oracle loader supports both text and data relocations. Source conversion
uses equivalent absolute label addresses in place of PC-relative table syntax;
this fixes a discovered two-byte assembler dialect displacement difference.
An early native blueprint byte access was corrected to native numeric word
order. No known differences remain in the listed passing comparisons.

Uncovered whole-game scenarios—initialization through title, input/tick replay,
complete combat/death flow, docking/hyperspace menu transitions and returning to
title—are NOT VERIFIED because the shell is not translated. This is not Gate 4
completion. The historical link and runtime tests are necessary but not proof
of whole-game feature parity.
