# qdos386-R14B-GCC-ENV-SDK

Base: frozen **KCTX-FIX2-MAINLINE** (archive `778a3d61…ed804`, source commit `b1db28aa`, tag `kctx-fix2-mainline`) and
the **R11L-SELFHOST-STRETCH** toolchain (archive `c314be0c…62c15c`, source commit `81d29ea9`). Branch `r14b-gcc-env`.
A guest-toolchain packaging and driver correction: no kernel, loader, shell, editor, process API, filesystem, libc or
console change. Start with **`HANDOVER.md`**. `SHA256SUMS` lists every file of the package except itself.

| Path | What |
|---|---|
| `HANDOVER.md`, `PACKAGE.md` | this release |
| `source/` | the full tree (git archive of the branch head): every KCTX-FIX2 file byte-identical to the KCTX-FIX2 archive (its own `source/SHA256SUMS`, unchanged, verifies them; the KCTX-FIX2 `logs/` directory is not carried), plus the R14B additions below |
| `source/toolchain/driver/gcc.c` | the new driver `GCC.AOU`/`CC.AOU` (C89); `host/` a host shim for its logic tests |
| `source/toolchain/r11l/` | R11L's toolchain material, byte copies: `src-guest-staged/` (GCC 1.40, GAS 1.38, binutils 1.94, bison sources with R11L's port edits), `ORIGINAL.SHA256` (all 820 original files), `objects/` (the 110 tool objects + generated sources and logs; `objects.sha256`), `host-bin/` (the x86-64 host GCC 1.40, GAS, ld; `cc1-lp64-repaired` for the oracle), `guest-bin/` (the R11L guest tools, for reference), `qdcc-r11l.c` (R11L's driver) |
| `source/scripts/r14b-*.sh` | `hosttools` (a QDOS386_TOOLS tree), `build-sdk` (libraries + tools + driver), `link-tools`, `build-driver`, `install-sdk` (the SDK installer), `mkpatch`, `package`, `verify-package` |
| `source/tests/r14b/` | `host-driver-tests.sh`, `stage-test-vol.sh`, `run-qemu.sh`, `seq-*.txt`, `judge.py`, `oracle.sh`, `src/` (test sources), `r11l-oracle/` (R11L's guest `HELLO.*` artifacts) |
| `source/docs/TOOLCHAIN-*.md` | `ENV-AUDIT`, `ENV-ABI`, `RUNTIME-REPORT`, `KNOWN-LIMITATIONS` |
| `guest-bin/` | `GCC.AOU CC.AOU CCCP.AOU CC1.AOU AS.AOU LD.AOU` and the (empty) link logs of the four tools |
| `sdk-stage/` | what the installer puts on a volume: `SDK/BIN`, `SDK/LIB`, `SDK/INCLUDE` (unchanged KCTX-FIX2 headers), `SDK/GCC.TXT`, `SDK/STARTUP.SDK`, `HOME/HI.C`, `TMP/` |
| `volumes/qdos386-r14b-sdk.img` | the KCTX-FIX2 public system volume with the SDK installed (`--demo`); `.manifest.txt` hashes of the installed files; `install.log` |
| `kernels/` | the unchanged KCTX-FIX2 kernels (`qdos386-vga.img` `f8e5d5e1…`, headless `qdos386.img` `7c675978…`) |
| `patches/` | `r14b-against-r11l.patch` (driver and link recipe; tool sources/objects checked identical), `r14b-against-kctx-fix2.patch` (additions only) |
| `logs/r14b/` | `stage0/` (the path-defect probe), `host/` (driver logic tests), `qemu/` (the three-boot run: transcripts, per-command times, judge, oracle, fsck, VFAT walker, start and final volumes), `inputs.sha256`.  The fresh-extraction check of this archive (`scripts/r14b-verify-package.sh`) is delivered beside it as `qdos386-R14B-GCC-ENV-SDK.verify.txt` |

Not included: emulator binaries (QEMU, Tiny386), the KCTX-FIX2 `logs/`, any story file.

Licences: as KCTX-FIX2 (see `source/PACKAGE.md`): **private archive** (SuperBASIC, Infocom interpreter source in
`source/`). GCC 1.40, GAS 1.38, binutils 1.94 and bison sources and binaries are GPL and keep their notices
(`source/toolchain/r11l/src-guest-staged/*/COPYING`). The R14B driver and scripts follow the project's own terms.

Final source commit (branch r14b-gcc-env): 5d852ac88a14b6db31e4db1487d7b0c29b9e6dcc; base tag kctx-fix2-mainline = b322195278cfa69eccc31a16046489e836bc0442.
