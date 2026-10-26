# QDOS/386 R3 - Firmware Validation (known-good Tiny386 BIOS)

Purpose: validate R3 against the known-good Tiny386 firmware supplied with
the R3 validation request, settle the physical memory-map question, review
the three interrupt changes, and freeze the R3 baseline. No kernel source
was changed in this pass. The R3 kernel, its tests and its ABI are as in
`qdos386-R3.tar.gz`. The original archive is unchanged.

## 1. Result

| Question | Answer |
|---|---|
| Does the known-good BIOS reproduce R3? | Yes. PC runs 3 of 3 print `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 1 n/a, 0 FAIL. |
| Is the memory map safe? | Yes, for the fixed arena `0x1064E0..0x600000` (PC). Evidence in §4. |
| Are the interrupt fixes correct? | Yes for the paths exercised. One latent gap on vector `0x2F` is recorded in §5.3; it is unreachable in the current PIC configuration and is not changed here. |
| Freeze state | R3 kernel frozen unchanged. Firmware-validation package created as a separate archive (§7). |

## 2. Firmware

All images are kept outside the source tree. Only hashes are recorded here.

| Image | Size | SHA256 | Source |
|---|---|---|---|
| Known-good `bios.bin` | 131072 | `7bd968419c81fcf8262c5c1182cef43bb0d5eb70f375f94567ae71fd64a15e9e` | supplied zip `6456262f-tiny386_bios.zip` (zip SHA256 `aa045d55c45635c9bbd4b36cf437ae47fcec2ddaec43782241678d1fc39bb0cf`) |
| Known-good `vgabios.bin` | 39424 | `f5aedb52cde5c88d85e5b304a4cc48459c231aef42b9950ccc82bd321d9c127e` | same zip |
| Previous local `bios.bin` | 131072 | `f33ed9836dd628c960757d43d9cbeaa037647063a2068cd3aabb143bf96adf91` | local SeaBIOS build, kept for comparison (`logs/seabios-local-build/`, `external/`) |
| Previous local `vgabios.bin` | 39936 | `192cce7be2adb9582a8f9c610b9f43af1e3edbd48a4a75b5ed14d3d1d75fa5b9` | same |

The known-good images are a dirty SeaBIOS build as well. Their banner is
`rel-1.17.0-24-g28aa4634-dirty-20261025_090831-runnervmr9t6g`, so their
provenance is the supplier's build, not an upstream-pristine build. Their
source tree was not supplied, so this package does not claim upstream
provenance for them.

Layout check (Tiny386 source, `main.c` `load_rom` and `pc.c` lines 925–927):

- `bios.bin` is loaded backward so that it ends at `0x100000`. For a 128 KiB
  image it occupies `0xE0000..0xFFFFF`.
- `vgabios.bin` is loaded forward at `0xC0000` (`0xC0000..0xC99FF`).
- Reset vector: both known-good and previous images hold `ea 5b e0 00 f0`
  at file offset `0x1FFF0`, i.e. `f000:e05b` at physical `0xFFFF0`.

## 3. Runtime matrix (known-good BIOS)

Command (from the package root, after building `build/pc/run.ini` by
`make`):

```
I386VM=<scratchpad>/x/i386vm/i386vm  TINY386=<scratchpad>/t386_headless \
SEABIOS=<scratchpad>/firmware-known-good  scripts/run-all.sh
```

Emulator hashes:

- `tiny386_headless` SHA256 `030cda746a37ab8e81bb21725232dfa70ca413ce448258e3f7e2c31791cce8a7`
  (unmodified Tiny386 source)
- `i386vm` SHA256 `baa683711ecfe674a0e3b226e166db3160aec59ab922b2ea264b146590082a6c`
  (with `patches/i386vm-synthetic-timer.patch`)

`build/pc/run.ini` used (paths are the local scratchpad locations):

```
[pc]
bios = <firmware>/bios.bin
vga_bios = <firmware>/vgabios.bin
mem_size = 8M
hda = build/pc/r3pc.img
```

Results (logs in `logs/known-good-bios/`):

| Run | Result | Checks | FAIL | Reclaims | Bytes reclaimed | Stack faults | MEM-1 hwm |
|---|---|---|---|---|---|---|---|
| PC 1–3 | MEMORY PASS | 32 ok, 1 n/a | 0 | 10 | 42208 | 0 | 308 |
| i386VM 1–3 | MEMORY PASS | 33 ok | 0 | 10 | 42208 | 0 | 288 |
| i386VM timer off | no PASS, `instruction limit exceeded` | - | - | - | - | - | - |

Heap at the end of each run: `used 4112`, `blocks 1/2` (IDLE stack only),
on both targets.

The PC counts are the same as the earlier local-build matrix
(`logs/seabios-local-build/`). The high-water values match it too.
Tick counts were not compared and are not required to match.

Evidence that the negative test does not false-pass: the timer-off build
prints `TIMER: disabled (negative test build)` and then stops at the
instruction limit, with no PASS line.

### 3.1 Additional PC run without a VGA option ROM

`vga_bios` was replaced by a 32 KiB zero-filled file (`novga/vgabios-zero.bin`,
not a valid option ROM). Removing the key entirely makes Tiny386 abort
(`load_rom: open vgabios.bin failed`), so the file must exist.

Result: `QDOS/386 R3 MEMORY PASS`, 32 checks ok, 0 FAIL
(`logs/known-good-bios/pc-novga.txt`).

So the port `0x402` debug console does not depend on the VGA BIOS. The
placeholder has no option-ROM signature, so SeaBIOS has no VGA ROM to run,
and the disk image still boots and passes.

## 4. Physical memory map

### 4.1 PC (Tiny386, 8 MiB)

Final E820 table seen by the guest, from `logs/known-good-bios/pc-run1.txt`
(the `e820 map has 5 items` dump, printed by `e820_prepboot`):

```
0: 0x000000000 - 0x00009fc00  RAM
1: 0x00009fc00 - 0x0000a0000  RESERVED
2: 0x0000f0000 - 0x000100000  RESERVED
3: 0x000100000 - 0x000800000  RAM
4: 0x0fffc0000 - 0x100000000  RESERVED
```

Why `0x7C0000..0x7FFFFF` is RAM in this table and not reserved. This was
checked in the SeaBIOS source at the tag used by the known-good build.

- `malloc_preinit` (`src/malloc.c` ~413–456) takes the top 256 KiB of RAM as
  the temporary high table, with `e820_add(highram_start, 256K, E820_RESERVED)`.
  With 8 MiB RAM the 16 MiB preference does not fit, so the fallback
  `BUILD_MIN_HIGHTABLE` (256 KiB) is used. That is `0x7C0000..0x800000`.
- `malloc_prepboot` (`src/malloc.c` line 531, run from `post.c` line 174,
  before `e820_prepboot` at line 176) gives back the unused high table with
  `e820_add(..., E820_RAM)`. The boot log line `Returned 262144 bytes of
  ZoneHigh` is that step, and nothing was allocated in it.
- `e820_prepboot` dumps the map after that, so the dump is the final table.
  This resolves the open question in the earlier draft.

So SeaBIOS reserves the top 256 KiB only while it runs, and then returns it
to RAM before handing off to the boot sector. The boot stack top
`0x7FFFF0` (`platform/pc/entry.s`) therefore sits in guest RAM. The stack is
not in a firmware-reserved region.

Regions, from source and the final map:

| Region | Range | Owner after boot |
|---|---|---|
| BIOS data, EBDA area | `0x00000..0x9FBFF` | firmware / BDA; not touched by R3 |
| Video and option ROM area | `0xA0000..0xFFFFF` | reserved; kernel does not use it |
| Kernel image (linked at) | `0x100000..` (`kernel.raw`) | kernel `.text`/`.data` |
| BSS | up to `_bss_end` = `0x1064E0` | kernel |
| Heap arena | `0x1064E0..0x600000` (5,217,056 bytes) | heap, job stacks, user blocks |
| Free, not used by R3 | `0x600000..0x7BFFFF` | RAM, unused |
| Former SeaBIOS high table | `0x7C0000..0x7FFFFF` | RAM, returned by SeaBIOS |
| Boot stack | grows down from `0x7FFFF0` | kernel boot path only |

Non-overlap:

- Kernel image and BSS end before the arena (`_bss_end` is the arena base,
  rounded up to 16).
- The arena ends at `0x600000`. Job stacks and every user block are inside the
  arena, so they cannot reach the boot stack region, which begins at
  `0x7FFFF0` and grows down. The gap `0x600000..0x7BFFFF` is never allocated.
- The boot stack would have to descend about 2 MiB before it reached the
  arena. The boot path only runs the heap tests and `main` before the first
  switch, and the job stacks are on the heap. The stack depth is not
  instrumented. The bound is by design and is not a measured number.
- The Tiny386 BIOS and VGA BIOS images are below `0x100000`. They are
  outside the arena and outside the kernel.

The arena is a fixed range. Its upper bound is `MEM_ARENA_END = 0x600000`,
as allowed by the validation request. It is not extended to 8 MiB.

### 4.2 i386VM

The same arguments apply with the i386VM load address. The kernel is loaded
at `0x000000`, `crt0` sets `esp = 0x7FFFF0`, and the arena is
`0x006430..0x600000` (6,265,808 bytes). The i386VM harness has no firmware
or reserved high memory to consider.

## 5. Interrupt review

### 5.1 Interrupts disabled at kernel entry

- `platform/pc/entry.s` starts `kstart` with `cli`. The reason is in
  `R3-MEMORY-DESIGN.md` §5.2: IF is inherited from the boot path, and a
  SeaBIOS tick can be pending at reprogram time.
- `kernel/context.s` `_yield_asm` begins with `cli`.
- The i386VM start code (`crt0`) is in the external harness, not in this
  archive, so its interrupt state at entry is not checked here. The i386VM
  runs are deterministic and pass on their own terms (§3).

Verified: the fixed PC run shows no `abort: -1` and no spurious-vector
failure in 3 of 3 runs, and in the first run of the no-VGA test.

### 5.2 Interrupt masking around cooperative scheduler transitions

- `_irq_save` is `pushfl; popl %eax; cli; ret`, so it returns the previous
  EFLAGS and clears IF.
- `_irq_restore` is `test $0x200; jz; sti`, so it sets IF only if the saved
  flags had IF set. A nested critical section therefore does not enable
  interrupts early.
- The heap mutators `mem_alloc` (`kernel/memory.c` line 138) and `mem_free`
  (line 205) wrap their bodies with `irq_save`/`irq_restore`, and every return
  path goes through `irq_restore` with the saved flags. `mem_reclaim_owner`
  has no wrapper of its own; it calls `mem_free` for each block, so each free
  is protected. `mem_stats` and `mem_validate` are read-only walks and do not
  change the heap.
- `_yield_asm` masks interrupts for the cooperative pass, and `resume_ctx`
  re-enables them with `sti` only for fmt-0 frames. This is the R2 latent
  race fix, retained unchanged.

Verified by code review and the matrix. The `irq_restore` semantics were
checked by inspection, not by a dedicated test.

### 5.3 Spurious IRQ7

Handler (`kernel/context.s`): `_spurious_stub: iret`, with no EOI. It is
installed on vector `0x27` (master IRQ7) and vector `0x2F` (slave IRQ7),
in `kernel/descriptors.c` lines 61–62.

- Vector `0x27`. A spurious master IRQ7 is not an in-service interrupt, so
  it must not be acknowledged. `iret` without EOI is correct. The master
  `ISR` bit is not set by a spurious request, so there is nothing to clear.
- Vector `0x2F`. A spurious slave IRQ7 is different. On real 8259 hardware
  the master's cascade line (IRQ2) is in service during the slave's
  acknowledge cycle, so a non-specific EOI to the master is required even
  for a spurious slave IRQ7. The current handler does not send it.
- Reachability in this platform: `platform/pc/platform.c` `pic_init` masks
  every slave line (`outb(0xa1, 0xff)`) and leaves only IRQ0 unmasked on
  the master, with IRQ2 masked. The slave cannot raise a request, so vector
  `0x2F` cannot be generated on this platform.
- The emulator behaviour that the handler covers is recorded in the design
  note. Its 8259 model returns `irq_base + 7` when no request is pending
  (`src/i8259.c`, `i8259_read_irq`), which is how the earlier abort was
  produced.

Conclusion: the `0x27` handling is correct. The `0x2F` handling is latent: it
would be wrong if the slave were ever unmasked. This pass does not change the
kernel. The fix is one line (send a master EOI before `iret`, in a separate
stub for `0x2F`) and belongs in a later revision with its own test. Recorded
as an open item in §8 of the runtime report.

The emulator's PIC state was not inspected directly. The result rests on the
8259 model and on the pass results.

## 6. What this pass did not do

- It did not rebuild SeaBIOS. The supplied images passed, so no source-level
  investigation was needed.
- It did not change the R3 kernel, tests, ABI or scheduler.
- It did not extend the heap to 8 MiB.
- It did not add host-side allocator tests.
- It did not measure the boot stack depth.

## 7. Package

- Frozen R3 archive: `/mnt/user-data/outputs/qdos386-R3.tar.gz`, SHA256
  `4ea1babe078b63a685f93302ab2db7eedfe7ca9c6cd7306adb77afb2d48643ac`.
  Not changed.
- This package: `qdos386-R3-FW1.tar.gz`, containing the same sources and
  tests as R3, plus this document, the updated runtime report and the
  known-good logs. It does not contain firmware binaries or emulator binaries.
  `build/pc/run.ini` is generated and not included. See the checksums in
  `SHA256SUMS`.
