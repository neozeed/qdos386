# Current kernel build and internal tests

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

From the repository root, use the existing historical tools:

```sh
make production HOSTTOOLS=/data/projects/qdos386/hosttools
make diagnostic HOSTTOOLS=/data/projects/qdos386/hosttools
```

The default `make` and `make r20` are aliases for production. Production emits `source/build/canonical/pc/qdos386.img` and `source/build/canonical/pc-vga/qdos386-vga.img`, with matching kernel.aout symbol files. The first is serial/headless, the second VGA. Diagnostic emits `source/build/diagnostic/pc/qdos386-diagnostic.img` and the original FAT fixture `source/build/fat16/win1.img`. Building diagnostics does not execute them. Its boot path is the historical test harness, not shell_boot.

`source/Makefile` owns compilation/linking; `source/mk/current.mk` lists current flags and ordered extras explicitly. Root historical kernel/platform copies are not production inputs. `qdos386-r20-pc` in source remains the compatibility composition; it includes the old harness and is not the lean default. `make r18` reproduces frozen R18. Do not run recursive historical builds with `-j`; they share boot/fixture paths. `-B` forces a rebuild. No modern compiler substitution or package installation is required.

## Compiler dependencies

The accepted external `/data/projects/qdos386/hosttools` contains the i386-gcc wrapper, GCC 1.40 cc1/cccp, GAS 1.38, binutils 1.94 linker, and `tools/aout-to-raw.py`. Set HOSTTOOLS at root, or both I386VM and QDOS386_TOOLS when invoking source directly. Compiler input hashes are preserved in `reports/r2a-mem-r2/evidence/inputs.sha256`, with host versions in `reports/r3/evidence/environment.json`; these tools are not vendored or installed by this cleanup. GNU Make, Python 3, bash and standard host utilities drive the build. Local verification used Make 4.4.1, Python 3.14.8 and QEMU 11.0.3 TCG with `/usr/share/seabios/bios.bin` (SHA-256 2ec4700d3d91b4b8c08712ff96ebf90c44464820a6cee1c05f007ee33d2b3015).

Native GCC and applications are existing volume inputs; the kernel build does not rebuild them. Accepted FIX1 is `../reference/qdos386/win1-dev-r2a-fix1.img`, SHA-256 0f26af0d78158374d4ab9ffc94205f57efd0b31754e0bfe95dd357d0ec1d9e62. All test runners below copy their input disks before QEMU writes. Choose a new output directory for each run. Do not attach a writable accepted disk directly.

## Bounded QEMU commands

```sh
python3 scripts/investigate-memory.py \
  source/build/canonical/pc-vga/qdos386-vga.img \
  ../reference/qdos386/win1-dev-r2a-fix1.img /tmp/qdos386-smoke-1 \
  --smp 1 --memory 16 --scenario A \
  --aout source/build/canonical/pc-vga/kernel.aout --timeout 120

python3 scripts/investigate-memory.py \
  source/build/canonical/pc-vga/qdos386-vga.img \
  ../reference/qdos386/win1-dev-r2a-fix1.img /tmp/qdos386-resident-8 \
  --smp 8 --memory 16 --scenario R2 --steps tests/host/r3/resident.json \
  --aout source/build/canonical/pc-vga/kernel.aout --timeout 240

python3 tests/host/r3/run-diagnostic.py \
  source/build/diagnostic/pc/qdos386-diagnostic.img \
  source/build/fat16/win1.img /tmp/qdos386-diagnostic --timeout 90
```

The first runs the existing CCCP/CC1/AS/LD pipeline and HI. Repeat at eight CPUs. The resident recipe keeps two existing Elites, another shell and Hack while compiling/running HI twice. The probe verifies the supplied a.out text/data matches the disk before reading stopped-guest symbols, heap/frame bitmaps and supervisor cache mappings. Use `tests/host/r3/summarize-canonical.py` to judge final acceptance records independently of completion flags.

Original cache recipes remain under `tests/cache/r20-pages/`: functional, kill, survivors, fallback, injected, reboot and pressure. Their volume builder preserves every original FIX1 file and adds existing regression helpers. Test-only kernels use distinct CURRENT_D values and R20_EXTRA; do not overwrite the production candidate. `tests/host/r3/functional.json` additionally needs an existing R16T.AOU built from the unchanged historical source, as recorded in the internal handoff. `QDOS386_TOOLS=/data/projects/qdos386/hosttools bash tests/host/r3/build-functional-volume.sh CACHE_VOLUME EXISTING_SDK_STAGE NEW_IMAGE NEW_WORK` reproduces it without editing application sources. It exercises the actual protected application and Bounce, not substitutes.

## Reproduction boundaries

The new production hash intentionally differs from accepted full R20 because test code/data/BSS are no longer linked. Compare it with the canonical manifest, not the old 250-output inventory. `make verify-identity` reads the preserved historical source/build; it is not a canonical production acceptance test. All 124 freshly rebuilt R18 outputs still match that inventory. Frozen R2 and original Gate 1 remain reconstructible from their untouched tags/history.

This is a private development handoff. The full tracked internal archive includes historical application material; it is not authorized for public redistribution. External toolchains, accepted disks, untracked resources and raw test runs remain in their original private locations and are referenced by the handoff manifest. No public export or exclusion manifest is created.
