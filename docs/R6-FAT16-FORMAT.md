# QDOS/386 R6 — WIN1_ FAT16 volume

## 1. What is on the volume

`build/fat16/win1.img` (made by `tools/make-fat16.py`) is a 3 MiB FAT16
volume: 6144 sectors of 512 bytes, one sector per cluster.

| Region | Sectors | Contents |
|--------|---------|----------|
| Boot sector | 0 | BPB and `55 AA` signature |
| FAT copy 1 | 1–24 | 24 sectors, 16-bit entries |
| FAT copy 2 | 25–48 | identical copy |
| Root directory | 49–80 | 512 entries of 32 bytes |
| Data | 81–6143 | 6063 clusters, numbered 2–6064 |

Files in the image: `HELLO.AOU` (387 bytes, first cluster 2, the hello
application) and `PREP.AOU` (666 bytes, first cluster 3, the R5 prep
application). The generator packs each file in a contiguous run of clusters.

The image's SHA-256 is printed by the generator and listed in `SHA256SUMS`.
The same file is the Tiny386 `hdb` disk and the RAM-disk region of the i386VM
image, byte for byte.

## 2. Why 3 MiB

The proposal suggested 4–8 MiB. R6 places the RAM disk for i386VM inside the
guest memory map, between the transfer area (ends `0x2C0000`) and the stack
and firmware floor (`0x600000`). That is 3 MiB exactly, so the volume is
3 MiB. The cluster count (6063) keeps it FAT16 with one sector per cluster.
A 4 MiB volume would overlap the stack area, so it was not used.

## 3. Classification

The kernel decides the FAT type from the cluster count alone, as Microsoft's
specification does:

- fewer than 4085 clusters: FAT12, refused (`-42`);
- 4085 to 65524 clusters: FAT16;
- more: refused (`-42`).

The FAT must hold entries 0 to clusters+1, which is checked against
`sectors_per_FAT × 256`.

## 4. Boot-sector checks (`fat16_mount`)

The kernel refuses the volume (`-41`) unless:

- signature `55 AA` at offset 510;
- bytes per sector = 512;
- sectors per cluster is a power of two, 1 to 128;
- reserved sectors ≥ 1;
- two FAT copies;
- root entries ≠ 0 and a whole number of sectors;
- total sectors (16-bit field, or the 32-bit field if that is 0) is non-zero
  and not larger than the device;
- sectors per FAT ≠ 0;
- the metadata (boot, FATs, root) is smaller than the total.

The volume label, OEM name and the `FAT16` text in the boot sector are not
used.

## 5. Directory and chain checks

Lookup reads the root directory until the end marker (`0x00`) and skips
deleted entries (`0xE5`), volume labels and subdirectories (attribute bits
`0x18`, which also skips long-name entries). A match gives the first cluster
and size.

At open the chain is validated:

- every cluster number is in 2..clusters+1;
- the chain never exceeds `ceil(size / 512)` clusters (a cycle makes it
  exceed this bound, so cycles are detected without a visited set);
- the chain ends with an end-of-chain value (≥ `0xFFF8`), at exactly the
  number of clusters the size needs.

A chain that ends early gives `-45`. Free (`0`), bad (`0xFFF7`) or
out-of-range entries give `-43`.

Every sector read checks `lba < total_sectors` (`-47` otherwise). The sector
source is never asked for a sector outside the volume.

## 6. Tests

The positive path is run on both targets. The negative cases use a sector
source that replaces one sector with a patched copy:

| Case | Patch | Expected |
|------|-------|----------|
| 1024-byte sectors | BPB bytes/sector | `-41` |
| FAT12-sized volume | total sectors = 2000 | `-42` |
| volume past device | total sectors = 32768 | `-41` |
| cyclic chain | first cluster points to itself | `-43` |
| bad-cluster mark | first cluster = `0xFFF7` | `-43` |
| size beyond chain | directory size = 2 MiB | `-45` |
| missing file | — | `-44` |
| lowercase name | — | `-46` |
| 9-character base | — | `-46` |
| seek past end | — | `-47` |

## 7. Limits

- Read only; no writes, no allocation, no FAT update.
- Root directory only. Subdirectories are not followed.
- No long file names; names are uppercase 8.3 only.
- Files up to the size the chain holds, and the R6 staging buffer (8 KiB) for
  loading.
- One sector per cluster. The reader supports more, but the generator makes
  one.
- One volume, mounted by `win1_mount()`.
