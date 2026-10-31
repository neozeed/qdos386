# KCTX-FIX2 — known limitations

## Kernel

* **One CPU.** `kx_resume_fmt` is a single kernel variable, correct because the kernel runs on one CPU and the window
  from its write to its read runs with IF clear (`docs/KCTX-FIX2-AUDIT.md` §4). An SMP/AMP kernel (the planned R15
  worker-CPU experiment) must make it per CPU, or carry the format in a register (e.g. EDX) from the C return to
  `resume_dispatch`. No per-CPU safety is claimed here.
* **Still ring 0.** Every process runs at CPL 0 (unchanged since R12A): the fix restores correct resumption, not
  isolation. A program can still corrupt the kernel.
* **Faults inside the hand-off window halt.** An exception or NMI between the C return and `resume_dispatch` is a
  kernel fault and halts the machine, as before; nothing in this release changes kernel-fault handling.
* **`kernel/context.s` and `kernel/paging.s`** keep the historical `andl $-4` decode. They are linked only by the
  frozen R6…R12 targets (and the retired i386VM images), which keep their released bytes. A revived target that links
  them gets the historical convention consistently (the Makefile selects the C side from the linked object) and
  inherits the misaligned-ESP defect; it would have to move to `kernel/kctx.s`.
* **`kernel/frozen/kctx-r12a-r14.s`** is the defective switcher kept only so R12A…R14 rebuild byte-identical.
  The frozen R12A…R14 / R14A releases are unchanged and still contain the defect; use the current kernel.
* **No new version string.** The current kernel boots with the same lines as R14 (`QDOS/386 R7`, `PAGING: R12A
  dynamic processes`, `SERIAL: …`); the shell on the volume prints its own banner. No unrelated change was made to
  label it. Identify the kernel by its hash (`HANDOVER.md`).

## Verification

* Real hardware: **not tested** (QEMU 8.2.2 `-cpu pentium` primary, Tiny386 R12C full machine secondary). The
  CPU-only i386VM harness is retired and was not used as a gate; its frozen images are only rebuilt and compared.
* The test-only check kernel (`make qdos386-check-pc`) checks the hand-off at the C side (IF clear when the format is
  written, frame shape); the instructions from there to `resume_dispatch` are covered by the static audit, not by a
  per-instruction trace.
* Tiny386 covers one bounded session: probes, the Hack smoke, shutdown; multi-session coexistence is QEMU only.

## GW-BASIC (interpreter side — for the GW-BASIC builder; not kernel faults as far as tested)

The frozen `GWBASIC.AOU` (`4ae47943…`) now completes every long loop, but **GW-BASIC is not fully fixed**:

1. `SAVE` → `Device I/O Error` for every name tried, then `LOAD` → `File not found` — identical on the checkpoint's own
   R13B kernel, on R14 and on the current kernel (`logs/kctx/stage3/saveload-ab/`).
2. `PRINT MID$(B$,7,5);LEFT$(B$,5);RIGHT$(B$,5)` → GW-BASIC page-faults in its own code: `#PF eip 00241495
   cr2 b9f80024` (EBX `b9f80024`: the halves of a 32-bit value read 2 bytes off). The kernel ends only GW-BASIC
   ("process ended"); the shell continues. Visible only now, because older kernels halted earlier.
3. `SYSTEM` (and `FILES`) answer `Advanced Feature`: GW-BASIC can be left only by the Task Manager or `KILL`.

## Package

* The archive is the **complete, private** source dump. It contains two restricted inputs: SuperBASIC
  (`apps/superbasic/`, private research, not freely redistributable) and the Task Force '87 Infocom interpreter source
  (`third_party/infocom/orig/`, no licence granted). `scripts/qdos386-public-source.sh` derives a public-safe source
  tree without them; the package verification builds the same kernels from that tree.
* The only volume shipped, `volumes/qdos386-public.img`, has no SuperBASIC, no Infocom interpreter and no story file.
  The internal test volumes (`scripts/r14a-sysvol.sh`, `tests/*`) still contain `/BIN/BASIC.AOU` and are built from
  the source when needed; they must not be published.
* The toolchain (I386VM R1: GCC 1.40, GAS 1.38, ld from binutils 1.94) is carried as the user-supplied source archive
  `toolchain/I386VM-R1-GCC140-GAS138-AOUT-FULL-SOURCE.zip`; building it is described in its own README. The host needs
  QEMU, mtools, dosfstools, Python 3 (with PIL for screenshots), lrzsz (R14 Gate 4 only) and gdb (optional).
