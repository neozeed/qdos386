# Build (canonical R18/R20 PC baseline)

## Toolchain

The kernel and the test programs are built with the historical GCC 1.40 / GAS 1.38 / binutils 1.94 i386 toolchain that
lives in an i386VM tree outside this repository. Set `QDOS386_TOOLS` (and `I386VM`) to that tree. The Makefile default
`I386VM ?= $(HOME)/i386vm` applies when it is not set.

Pinned identities used for the freeze (`docs/R20C-BASELINE-HASHES.txt` is the output side; these are the tool side):

| tool | path in the i386VM tree | SHA-256 |
|---|---|---|
| assembler | `bin/as` (GNU assembler 1.38) | `47369dd54851448080ce6836304829ea6ae98fbcb8efa944386c0ec0239d1009` |
| compiler driver | `bin/i386-gcc` | `3567f3beab83e17d7f7b5ff5844ee1dd85fd452b84128bdc0a7d46bb103d7904` |
| compiler proper | `bin/cc1` | `1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb` |
| linker | `bin/i386-ld` | `dfdb396514559958d9bd1ed651337abded3565f6665d8bad05dcaeec92d07c20` |

The compiler driver does not accept `--version`; its identity is the binary hash. Python 3 (3.13 used) runs the image tools.

## Supported targets

    make qdos386-r18-pc       # R18 frozen kernel: build/qdos386-r18/pc/ and pc-vga/
    make qdos386-r20-pc       # R20 kernel:        build/qdos386-r20/pc/ and pc-vga/

The VGA images are `qdos386-r18-vga.img` and `qdos386-r20-vga.img`. Their SHA-256 values are in
`docs/R20C-BASELINE-HASHES.txt` (`d42497d5…` and `dd4c9129…`).

Other targets (`make` default, `qdos386-smp-pc`, `qdos386-r16b-pc`, `qdos386-r17-pc`, the `i386vm` targets) still exist in the
Makefile. They are not supported on this branch and are not verified by it.

## Test programs

`scripts/a16-p2b-build.sh` builds the nine R20 test programs (`R20GS`, `R20GW`, `R20GL`, `R20C`, `R20P`, `R20X`, `R20XK`,
`R20D`, `R20K`) and their protected copies in `build/a16/`. It needs `QDOS386_TOOLS` and the guest-tool chain below.

Verified against the release binaries: all nine outputs are byte-identical with the corrected script (`R20DP` needs
`-DR20D_WAIT=40000`; `R20KP` and `R20XKP` need the `R20K` and `R20X_N=1000` recipes in the script).

## Guest-tool chain (external to the release archive)

The test programs need the guest-tool chain, which the release archive does not contain:

    scripts/a16-p2b-build.sh
      -> scripts/a14-build-guest.sh   (tests/a14/planar.c, view.c)
           -> scripts/a13-build-guest.sh   (tests/a13/*.c, sdk libraries, BOUNCE.aout)
                -> scripts/r16b-build-guest.sh   (tests/r16b/*.c: TOP, CPUSET)
      SDK libraries: sdk/lib, sdk/libc, sdk/libm, sdk/compiler, sdk/examples

The full chain is in this repository's git history at the freeze tag. The release archive
`qdos386-APOLLO16-R20-PHASE2B-CACHE.tar.gz` does not include it, so a rebuild of the test programs from the archive alone
is NOT VERIFIED. A rebuild from the repository is verified for the nine programs.

## Test volume (external)

The R20 runtime tests need a FAT16 volume with the test programs and `STARTUP.CMD`. `scripts/a16-p2b-sysvol.sh` derives it
from a base volume that is not part of this repository or the release. Without that base volume the runtime tests cannot be
run; the kernel build does not need it.

## Verifying a build

    . <toolchain env>                       # sets QDOS386_TOOLS
    make qdos386-r20-pc && make qdos386-r18-pc
    cd build && find qdos386-r20 qdos386-r18 -type f \( -name '*.o' -o -name '*.img' -o -name '*.bin' -o -name '*.raw' -o -name '*.aout' \) | sort | xargs sha256sum > /tmp/hashes.txt
    diff docs/R20C-BASELINE-HASHES.txt /tmp/hashes.txt   # run from the repository root, paths as in the baseline file

The baseline file lists 250 outputs. A clean build of this branch reproduces all 250 (verified at the final head).
