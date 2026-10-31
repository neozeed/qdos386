# HANDOVER — QDOS/386 R15 AMP0 (master / worker multiprocessing)

CPU 0 runs the whole, unchanged QDOS/386 kernel and every device; one application processor (the **worker**) runs
only fixed compute commands on pinned buffers, reached through one mailbox. Asymmetric, not SMP. Opt-in: the
ordinary `make` kernels are byte-identical to KCTX-FIX2.

## Frozen baseline

KCTX-FIX2 mainline: `qdos386-KCTX-FIX2-MAINLINE.tar.gz` `778a3d61…`, tag `kctx-fix2-mainline` (`b1db28a`), verification
`5b6fc47` — untouched. R15 branch: `r15-amp-worker` (commit in `PACKAGE.md`), tag `r15-amp0`.

## Build

    export QDOS386_TOOLS=<I386VM R1 tree>       # toolchain/ in this archive
    make                                       # ordinary kernels (unchanged): build/qdos386/pc-vga/qdos386-vga.img, pc/qdos386.img
    make qdos386-amp-pc                        # AMP kernels: build/qdos386-amp/pc-vga/qdos386-amp-vga.img, pc/qdos386-amp.img
    sh scripts/r15-build-guest.sh              # build/r15/AMPT.aout (tests), build/r15/WORKER.aout (demo)

## Run (QEMU)

    cp volumes/r15-public.img /tmp/r15.img
    qemu-system-i386 -machine pc -cpu pentium -smp 2 -m 32 -vga std \
      -drive file=kernels/qdos386-amp-vga.img,format=raw,if=ide,index=0,media=disk \
      -drive file=/tmp/r15.img,format=raw,if=ide,index=1,media=disk -boot order=c

Boot shows `AMP: ACPI MADT: 2 enabled processor(s), BSP LAPIC id 0` and `AMP: worker CPU LAPIC id 1 READY …`. Then:
`AMPT INFO` (state, heartbeat, counters), `AMPT RUN 1000` (checked requests), `AMPT PROOF 20000000` (CPU 0 spins
with interrupts off while the worker computes; the result is checked), `WORKER VECTOR 7`,
`WORKER FILE /ETC/STARTUP.CMD`, `AMPT EDGE`, `AMPT FAULT` (worker fault, record, restart). With `-smp 1` (or
`-cpu pentium,-apic`) the boot says `AMP: worker service unavailable - …` and everything else works as before.

## Programs

An application includes `qdos386amp.h` and calls `qtrap(QT_AMP, op, b, c)`: SUBMIT (buffers are copied by the
kernel), POLL, RESULT, CANCEL, INFO — `docs/R15-AMP-MAILBOX-ABI.md`. `sdk/examples/r15/worker.c` is the example.

## Results

`docs/R15-AMP-RUNTIME-REPORT.md` (exact numbers, commands, hashes). Gate 1, 2, 3 pass; Gate 4: no regression
attributable to AMP, with two recorded non-AMP results (an R14A Gate 3 deterministic-replay divergence caused by
QEMU `-icount` counting a busy worker's instructions, and a one-off R14A Gate 4 CPU-meter timing failure that passed on
rerun). Static check: the worker's code references nothing outside itself.

## Known limitations

`docs/R15-AMP-KNOWN-LIMITATIONS.md`: no system calls on the worker, no SMP, no protection (ring 0), one worker and one
outstanding request, polling worker (busy host thread under QEMU), cooperative cancellation only, worker stack
corruption → triple fault (machine reset), QEMU only (real hardware not tested), `-smp 3` two-worker stretch not
attempted.

## Next work (separate milestones; AMP0 is frozen at delivery)

1. IPI wake-up (map the LAPIC for CPU 0 permanently, `hlt` in the worker loop) — removes the busy idle loop.
2. Two workers (`-smp 3`): one mailbox, stack, fault record and buffer pair per worker; per-worker generation.
3. A larger command set (vector/matrix kernels) with the same copy-in/copy-out rule.
Full SMP (kernel on several CPUs) would first need per-CPU `kx_resume_fmt`, per-CPU scheduler state and kernel locking.
