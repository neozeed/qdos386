# QDOS/386 R20C: canonical source consolidation (handover)

Source-only consolidation of the frozen Apollo 16 Phase 2B tree. No functional change is intended, and the
verification below shows that the supported production outputs are byte-identical to the frozen build.

## Identity

| item | value |
|---|---|
| freeze tag | `a16-r20-phase2b-frozen` -> commit `93fd6a503a2720f08e6bf7edd0c48d17e6967526` (annotated, SSH-signed; signature not re-verified here) |
| canonical branch | `r20c-canonical-consolidation`, head `ba47006` |
| canonical tag | `r20c-canonical-source` on `ba47006` (see "Tag" below) |
| commits since freeze | 6 (listed in `reports/commits.txt`) |
| mechanical diff | 19 files, +941 / -142 (`reports/source-diff-stat.txt`, `reports/source-diff-numstat.txt`, full patch in `patch/`) |
| R18 VGA image | `d42497d51da87909e0cb0e52259d2bfd42807923e710d9945285db166a228e97` (= `binaries/r18-vga.img`) |
| R20 VGA image | `dd4c9129c476b6da8abf8b657df65543c9300355686d7a9b30d5ea43b49f5bca` (= `binaries/r20-2b-vga.img`) |

## What is in this package

- `source/`: the canonical tree, exported with `git archive` from `ba47006`. It contains the kernel, platform, SDK,
  the kernel test sources, the non-private application sources the PC build links, the Apollo 16 scripts, and the
  design and inventory documents. It does not contain generated products, the private trees, or the run logs (see
  "Excluded").
- `binaries/`: reference outputs from the freeze release: `r18-vga.img`, `r20-2b-vga.img`, and the nine protected test
  programs (`R20XP`, `R20XKP`, `R20DP`, `R20KP`, `R20GSP`, `R20GWP`, `R20GLP`, `R20CP`, `R20PP`).
- `patch/r20c-from-a16-r20-phase2b-frozen.patch`: the complete diff from the freeze tag to the canonical head.
- `reports/`: diff statistics, commit list, the fresh-rebuild verification log, and the smoke-run serial log.
- `docs/`: the module, ABI, locking, storage, display, configuration and build documents, the conditional and limits
  inventories, `SOURCE-INVENTORY.csv` (135 rows), and `R20C-BASELINE-HASHES.txt` (250 output hashes).
- `SHA256SUMS`: hashes of every file in this package except itself.

## Build and check

Requirements: the i386VM toolchain tree set as `QDOS386_TOOLS` (see "External dependencies"), and Python 3.

    tar -xzf qdos386-R20C-CANONICAL-SOURCE.tar.gz && cd qdos386-R20C-CANONICAL-SOURCE
    sha256sum -c SHA256SUMS
    cd source
    . <your toolchain env>                # sets QDOS386_TOOLS
    make qdos386-r20-pc && make qdos386-r18-pc
    ( cd build && find qdos386-r20 qdos386-r18 -type f \( -name '*.o' -o -name '*.img' -o -name '*.bin' -o -name '*.raw' -o -name '*.aout' \) | sort | xargs sha256sum ) > /tmp/hashes.txt
    diff docs/R20C-BASELINE-HASHES.txt /tmp/hashes.txt     # must print nothing (250 lines)
    cmp build/qdos386-r20/pc-vga/qdos386-r20-vga.img ../binaries/r20-2b-vga.img
    cmp build/qdos386-r18/pc-vga/qdos386-r18-vga.img ../binaries/r18-vga.img
    sh scripts/a16-p2b-build.sh                         # builds the nine test programs in build/a16/
    for f in R20XP R20XKP R20DP R20KP R20GSP R20GWP R20GLP R20CP R20PP; do cmp build/a16/$f.aout ../binaries/$f.aout; done

## Verification results

| check | result |
|---|---|
| fresh extraction of this tarball into an empty directory, then `make qdos386-r20-pc` and `make qdos386-r18-pc` | rc 0 / rc 0 |
| all 250 outputs against `docs/R20C-BASELINE-HASHES.txt` | 250/250 identical |
| R18 VGA and R20 VGA against the freeze release images | identical |
| nine test programs (`scripts/a16-p2b-build.sh`) against the packaged copies | 9/9 identical |
| QEMU smoke (headless R20, external test volume, see below) | no panic, fault or trap lines; readers and kill helper completed |

The rebuild log is in `reports/fresh-rebuild-verification.txt`. Every check above was run on the extracted copy in a
clean directory, not on the working tree.

## Source changes (summary; full detail in `docs/R20C-CONDITIONAL-INVENTORY.md`)

- Removed six never-defined or dead conditional macros: `QDOS_NO_RTC`, `QDOS_R16_APNOTIMER`, `QDOS_R17_TEST_APFAIL`,
  `QDOS_TEST_SPARE` (with its R11I self-test block in `kernel/aout.c`), `QDOS_CON_SERIAL` and `QDOS_HALT_DEBUG_EXIT`.
  Eight conditional openers were removed (kernel and platform: 621 at the freeze tag, 613 now).
