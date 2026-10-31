# QDOS/386 R6 - audit of the R5 loader and the Tiny386 IDE path

Status: audit only. No R6 code yet. The R6 branch starts from the frozen R5
archive (`qdos386-R5.tar.gz`, sha256 `ae173170…`). R5 itself is unchanged.

## 1. R5 loader: what R6 can reuse

| Item | Finding | Impact on R6 |
|---|---|---|
| `aout_check(img, len, out)` | Pure. Takes any byte buffer and an exact length. No knowledge of where the bytes came from. | Reusable unchanged for file-loaded images. |
| `app_load(index)` | Gets bytes through `app_image_bytes(index)`, which reads the transfer directory at `0x280000`. | Needs a source abstraction. R6 adds a file source that fills a staging buffer, then calls the same `aout_check` and the same copy and job path. |
| Staging for file reads | None. The transfer area is read-only in spirit and must stay as R5 left it. | R6 needs its own staging buffer, not `0x280000`. Max size is 256 KiB (one slot). The heap can hold it (about 1 MB on PC, about 2 MB on i386VM), but a fixed region is simpler and avoids heap pressure. |
| Exact length | `aout_check` requires the length to equal the end of the string table. | A FAT16 file gives its size in the directory entry. Clusters are padded, so the loader must be given the directory size, not the cluster-rounded size. This is the same padding rule as R5. |
| Slots | Two slots, 256 KiB each, linked at fixed addresses. | R6 can load into the same slots. Only the image source changes. |
| Ring 0, no protection | Unchanged. | Out of scope for R6. |

Conclusion: the loader needs one new entry point, `app_load_bytes(buf, len, slot_hint)`,
with `app_load(index)` kept as a wrapper. No change to validation rules.

## 2. Tiny386 IDE: what the PC path provides

Source: `x/tiny386full/tiny386-master/ide.c`, `ide.h`, `pc.c`.

- **Controller.** PIIX3 IDE on the standard primary and secondary ports
  (`0x1F0`–`0x1F7`, control `0x3F6`; secondary `0x170`–`0x177`, `0x376`), wired in
  `pc.c` (lines 183–194, 291–348).
- **Disk options.** `hda=`, `hdb=`, `hdc=`, `hdd=` attach the first four drives.
  `hdb` is the primary slave. A data disk therefore needs no change to the boot
  disk.
- **Commands implemented.** IDENTIFY, SPECIFY, RECALIBRATE, SET MULTIPLE, READ
  (and READ ONCE), WRITE (and WRITE ONCE), MULTIPLE READ/WRITE, READ NATIVE MAX.
  PIO read (`WIN_READ`, `0x20`) is what the R6 driver needs.
- **Data path.** PIO through the data port (`ide_data_readw` / `readl`, lines
  1681–1748), one sector per transfer, with status polling on the command register.
- **File backing.** `block_device_init` opens the image file. Reads are
  `bf_read_async`. A debug dump to `/tmp/read_sect.txt` exists under the
  `DUMP_BLOCK_READ` macro, which is off by default.

### Read-only problem (decision needed)

`ide_attach` always calls `block_device_init(filename, BF_MODE_RW)`, which opens
the file with `"r+b"`. A read-only file therefore makes the emulator exit with
`perror` (`fopen` failure, `ide.c` around line 2030). There is no read-only option
for hard disks in the emulator.

Options:

1. Accept RW and enforce read-only in the guest. The R6 driver issues no write
   commands, and the test records the data disk's SHA256 before and after each
   run. Requires no Tiny386 change.
2. Add a `hdb_ro=` option (a small Tiny386 change). This is an emulator change,
   which the R4 policy forbids unless a defect requires it. It is not a defect, so
   option 1 is the default.

Recommendation: option 1 for R6, with the hash check as evidence.

## 3. i386VM: no IDE, a synthetic block device is needed

The i386VM harness (`bin/i386vm`, `emulator-tiny386/machine.c`) has no disk
controller. Its only devices are the byte console at `0x800001` and the synthetic
timer at `0x800003`. The flat image is the only storage, loaded at guest address
0.

Two ways to present a block device on i386VM:

- **A. Preloaded RAM disk (no harness change).** The FAT16 image is zero-padded
  into the flat file at a fixed address (a second region after the transfer area,
  for example `0x300000`). The guest block driver reads sectors from that memory.
  The bytes are identical to the PC disk image, so the FAT16 code is shared. This
  is the same approach the approved R5 transfer area used, and it needs no harness
  change. It is not a real device, so the test proves the filesystem path, not
  device access.
- **B. Harness block port (harness change).** Add a sector-read port to
  `machine.c` that reads a host file. This exercises a real request path, but
  changes the external harness, which R5 did not require.

Recommendation: A, since the R5 instruction was to avoid harness changes
unless needed. The request says "minimal synthetic block-device backend", which
fits A if the device interface is a small set of sector-read functions.

## 4. Common FAT16 semantics

Both backends present the same interface: `blk_read(lba, count, buf)` returning
bytes read, with a fixed sector size of 512. Everything above it (boot sector
parse, FAT chain walk, directory scan, 8.3 name match, `WIN1_` namespace) is the
same code on both targets.

## 5. Open items from the request

The request was cut off after "Please first audit the existing R5 loader,
Tiny386 IDE implementation and". The remaining audit items were not given. Before
R6 implementation starts, the rest of the audit list and the decisions in
sections 2 and 3 are needed.

## 6. Decisions requested

1. Read-only data disk: option 1 (guest-enforced, hash-checked) or option 2 (emulator change)?
2. i386VM block device: option A (preloaded RAM, no harness change) or option B (harness port)?
3. `WIN1_` namespace: `WIN1_` as a prefix on a file name, or a directory? (for example `WIN1_HELLO.AOU` or `WIN1_\HELLO.AOU`).
