# Tiny386 and i386VM compatibility (R1 Phase 1)

## What was supplied

`tiny386.zip` contains only the CPU core: `i386.c` (5636 lines), `i386.h`,
`i386ins.def`, `fpu.c`, `fpu.h`, `LICENSE.txt`, `UPSTREAM.md`. It has no board:
no BIOS, no disk controller, no 8259 PIC, no 8254 PIT, no VGA, no serial port.
The R1 target as specified (BIOS boot sector, PIC/PIT timer, Tiny386 machine)
cannot be built from this archive alone.

The i386VM harness (`I386VM-R1…zip`) embeds this core in `emulator-tiny386/`
with its own machine code (`machine.c`). R1 runs on that harness.

## Answers to the Phase 1 questions

1. **Boot of PC disk images.** Not supplied. The harness loads a raw image
   at physical 0 (`cpui386_reset_pm_flat(cpu, 0)`), with the 8 MiB RAM array.
   No boot sector path exists.
2. **80386 protected mode.** Yes. The core is created as generation 3 (386).
   R1 loads its own GDT (`lgdt`), IDT (`lidt`), and reloads segments and CS
   with a far jump. Verified by the run (see R1-RUNTIME-REPORT.md).
3. **GDT, IDT, interrupts.** `cpui386_step` checks `cb->pic_read_irq` and
   `IF` at the start of each step. It delivers the vector through `call_isr`.
   `cpui386_raise_irq` sets the pending flag; `HLT` is cleared on delivery.
   Verified: IRQ0 on vector 0x20 through an interrupt gate, `iretl` restore.
4. **PIT and PIC.** Neither is present. The core has hooks (`cb.pic`,
   `cb.pic_read_irq`) for an external interrupt controller but no implementation.
5. **Output mechanism.** Byte MMIO at `0x800001` (write), `0x800002` (read).
   Serial and VGA are not present.
6. **Headless running.** Yes. `i386vm` reads stdin and writes stdout, exits with
   the guest's `EAX` at `HLT` as status, and enforces an instruction budget.
7. **Raw floppy image.** Not applicable. The harness takes a raw image, with
   no boot sector, loaded at address 0.

## Changes made

- **Tiny386 core: unchanged.** No file under `emulator-tiny386/` other than
  `machine.c` was modified.
- **i386VM harness `machine.c`: changed**, as a synthetic timer. The change
  is supplied as `patches/i386vm-synthetic-timer.patch`, which is relative to
  the original `machine.c`. Details:
  - Byte register `0x800003`. Write N: IRQ0 every N×1024 steps; N = 0 disables
    and clears the count. Read: low 8 bits of delivered IRQ0 count.
  - IRQ0 is delivered on vector 0x20 through `cb.pic_read_irq`.
  - Delivery depends only on the step count, so runs are deterministic.
  - This is a substitute for the PIT. It is not a PIT model and has no 8254
    registers, no EOI, and no 8259 behaviour.

## Missing emulator functionality (not added)

- `IN`/`OUT` port I/O: the `io_*` callbacks are not set. Any port instruction
  will dereference a NULL callback. Not tested; R1 does not use port I/O.
- BIOS and real-mode entry, disk, and boot sector: none.
- PIC (8259) and PIT (8254) emulation: none. The synthetic timer replaces IRQ0 only.
- VGA text and serial: none.

## Recommendation

For the spec's full target (PC boot and real PIC/PIT timer), supply the full
upstream Tiny386 tree with its machine code. The timer patch above should then
be re-derived against that machine as a separate patch.
