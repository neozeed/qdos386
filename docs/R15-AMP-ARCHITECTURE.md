# R15 AMP0 — architecture (asymmetric master / worker multiprocessing)

**CPU 0 owns QDOS/386.** The whole existing kernel — scheduler (`jobs.c`), context switch (`kctx.s`, KCTX-FIX2),
traps, frame allocator, heap, FAT/VFAT, UART, VGA, keyboard, PIT, PIC, process table — runs only on the bootstrap
processor, unchanged. **The worker CPU runs only `kernel/amp_ap.c` + `kernel/amp_ap_s.s`**: syscall-free compute
kernels over pinned buffers, reached through one mailbox. This is AMP, not SMP: the ordinary kernel never runs on two
CPUs, nothing migrates, and (ring 0 everywhere) no process is isolated from another.

## 1. Build

| Target | Images | Notes |
| --- | --- | --- |
| `make` / `make qdos386-pc` | `qdos386-vga.img`, `qdos386.img` | the ordinary UP kernels, **byte-identical** to KCTX-FIX2 (`f8e5d5e1…`, `7c675978…`) |
| `make qdos386-amp-pc` | `build/qdos386-amp/pc-vga/qdos386-amp-vga.img`, `…/pc/qdos386-amp.img` | `QDOS386_OPTS` + `-DQDOS_AMP` + `amp.o amp_s.o amp_ap.o amp_ap_s.o` |
| `make qdos386-amp-variant AMPV=… AMP_EXTRA=…` | test-only: `nosipi` (`-DQDOS_AMP_TEST_NOSIPI`), `late` (`-DQDOS_AMP_TEST_LATE=n`), `burn` (`-DQDOS_AMP_TEST_BURN`) | never shipped as the AMP kernel |

Changed shared files (all additions under `#ifdef QDOS_AMP`, so the default bytes do not change): `kernel/jobs.c`
(QT_AMP dispatch; `amp_job_ended` after the three places a job is marked dead), `kernel/shellboot.c` (`amp_boot`
before `pg_init`), `Makefile` (targets, `amp_s.o`/`amp_ap_s.o` rules, `qdos386amp.h` in the header dependencies).
New: `kernel/amp.h`, `kernel/amp.c`, `kernel/amp_s.s` (CPU 0), `kernel/amp_ap.c`, `kernel/amp_ap_s.s` (worker),
`sdk/include/qdos386amp.h` (application ABI), `sdk/examples/r15/worker.c`, `tests/r15/*`, `scripts/r15-*`.

## 2. Processor-local state of the worker

| | Worker CPU | CPU 0 |
| --- | --- | --- |
| Mode | 32-bit protected, **paging off** (physical) | protected, paging (template / process directories) |
| GDT | `amp_ap_gdt` (own copy: null, code 0x08, data 0x10) | kernel GDT |
| IDT | `amp_ap_idt`: 32 interrupt gates → `apx0..31` → `ap_fault` (record, halt this CPU) | kernel IDT (PIC IRQs, trap 0x30, exceptions, #DF task) |
| Stack | `amp_ap_stack` (8 KiB, static) | job kernel stacks, boot stack |
| Interrupts | IF always 0; after INIT its LAPIC is reset (LVT entries masked, software-disabled) and the worker never enables it; the 8259's ExtINT reaches CPU 0's LINT0 only | PIC IRQ0/1/3/4/7/15 |
| CR0 | PE, MP, EM (x87 use faults), NE; CD/NW cleared | as before |
| LAPIC | its own (base from its IA32_APIC_BASE); only its ID register is read | used only at start-up / restart (ICR) |
| Shared with CPU 0 | `amp_mbox`, `amp_in`, `amp_out` — nothing else | |

The worker never uses `kx_resume_fmt`, any scheduler global, the trap path, `resume_dispatch` or any kernel
function: `tests/r15/check-ap-objects.py` proves at object level that the worker's two objects reference only their
own symbols, and on the linked kernel that the worker's code range contains no INT/far call, no jump or call outside
itself, and no absolute memory operand outside its own data.

## 3. Start-up and failure handling

Discovery and start-up are in `docs/R15-AMP-AUDIT.md` §2–3. Every failure is reported once at boot
(`AMP: worker service unavailable - <reason>`) and the boot continues on CPU 0: no CPUID, no APIC/MSR, APIC disabled,
no valid MADT or MP table, one enabled processor (`-smp 1`), ICR delivery stuck, no READY within 200 ms (missing or
late worker), identity mismatch, PIT not counting. A worker that turns READY after the deadline is observed
(INFO `late 1`) and never adopted; its memory is static, so nothing it could still touch is ever reused.

## 4. Synchronization

One request slot, generation counters, locked state transitions, field ownership and x86 ordering:
`docs/R15-AMP-MAILBOX-ABI.md`. CPU 0's side runs in the QT_AMP trap (interrupt gate: IF clear, single CPU), so the
CPU 0 bookkeeping (`owner`, `cur_gen`, `next_gen`, counters) needs no lock of its own. The worker polls the mailbox
(with `pause`); there is no IPI wake-up in AMP0, so an idle worker keeps its host thread busy under QEMU.

## 5. Faults, cancellation, reclamation

* Worker exception → recorded in the mailbox, the worker halts; CPU 0 is never involved (its handlers are not in the
  worker's IDT). The request that was running reports `AMP_ERR_FAULT`; the service shows FAULTED; a diagnostic
  restart (INIT/SIPI) is allowed because the halt was positively acknowledged. A fault that destroys the worker's own
  stack becomes a triple fault, which QEMU treats as a machine reset: not contained (known limitation).
* Cancellation is cooperative only: CANCEL (or the owner's death) sets `cancel_gen`; the slot stays BUSY until the
  worker publishes CANCELLED/DONE/ERROR. No timeout ever frees anything.
* Pinned memory is static: there is nothing to leak; LEAK (kernel heap, channels, address spaces) is unchanged by
  submissions, kills and restarts.

## 6. QEMU

`qemu-system-i386 -machine pc -cpu pentium -smp 2 -m 32 …` (SeaBIOS). QEMU 8.2 runs each vCPU on its own host thread
(multi-threaded TCG), which the runtime evidence shows. `-smp 1` and `-cpu pentium,-apic` give the unavailable paths.
`-m 32` changes nothing for the kernel (RAM is capped at the 16 MiB identity map); it moves the ACPI tables above
16 MiB, which the pre-paging discovery reads physically. Tiny386 has no second CPU: the AMP kernel is expected to
report the service unavailable there.
