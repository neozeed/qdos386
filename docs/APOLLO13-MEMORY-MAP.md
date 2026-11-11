# Apollo 13 (R17) — physical memory map and frame pool

Opt-in: `make qdos386-r17-pc` (`-DQDOS_R17`). The KCTX-FIX2, R16A and R16B targets keep the R16B rule (CMOS, at most
16 MiB) and are byte-identical to their frozen images.

## 1. Where the map comes from

| Step | Where | What |
| --- | --- | --- |
| E820 query | `platform/pc/boot3.s` (QBOOT3, only on the R17 images) | Before loading the kernel the boot sector runs BIOS INT 15h EAX=E820h (`'SMAP'`, 24-byte buffer, at most 32 calls, stops on CF, on EBX=0 or on a wrong signature). Records go to 0x504 + 24·i; the end pointer is stored at 0x500. The sector is tagged `QBOOT3` at offset 0x1CA so the kernel can tell it from QBOOT2. Code ends at 0x1A9 (limit 0x1B0, below the partition table). |
| Capture | `memmap_capture()`, first call in `shellboot` | Copies the records out of low memory before anything can reuse it. No tag, an end pointer below 0x504, above 0x504 + 32·24, or not a multiple of 24 → *no map*. |
| Build | `memmap_build()` in `proc_boot` | Builds a one-bit-per-page bitmap for 0…256 MiB (see §2). |
| Fallback | `proc_boot` | *No map*, or a map with no usable page above 1 MiB → the R16B rule: CMOS 0x30/0x31 extended KiB + 1 MiB, at most 16 MiB. Logged as `MEMMAP: … CMOS fallback (16 MiB rule)`. |

The firmware map is never written, and no firmware setting is changed.

## 2. Validation rules (`kernel/memmap.c`)

1. Only type 1 (usable) records add pages; ACPI reclaim (3), NVS (4), bad (5) and any unknown type count as reserved.
2. Usable records are rounded **inwards** to whole pages. Reserved records are rounded **outwards**.
3. **Reserved wins**: every page touched by any non-usable record is removed, even if a usable record also covers it
   (overlapping or contradictory records cannot put a reserved byte in the pool).
4. Nothing below 1 MiB (pages 0…255) is ever pool. The BIOS data area, EBDA, VGA memory and ROMs stay out by construction.
5. Zero-length records and records starting at or above 4 GiB (`base_hi ≠ 0`) are ignored. A record whose end wraps past
   4 GiB is clamped at 4 GiB.
6. Everything at or above the **256 MiB ceiling** (`MM_CEILING` 0x10000000) is counted (`above-ceiling KiB`), reported
   and not used.
7. MMIO is never allocated. The LAPIC (0xFEE00000) and IOAPIC are above the ceiling, and PCI holes are reserved or
   absent in the map. The LAPIC page is mapped separately, uncached and supervisor-only (`pg_map_lapic`).

`MEMINFO` (`QT_SMP` op 5) returns the raw records, the totals and the pool counters to CPL3. It is shown by
`/BIN/MEMINFO`.

## 3. Why 256 MiB, and not 4 GiB

The kernel reaches a frame only through the identity mapping in the template directory. Process windows start at
0x10000000 (heap) and 0x1FFF0000 (stack). The identity map can therefore cover directory entries 0–63 (0…256 MiB)
without overlapping any user window.

Going further would need:

* a kernel mapping window (temporary mappings), or
* PAE, or
* moving the user windows.

Each of these is an ABI or design change outside Apollo 13's scope. **R17 supports up to 256 MiB of RAM.** More RAM
boots, is clamped, and is reported (`-m 512` → `262016 KiB above it not used`).

## 4. Physical layout (R17)

