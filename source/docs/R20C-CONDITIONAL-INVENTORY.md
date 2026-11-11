# R20C conditional inventory

Scope: every preprocessor conditional in `kernel/` and `platform/` (613 conditional openers in 84 files now;
621 in 84 files at the freeze tag `a16-r20-phase2b-frozen`). Source-only; the production images are byte-identical
(see `docs/R20C-BASELINE-HASHES.txt`).

## Supported builds (this branch)

Only the four R18 and R20 PC builds that the freeze, the release archive and the baseline hashes cover are supported
here. Every other Makefile target (`qdos386-pc`, `qdos386-smp-pc`, `qdos386-r16b-pc`, `qdos386-r17-pc`, the i386VM
targets, the R9–R16 historical images) is NOT supported on this branch: it may still compile, but nothing verifies it, and
its conditionals are kept, not removed.

| build | Makefile target | output (baseline SHA-256 in `R20C-BASELINE-HASHES.txt`) |
|---|---|---|
| R20 headless | `make qdos386-r20-pc` | `build/qdos386-r20/pc/qdos386-r20.img` |
| R20 VGA | `make qdos386-r20-pc` | `build/qdos386-r20/pc-vga/qdos386-r20-vga.img` (= `binaries/r20-2b-vga.img`) |
| R18 headless | `make qdos386-r18-pc` | `build/qdos386-r18/pc/qdos386-r18.img` |
| R18 VGA | `make qdos386-r18-pc` | `build/qdos386-r18/pc-vga/qdos386-r18-vga.img` (= `binaries/r18-vga.img`) |

Compile-time macros of the R20 headless build (`-D`, from `make -n qdos386-r20-pc`); the VGA build adds `-DQDOS_CON_VGA`;
R18 is the same list without `-DQDOS_R20`:

`-DQDOS_APP_STACK=65536u`, `-DQDOS_DYNPROC`, `-DQDOS_DYNPROC_SHELL`, `-DQDOS_ENV`, `-DQDOS_FS_WRITE`, `-DQDOS_JOBCTL`, `-DQDOS_LAYOUT_R9`, `-DQDOS_PAGING`, `-DQDOS_R13A`, `-DQDOS_R13B`, `-DQDOS_R14`, `-DQDOS_R16`, `-DQDOS_R16B`, `-DQDOS_R17`, `-DQDOS_R18`, `-DQDOS_R20`, `-DQDOS_SURFACES`, `-DQDOS_SYNC`, `-DQDOS_TASKMGR`, `-DQDOS_TITLES`, `-DQDOS_VCON`, `-DQDOS_VFAT`, `-DR7_SHELL_MODE`

The `R20` and `R18` builds select `kctx17.o` as the context switcher, so `-DQDOS_KCTX_PTRBITS` is not passed (it is used by
the older `kctx.o` targets only, see the Makefile comment at `PCKCTX`).

## Method

1. Every `#if`, `#ifdef`, `#ifndef`, `#elif` in `kernel/` and `platform/` was extracted with its macro names.
2. Each macro was classified against the matrix above:
   - **ACTIVE**: passed by the R20 or R18 build (the list above, plus `QDOS_CON_VGA` for the VGA build). Its branches are
     kept, because they are selected in a supported build. The `#if` of an ACTIVE macro is still a conditional but its
     selection is fixed by the build.
   - **HDR-DEFINED**: header include guards and the `APP_*`/`DEVICE_*` layout macros, defined unconditionally in a header.
     Kept.
   - **INACTIVE**: not passed by any supported build and not defined unconditionally anywhere. Its branches are dead in
     all four supported builds.
3. For INACTIVE macros the disposition below records whether the branch was removed, kept as a guarded default, or kept
   because a non-supported target or a historical script still names it.

The classification is a worklist, not a proof: it does not evaluate arbitrary `#if` expressions. Removals were made by
hand, one site at a time, and each removal group was checked by rebuilding all four supported builds and comparing every
output (250 files: every object, image and boot binary) with the baseline.

## Removals in this branch

