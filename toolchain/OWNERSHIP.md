# Historical toolchain source ownership

The current host build is `tools/build-toolchain.py`. It builds the historical
programs with host GCC; it does not use modern GCC/GAS/LD to compile or link the
QDOS kernel or SDK. The accepted Apollo BIOS MBR/VBR/stage2 already used host GNU
binutils and continues to do so, with exact binary comparisons.

| Component | Authoritative source/input | Configuration |
| --- | --- | --- |
| Host GCC 1.40 cc1 and cccp | `toolchain/host/gcc-1.40/` | Recovered LP64 source at compiler commit `9491134ab6696591fc726dc1d81446d65b8de502`, including the accepted aggregate-call correction |
| GCC generated parsers/backend | `toolchain/generated/` | 13 pinned generated inputs; original grammar, machine description and generator sources remain owned by the compiler tree |
| GAS 1.38 | `toolchain/src-guest-staged/gas-1.38/` | Existing host64 serialization implementation, i386/USG; no new assembler patches |
| LD/BFD 1.94 | `toolchain/src-guest-staged/binutils-1.94/` | Existing host64 BFD port, i386 a.out, owned ldgram/ldlex generated inputs |
| LD host/default-script inputs | `toolchain/host/ld-config/` | Named header and pinned default scripts, replacing build-time source copies/generation |
| Host compile driver | `toolchain/bin/i386-gcc` | Explicit owned include paths (`-nostdinc`); SDK owns startup/linking |
| Compiler runtime | `source/sdk/compiler/gnulib.c`, `gnulib2.c`, `fixdfsi.s`, `config.h` | LF-normalized historical runtime sources; x87 fixdfsi preserves the accepted recursion workaround |
| QDOS API/runtime | `source/sdk/lib/`, `source/sdk/include/`, `source/sdk/ld/` | Public ABI unchanged; compiler outputs go to the generated SDK directory |
| C/math/C68 libraries | `source/sdk/libc/`, `source/sdk/libm/`, `source/port/include/` | All archives built from source, explicitly recreated with deterministic ar |

The existing `src-guest-staged/gcc-1.40` is a **guest-hosted** ILP32 configuration,
not a second source for the Linux LP64 host build. It and all historical
application/toolchain source are retained. The current host source was imported
byte-for-byte from the verified repaired compiler checkpoint; its original
separate Git history and rollback binaries are untouched. `SOURCE-IMPORT.json`
records file hashes and provenance. Identical generated GCC inputs are shared,
not copied or patched at build time.

Eleven missing `include/aout` inputs were recovered from the preserved R5 tree
and match `toolchain/ORIGINAL.SHA256` exactly. They had been omitted by the broad
`aout/` ignore rule; explicit exceptions now retain them. Two associated source
tests are also recovered. The LD parsers/lexer and GCC generated inputs are
checked in deliberately: no old bison/flex installation is required for an
ordinary build. Regeneration is not silently performed with modern generators.

The accepted compiler repair only corrects parentheses in the
`emit_push_insn` → `move_by_pieces` call. See
`toolchain/provenance/r7b2/aggregate-call-repair.patch` and the R7B2 compiler report.
Host debug/macro paths are normalized using the host compiler's prefix-map
flags. This gives new reproducible host executable hashes; it does not claim
byte identity with the older compiler binary containing different build paths.
Kernel/SDK/shell/disk results are independently required to match accepted R7B2.

Supported qualification host: Linux x86-64 LP64, GCC 15.3.0 (Ubuntu
15.3.0-4ubuntu1), GNU89, glibc. GNU Make, Bash, Python 3, GNU ar/coreutils,
host binutils and dosfstools are required. QEMU is needed only for runtime
verification. Original licenses, generator sources and provenance remain with
these trees. No package install, network fetch or source patch occurs during build.
