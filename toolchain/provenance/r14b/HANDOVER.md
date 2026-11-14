# HANDOVER — QDOS/386 R14B: environment-aware native GCC 1.40 toolchain

On the **unmodified** KCTX-FIX2 operating system, a user can now type

```
/HOME> GCC HI.C -o HI.AOU
/HOME> HI
Hello from QDOS/386!
```

and the C source is preprocessed, compiled, assembled and linked inside QDOS/386 by the historical GCC 1.40
(cccp, cc1), GAS 1.38 and binutils-1.94 ld, which the new driver `GCC.AOU` finds through `PATH`, with headers from
`INCLUDE`, `CRT0.O`/libraries from `LIB` and intermediates in `TEMP`. **No OS file changed**: same kernel
(`f8e5d5e1…`), same shell, same libraries; the KCTX-FIX2 system volume only gains new files under `/SDK`.

This is a toolchain release, frozen at delivery. Results: `source/docs/TOOLCHAIN-RUNTIME-REPORT.md`; contract:
`source/docs/TOOLCHAIN-ENV-ABI.md`; audit: `source/docs/TOOLCHAIN-ENV-AUDIT.md`; limits:
`source/docs/TOOLCHAIN-KNOWN-LIMITATIONS.md`.

## 1. Use the ready volume (no build)

```sh
cd qdos386-R14B-GCC-ENV-SDK
cp volumes/qdos386-r14b-sdk.img /tmp/r14b.img          # always a copy
qemu-system-i386 -machine pc -cpu pentium -m 16 -bios /usr/share/seabios/bios.bin -vga std \
  -drive file=kernels/qdos386-vga.img,format=raw,if=ide,index=0,media=disk \
  -drive file=/tmp/r14b.img,format=raw,if=ide,index=1,media=disk -boot order=c
```

`volumes/qdos386-r14b-sdk.img` is the KCTX-FIX2 public system volume (`volumes/qdos386-public.img`, `6731bd95…`)
after `scripts/r14b-install-sdk.sh --demo`. Its `/ETC/STARTUP.CMD` (unchanged) already sets `PATH=/BIN;/SDK/BIN;/GAMES`,
`INCLUDE`, `LIB`, `TEMP` and `TMP`, so after boot `GCC` works at once. To set the environment by hand (or after
`UNSET`), the guest lines are:

```
/> CD /HOME
/HOME> SET PATH=/SDK/BIN;/BIN
/HOME> SET INCLUDE=/SDK/INCLUDE
/HOME> SET LIB=/SDK/LIB
/HOME> SET TEMP=/TMP
/HOME> GCC HI.C -o HI.AOU
exit status 0
/HOME> HI
Hello from QDOS/386!
exit status 0
```

(`exit status N` is the shell's own report.) `GCC -v …` shows each tool command; `GCC -c`, `-S`, `-E`, `-O`, `-I`,
`-D`, `-L`, `-l` work as in GCC 1.40; `CC` is the same program with the R11L default output name. `/SDK/GCC.TXT` on
the volume is a one-page summary.

## 2. Make an SDK volume on the host

```sh
cd qdos386-R14B-GCC-ENV-SDK/source
sh scripts/r14b-hosttools.sh /tmp/qt && export QDOS386_TOOLS=/tmp/qt     # the vendored host GCC 1.40 / GAS / ld (x86-64 Linux)
sh scripts/r14b-build-sdk.sh                  # SDK libs (checked = the volume's), tools relinked, GCC.AOU -> build/r14b/guest
cp volumes/qdos386-public.img /tmp/my-sdk.img # a COPY of the KCTX-FIX2 system volume (or any copy of yours)
sh scripts/r14b-install-sdk.sh --demo /tmp/my-sdk.img /tmp/my-sdk.manifest.txt
```

The installer only adds files: `/SDK/BIN/{GCC,CC,CCCP,CC1,AS,LD}.AOU`, `/SDK/LIB/{CRT0.O,SLOT1.LD,LIBM.A}`,
`/SDK/GCC.TXT`, `/SDK/STARTUP.SDK`, and with `--demo` `/HOME/HI.C` (only if absent). It stops, writing nothing more,
if a target exists with other content or the volume's headers/libraries are not the KCTX-FIX2 ones. It never changes
`/ETC/STARTUP.CMD` unless `--add-startup` is given (then it appends only the missing `SET` lines and keeps
`/ETC/STARTUP.BAK`). The ready-made `guest-bin/` and `sdk-stage/` hold the same files for other ways of installing.

## 3. What changed, in one paragraph

The R11L tool binaries could not see directories: they carried the R11L C library, whose file-name mapping refuses
`/` and opens every name in the volume root (demonstrated in QEMU, `logs/r14b/stage0/`). R14B relinks the **same R11L
objects** of cccp, cc1, GAS and ld (no source or code-generation change) against the KCTX-FIX2 libraries, and replaces
R11L's root-only `CC.AOU` with `GCC.AOU` (`source/toolchain/driver/gcc.c`, C89), which reads `PATH` (through the
kernel's program search), `INCLUDE`, `LIB`, `TEMP`/`TMP`, runs the tools with `qd_spawn`/`qd_wait`, checks every
status and output, never leaves a stale program, and handles QDOS/386's 15-argument / 432-byte limit (a link script
with `INPUT()`) and ld's way of reading its arguments (names with blanks are quoted). Patches:
`patches/r14b-against-r11l.patch`, `patches/r14b-against-kctx-fix2.patch`.

