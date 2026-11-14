# R14B missing-input restoration (R2A-DISK-FIX1)

Source: user-supplied qdos386-R14B-GCC-ENV-SDK.tar.gz; exact SHA-256 and archive integrity in reports/r2a-disk-fix1/evidence. Original HANDOVER.md/PACKAGE.md are preserved beside this file. The existing toolchain/src-guest-staged is the authoritative historical guest source tree.

Before import, all 809 existing files corresponding to archive source/toolchain/r11l matched exactly; zero differed. Only missing inputs were added: 13 binutils source/header files, 13 generated GCC source/header/manifest inputs under toolchain/generated, and the environment-aware driver toolchain/driver/gcc.c. No existing source was replaced, and no second vendored GCC tree was created. The generated files are preserved bootstrap inputs, not a new independently maintained compiler source fork.

The 110 saved historical tool objects and guest binaries remain in the external archive, with hashes verified before extracting them into ignored build staging. R14B guest binaries serve as the runtime/link identity oracle while source rebuilding is checked separately. The existing external hosttools directory is unchanged. The repaired R11L host cc1, if used for guest compiler source rebuilding, is an archive-pinned bootstrap tool confined to staging, never a substitute for kernel compilation.
