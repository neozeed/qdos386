# Memory and lifetime contracts

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

Current owners are memory.c (small general kernel heap), frames.c (physical pool), vas.c (page tables), pheap.c (per-process heap), proc.c (address spaces/stack/fault lifecycle) and r20w.c (cache contents/lifetimes). They are separate resource domains.

The general heap starts at aligned bss_end and ends at the existing fixed 0x200000 ceiling. Its capacity depends on the linked kernel's text/data/BSS, not guest RAM. Allocation has 16-byte headers/alignment, owner tags, coalescing and deferred owner reclaim. Kernel-entry stacks and ordinary small kernel objects use it. A 16384-byte entry stack needs a 16400-byte block; do not infer capacity from free physical RAM.

Frames are 4096 bytes below the 256 MiB identity ceiling. Membership and used bits are distinct; ordinary allocation returns only free members. A VAS owns its directory/private tables and data PTEs marked VAS_OWN. Teardown returns exactly those pages. Template access requires the template CR3 and a kernel stack. Current processes have private image/argument pages, heap window starting at 0x10000000, stack ending at 0x20000000 and a guard; virtual heap maximum is 64 MiB, not precommitted RAM.

## Cache backing

At serialized boot after paging and before process/AP creation, proc_cache_reserve removes one contiguous free run above 8 MiB from pool membership. Those nonpool pages retain supervisor identity mappings in every subsequently built CR3. They are not user-accessible or process-owned. All four arrays (entries, sectors, buckets, queue) share this arena and consume **zero general-heap allocation**.

Logical policy is min(5% firmware-usable RAM, 2 MiB), including metadata; reserved page rounding is accounted separately and total backing never exceeds 512 pages/2 MiB. Current typical reservations are 191 pages at 16 MiB, 395 at 32 MiB and 512 at 64/256 MiB. Failure halves entries, then falls back synchronously below the minimum. Initialization rollback releases the entire reservation before publishing it. Membership is immutable once processes exist; no runtime cache reclaim is supported.

Moving/removing diagnostic code changes bss_end and therefore heap headroom; that is an intentional layout effect, not a larger numeric heap limit. R3 preserves every allocator bound/ABI. Actual block counters, peak/low-water, frame bitmaps and page mappings are verified by symbol-based stopped-guest probes. Diagnostic symbols are not a new user syscall. A largest-block check is separate from total free/low-water accounting.

The original frozen R20 failure was heap starvation from 523280 bytes of cache arrays despite abundant frames. The accepted page-backed design remains intact. Frame exhaustion now remains a genuine -60 path, distinct from heap -3/launch -30. Kernel faults, arbitrary firmware holes and generation wrap are not exhaustively qualified. See STORAGE, ABI and REMAINING-DEBT.