| Range | Use |
| --- | --- |
| 0x000000–0x0FFFFF | real-mode area, boot sector, E820 buffer (0x500), AP trampoline (0x9000), VGA, ROMs — never pool |
| 0x100000–bss_end | kernel image + BSS (frame bitmaps for 65,536 frames, memmap bitmap, template low tables) |
| bss_end–0x1FFFFF | kernel heap (stacks, channels, surfaces) — unchanged |
| 0x200000–0x33FFFF, 0x380000–0x67FFFF | pool (as R16B, if usable per the map) |
| **0x680000–0x6FFFFF** | **R17 kernel area** (bump allocator `r17_karea_alloc`): template page tables for 16…256 MiB (one 4 KiB table per 4 MiB span, allocated only for spans with RAM), AP stacks (16 KiB each, 7 APs). Removed from the pool in R17. |
| 0x700000–0x7FFFFF | boot stack area, as R16B (not pool) |
| 0x800000–top (≤ 256 MiB) | pool, page by page as the bitmap allows |

The template directory identity-maps every span with RAM, supervisor-only. In a process directory, a span that is
entirely pool is *not present*. A partly-pool span gets a private table copy with the pool PTEs cleared. This is the
R12A rule, unchanged, so user code cannot see pool frames or kernel memory. The kernel touches pool frames only while
the template directory is active.

Limits raised for R17 only:

| Limit | R16B | R17 |
| --- | --- | --- |
| `FR_LIMIT` / frame bitmaps | 16 MiB / 4,096 | 256 MiB / 65,536 |
| `TPL_DIRS` (identity tables) | 4 (BSS) | 4 low (BSS) + up to 60 high (kernel area) |
| `VAS_MAX_TABLES` per process | 16 | 32 |
| `QDOS_PROC_HEAP_MAX` | 8 MiB | 64 MiB |

## 5. Measured (QEMU `pc`, SeaBIOS, R17 VGA kernel; logs/a13/p1)

| `-m` | E820 usable below ceiling | identity | pool frames (above 16 MiB) | notes |
| --- | --- | --- | --- | --- |
| 16 | 15,232 KiB, top 0x00FE0000 | 0–16 MiB | 3,104 (0) | the CMOS rule (R16B) would also pool 0xFE0000–0xFFFFFF, which SeaBIOS reports **reserved** (ACPI tables); R17 excludes it |
| 32 | 31,616 KiB | 0–32 MiB | 7,200 (4,064) | |
| 64 | 64,384 KiB | 0–64 MiB | 15,392 (12,256) | |
| 128 | 129,920 KiB | 0–128 MiB | 31,776 | |
| 256 | 260,992 KiB, top 0x0FFE0000 | 0–256 MiB | 64,544 (61,408) | |
| 512 | 261,120 KiB, top 0x10000000 | 0–256 MiB | clamped | `262016 KiB above it not used` |

The pool at `-m 16` is smaller than R16B's 3,264 frames for two reasons: the reserved ACPI frames are excluded, and
512 KiB is moved to the kernel area.

### CPL3 RAM checks (`/BIN/RAMCHK`)

RAMCHK allocates N MiB from the heap at CPL3, writes an address-dependent pattern to every word, verifies it (optionally
several passes), and frees it.

| Run | Result |
| --- | --- |
| `-m 256`, `RAMCHK 64` ×3 | PASS. Frames up to physical 0x0FF95000 (all above 16 MiB). |
| `RAMCHK 200` (more than the 64 MiB heap cap or the free pool) | Allocation is refused cleanly (`refusals` +1); the program gets what it can and passes. Afterwards `MEMINFO free` returns to its baseline (64,477) and `check=OK`. |
| `-m 64`, two concurrent RAMCHKs (40 MiB, then 18 of a further 40 MiB) | Both PASS; the second stops at the pool end. |
| `-m 64`, `RAMCHK 200` | Gets 58 MiB, then refused; the pool recovers to baseline. |
| Spawn/exit bursts (`R16T STRESS 20`) | All ok. LEAK identical before and after. |

### Reserved-region injection (test kernel)

Build: `make qdos386-r17-pc R17D=qdos386-r17-res R17_EXTRA=-DQDOS_R17_TEST_RESERVE`.

This test build adds a type-2 record at 32 MiB…33 MiB over usable RAM, then poisons that region at boot. Result
(logs/a13/p1/qemu-res64.summary):

* `reserved=1152 KiB`
* `frames_in_pool=0`
* `poison_mismatch=0 → PASS` at boot and again after the pool had been exhausted twice by RAMCHK

So no frame in the injected reserved region was ever handed out or overwritten.
