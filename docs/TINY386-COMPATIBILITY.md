# Tiny386 compatibility (R1, extended)

## Supplied material

- **Earlier archive** (`tiny386.zip`): CPU core only (`i386.c`, `fpu.c`, headers).
  Used for the i386VM harness (`emulator-tiny386/` in the harness tree).
- **Full project** (`d176d618-tiny386-master.zip`): the complete Tiny386 PC
  emulator, with 8259 PIC, 8254 PIT, 8042, CMOS RTC, ISA VGA with VBE, IDE, NE2000,
  8257 DMA, PC speaker, Adlib, SB16, and a SeaBIOS firmware build script.
  Built headless with `gcc -O3 *.c -lm` (9 warnings, no errors).

R1 now targets the full Tiny386 PC. The i386VM harness target is kept as a
regression target for the job model.

## Phase 1 answers (full Tiny386)

1. **Boot.** SeaBIOS (built from its git source with `scripts/build.sh` steps,
   `seabios/config` and `seabios/patch`) loads the MBR at 0000:7C00 from the
   IDE disk `hda`, as Tiny386's ini file configures. The boot sector is
   `platform/pc/boot.s`.
2. **Protected mode.** Yes. The kernel loads its own GDT and IDT and runs with
   paging off (checked by reading CR0 in the guest).
3. **GDT, IDT, interrupts.** As before. IRQ0 goes through the 8259 PIC (ICW
   sequence in `platform/pc/platform.c`, EOI sent from `sched_irq_tick`).
4. **PIT and PIC.** Both present and used. The PIT runs in mode 3 at ~100 Hz,
   divisor 11932. The PIC maps IRQ0 to vector 0x20 and masks everything else.
5. **Output.** Tiny386's debug port 0x402 (one byte per OUT, straight to stdout).
   Serial is present but not used. VGA text is not used.
6. **Headless.** Yes. `tiny386_headless <ini>`. The guest ends the run by writing
   "Shutdown" byte by byte to port 0x8900 (the emulator's power-off hook). The
   exit status is not carried; the PASS/FAIL line in the output is the result.
7. **Image.** A raw disk image: boot sector at LBA 0, kernel from LBA 1, read by
   INT 13h AH=42h into 0x10000 and copied to 0x100000.

## Notes

- **Timing.** The PIT uses host monotonic time (`get_uticks()` in `main.c`), so
  timer-driven output is not clock-paced. The test flow is paced by tick counts,
  so the same program produces the same tick totals. Ordering could in principle
  vary with host load; in the runs here it did not.
- **Device-side tick counter.** The PC platform has none, so the "device ticks"
  check is reported as n/a. The i386VM platform keeps it.
- **Port I/O.** Used: 0x20/0x21, 0xA0/0xA1 (PIC), 0x40/0x43 (PIT), 0x92 (A20),
  0x402 (console), 0x8900 (exit). Not tested: any other device.

## Changes to Tiny386

- **Full Tiny386 tree:** no changes made.
- **Earlier CPU core:** no changes made.
- **i386VM harness:** only the synthetic timer patch (`patches/`), used by the i386VM target.
