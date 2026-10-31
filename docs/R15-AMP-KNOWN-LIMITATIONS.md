# R15 AMP0 — known limitations

## Scope (by design)

* **Not SMP.** The QDOS/386 kernel runs only on CPU 0. No scheduler on the worker, no migration, no kernel locking,
  no per-CPU kernel data apart from what the worker owns. KCTX-FIX2's single `kx_resume_fmt` stays correct because
  the worker never enters the kernel; real SMP would have to make it per CPU first.
* **No ordinary system calls on the worker.** The worker cannot print, allocate, open files, read the clock, touch a
  device or raise a QDOS trap; it runs three fixed compute commands (CRC32, XFORM, NUMERIC) and three test commands
  over the two pinned 16 KiB buffers. No arbitrary application code runs on it; no syscall forwarding.
* **No protection.** Every process still runs at ring 0. A native program could ignore the ABI and write the mailbox,
  the worker's memory or the LAPIC directly; nothing prevents it. The ABI is honest only for programs that use it.
* **One worker, one outstanding request.** Further workers (`-smp 3`, separate mailboxes) were not attempted
  (optional stretch: NOT TESTED). A second submitter gets `AMP_ERR_BUSY`.
* **Polling.** The worker polls its mailbox (`pause` between polls); there is no IPI wake-up and no completion
  interrupt. Under QEMU an idle worker keeps one host thread busy. Applications wait by polling + sleeping.

## Behaviour

* **Cancellation is cooperative.** NUMERIC checks every 65,536 iterations; CRC32 / XFORM (≤ 16 KiB) are short and do
  not check; the T_SLOW test command never checks. A request is never reclaimed before the worker publishes a state —
  a stuck worker therefore keeps the slot BUSY for ever (correct, by rule 4 of the handoff).
* **Reclamation of an ended job's request is lazy.** Cancellation is sent the moment the job ends; the slot is freed
  by the next QT_AMP call after the worker's acknowledgement.
* **Worker faults.** Recorded and contained to the worker (it halts) for any exception its own stack can handle. A fault
  that corrupts the worker's stack escalates to a triple fault, which QEMU handles as a machine reset — not contained.
  There is no NMI or machine-check handling beyond the generic gate.
* **Restart** after a fault or quiesce is a diagnostic operation (`AMP_T_RESTART`); it maps the LAPIC page into the
  kernel template directory for the duration of the INIT/SIPI (interrupts off) and refuses if that 4 MiB slot is in use.
* **Start-up timing** uses PIT channel 2 (polled); on a machine where channel 2 is unusable the service reports
  "PIT channel 2 not counting".
* **Firmware tables**: ACPI MADT via the RSDT (the XSDT is not used), else the MP table; default MP configurations and
  unknown MP entry types are refused (fail closed). Tested only with QEMU's SeaBIOS tables.
* **Memory cost**: the AMP kernel's BSS is 57 KiB larger (worker stack, buffers, mailbox, LAPIC page table), so its
  kernel heap is 57 KiB smaller; the frame pool is unchanged.

## Verification

* QEMU 8.2.2 (`-machine pc -cpu pentium`, SeaBIOS) only. **Real hardware: NOT TESTED.** Tiny386 has no second CPU
  (no SMP expected): only the fail-closed path and the single-CPU smoke are tested there.
* Multi-threaded TCG puts each vCPU on its own host thread; the evidence of parallel execution (both threads consume
  host CPU time over the same interval; CPU 0 spinning with interrupts off while the result arrives) is QEMU-specific.
  `-smp 2` alone is not claimed as proof.
* Input latency was not measured as a number: keyboard commands typed during worker computation were answered within
  the harness's per-command timeouts.
* GW-BASIC, SuperBASIC and the Infocom interpreter are not part of this work; the private-material rules of KCTX-FIX2
  still apply to the source archive (see its HANDOVER).
