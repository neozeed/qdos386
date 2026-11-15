# QDOS/386 — Apollo R7B2

The sole production configuration is the accepted writable FAT32 system, with
FAT16 BIOS boot and recovery. Kernel/application sources live under `source/`;
the repaired historical host compiler sources are owned by this repository.

```sh
make
# build/current/products/qdos386-system.img — complete bootable disk
# build/current/products/qdos386-vga.img    — kernel package, not the full disk
make clean
```

The build compiles GCC 1.40, GAS 1.38 and LD 1.94 with the host compiler, then
uses those historical tools for the kernel, shell, SDK and disk utilities.
No `/root/i386vm`, prebuilt SDK archives, external compiler sources or downloads
are required. All generated files live under `build/current/`; use
`BUILD_DIR=/absolute/new/output` to put them elsewhere. Current targets are
`toolchain`, `sdk`, `shell`, `kernel`, `disk-tools`, `image` and `clean`.
`production` and `apollo-r7-rw` are aliases for the complete current build.
Older profiles remain reproducible from their frozen Git tags.

Plain `make` creates a source-only bootable system with shell/SH, fresh SDK,
FDISK/FORMAT/CHKDSK and filesystem probes. The optional private historical
payload supplies the existing guest GCC, games, interpreters and assets:

```sh
make PAYLOAD_IMAGE=/absolute/path/to/accepted-fat16-fallback.img
```

This input is explicit, read only and validated against freshly rebuilt BIOS
code. With the accepted R6A fallback it reproduces the complete accepted R7B2
disk exactly. It is not needed for the default source-only disk. Recovered guest
compiler binaries and third-party application assets are not claimed to be
rebuilt by the default target. All existing private application sources and
resources remain intact.

See [build and boot instructions](docs/BUILD.md), [source ownership](toolchain/OWNERSHIP.md)
and [R7 consolidation handover](reports/r7/R7-CANONICAL-HANDOVER.md).
Earlier root/build instructions are preserved verbatim in
[the historical README](reports/r7/PRE-CANONICAL-README.md) and
[historical build guide](reports/r7/PRE-CANONICAL-BUILD.md).
