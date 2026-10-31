# QDOS/386 R4 - Runtime Report

R4 adds a native channel manager with a console driver, and corrects the
vector 0x2F (IRQ15) handler. The R2 and R3 regressions are unchanged in meaning
and still run in the same matrix. The R3 kernel is in `qdos386-R3.tar.gz`,
and the validated R3 firmware baseline is in `qdos386-R3-FW1.tar.gz`. Neither
was modified. This tree is a separate source tree.

## 1. Status

| Target | Runs | Result |
|---|---|---|
| i386VM (synthetic timer) | 3 of 3 | `QDOS/386 R4 CHANNEL PASS`, 54 checks ok, 1 n/a (PIC, no PIC on this target), 0 FAIL |
| Tiny386 PC (known-good BIOS) | 3 of 3 | `QDOS/386 R4 CHANNEL PASS`, 54 checks ok, 1 n/a (device ticks), 0 FAIL |
| i386VM, timer disabled (negative) | 1 | no PASS, `instruction limit exceeded` |

The PASS line is printed only after every R2, R3 and R4 check passes, and the
heap and stack checks pass too. Every check is a guest-side assertion made by
the running kernel.

Check breakdown per run: 8 R2 checks, 25 R3 checks, 4 PIC checks and 18
channel checks, with one n/a on each target.

## 2. Firmware and emulator dependencies

External inputs are not in the archive. Hashes are recorded so that a run can
be reproduced.

| Item | Path (local to the validation host) | SHA256 |
|---|---|---|
| Tiny386 `bios.bin` (known-good, 131072 bytes) | `scratchpad/firmware-known-good/bios.bin` | `7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e` |
| Tiny386 `vgabios.bin` (known-good, 39424 bytes) | `scratchpad/firmware-known-good/vgabios.bin` | `f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e` |
| Supplied firmware zip | `6456262f-tiny386_bios.zip` | `aa045d55c45635c9bbd4b36cf437ae47fcec2ddaec43782241678d1fc39bb0cf` |
| `tiny386_headless` (unmodified Tiny386 source) | `scratchpad/t386_headless` | `030cda746a37ab8e81bb21725232dfa70ca413ce448258e3f7e2c31791cce8a7` |
| `i386vm` (with `patches/i386vm-synthetic-timer.patch`) | `scratchpad/x/i386vm/i386vm/bin/i386vm` | `baa683711ecfe674a0e3b226e166db3160aec59ab922b2ea264b146590082a6c` |

The known-good BIOS banner is `rel-1.17.0-24-g28aa4634-dirty-20261025_090831-runnervmr9t6g`.
It is a dirty SeaBIOS build, and its source tree was not supplied. See
`R3-FW-VALIDATION.md` §2. Tiny386 is not modified (`Do not modify Tiny386`).

Tiny386 is run with the generated `build/pc/run.ini`:

```
[pc]
bios = <SEABIOS>/bios.bin
vga_bios = <SEABIOS>/vgabios.bin
mem_size = 8M
hda = <tree>/build/pc/r4pc.img
```

## 3. Commands

Full matrix (three runs per target, then the negative):

```
I386VM=<i386vm tree> TINY386=<path>/tiny386_headless SEABIOS=<dir with bios.bin, vgabios.bin> \
  scripts/run-all.sh
```

Individual steps:

```
make i386vm I386VM=... TINY386=... SEABIOS=...         # build/i386vm/qdos386-r4.bin
make pc     I386VM=... TINY386=... SEABIOS=...         # build/pc/r4pc.img
make build/pc/run.ini I386VM=... TINY386=... SEABIOS=...
<i386vm tree>/bin/i386vm build/i386vm/qdos386-r4.bin
(cd build/pc && <path>/tiny386_headless run.ini)
make notimer I386VM=...                                # build/i386vm/qdos386-r4-notimer.bin
<i386vm tree>/bin/i386vm build/i386vm/qdos386-r4-notimer.bin
```

Logs: `logs/i386vm-run{1,2,3}.log`, `logs/pc-run{1,2,3}.txt`,
`logs/i386vm-notimer.log`. Earlier R3 logs for comparison are in
`logs/known-good-bios/` (R3 with the known-good BIOS) and `logs/seabios-local-build/`
(R3 with the previous local build). `logs/r2-baseline/` holds the R2 logs.

## 4. Phase 0: vector 0x2F and vector 0x27

### 4.1 What changed

- Vector `0x27` (master IRQ7) and vector `0x2F` (slave IRQ7 = IRQ15) now go to
  dispatchers that read the in-service register before deciding on an EOI
  (`kernel/irq.c`, stubs in `kernel/context.s`). The old `spurious_stub` (iret
  only) is no longer used.
- Rules implemented (`kernel/irq.h`):

| Case | Test | Action |
|---|---|---|
| master IRQ7, genuine | master ISR bit 7 set | EOI master |
| master IRQ7, spurious | master ISR bit 7 clear | no EOI |
| slave IRQ15, genuine | slave ISR bit 7 set | EOI slave, then EOI master |
| slave IRQ15, spurious | slave ISR bit 7 clear | EOI master only |

- The PIC registers are read with OCW3 `0x0B` (ISR) on ports `0x20` and `0xA0`
  (`platform/pc/platform.c`). The emulated 8259 supports this
  (`x/tiny386full/tiny386-master/i8259.c`, read select on bit 1 of OCW3).
- On i386VM there is no PIC. `pic_present()` returns 0, so every IRQ7 is
  counted as spurious and no EOI is sent.

### 4.2 Why the master EOI is sent on a spurious IRQ15

