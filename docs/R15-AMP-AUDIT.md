# R15 AMP0 — Phase 0 platform and source audit

Branch `r15-amp-worker` from the frozen KCTX-FIX2 mainline (`5b6fc47`, tag `kctx-fix2-mainline` = source `b1db28a`;
archive `qdos386-KCTX-FIX2-MAINLINE.tar.gz` `778a3d61…` re-hashed and matching). Nothing in the frozen release or tag
was changed; the default `make` kernels still build to `f8e5d5e1…` (QEMU) and `7c675978…` (Tiny386).

## 1. What the existing kernel does (read from the source)

| Area | Finding | Source |
| --- | --- | --- |
| Boot | `boot2.s` (real mode, hand-encoded): reads kernel + transfer area into 0x10000–0x7FFFF, A20, flat GDT, PE; copies the kernel to 0x100000, the transfer area to 0x340000; jumps to 0x100000. No AP handling. | `platform/pc/boot2.s` |
| Descriptors | one GDT (code 0x08, data 0x10), one IDT; every gate is an interrupt gate (0x8E); #DF is a task gate with its own TSS | `kernel/descriptors.c` |
| Interrupts | 8259 PIC + PIT channel 0 at ~100 Hz (divisor 11932); no I/O APIC, no LAPIC use, no IPI | `platform/pc/platform.c` |
| Processors | no CPUID/APIC/MP/ACPI code anywhere (grep `apic|madt|rsdp|_MP_|cpuid|rdmsr`: none) | — |
| Paging | `pg_init` → `proc_boot`: template directory identity-maps 0–16 MiB (supervisor, 4 tables); every process directory copies all 1024 template entries and replaces spans that contain pool frames with private tables | `kernel/proc.c`, `kernel/vas.c` |
| RAM | 1 MiB + CMOS 0x30/0x31, **capped at 16 MiB** (the identity map): `-m 32` gives the same 16,384 KiB, 3,264-frame pool | `kernel/proc.c` |
| Memory map | 0–1 MiB real-mode area, BIOS, VGA; 0x100000–bss_end kernel; bss_end–0x1FFFFF kernel heap; 0x200000–0x33FFFF and 0x380000–0x6FFFFF and 0x800000–RAM top frame pool; 0x340000 transfer area; 0x700000 boot stack | `docs/R12A-MEMORY-MAP.md` |
| Low memory after boot | nothing in the kernel uses 0x500–0x9FFFF (the loader buffer 0x10000–0x7FFFF is dead after the copy); the BDA (0x400) and EBDA are only read | grep of `kernel/`, `platform/pc/` |
| Context switch | KCTX-FIX2: one `kx_resume_fmt`, safe on ONE CPU with IF clear (`docs/KCTX-FIX2-AUDIT.md` §4) — must never be touched by a second CPU | `kernel/kctx.s`, `kernel/jobs.h` |
| Linker | `.text` at 0x100000, `.data`, `.bss` (a.out, `-N`); kernel symbols are physical = linear (identity) | `platform/pc/kernel.ld` |

## 2. Decisions

* **AP discovery** (fail closed at every step, `kernel/amp.c`): CPUID present (EFLAGS.ID) → CPUID.1:EDX.APIC and
  .MSR → IA32_APIC_BASE (MSR 0x1B) enabled, base below 4 GiB → processor inventory from the **ACPI MADT** (RSDP
  "RSD PTR " in the first KiB of the EBDA or 0xE0000–0xFFFFF, 20-byte checksum; RSDT signature, length bounds,
  checksum; "APIC" table checksum and length bounds; type-0 entries with the *enabled* flag; entries walked only while
  each length is ≥ 2 and inside the table), else the **Intel MP table** (_MP_ checksum, default configurations
  refused, PCMP checksum, unknown entry type → refuse). The BSP's LAPIC ID is read from its LAPIC; the worker is the
  first enabled processor with a different ID. IDs 0/1 are never assumed. QEMU/SeaBIOS provides a valid MADT.
* **Where**: `amp_boot()` runs in `shell_boot` **before `pg_init`**: paging is still off, so the LAPIC page
  (0xFEE00000) and the ACPI tables (near the top of RAM, above the 16 MiB map with `-m 32`) are reached physically
  and **nothing new is mapped into any address space**. Afterwards CPU 0 never touches the LAPIC, except in an
  explicit worker restart (§4).
* **Trampoline**: physical page **0x8000** (SIPI vector 0x08; 4 KiB aligned; below the old loader buffer, above the
  BDA and the boot sector; used by nothing after boot; never handed to the frame pool, which starts at 0x200000).
* **Worker memory**: all static kernel BSS — mailbox `amp_mbox`, `amp_in` / `amp_out` (16 KiB each), the 8 KiB worker
  stack, its GDT/IDT (`.data`). It is inside the kernel image range, so it is identity-mapped for CPU 0 in every
  address space, never part of the frame pool, the heap, DMA or video memory, and **never freed or remapped** —
  "pinned" by construction. Cost: 57 KiB less kernel heap in the AMP kernel only.
* **Worker execution**: flat protected mode with **paging off** (it never uses CPU 0's page tables, so CPU 0's CR3
  switches cannot affect it), own GDT/IDT/stack, IF always clear, CR0.EM set (any x87 instruction faults), caches on.
* **Delays**: PIT channel 2 one-shot, polled (works before the timer runs and with interrupts off; bounded).
* **Opt-in**: `-DQDOS_AMP`, target `make qdos386-amp-pc`; default builds unchanged.

## 3. Start-up sequence (implemented)

1. CPU 0: copy the trampoline to 0x8000; fill the worker IDT (32 gates → worker stubs); set the worker stack top;
   reset the mailbox; bump the boot generation.
2. ICR: wait idle; INIT (level assert) to the worker's APIC ID; poll delivery status (bounded); wait 10 ms.
3. STARTUP IPI vector 0x08; poll; wait 200 µs; a second STARTUP only if the worker has not reached 32-bit code.
4. Worker: real mode → trampoline GDT → PE → `ap_entry32` (flat segments, own stack, CR0 PE|MP|EM|NE, own GDT and
   IDT) → `ap_main`: reads its LAPIC ID (MMIO) and CPUID initial APIC ID, writes magic, ABI version, the boot
   generation it saw, then `ap_state = READY` with a locked exchange.
5. CPU 0 waits up to 200 ms; READY + magic + version + boot generation + APIC ID must all match, else the service is
   unavailable. A late worker is never adopted.

## 4. Mailbox, service and restart

ABI, states, ownership and ordering: `docs/R15-AMP-MAILBOX-ABI.md`. A restart (diagnostic `AMP_T_RESTART`) is allowed
**only** after the worker positively acknowledged a halt (it wrote FAULTED or QUIESCED as its last store before
`cli; hlt`); it maps the LAPIC page into the template directory for the INIT/SIPI only (interrupts off, the entry is
removed again, nothing else is changed) and refuses if that directory slot is already in use.

## 5. Capability report

INFO word 3 gives the table used (1 MADT, 2 MP), word 4 the enabled processors, words 5–8 the BSP ID, the chosen
worker ID and the IDs the worker itself reported (LAPIC MMIO, CPUID), word 2 the reason when unavailable
(`kernel/amp.h` `AMP_R_*`: no CPUID, no APIC/MSR, APIC disabled, no valid tables, one CPU, ICR stuck, no READY in
200 ms, identity mismatch, PIT not counting).
