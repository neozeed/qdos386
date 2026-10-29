# Provenance and licensing

**Status: private research. Not for publication or redistribution.** This
package does not grant any licence to the Sinclair ROM or to the
reconstruction it was checked against, and it does not claim that the
interpreter is free to redistribute.

## What the work is based on

| Input | Source | Licence position |
|---|---|---|
| JS ROM (Sinclair QL, SuperBASIC) | Original Sinclair/QL ROM image. | Subject to its owners' copyright. No licence is granted by this package. |
| ROM reconstruction in GAS syntax | `rom/js-rom-gas-r2/src/` of the QLBox archive, a disassembly by Richard Zidlicky. | Its README states no licence is granted to the reconstructed ROM. Not copied into the interpreter. |
| QLBox emulator (headless) | `bin/linux64/qlbox-headless` in the QLBox archive. | Used only as a test oracle on this machine. Not distributed. |
| Musashi 68000 core | Inside QLBox. | Licence as stated in the QLBox archive (not reviewed for this package); not distributed here. |
| Keyboard tables | Copied in the ROM reconstruction from the Sinclair ROM. | Not used by the interpreter. |

## What is in this package

- Hand-written C89 source for the interpreter, its test runner and build
  scripts. Written from the behaviour documented in `JS-SUPERBASIC-MAP.md`,
  which was measured on the oracle.
- The corpus (`tests/corpus/*.bas` and `.out`), known-difference tests
  (`tests/known/`), and examples. The `.out` files are the ROM's output on
  these programs. They record behaviour, not ROM code.
- Tools: `tools/oracle.py`, `tools/gen_expected.py`, `tools/sweep.py`. These
  need an external QLBox checkout (`QLBOX_DIR`), which is not included.

## Known overlap with the ROM's text

The following are short strings copied from the reconstruction, not
written from scratch:

- Error message texts in `src/sb_error.c` (from `sys_mess.s`).
- Keyword spellings in `src/sb_lex.c` (from `basic_commands.s`), used for
  LIST output.

These are a few dozen words. They are the most likely item to need changing
before any wider release; replacing them with the behaviour-based names is a
mechanical change.

## What was deliberately not done

- No ROM code, disassembly text, or reconstructed routine is reproduced in
  the C source. The C implementation is written from observed behaviour and
  from reading labels, not translated instruction by instruction. Rewriting
  assembly in C does not by itself remove copyright or licence restrictions;
  this is a separate question that was not assessed here.
- The ROM reconstruction and QLBox are not in the tarball. The manifest lists
  only this package's files.

## Before any distribution

A separate licensing assessment is needed covering: the Sinclair ROM and
its reconstruction, QLBox and Musashi (the oracle), and the error and keyword
strings listed above. Until then this package stays private.

## SB3 additions

- The SB3 map (`docs/JS-SB3-MAP.md`) cites ROM source labels, table entries and
  addresses, and describes code paths in prose. It does not reproduce ROM
  instructions.
- The SB3 test expectations are ROM output captured through QLBox: the corpus
  `.out` files and the `tests/known/*.out` files. They are ROM-derived and have
  the same private-research status as the SB1 and SB2 expectations.
- The SB3 interpreter source (`src/`) was written from the oracle results and
  the map. It does not contain ROM instructions. Rewriting in C does not, by
  itself, remove copyright or redistribution restrictions on the ROM, so this
  package stays private research until a separate licensing assessment is done.
- No licence is granted for SB3. Do not publish reconstructed ROM code, and do
  not describe the interpreter as freely redistributable.
- The QDOS/386 adapter contract in `docs/SB3-CHANNEL-ABI.md` section 6 is a
  design only. The SB3 source and tests contain no QDOS/386 or R8 code, and SB3
  does not require the R8 kernel. Any R8 documents used for later design work
  are not part of this package and carry their own terms.

## SB4 additions

- The SB4 map (`docs/JS-SB4-MAP.md`) cites probe and corpus results, and
  ROM source labels only where it says so. It reproduces no ROM instructions.
- The SB4 expectations (`tests/corpus/68` to `99`, `tests/known/defproc_*`,
  and `tests/probes/sb4/*.out`) are ROM output captured through QLBox. They
  have the same private-research status as the earlier expectations.
- Two corpus programs were changed in SB4 to keep the oracle's channel rewrite
  out of their listings (`86_proc_list`: explicit `PRINT #3`, and `LIST` as a
  direct line). Their expectations were regenerated from the ROM. Nothing was
  edited by hand to match the interpreter.
- The SB4 interpreter source was written from the oracle results and the map.
  It contains no ROM instructions. Rewriting in C does not, by itself, remove
  copyright or redistribution restrictions on the ROM. No licence is granted
  for SB4. Do not publish reconstructed ROM code, and do not describe the
  interpreter as freely redistributable.
- The readiness audit (`docs/SB4-QDOS386-READINESS.md`) is a design document. It
  does not integrate with QDOS/386 and does not use or modify its kernel or any
  R8 code.
