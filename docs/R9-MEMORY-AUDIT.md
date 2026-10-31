# R9 memory audit

This audit covers the address map used by the R9 kernel, why the i386VM
harness needed a change to reach memory above 8 MiB, where the RAM disk moved,
and what the child slot can now hold. The Tiny386 CPU core is not modified.

## 1. The 8 MiB ceiling in the stock harness

The stock i386VM core decodes every physical address from `0x800000` upward as
I/O. `in_iomem()` in `emulator-tiny386/i386.c` makes the decision before the
RAM bound check, and `load8` and `store` test it first. The harness's
`rd`/`wr` callbacks handle only the ports (timer, console out, input data,
input status). Any other address in that range ends the run with
`read/write outside RAM`.

So the stock harness could never provide RAM at or above `0x800000`, whatever
its `RAM` size. Setting `RAM` to 16 MiB in the harness alone would have
allocated memory the core never addresses.

## 2. The change: serve RAM through the I/O callbacks

The fix is confined to `machine.c`, as an optional build flag:

- `-DRAM_MB=n` selects the guest RAM size. The default is 8, which reproduces
  the R8 baseline.
- The device page `0x800000..0x800FFF` keeps its port behaviour.
- Addresses from `0x801000` up to `RAM` are served from the RAM buffer by
  `rd`, `rd16`, `rd32`, `wr`, `wr16` and `wr32`, little-endian, with bounds
  checking (`ram_addr()`), so a multi-byte access that crosses the top of RAM
  is rejected rather than wrapped.
- The core files (`i386.c`, `fpu.c`, `i386.h`, `fpu.h`) are byte-identical to
  the R7 copies.

The patch is `patches/i386vm-r9-ram16.patch`. It applies after
`patches/i386vm-input-status.patch` to the stock harness. Verified in this
round: the stock `machine.c` (SHA-256 `8d92dbad…`) plus both patches reproduces
the built R9 harness source, and the rebuilt binary is byte-identical to the
one the matrix ran (`a4724f79…`). Build command:

    cc -O2 -DI386_ENABLE_FPU -fwrapv -fno-strict-aliasing -DRAM_MB=16 \
       emulator-tiny386/i386.c emulator-tiny386/fpu.c emulator-tiny386/machine.c -lm

Tiny386 is unchanged and keeps 8 MiB (`mem_size = 8M` in `run.ini`).

## 3. Address decoding checks

| Address range | Owner | Notes |
|---|---|---|
| `0x000000..0x7FFFFF` | RAM (both targets) | The kernel, stacks and firmware sit below `0x800000`. |
| `0x800000..0x800FFF` | Device page (i386VM) | Timer `0x800003`, console out `0x800001`, input data `0x800002`, input status `0x800004`. |
| `0x801000..0xFFFFFF` | RAM (i386VM, `RAM_MB=16`) | Served by the patched callbacks. |
| beyond RAM | none | Any access ends the run, as before. |

Writes to other device-page addresses still exit with an error. The patch
changes no existing port behaviour.

## 4. R9 memory map

Defined in `kernel/appmap.h` (`QDOS_LAYOUT_R9`). The default map (R5–R8) is
kept unchanged and is still the build used for the regression.

| Region | R9 start | R9 end (exclusive) | Size | Owner |
|---|---|---|---|---|
| Kernel heap | `0x000000` | `0x200000` | up to 2 MiB | kernel |
| Slot 0 (shell) | `0x200000` | `0x240000` | 256 KiB | shell, resident |
| Slot 1 (child) | `0x240000` | `0x340000` | 1 MiB | loaded applications |
| XFER | `0x340000` | `0x380000` | 256 KiB | loader, FAT16 staging |
| Free | `0x380000` | `0x600000` | 2 MiB | kernel heap extension |
| Stacks and firmware | `0x600000` | `0x7FFFF0` | up to 2 MiB | kernel and firmware |
| Device page (i386VM) | `0x800000` | `0x801000` | 4 KiB | harness ports |
| RAM disk (i386VM) | `0x900000` | `0xC00000` | 3 MiB | FAT16 volume |
| RAM top (i386VM) | `0x1000000` | | 16 MiB | |

For comparison, the default R5–R8 map has slot 0 at `0x200000` (256 KiB),
slot 1 at `0x240000` (256 KiB), XFER at `0x280000` (256 KiB), the RAM disk at
`0x2C0000` (3 MiB) and RAM top at 8 MiB.

### Changes from R8

- Slot 1 grows from 256 KiB to 1 MiB (`APP_SLOT1_SIZE`).
- XFER moves from `0x280000` to `0x340000`. The RAM disk moves from `0x2C0000`
  to `0x900000` on i386VM. Slot 1 now spans `0x240000..0x33FFFF`, so the R8
  RAM disk address (`0x2C0000`) would have sat inside it; the disk had to move
  out of the way.
- The PC (Tiny386) boot sector is a variant of `platform/pc/boot.s` with the
  transfer address changed from `0x280000` to `0x340000`
  (`build/pc-r9/boot.bin`). Tiny386 has no RAM disk in either layout.
- `QDOS_LAYOUT_R9` is a compile-time choice. The R9 images use it; the R5–R8
  images do not.

## 5. Per-slot limits

`kernel/aout.h` gives each slot its own image limit:

- slot 0: `0x38000` (224 KiB) of text, data and BSS, as in R8;
- slot 1: `0xFFE00` (1 MiB less the 512-byte argument block at the slot top).

`aout_check_src` tests each header against the limit for the slot it targets,
so an oversized image is refused with `APP_ERR_LAYOUT` (`-24`) before any slot
is reserved. The R8 negative tests keep their slot 0 limit (`APP_IMAGE_LIMIT_OF(0)`).

Evidence: BIGAPP's a.out header gives text 4,688 + data 65,616 + BSS 409,600 =
479,904 bytes (72,023 bytes on disk). That is above the 224 KiB slot 0 limit,
so it could only run in slot 1 (see `docs/R9-RUNTIME-REPORT.md`).

## 6. Stack placement and ownership

- The kernel and shell stacks stay in `0x600000..0x7FFFF0`, below the device
  page. The top of the stack is `0x7FFFF0`, 16 bytes below `0x800000`.
- Slot 1 is owned by one job at a time (`APP_ERR_SLOTBUSY`, `-28`). The loader
  creates the job before it reserves a slot and writes the argument block at
  the top of the slot, so a failed load leaves no owner behind.
- Heap blocks taken by a job are owned by that job and reclaimed by
  `mem_reclaim_owner` at exit (see `docs/R9-LIBC-ABI.md`).

## 7. Known limits of this audit

- The 16 MiB harness is an i386VM-only configuration. Tiny386 runs the same
  kernel in 8 MiB; the R9 PC image uses the R9 slot map but does not exercise
  memory above 8 MiB.
- The per-slot untouched-memory check in `tests/test_app.c` still samples only
  the first 256 KiB of slot 1. Slot 1 is fully writable and the BIGAPP run
  touches memory across it, but the check itself was not widened.
- The i386VM device page assumes that the harness's `0x800000` ports stay
  exactly as they are. Any future port must be added in the 4 KiB device page.
