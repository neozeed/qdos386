# R3 shell translation and evidence

The original `reference/src/shell.s` remains unchanged: 6,666 lines and 1,112
labels, including data and internal branch targets. Every original instruction
line and label now has native translation ownership in
`R3-SHELL-INVENTORY.json`. `R3-SHELL-DATA.json` records the data layout.

This inventory is a source audit, not a feature-completion percentage. Functional
claims require execution evidence. The full original shell is assembled for a
68000 oracle; the native side executes bytes assembled by genuine GAS 1.38 and
linked by binutils 1.94. The original source is not replaced with test algorithms.

Current differential suites cover scene setup; remember/restore/title/death;
compact universe removal and spawning; complete save/load orchestration; full
menu/chart pages; all six existing cheats, warp and encounter orchestration;
and 800 actual main-loop iterations through both complete title approaches,
launch, firing, steering, status overlays, quit cancellation, death and restart. They compare original state and complete
packed framebuffers. The frozen R2 suites retain the arithmetic, movement,
rasterizer, rendering, effects, gameplay and platform regressions.

`src/shell_flow.s` is source-specific static native assembly with explicit
hardware-register allocation. It contains no runtime decoder or emulated
register bank. Handwritten assembly implements the more register-intensive
scene, child-spawn, universe, world and platform procedures. Source-line comments
and the lowering script make each generated instruction sequence reviewable.

`build-shell-audit.py` produces a test-only image whose unresolved OS boundaries
trap immediately. The production `ELITE.AOU` links the real platform adapter and
contains none of these guards. Production OS session evidence is tracked
separately from the differential suites.

The title-load failure is reproduced in the original oracle and documented in
`R3-VALIDATION.md`. Translation coverage does not hide that source defect, the
unsupported QL IPC output, or the unavailable QEMU acceptance run.