## 4. Re-run the evidence

```sh
cd source && export QDOS386_TOOLS=/tmp/qt                           # after section 2
sh tests/r14b/host-driver-tests.sh /tmp/r14b-host                   # driver logic on the host, 35 cases
sh tests/r14b/run-qemu.sh /tmp/my-sdk.img /tmp/r14b-run             # 3 QEMU boots (~5-15 min), volume checks, oracle, judge
sh scripts/r14b-verify-package.sh ../../qdos386-R14B-GCC-ENV-SDK.tar.gz /tmp/r14b-verify   # fresh extraction + QEMU smoke
```

`run-qemu.sh` ends with `RESULT: PASS` in `OUT/summary.txt` when: the kernel hash is KCTX-FIX2's; `fsck.fat -n`,
both FAT copies and the VFAT walker are clean; every file of the input volume is unchanged; every guest output equals
the host oracle (`tests/r14b/oracle.sh`); and the judge passes (`tests/r14b/judge.py`).

## 5. Hashes (SHA-256)

| Item | Hash |
|---|---|
| KCTX-FIX2 input archive | `778a3d612d924a06f17bd8c73fb910406768264a46fdb7391bc82c36db3ed804` |
| R11L input archive | `c314be0c36a421ef3500c5749159c9e6a105e4c5358b45b4fc0882962f62c15c` |
| kernel `kernels/qdos386-vga.img` (unchanged) | `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3` |
| `GCC.AOU` = `CC.AOU` | `469d0ce76e805445872b40dfc4a8197b457fbf8b52449ea13c77476d5cfeb398` |
| `CCCP.AOU` / `CC1.AOU` | `816da68994e03a4f2b56c7cecf0be057a9f93c48c3d7f567e115fdf4326d6317` / `f0323093f417b58ae95ccf9e3e27a87459addc1e78593d97711ab94b84a3fbc9` |
| `AS.AOU` / `LD.AOU` | `c79736b05f01c7091d090cc4b3fd4279f878dbebe4ded5e5c1fdc435a3dfcc7c` / `c8b207ea63ddca0af66313700b60ab6d774045c66c220fc9e0da1122c22712b5` |
| `CRT0.O` / `LIBM.A` | `59a905c7c352345fd744775abd8d7d9dcd0817e4a06deed247a7054b3041dd4b` / `ad9bd7afa2352a1632d89c7822c4fd7bf0d63ac27c6b1d3d066f2c870bcb32af` |
| `volumes/qdos386-r14b-sdk.img` (deterministic install) | `f991456852d79998fa1b91fcadf26efc01abb9ddc9993c98904e096a2716a523` |
| guest-built `HI.AOU` (= host oracle) | `53791f376a836dec7a4cc0ae1e2f46ae33d1cab1352e4d699a85bfb187a80a3a` |

## 6. Boundaries

Not self-hosting GCC (the tools are host-built R11L objects); no `ar`/`make` on the guest; GCC 1.40 C only; QEMU
only (Tiny386/i386VM not used); the old large GCC-in-guest matrix not rerun. Private archive like KCTX-FIX2 (its
`source/` carries SuperBASIC and the Infocom interpreter source); the toolchain parts are GPL (GCC, GAS, binutils).
Details: `source/docs/TOOLCHAIN-KNOWN-LIMITATIONS.md`.
