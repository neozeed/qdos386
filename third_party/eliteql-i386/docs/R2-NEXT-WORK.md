# Continuation from this checkpoint

Read the supplied original R2 handoff, this README and known limitations first.
Do not restart the verified core or add QDOS386 drivers. The main remaining
assembly work is `reference/src/shell.s`; it is not merely a hardware adapter.

Preserve its original initialization, input ordering, menus, title/death flow,
modal pause behavior, universe insertion/removal, save/restore sequencing, text
font, developer behavior and existing cheats. These are already in the source;
do not invent substitutes. Translate internal shell logic into native i386 and
isolate only the external operations listed in R2-PLATFORM-BOUNDARY.md.

Keep byte-encoded state big-endian where documented. Model, matrix and render
cache numeric words are now native; shell byte accesses into former 68000 word
variables need a deliberate audit (for example key-word low byte at +1 in the
original). Commander CASH remains BE32 even though ordinary native shell longs
will be little-endian. A generic register-bank interpreter is not an acceptable
replacement for this work.

Build a deterministic headless driver for the actual translated main loop,
with recorded key rows/ticks and explicit file/sound/frame adapters. Keep game
logic in assembly. Extend original-68000 comparisons to the remaining core and
whole-game flows. Only then change gate results or produce a COMPLETE archive.

Current oracles: `make oracle-check`. All original fixture drivers remain
unchanged: `make check`. Recording boundary tests: `make platform-check`.
