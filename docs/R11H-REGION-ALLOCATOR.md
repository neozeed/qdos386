# R11H application-region allocator

Source: `kernel/region.h`, `kernel/region.c`. Pure C89 with no kernel dependencies, so the host tests
compile it directly (`tests/host/r11h/run.sh`, 5,052 checks, 0 failures).

## 1. What it manages

A pool holds a platform memory map as a short list of **spans**. Each span is either:

- `REGION_SPAN_FREE`: may hold application regions;
- `REGION_SPAN_RESERVED`: never allocated (kernel, heap, transfer area, boot stack, device page, transport).

The pool knows nothing about what a span is for. The loader builds its pool from the four profiles
(`kernel/aout.c`, `app_pool_init`): FREE spans are the profile ranges, and nothing else is in the pool.
A RAM-backed transport is therefore not in the pool and cannot be claimed through it.

A **region** is a page-aligned byte range `[base, end)` inside one FREE span. Pages are 4096 bytes.

## 2. Operations

| Function | Behaviour |
|---|---|
| `region_init(p, spans, n)` | Validates spans: page-aligned, non-empty, sorted, non-overlapping, known kind. Clears all regions. |
| `region_alloc(p, bytes, owner)` | First-fit within FREE spans, rounded up to pages. Returns a positive handle or a negative code. |
| `region_reserve_at(p, base, bytes, owner)` | Fixed address. The whole range must lie in one FREE span and overlap no live region. Used by the loader for a profile origin. |
| `region_release(p, handle, owner)` | Frees the region. The owner must match. |
| `region_range(p, handle, &base, &end)` | Address range of a live handle. |
| `region_check(p)` | Invariants: every live region is inside one FREE span; live regions are pairwise disjoint. |
| `region_live(p, &bytes)` | Live region count and total bytes. |

Error codes: `REGION_ERR_ARG` (-1), `REGION_ERR_NOMEM` (-2), `REGION_ERR_BUSY` (-3),
`REGION_ERR_HANDLE` (-4), `REGION_ERR_OWNER` (-5), `REGION_ERR_FULL` (-6).

## 3. Handles

A handle is a positive `long`: the slot index plus one in the low 8 bits, and a generation number above.
The generation is issued by the pool on each allocation, so a handle to a released and reused slot is
rejected with `REGION_ERR_HANDLE`. A zero or negative handle is always rejected. `REGION_MAX` is 8 slots,
`REGION_MAX_SPANS` is 16.

## 4. Limits

- The pool holds at most 8 live regions. The R11H loader uses at most 4 (one per profile).
- Allocation is first-fit and never moves a live region.
- The allocator does not coalesce; with page-aligned regions and fixed profiles this does not matter in
  R11H, because the loader only reserves fixed profile ranges.
- Region exhaustion (`REGION_ERR_NOMEM`, `REGION_ERR_FULL`) is tested on the host. The guest cannot reach
  it: the loader reserves fixed profile ranges, and each profile can hold one region at a time.

## 5. What the host tests cover

`tests/host/r11h/region_test.c` (5,052 checks): init validation (bad spans, unsorted, overlapping,
unaligned, empty); first-fit placement and page rounding; fixed-address reservation at span edges and
across spans; overlap refusal; handle staleness after release and reuse; owner checks; slot exhaustion
(`REGION_ERR_FULL`); fragmentation and reuse of holes; invariants after every operation; and a seeded
pseudo-random churn of 10,000 allocate and release steps on the sample i386VM map, with the
invariants checked along the way.

## 6. Not in the allocator

- Coalescing or compaction. Not needed for fixed profiles.
- Platform discovery of memory. The map is supplied by the caller.
- Protection. The allocator tracks ownership of ranges, not access rights.
