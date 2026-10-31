# HANDOVER — QDOS/386 KCTX-FIX2 mainline (corrected context resume)

The misaligned-ESP resume defect (R12A…R14A: a job interrupted with ESP ≢ 0 mod 4 was resumed 1–3 bytes low, then
#DF/#GP and a kernel halt) is fixed **in the normal kernel**: `make` builds it, no special flag. The frozen R13B, R14
and R14A releases are unchanged and still contain the defect.

## For the GW-BASIC builder

**Boot this kernel** (QEMU, VGA): `kernels/qdos386-vga.img`
SHA-256 `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`
(headless, for Tiny386: `kernels/qdos386.img` `7c67597862f24de2ef2237d32540976b4285ed20f7a4bfe1dd6508248af395b0`).
These are byte-identical to the KCTX-FIX1 kernels you may already have (`kfix1-vga.img`, `kfix1.img`).

**Your volume, unchanged**: give a *copy* of `launch-volume.img` (`ed03cd27…`) as the second IDE disk. Tested with
your `GWBASIC.AOU` `4ae47943c9015086c49fcb7e1ecb385ce1fe0fb28a16ca5bc103f8ca023f461d`, unchanged.

    cp launch-volume.img /tmp/gw-vol.img
    qemu-system-i386 -machine pc -cpu pentium -m 16 -bios /usr/share/seabios/bios.bin -vga std \
      -drive file=kernels/qdos386-vga.img,format=raw,if=ide,index=0,media=disk \
      -drive file=/tmp/gw-vol.img,format=raw,if=ide,index=1,media=disk -boot order=c

(The automated runs add `-display none -monitor tcp:127.0.0.1:PORT,server,nowait -no-reboot` and type through the
monitor: `scripts/r12-run-qemu.sh`; the GW-BASIC run is `tests/kctx/run-gwbasic.sh KERNEL CHECKPOINT_DIR OUT TAG`.)
In the shell: `START GWBASIC /NEW`, then `CON 4` (or Ctrl+Esc).

**Expected** (all reproduced on this kernel, `logs/kctx2/stage2/gwbasic/`):

| Program | Result |
| --- | --- |
| `FOR I=1 TO 16000 / 16400 / 20000 / 200000 : NEXT` | `DONE`, `Ok` each |
| nested `FOR I=1 TO 100 : FOR J=1 TO 1000` | `DONE`, `Ok` |
| `GOSUB` 20000 times | `DONE`, `Ok` |
| no kernel halt; after `KILL /CON:4`: LEAK back to its start value, `SYNC`, volume fsck clean | yes |

On the original R14 kernel the first program never prints `DONE` and the machine stops answering (negative control,
`logs/kctx2/stage2/negative/`).

**Still broken in GW-BASIC** (interpreter/port side; GW-BASIC was not changed and is **not** claimed correct):

1. `SAVE` → `Device I/O Error` (every name; same on the checkpoint's own R13B kernel), `LOAD` → `File not found`.
2. `PRINT MID$(B$,7,5);LEFT$(B$,5);RIGHT$(B$,5)` → GW-BASIC `#PF eip 00241495 cr2 b9f80024` (EBX = `b9f80024`,
   a 32-bit value read 2 bytes off); the kernel ends only GW-BASIC. `B$=A$+" WORLD"` and `LEN` are correct.
3. `SYSTEM` / `FILES` → `Advanced Feature`.

## What changed in the kernel

* `kernel/kctx.s` `resume_dispatch`: `movl %eax,%esp; cmpl $0,_kx_resume_fmt` — the full saved ESP; the frame format
  comes from `kx_resume_fmt`, set by `KX_RESUME(sp, fmt)` (`kernel/jobs.h`) at every return site (`jobs.c`,
  `proc.c`, `paging.c`). Before: `andl $-4` on `saved_sp | fmt`.
* The old switcher is kept only as `kernel/frozen/kctx-r12a-r14.s` for the frozen targets; the Makefile picks the
  matching C side from the linked object, so the two can never be mixed.
* Nothing else: same features as R14 (R13B + serial), no GW-BASIC change, no alignment rule for applications.

Minimal patch: `patches/kctx-fix2-against-r14a.patch` (files and reasons: `docs/KCTX-FIX2-IMPLEMENTATION.md` §2).

## For the next OS builder

* Start from branch `r14a-kctx-mainline-fix` (commit in `PACKAGE.md`) or this archive's tree. Toolchain:
  `toolchain/` (I386VM R1), `export QDOS386_TOOLS=…`. `make` builds the current kernels; `make qdos386-check-pc` a
  test-only kernel that checks every resume hand-off.
* Before changing the context code, run: `sh tests/kctx2/run-stage2.sh OUT CHECKPOINT_DIR NEGATIVE_KERNEL` (OS
  suites, probes, GW-BASIC, negative control, Tiny386), `sh tests/kctx2/run-check-kernel.sh CHECKPOINT_DIR OUT`,
  `sh tests/r14/check-frozen.sh`, `sh tests/r14a/check-r14-identical.sh R14_PKG LOG`,
  `sh tests/kctx2/check-legacy-targets.sh R14A_CHECKOUT LOG`, `sh tests/kctx2/check-mismatch.sh OUT`.
* **This mainline is frozen at delivery.** For the separate R15 worker-CPU experiment: `kx_resume_fmt` is one global,
  safe only on one CPU with IF clear between its write and `resume_dispatch` (`docs/KCTX-FIX2-AUDIT.md` §4); make it
  per CPU (or pass the format in a register) before a second CPU runs kernel entries.
* Publishing: this archive is private (SuperBASIC, Infocom interpreter source). Use
  `scripts/qdos386-public-source.sh` and `volumes/qdos386-public.img`.

Results: `docs/KCTX-FIX2-RUNTIME-REPORT.md`; limits: `docs/KCTX-FIX2-KNOWN-LIMITATIONS.md`.