- The removed branches were inactive in all four supported builds. The selected branch is the one that the build
  already compiled.
- Documentation and inventories were added (no code change in commit `ba47006`).
- Repaired the test-tool recipes in `scripts/a16-p2b-build.sh`: `R20DP` uses `-DR20D_WAIT=40000`; `R20KP` and
  `R20XKP` use the `R20K` and `-DR20X_N=1000` recipes. Corrected the stale scratch path in `docs/A16-PHASE2A.md`.

Known caveat about the record: the commit messages of the removal commits say "250/250 identical". That result was
verified on the final head `ba47006`, not after each intermediate commit. The intermediate states were not rebuilt
separately.

## Limits (unchanged)

No limit was changed. The values are recorded in `docs/R20C-LIMITS-INVENTORY.md`. The job table is 24 slots in the
R20 build (`R17_JOB_MAX`, selected by `QDOS_DYNPROC && QDOS_R17`), not 16: 16 ordinary slots, plus up to 8 AP idle
jobs. Other values include `CHAN_MAX` 16, `VCON_JOBS` 16, `QD_READ_MAX` 256, `R20_MAX_MISS` 64, and the cache budget
`R20C_BUDGET_MAX` 2 MiB (min with 5 % of usable RAM).

## Smoke run (what was run, what it showed)

Headless R20 image (`qdos386-r20/pc/qdos386-r20.img`) from the extracted tree, with the external test volume
`build/a16/p2b-xvol.img` attached read-only through `snapshot=on`. Debug port 0x402 (`isa-debugcon`) to a file. `-smp 2`,
16 MiB. Run for 300 seconds with `timeout`. The volume's `/ETC/STARTUP.CMD` runs eight `START /BIN/R20XK` readers,
then `R20K` (kill helper) and `R20D` (state dump). Serial log: `reports/smoke-serial.txt`.

- Eight readers started (job ids 65541 to 65548).
- Four readers completed: `expected=1000 completed=1000 ok=1000 short=0 mismatch=0 ioerr=0 giveup=0`.
- `R20K` killed the other four (`rc=0`).
- The `R20D` dump was taken while readers were still running. At that point it showed `bits=12 held=12`, `waiters=1`,
  `queue=1`, `misses=423`, `fill_err=0`, `gaveup=0`. This is not the idle zero baseline, which the Phase 2B 1500 s run
  reached. This short run did not capture a quiescent end state.

## External dependencies (not in this package)

1. The i386VM toolchain tree, set as `QDOS386_TOOLS` (and `I386VM`). The pinned tool hashes are in `docs/BUILD.md`:
   `as` `47369dd5…`, `i386-gcc` `3567f3be…`, `cc1` `1dd52eeb…`, `i386-ld` `dfdb3965…`.
2. The historical GCC 1.40 gnulib sources at `$(QDOS386_TOOLS)/gcc-1.40/gnulib.c` and `gnulib2.c`. The SDK compiler
   library is generated from them (`sdk/compiler/Makefile`, `GNUSRC`), and the generated `gnulib*.c` copies are
   git-ignored. Without that tree the SDK libraries cannot be rebuilt.
3. The external test volume base, from which `scripts/a16-p2b-sysvol.sh` derives the test volume. The base volume is
   not part of this repository or the package. The kernel build does not need it; the runtime tests do.

The package does not reproduce this environment on a machine other than the one used for the verification. The clean
rebuild was done in the same toolchain environment, extracted into an empty directory.

## Excluded from this package, and tracked on the branch

The package excludes SuperBASIC, Infocom, Hack, QLTerm, Minerva, Phoon and kctx evidence, the `third_party/` tree, the
`logs/` tree (about 167 MB of run logs), the `patches/`, `harness/` and `launch/` trees, and the 30 generated SDK
products and the two generated `gnulib*.c` files. All of these are normal build outputs or private content.

The branch itself still tracks several of these trees (`apps/superbasic`, `third_party/`, `logs/`), and they are in
the history of the freeze tag too. Removing them from the branch would be a repository decision, and I did not make it
in this consolidation. Their presence on the branch is a known open item.

## Not done or not verified

- Rebuild on a clean machine with a separately pinned toolchain. The rebuild used the same toolchain environment.
- Re-verification of the freeze tag's SSH signature (no `ssh-keygen` here).
- Runtime tests beyond the short smoke run above. The full R20 suites were not re-run.
- The Phase 2A cause, the injected-device-failure masking, and cold streaming performance: unchanged from
  `docs/A16-CACHE-RESULTS.md`.
- About 30 inactive macros named only by non-supported targets remain in the source. They are listed with their
  disposition in `docs/R20C-CONDITIONAL-INVENTORY.md`. Compound `#if` expressions and the ACTIVE conditionals were not
  simplified.

## Tag

Tag `r20c-canonical-source` is an annotated tag on `ba47006`, created with `git tag -a -s`. It carries an SSH signature
block with the same signing-key prefix as the freeze tag. This environment has no `ssh-keygen` and an empty
`~/.ssh/commit_signing_key.pub`, so the signature was not verified here. The freeze tag `a16-r20-phase2b-frozen` was not
moved or re-created.