| commit | file | removed condition | selected branch | active variants | evidence |
|---|---|---|---|---|---|
| `d992905` | `kernel/shellboot.c`, `kernel/clock_boot.c` | `QDOS_NO_RTC` (term in 3 conditions) | the remaining terms | R18 and R20, headless and VGA | 250/250 output hashes identical |
| `b97bfb5` | `kernel/smp.c` | `#ifdef QDOS_R16_APNOTIMER` (its `#else` kept) | the `#else` line | R18 and R20 | 250/250 identical |
| `b97bfb5` | `kernel/smp17.c` | `#ifdef QDOS_R17_TEST_APFAIL` (2 sites, `apfail_done` state) | the default path (no suppression) | R18 and R20 | 250/250 identical |
| `9783802` | `kernel/aout.c` | `#if defined(QDOS_I386VM_BLOCK) && defined(QDOS_TEST_SPARE)` (the R11I self-test, 110 lines, and its call) | none (dead) | none | 250/250 identical |
| `b7cc3fa` | `platform/pc/platform.c` | `#ifdef QDOS_CON_SERIAL` (2 sites), `#ifdef QDOS_HALT_DEBUG_EXIT` | the default path | R18 and R20 | 250/250 identical |

Eight conditional openers were removed (621 → 613). Commit messages for the first two groups state the 250/250 result of
the final head, not of each intermediate commit; the intermediate states were not rebuilt separately.

## Disposition of every INACTIVE macro

| macro | conditional sites | disposition |
|---|---|---|
| `ENV_HOST` | 2 | KEPT: named by a historical or non-supported target (env.h, run.sh, test_env.c) |
| `G12_ROWS` | 1 | KEPT: default selected by #ifndef in the defining file (`platform/pc/vgapres13a.c`); live branch |
| `KFONT_HOST_TEST` | 1 | KEPT: named by a historical or non-supported target (run-host.sh) |
| `PH_HOST` | 1 | KEPT: named by a historical or non-supported target (pheap.h, run.sh) |
| `QDOS_AMP` | 7 | KEPT: named by a historical or non-supported target (Makefile, amp.c, amp.h) |
| `QDOS_AMP_TEST_BURN` | 3 | KEPT: named by a historical or non-supported target (r15-package.sh, r15-verify-package.sh) |
| `QDOS_AMP_TEST_LATE` | 1 | KEPT: named by a historical or non-supported target (r15-package.sh, r15-verify-package.sh) |
| `QDOS_AMP_TEST_NOSIPI` | 1 | KEPT: named by a historical or non-supported target (r15-package.sh, r15-verify-package.sh) |
| `QDOS_CLOCK_EPOCH` | 4 | KEPT: named by a historical or non-supported target (Makefile, README.md, clock_boot.h, r11f-images.sh) |
| `QDOS_CLOCK_PINNED` | 1 | KEPT: named by a historical or non-supported target (Makefile, README.md, clock_boot.h, r11f-images.sh) |
| `QDOS_CON_SERIAL` | 2 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `QDOS_FONT_QDOS5X9` | 1 | KEPT: named by a historical or non-supported target (Makefile, font.h, run-host.sh) |
| `QDOS_FS_STAMP_DATE` | 1 | KEPT: default selected by #ifndef in the defining file (`kernel/fat16.c`); live branch |
| `QDOS_FS_STAMP_TIME` | 1 | KEPT: default selected by #ifndef in the defining file (`kernel/fat16.c`); live branch |
| `QDOS_HALT_DEBUG_EXIT` | 1 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `QDOS_I386VM_BLOCK` | 7 | KEPT: named by a historical or non-supported target (Makefile, layout_test.c, r11j-images.sh, r11k-images.sh, r11l-default-check.sh, run.sh) |
| `QDOS_KCTX_CHECK` | 2 | KEPT: named by a historical or non-supported target (Makefile) |
| `QDOS_KCTX_PTRBITS` | 2 | KEPT: named by a historical or non-supported target (Makefile, kctx.s, mismatch.mk, r11h-maps-link.sh) |
| `QDOS_NO_RTC` | 3 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `QDOS_NO_TIMER` | 2 | KEPT: named by a historical or non-supported target (Makefile) |
| `QDOS_PAGING_POC` | 1 | KEPT: named by a historical or non-supported target (r11j-images.sh) |
| `QDOS_PG_WIN3` | 2 | KEPT: named by a historical or non-supported target (Makefile) |
| `QDOS_R16_APNOTIMER` | 1 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `QDOS_R17_TEST_APFAIL` | 2 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `QDOS_R17_TEST_RESERVE` | 5 | KEPT: named by a historical or non-supported target (memmap.c) |
| `QDOS_TEST_BIGK` | 2 | KEPT: named by a historical or non-supported target (bigk_check.c, r11k-boot-images.sh, r11k-images.sh) |
| `QDOS_TEST_FAULT_LAUNCH` | 5 | KEPT: named by a historical or non-supported target (fault.c, r11j-images.sh, r11k-images.sh, samelnch.c, twicet.c) |
| `QDOS_TEST_FAULT_READ` | 1 | KEPT: default selected by #ifndef in the defining file (`kernel/aout.c`); live branch |
| `QDOS_TEST_PG_INT_AROUND_CR3` | 2 | KEPT: named by a historical or non-supported target (r11j-images.sh, r11k-images.sh) |
| `QDOS_TEST_PG_POOL_BYTES` | 1 | KEPT: named by a historical or non-supported target (r11j-images.sh, r11k-images.sh) |
| `QDOS_TEST_PG_SHARE_FRAMES` | 1 | KEPT: named by a historical or non-supported target (r11j-images.sh, r11k-images.sh) |
| `QDOS_TEST_PG_TASKS` | 4 | KEPT: named by a historical or non-supported target (r11k-images.sh, twicet.c) |
| `QDOS_TEST_SPARE` | 2 | REMOVED in this branch (commits d992905, b97bfb5, 9783802, b7cc3fa) |
| `R16B_POLICY_DEFAULT` | 2 | KEPT: default selected by #ifndef in the defining file (`kernel/smp17.c`); live branch |
| `R16B_POL_RESIDENCY` | 2 | KEPT: default selected by #ifndef in the defining file (`kernel/smp17.c`); live branch |
| `R16B_POL_SWEEP` | 2 | KEPT: default selected by #ifndef in the defining file (`kernel/smp17.c`); live branch |
| `R20C_INJECT_EVERY` | 1 | KEPT: test-only switch; archived injection and contention evidence depends on it |
| `R20C_TEST_ENT` | 1 | KEPT: test-only switch; archived injection and contention evidence depends on it |
| `RTC_HOST_TEST` | 1 | KEPT: named by a historical or non-supported target (run-host.sh) |
| `VAS_HOST` | 1 | KEPT: named by a historical or non-supported target (run.sh, vas.h) |