When the slave raises IRQ15 the master's cascade input (IRQ2) is accepted and
set in service. If the slave request has gone before the acknowledge cycle,
the CPU gets vector `0x2F` and the slave's ISR is clear, but the master's ISR
bit 2 is still set. Only the master EOI clears it. The emulator behaves the
same way: `pic_intack(&s->pics[0], 2)` sets master ISR bit 2 before the
spurious slave result is returned (`i8259.c`, `i8259_read_irq`).

### 4.3 Test coverage (be exact)

| Path | Exercised? | How |
|---|---|---|
| Vector 0x27, spurious | yes, on both targets | software `int $0x27` (`sw_int27`): no ISR bit set, classified spurious, counter +1 |
| Vector 0x27, spurious from hardware | yes, on the PC | the PC log shows one real spurious IRQ7 (counter 2 = one injection plus one hardware event) in all three runs |
| Vector 0x2F, spurious | yes, on both targets | software `int $0x2F` (`sw_int2f`): classified spurious, counter +1, master EOI sent |
| Vector 0x2F, genuine (slave ISR bit 7) | **no** | this platform masks every slave line, so no slave IRQ15 can be raised. Not claimed as hardware-tested. |
| PIC in-service registers after injection | yes on the PC, n/a on i386VM | both ISRs read 0 after the spurious injections |
| ISR rule (bit 7 means genuine) | pure rule check on both targets | `irq_isr_bit_set(0x80,7)==1` etc. |

The genuine IRQ15 EOI sequence is code-reviewed, not executed. Any future test
for it needs an IRQ15 source on the platform (for example an IDE secondary
channel). That is not added in R4.

The timer-off negative does not depend on these handlers.

### 4.4 Retained R3 interrupt rules

- `kstart` clears IF (`platform/pc/entry.s`).
- `_yield_asm` masks interrupts for the cooperative pass, and `resume_ctx`
  re-enables them.
- `irq_save`/`irq_restore` restore the previous IF.

## 5. Phase 1: JS ROM channel audit

`docs/JS-ROM-CHANNEL-MAP.md` records the source addresses, register
conventions, error codes and lookup. Summary of what is now known:

- TRAP #3 output: operation 5 writes one byte (D1) to the screen. Operation 7
  writes D2 bytes from A1, counting in D1. Operation 6 is "not used" and
  returns -15.
- The ROM's comment on operation 7 says "put in queue". The code writes to the
  screen, and the code is taken as authoritative.
- Channel id is `(tag << 16) | index`, as in R4. Lookup failure is -6 in the
  ROM, -12 in R4.

## 6. Phase 2 and 3: channel manager and console driver

- Table: 8 slots, lowest free slot, tag per slot, object in the kernel heap
  (`kernel/channel.c`).
- Validation: index, slot in use, tag, owner. A failed check changes nothing
  and increments `chan_rejected`.
- Job exit: `reclaim_dead` (`kernel/jobs.c`) calls `chan_close_owner` before
  `mem_reclaim_owner`. Channel close frees the object first, so the heap pass
  finds only the stack and user blocks.
- Driver: `CON` (`struct chan_driver`), whose write calls the platform's
  `con_putc`. On the PC that is port `0x402`; on i386VM it is the MMIO byte at
  `0x800001`. Nothing else in the channel layer touches the platform.
- The kernel diagnostic console (`con_puts`) is not changed. The R4 log shows
  it working before, during and after the channel phase.

## 7. Phase 4: test results

Guest checks, from the R4 logs (`CHECK ...`):

| Requirement | Check | Result |
|---|---|---|
| 1 | open console channel: valid tagged id | ok |
| 2 | write one character returns 1 | ok |
| 3 | write string returns verified byte count (18) | ok |
| 4 | close returns 0 | ok |
| 5 | write after close rejected | ok |
| 6 | double close rejected | ok |
| 7 | reopen reuses the slot with a new tag | ok |
| 8 | stale channel id rejected | ok |
| 9 | other job cannot write, putc or close; owner still open | ok |
| 10 | job exit closes its open channel; its id is stale | ok |
| 11 | no channel objects live; heap restored after the channel phase | ok |
| 12 | writes complete under preemption; CHAN-A was preempted | ok |
| 13 | R2 and R3 checks | ok (all 33 carried over) |
| 14 | timer-off negative does not PASS | ok |

Counters from every run: channels opened 3, closed 3, closed-on-exit 1,
rejected 9 (CHAN-A: 5, CHAN-B: 3, SYSTEM's stale probe of CHAN-C: 1), RECLAIM
jobs 13 (10 from R3 plus 3 channel jobs).

Run-to-run differences are in the preemption counts, tick counts, the MEM-1
high-water mark (288 or 312 bytes), and the number of spurious IRQ7 events on
the PC. The checks assert invariants, not those numbers.

## 8. Known limitations

- One driver (`CON`). No keyboard, files, serial, VGA, directories, or
  SuperBASIC. These are not in R4.
- Synchronous console only. No `-1` "not complete" result and no blocking.
- Raw bytes. No cursor, scroll or LF handling. Output is not identical to the
  JS ROM screen.
- Eight channel slots, fixed.
- User pointers are range-checked only, `0..0x600000`. A job can name memory
  of another job inside that range. This follows the R3 policy.
- Tags wrap after 32 767 opens of one slot. The wrap is not tested.
- The genuine IRQ15 path is not exercised on hardware (§4.3).
- The i386VM start code (`crt0`) is in the external harness, not in this archive.
- Build prints two `volatile` pointer warnings from `kernel/jobs.c`
  (pre-existing in R3; not changed).

## 9. Freeze state

- R3 kernel `qdos386-R3.tar.gz`: unchanged.
- R3-FW1 `qdos386-R3-FW1.tar.gz`: unchanged.
- R4 `qdos386-R4.tar.gz`: this archive, a separate tree. Its manifest is
  `SHA256SUMS`.
