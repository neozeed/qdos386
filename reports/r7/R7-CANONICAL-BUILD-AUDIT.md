# R7 canonical source/build ownership audit

Baseline `dafefb62fbc21f2753bca0cc48972805fdab1c09` is preserved by
`r7-canonical-safety-dafefb6` and its existing R7B2 tags. The kernel implementation
is byte-identical to that baseline. No historical application source changed.

| Previous active dependency/behavior | Current authority and verification |
| --- | --- |
| Root default 24-job R20 and release-ladder forwarding | One fixed R7B2 64-job FAT32 composition in `source/mk/current.json`; prior flags and object order preserved exactly |
| `$HOME/i386vm` host tools | Owned compiler/assembler/linker source; `tools/build-toolchain.py`; built tools under the generated tree |
| SDK `QDOS386_TOOLS/gcc-1.40/gnulib*.c` | Owned, normalized SDK runtime source in `source/sdk/compiler/`; no runtime source transformation |
| SDK archives written/read in source folders | Explicit fresh archives in generated `sdk/lib`, deterministic ar, SDK driver defaults to generated libraries |
| Boot3 sed `0x280000` → `0x340000` | Named `QDOS_TRANSFER_BASE`; binary equals accepted boot3 SHA-256 `74544eb0c64244a4484a21da866ecbd03a281838e5100fffa846786c927b2f19` |
| LD host header/parser copies and Python-generated default-script source | Owned host header/default scripts; parsers/lexer compiled directly from pinned source inputs |
| Shared profile intermediates and partial clean rules | Single marked output tree, content/configuration fingerprints and output hash checks; complete guarded clean |
| Temporary fallback disk hidden in a recipe | Source-only default disk; optional explicit relocatable `PAYLOAD_IMAGE`, read only, with loader/payload checks |

All active Makefiles are small entry points into the same production builder.
No sed/awk substitutions, source copies followed by edits, grammar regeneration,
or private absolute source paths occur in the production build. The installed
compiler driver is copied as an executable tool, not rewritten source. Images
copy existing historical payload bytes deliberately, with source ownership and
hashes reported separately.

Historical third-party and compiler Makefiles remain with their original source
for provenance; they are not silently reused as production recipes. Their old
host/profile/generator assumptions do not select the current build. Existing
application/test references are retained. Root README and docs/BUILD.md were
preserved verbatim under reports/r7 before replacing their obsolete entry points.

The existing Apollo BIOS stages continue to use host GNU as/ld, exactly as the
accepted R7B2 build did. This is not a replacement of historical GAS/LD for the
kernel or native applications. The newly built GAS 1.38/LD 1.94 produce exact
accepted kernel, shell, SDK and utility bytes.

Fresh-source verification at code commit `c813f2a` uses no prebuilt historical
host binaries or SDK archives. A full default rebuild was also traced with
`strace -f -e trace=openat,execve`. It accessed no `/root/i386vm`, old private
repository, private reference compiler tree, accepted R7B2 temporary workspace,
or external historical source. External source/header opens were host system
inputs under `/usr` or `/lib`, as expected when compiling the host tools.
The optional full private disk is separately verified with its explicit data
input; no source-only independence claim is made for recovered game assets or
the reused native guest compiler binaries.

Clean was checked both positively and negatively: source root, unmarked tree,
symlink output and `/tmp` itself are refused. A successful clean leaves no
tracked changes, untracked files or ignored generated outputs in the fresh
checkout. Rebuilding returns the same 37 binary/library/image products for both
payload modes. Logs and disposable runtime disks remain outside the source tree
under `/tmp/qdos386-r7-canonical-dqnq2r1o/`.