Kept entries are not dead code on this branch in the sense of the mission: each is named by a non-supported target,
a historical script, a host test or a guarded default. They are documented so that their removal can be decided per target.
Entries marked "test-only switch" must stay for the archived evidence to be reproducible.

## ACTIVE macros (selected in the supported builds)

`QDOS_APP_STACK`, `QDOS_DYNPROC`, `QDOS_DYNPROC_SHELL`, `QDOS_ENV`, `QDOS_FS_WRITE`, `QDOS_JOBCTL`, `QDOS_LAYOUT_R9`, `QDOS_PAGING`, `QDOS_R13A`, `QDOS_R13B`, `QDOS_R14`, `QDOS_R16`, `QDOS_R16B`, `QDOS_R17`, `QDOS_R18`, `QDOS_R20`, `QDOS_SURFACES`, `QDOS_SYNC`, `QDOS_TASKMGR`, `QDOS_TITLES`, `QDOS_VCON`, `QDOS_VFAT`, `R7_SHELL_MODE`

## Still unresolved

- The remaining INACTIVE conditionals named only by non-supported targets (about 30 macros: AMP, host tests, i386VM
  blocks, the R9–R16 switches). They can be removed once each target is retired or moved to a frozen checkout.
- Arbitrary compound expressions (`#if X > n`, mixed `#if`/`#elif` chains) were not simplified mechanically; none of the
  removals touched such an expression.
- The ACTIVE `#if`s (for example the `QDOS_R16`/`QDOS_R17` chains in `kernel/jobs.h`, `kernel/bkl.h`, `kernel/smp.c`) are
  kept: they select code in every supported build, so simplifying them needs a per-option byte-identity check that was not
  run in this branch.
