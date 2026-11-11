/*
 * QDOS/386 R17 Apollo 13 - firmware memory map (docs/APOLLO13-MEMORY-MAP.md).  -DQDOS_R17 kernels only.
 *
 * The QBOOT3 boot sector stores the BIOS INT 15h E820 records at 0x504 (24 bytes each) and the end pointer at 0x500.
 * memmap_capture() copies and validates them at the very start of the kernel; memmap_build() turns them into a
 * page bitmap of RAM that is usable for the frame pool: type 1 only, page-aligned inwards, every page touched by a
 * non-usable record removed (reserved wins over usable), nothing below 1 MiB, nothing at or above MM_CEILING.
 * Without a trustworthy map (no QBOOT3 tag, bad end pointer, no usable record above 1 MiB, overflowing records only)
 * memmap_usable() reports "no map" and the kernel keeps the R16B rule (CMOS, at most 16 MiB).
 */
#ifndef QDOS386_MEMMAP_H
#define QDOS386_MEMMAP_H

#define MM_CEILING    0x10000000u     /* 256 MiB: identity spans 0..63; the user heap window starts here */
#define MM_MAX        32
#define MM_SRC_NONE   0               /* no boot map: CMOS fallback */
#define MM_SRC_E820   1

struct mm_rec {                       /* one raw E820 record as the BIOS returned it */
	unsigned int base_lo, base_hi, len_lo, len_hi, type;
};

void memmap_capture(void);            /* first thing in the kernel: copy 0x500.. before anything can reuse it */
int memmap_build(void);               /* after capture: validate and build the usable bitmap; returns MM_SRC_* */
int memmap_source(void);
/* 1 if every byte of the page at pa is usable RAM per the map (pa < MM_CEILING) */
int memmap_page_usable(unsigned int pa);
unsigned int memmap_top(void);        /* end of the highest usable page below MM_CEILING (0 without a map) */
/* summary for the boot log and QT_SMP SMP_OP_MEMINFO */
struct mm_summary {
	unsigned int nraw, nused_recs, nreserved_recs, nignored;   /* records: total, type 1, other types, unusable (zero length, above 4 GiB, overflow, bad) */
	unsigned int usable_kib, reserved_kib;                     /* below MM_CEILING, after reserved-wins */
	unsigned int above_ceiling_kib;                            /* usable RAM the 32-bit identity map cannot reach */
	unsigned int top;
};
void memmap_summary(struct mm_summary *s);
const struct mm_rec *memmap_raw(int *n);
void memmap_print(void);
#ifdef QDOS_R17_TEST_RESERVE
#define MM_TEST_BASE  0x02000000u     /* 32 MiB .. 33 MiB: an injected "reserved" record over usable RAM */
#define MM_TEST_LEN   0x00100000u
void memmap_poison_write(void);
unsigned int memmap_poison_check(void);
#endif

#endif
