# Canonical R7B2 build

The current configuration is fixed in `source/mk/current.json`: the accepted
64-job Apollo R7B2 writable FAT32 kernel, unchanged flags and link order. No
historical profile selection or source substitutions occur in the current build.
`source/platform/pc/boot3.s` names the accepted `QDOS_TRANSFER_BASE = 0x340000`
constant directly; the previous sed address replacement is gone. The actual
FAT16 BIOS loader stages remain the accepted Apollo implementation.

## Build from a fresh checkout

On the qualified Linux x86-64 host, install/provide dependencies yourself:
GCC 15.3, GNU Make, Bash, Python 3, GNU ar/coreutils and binutils, dosfstools;
QEMU for tests. The build performs no package installation or network operation.

```sh
make
# Complete minimal disk: build/current/products/qdos386-system.img
# VGA kernel package:   build/current/products/qdos386-vga.img
# SDK libraries:        build/current/sdk/lib/
# Host historical tools: build/current/toolchain/bin/
```

The host compiler builds owned GCC 1.40/CCCP, GAS 1.38 and LD 1.94 source.
Those historical executables build every kernel/SDK/native utility object.
Host GNU binutils are used only for the already accepted BIOS-only MBR/VBR/
stage2 assembly. The complete source/generated-input inventory and import hashes
are in [toolchain ownership](../toolchain/OWNERSHIP.md).

`make toolchain`, `make sdk`, `make shell`, `make kernel`, `make disk-tools` and
`make image` expose the current stages. `production` and `apollo-r7-rw` build the
whole system. `source/Makefile` and SDK/disk-tool Makefiles forward to this one
build; they no longer emit in-source objects. Frozen tags own old profiles.
Build dependencies are checked by content and command-configuration fingerprints,
with output hashes checked before reuse. A lock serializes access to each tree.

To keep outputs entirely outside the checkout:

```sh
make BUILD_DIR=/absolute/new/generated-output
make clean BUILD_DIR=/absolute/new/generated-output
```

The default directory is `build/current`. `make clean` only removes a directory
bearing this checkout's build ownership marker. It refuses an unmarked/foreign
existing directory, source roots, symlink output paths and tracked files. All
current objects, libraries, executables, image intermediates, scratch files and
build logs are beneath that one tree. Existing historical build directories and
private application assets elsewhere are not clean targets. Keep acceptance logs
and test-disk copies outside the build directory before testing clean.

## Private historical applications

The default disk contains current shell/SH, fresh SDK libraries/headers,
FDISK/FORMAT/CHKDSK and native FAT32 probes. It is usable without any private
payload input. It does not pretend to recreate unavailable historical assets.
To reproduce the fully populated accepted private disk:

```sh
make PAYLOAD_IMAGE=/absolute/path/to/fat16-fallback.img
```

Use the accepted 33 MiB R6A FAT16 fallback with SHA-256
`3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a`.
This is an explicit relocatable data input, not a hard-coded private path.
It contains the existing guest GCC/CCCP/GAS/LD, Elite, Hack, SuperBASIC,
GW-BASIC, Infocom assets and other regression files. Their byte identity and
long names are preserved. No historical application sources are changed.
Fresh SDK libraries are checked against the supplied payload and used for
FAT32 population; stale or differing archives fail the build. Kernel, disk
utilities and probes are freshly built. Fresh shell/SH already match the accepted
payload bytes. The source-only and full-private images deliberately differ in
payload; only the latter is compared against the accepted whole-disk oracle.

The accepted full disk SHA-256 is
`34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.
VGA kernel package SHA-256 is
`cc3466efd94699f1e61ccd7d6b5a99a10a0aeb55d9f1d2e5f95d088223305059`.
Do not compare the kernel package to the entire disk. Build logs, input/output
hashes and image receipts are in the selected generated-output tree.

## Boot and native tools

Use an ordinary disposable copy of the disk, or QEMU snapshot mode:

```sh
qemu-system-i386 -machine pc -accel tcg -cpu pentium -m 32 -smp 4 \
  -vga std -nic none -snapshot -boot order=c -no-reboot \
  -drive file=build/current/products/qdos386-system.img,format=raw,if=ide,index=0
```

`RUN F32IO` verifies actual FAT32 channel operations and process cleanup on a
fresh disposable disk. F32LIVE additionally requires the PAT2 regression fixture
from the private payload. `RUN SH` / `EXIT` tests the native shell. On the full
private disk, `RUN GCC -o /TMP/HI.AOU /HOME/HI.C`, then `RUN /TMP/HI.AOU`,
uses the existing native compiler. `START ELITE /new` starts Elite; select its
window with Ctrl+Esc and use the game's prompts/quit controls. Automated
polled-key tests use long enough key holds.

For compiling additional native SDK programs without source-tree archives:

```sh
QDOS386_TOOLS="$PWD/build/current/toolchain" \
QDOS386_LIBDIR="$PWD/build/current/sdk/lib" \
TMPDIR="$PWD/build/current/tmp" \
source/sdk/bin/qdos386-gcc -O -o build/current/products/hello.aout source/sdk/examples/hello/hello.c
```

The SDK's public ABI is unchanged. Historical build instructions, diagrams,
source profiles and regression references remain in the reports and accepted
Git tags; the superseded build guide is retained verbatim under reports/r7/.
