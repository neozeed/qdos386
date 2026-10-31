# R3 intake and initial native shell checkpoint — 30 October 2026

Frozen R2 source: 86bc95e5772e8da4b90b510b3dc3c450e256f0ed.
Separate working branch: elite386-r3-shell-qdos.

KCTX-FIX2 supplied archive and normal VGA kernel hashes match the handoff.
The OS's complete supplied SHA256SUMS verification passed: R3-OS-MANIFEST.log.
No OS file was changed. No OS/private components are bundled in this checkpoint.

R2 `make check platform-check oracle-check` passes unchanged. Actual bundled
GAS 1.38 and historical binutils 1.94 linker are used. New native shell routines
link with the core and pass independent original-68000 comparisons (see results).
The original reference shell remains byte-for-byte unchanged.

Gate 0: R2 regressions green, every shell label inventoried; platform calls
inspected and mapped. Detailed key-action, state layout and whole-palette audits
still remain alongside full translation.
Gate 1: NOT MET — 1,091 original shell labels unported.
Gate 2: NOT MET — no actual full-game headless execution.
Gates 3/4: NOT TESTED — no integration or QEMU gameplay.
Gate 5: NOT MET — reproducible WIP source checkpoint only.

No native game runtime, FPS, live graphics, sound, focus or file result is claimed.

A fresh source-only extraction rebuilt and passed `make shell-check`. Both new
assembly objects and the final historical link image match byte-for-byte.
Log: R3-FRESH-SHELL-TEST.log. R2 native source and differential scripts were
compared against the frozen working tree and are unchanged.
