# QDOS/386 R6 — runtime report

## Result

Both targets pass three runs each. The guest prints, in order, after real
FAT16 reads, a real load and a real preemptive run:

    QDOS/386 R6
    WIN1_: FAT16 mounted
    WIN1_HELLO.AOU: opened
    HELLO: loaded from filesystem
    Hello from an independently linked i386 a.out!
    HELLO: exited, status 7
    QDOS/386 R6 FAT16 EXEC PASS

The R2–R5 checks are still run and still pass. Logs: `logs/r6/`.

| Target | Run | Checks | Result |
|--------|-----|--------|--------|
| i386VM | 1, 2, 3 | 129: 128 ok, 1 n/a (PIC in-service registers), 0 FAIL | pass, order ok |
| Tiny386 PC | 1, 2, 3 | 129: 128 ok, 1 n/a (device ticks), 0 FAIL | pass, order ok |
| i386VM, timer off | 1 | — | no PASS; instruction limit reached |

Of the 129 checks per run, 25 are new in R6: 14 positive file checks and 11
negative FAT16 checks.

## What the R6 phase checks

Positive (on the real disk): mount; open through `QT_CH_OPEN`; read to the end
in 256-byte chunks; read at end returns 0; the byte count equals the directory
size; the bytes pass the R5 a.out check; seek to 0 and re-read gives the same
16 bytes; seek past the end is refused; write to a file channel is refused;
close; `app_load_bytes` loads into slot 0; exit status 7; slot released.

Negative (in the kernel, using the same sector source with one sector
replaced): 1024-byte sectors, a FAT12-sized volume, a volume past the device,
a cyclic chain, a bad-cluster mark, a size beyond the chain, a missing file, a
lowercase name, a 9-character base name, and a seek past the end. Each is refused
with the expected error code (table in `docs/R6-FAT16-FORMAT.md`).

## Disk integrity

`build/fat16/win1.img` SHA-256 before the matrix and after it:

    c6fcfcdd5b7f399b4fc9f0ee8c7283a362a52e46a2a28dacebe5ab101b6ce774

Unchanged after all seven runs. Tiny386 was attached with `hdb` (primary
slave) in read-write mode, as its IDE code requires; the driver issues only
IDENTIFY DEVICE and READ SECTORS.

## Memory layout (verified before implementation)

Measured on the built images:

| Region | Address | Source |
|--------|---------|--------|
| i386VM kernel image | 0x000000–0x0085FB (34300 bytes) | `kernel-raw.bin` |
| Kernel BSS end (i386VM heap start) | 0x00E310 (58128) | "Heap: arena" line |
| Kernel BSS end (PC heap start) | 0x10E0F0 (1107504) | "Heap: arena" line |
| Heap | to 0x200000 | `HEAP_END` |
| Application slots 0, 1 | 0x200000–0x27FFFF | `appmap.h` |
| Transfer area | 0x280000–0x2BFFFF (1600 bytes used) | `XFER_BASE` |
| **RAM disk (i386VM)** | **0x2C0000–0x5BFFFF (3 MiB)** | `RAMDISK_BASE` |
| Free | 0x5C0000–0x5FFFFF | — |
| Stacks and firmware floor | 0x600000 and up; boot stack top 0x7FFFF0 | `runtime/crt0.s`, `RAMDISK_FLOOR` |

`appmap.h` checks the placement at compile time (`#error` if the RAM disk
overlaps the transfer area or the stack floor). The i386VM file is
0x5C0000 bytes, under the 8 MiB RAM.

The R5 transfer area is unchanged: `build/xfer.bin` is byte-identical to the
R5 archive's, and the image holds it at 0x280000 with zero padding to
0x2C0000. The RAM-disk bytes at 0x2C0000 are byte-identical to `win1.img`.

## Build environment

- Toolchain: GCC 1.40, GAS 1.38, `i386-ld` 1.94-beta, unchanged.
- SeaBIOS: `firmware-known-good/bios.bin` (SHA-256 7bd96841…), `vgabios.bin`
  (f5aedb52…).
- Tiny386: built from a copy of the source tree (`make tiny386_headless
  USE_SLIRP=n`). The source tree was not modified. `USE_SLIRP=n` was needed
  because libslirp headers are not installed here; it affects only the NE2000
  network device, not the IDE disk or the console. Resulting binary SHA-256
  fb63b405…. The binary is not redistributed.

## Limits and deviations

- **Volume size: 3 MiB, not 4–8 MiB.** The i386VM memory map leaves 3 MiB
  between the transfer area and the stack floor. A larger volume would overlap
  the stacks. The volume is still FAT16 (6063 clusters).
- **Trap numbers 0x1A and 0x1B** are new. `CHAN_NAME_MAX` rose from 8 to 16 so
  that `WIN1_HELLO.AOU` fits; R4's existing name tests are unchanged and pass.
- **The negative tests substitute sectors in memory.** They do not provide
  corrupt FAT16 images as files on disk. The same parser code reads the real
  disk, so the checks cover the code path, not the disk's contents.
- **Timer-off negative:** as in R5, it shows that the PASS line is not printed
  without the timer, but it stalls in the R2 timeout stage. It does not isolate
  preemption on its own.
- **Read only, root directory only, uppercase 8.3 only, 8 KiB load staging.**
- **Not repeated for R6:** the R5 fresh-extraction rebuild, and any timing of the
  PC disk path beyond the four-second runs.
- **Repository:** the `build/` directory is still in the baseline commit
  (not rewritten, as instructed). The R6 commit removes it from tracking;
  `.gitignore` keeps it out.
