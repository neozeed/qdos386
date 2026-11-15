# R7B2 handover — compiler accepted; FAT32 writer pending

**Part I is accepted; the two-part mission remains incomplete.** The aggregate
compiler crash is repaired and the native differential tests pass. The user
confirmed Elite runs and quits with the sequence below, closing the sole
remaining C4 runtime item. Part II has not started; the FAT32 write guard and
accepted filesystem source are unchanged. Manual acceptance supplements the
existing evidence; the earlier automation timeouts remain recorded as timeouts.

| Gate | Status and evidence |
| --- | --- |
| C0 provenance/reproducer | PASS: accepted binary pinned to R14B archive; all 289 normalized sources and 13 generated inputs verified; original failure reproduced from C and .i at both settings. |
| C1 causal diagnosis | PASS for immediate cause: malformed legacy call supplies one argument to a four-argument function. Same fault on LP64 and configured ILP32 hosts. Stock-GCC versus later legacy-edit attribution remains unresolved. |
| C2 minimal repair | PASS: two parentheses corrected in isolated compiler source; native minimal executables exit 0 at no optimization and `-O`. |
| C3 bounded ABI suite | PASS: 131 aggregate/ABI plus 110 representation checks per setting in QDOS; exact modern i386 GCC/Clang agreement; native GCC/HI also passes. |
| C4 compiler acceptance | PASS: existing production/runtime checks plus user-confirmed manual Elite run/quit. Compiler-only checkpoint: `apollo-r7b2-compiler`. Original external toolchain retained for rollback. |
| F0–F5 FAT32 implementation/acceptance | NOT TESTED / NOT STARTED. C4 prerequisite satisfied; no writable FAT32 candidate or combined FAT32 system disk. |

## Source and compiler identity

Project baseline `bd6f55f51acfb75acfd728a74484f30323ba335e`; safety ref
`r7b2-safety-bd6f55f`; current branch `r7b2-compiler-hardening`. Project receipts:
`3132cc0` (C0), `71fe379` (repair/tests), and `f9a3878` (initial C4 stop).
The manual-acceptance commit is frozen by new tag `apollo-r7b2-compiler`.
Existing tags remain unchanged: `apollo-r7a-disk-tools` → `3e49154`,
`apollo-r7b1-readonly` → `3aa5998`.

Separate compiler source history: baseline
`48929349e96e5401ef891d321f4958c64e46ed1f`, repair
`9491134ab6696591fc726dc1d81446d65b8de502`. The recovery archive SHA-256 is
`8186c252b318c6df1742d1de5af147840f8ca68fc92e197486b8ce5e3a0590e0`.
Original cc1 remains
`1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb`;
accepted repaired cc1 is
`06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.
Target remains i386 a.out/C89 with historical assembler/linker and SDK ABI.
The repaired compiler is built by installed host GCC 15.3; original host GCC 13.3 is
unavailable. `tools/r7b2-build-cc1.py` is the checked standalone build recipe.

The exact patch and permanent regressions are under
`toolchain/provenance/r7b2/` and `validation/r7/compiler/`.
[Compiler diagnosis and qualification details](R7B2-COMPILER-REPAIR.md) record
normalization provenance and unrelated pre-existing UBSan signed-overflow
findings. Those are not silently called sanitizer-clean or repaired here.

## C4 results and manual acceptance

Fresh sequential R5 24/64, R6A, R7A and R7B1 builds pass. **630 comparable
files, zero SHA-256 differences**, including accepted kernels, shells,
libraries and objects. SDK examples and base application targets build.
Fresh Hack and Elite also exactly match the private volume's existing binaries.
[Machine-readable comparisons](R7B2-C4-RESULTS.json) retain selected hashes and
full gate results; all 630 comparisons remain in the private evidence.

Native four-CPU/32-MiB checks pass for actual GCC compile/link/run of HI,
interactive SH/EXIT, storage worker requests/races/stale/full cases, Bounce
visible/hidden rendering, and Hack startup/movement/quit. Reused private game
assets and the existing guest GCC executable are identified as reused; no
claim is made that every historical payload or the guest compiler was rebuilt.
Runtime kernel/game bytes match freshly rebuilt outputs where compared.

The user confirmed Elite runs and quits: **`START ELITE /new`, switch to the
Elite window, then backslash, Q, Y**. This is manual user acceptance, recorded
verbatim in [the acceptance receipt](R7B2-ELITE-MANUAL-ACCEPTANCE.json); it is
not a new automated test. The earlier attempts used different startup/input
sequences and timed out after reaching the commander screen. Their logs,
screenshots and disk remain intact. The exact reason for those automation
timeouts is not asserted as proven. No kernel fault was recorded, and kernel
and Elite hashes match accepted bytes. Prior automated runtime partitions
passed `fsck.fat -n`, with FAT32 unchanged; no new post-manual-run fsck or disk
hash was supplied or inferred.

## Artifacts, preservation and next step

Temporary workspace: `/tmp/qdos386-r7b2-hardening-urqeomc5/`.
Durable private candidate/evidence directory:
`/data/reference/qdos386/r7b2-compiler-candidate-20261115-urqeomc5/`.
It contains the complete compiler source Git bundle, original/candidate
hosttools, build recipe, tests, compact logs/screenshots/receipts and SHA-256
manifest. This original candidate archive remains an immutable historical
record of the C4 stop. The new compiler-only acceptance receipt is stored under
`/data/reference/qdos386/r7b2-compiler-accepted/`; it binds the same verified
compiler/source hashes to the acceptance commit without altering that archive.
Large raw test disks and object files remain in the temporary workspace.

Exact rebuild: clone the private `compiler-source.bundle` to a new directory
under `/tmp`, select `aggregate-call-repair`, then run
`python3 tools/r7b2-build-cc1.py /tmp/<source-copy> /tmp/<new-output>`.
For the earlier automation timeout, durable evidence includes `elite-prompt-smoke.py`
and `guest.py`; the recorded QEMU invocation, source disk hash, key sequence,
screenshots and timeout are under `logs/elite-prompt-smoke/` and
`evidence/elite-prompt-smoke.json`. Start from a new copy of the accepted R7B1
image; no real or accepted disk is ever a writable test target.

All original project file contents and modes, private resources, external
original toolchain, and accepted/evidence image hashes were checked unchanged.
Only reviewed additive tests, compiler patch/build recipe and reports are
committed; pre-existing untracked instructions remain untouched. No network,
package installation, public export, push or destructive cleanup occurred.

**Next step:** begin Part II at F0's mutation ledger with the explicitly selected
accepted repaired compiler. Do not restart C0–C4 or repeat passing tests without
new cause. Retain the FAT32 write guard until the writer is independently
qualified. The previous high-cluster corruption remains negative evidence,
not fixed by the compiler repair or this manual acceptance.
