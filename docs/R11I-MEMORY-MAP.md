# R11I — i386VM memory map, legacy RAM disk vs host-backed block device

Both maps are the R9 layout (`QDOS_LAYOUT_R9`) on the 16 MiB i386VM harness.  The block map is
selected only by `-DQDOS_I386VM_BLOCK`; the legacy i386VM map and the PC map are unchanged
(image hashes in `R11I-RUNTIME-REPORT.md` §1).

| Range | Legacy (R11H) | Block (R11I) | Allocator (block) |
|---|---|---|---|
| `0x000000–0x1FFFFF` | kernel, data, heap | same | not in pool |
| `0x200000–0x23FFFF` | slot 0 (shell) | same | FREE, origin |
| `0x240000–0x33FFFF` | slot 1 (1 MiB) | same | FREE, origin |
| `0x340000–0x37FFFF` | transfer area | same | not in pool |
| `0x380000–0x47FFFF` | profile 2 | same | FREE, origin |
| `0x480000–0x4FFFFF` | unassigned (between profiles) | same | not in pool |
| `0x500000–0x5FFFFF` | profile 3 | same | FREE, origin |
| `0x600000–0x7FFFFF` | boot stack (top `0x7FFFF0`), firmware data | same | not in pool |
| `0x800000–0x800FFF` | device page (ports `…01–…04`) | + block ports `0x800010–0x80001A` | not in pool |
| `0x801000–0x8FFFFF` | RAM, unused | same | not in pool |
| `0x900000–0xBFFFFF` | **RAM disk** (FAT16 volume, 3 MiB) | **spare RAM** | **FREE, no origin** |
| `0xC00000–0xFFFFFF` | RAM, unused | spare RAM | **FREE, no origin** |

Spare span (block only): `SPARE_BASE 0x900000` … `SPARE_END 0x1000000` (7 MiB), `RAMDISK_SIZE 0`.

## Images

| Image | Size | Contents |
|---|---|---|
| legacy `qdos386-r11i-legacy.bin` (= R11H release) | 12,582,912 | kernel, transfer area, FAT16 volume at `0x900000` |
| block `qdos386-r11i-block.bin` | 3,670,016 | kernel and transfer area only (ends at `0x380000`) |

## Invariants verified (`tests/host/r11i/layout_test.c`, 39 checks)

The pool is built exactly as `kernel/aout.c app_pool_init` builds it in block mode.

* Spare span: page-aligned, non-empty, ends at RAM top, above and disjoint from the device page,
  disjoint from the stack/firmware area, heap, both slots, the transfer area and both profiles;
  no profile origin lies in it.
* `region_init` accepts the span table (no overlap, ordered, aligned).
* Every profile origin is still reservable once; a second reservation is `BUSY`.
* With all origins reserved, a single 7 MiB allocation succeeds and lands exactly on
  `0x900000…0x1000000`, not on any live region; after it, one more page is `NOMEM`.
* Ownership: release by another owner → `OWNER`; stale handle → `HANDLE`; zero size → `ARG`;
  oversized → refused; `region_check` holds throughout.

## What is and is not proven in the guest

The R11H loader only places images at fixed profile origins, and R11I does not widen that
(the plan forbids it).  No application can allocate the spare span, so the guest proof is a
**bounded, test-only kernel hook** (`-DQDOS_TEST_SPARE`, compiled only together with
`-DQDOS_I386VM_BLOCK`; the release block image is byte-identical with or without the hook source).

On the first pool build (`app_pool_init`, before the shell loads) the hook:

1. checksums the transfer area and `0x801000…0x8FFFFF`;
2. allocates `SPARE_END − SPARE_BASE` (7 MiB) through `region_alloc` and checks the range is exactly
   `0x900000…0x1000000` and `region_check` holds;
3. allocates one more page and checks it lands **outside** the spare span, then releases it;
4. writes a distinct word at the first and last 4 bytes of every 4 KiB page of the span, then reads all
   of them back (the harness exits on any access outside guest RAM, so this also proves the RAM exists);
5. re-checksums the two neighbouring areas (unchanged) and reads sector 0 through the block device
   (signature `55 AA`);
6. checks a foreign-owner release is refused, the owner release succeeds, a stale handle is refused, and
   the pool is empty afterwards.

Result (`logs/r11i/spare/i386vm-spare.txt`):

    SPARE: base 9437184 end 16777216 pages 1792
    SPARE: PASS

after which the shell loads and `RUN MULTI.AOU` → `MULTI: PASS`, `DIR` → `53 files`, with the volume
hash unchanged.  The host test above covers the same allocator paths exhaustively.  An application-level
allocator syscall for spare RAM remains follow-on work.
