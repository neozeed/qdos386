# Shared-state and blocking contracts

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

The current kernel uses one recursive, owner-checked BKL (`bkl.c`, bkl.h). Entry code runs with interrupts disabled; shared kernel execution is serialized before touching jobs, filesystem/session state or process ownership. Current acquisition uses compare-and-swap so a losing CPU cannot overwrite the owner; release clears through xchg. Per-CPU depth and counters retain eight entries. Disabling local interrupts alone is not an SMP lock.

| State/operation | Preconditions and ownership | Blocking/postcondition |
|---|---|---|
| Job allocation/status/scheduling/reclaim | BKL, entry IF clear; full tagged IDs validated | Deferred reclaim runs in another job's trap/yield context, not the timer ISR or victim's active stack |
| General heap | Boot serialized or kernel caller under BKL; allocator also masks/restores local IF | No moving blocks; correct owner required; freed blocks coalesce; no remote-CPU exclusion supplied by IF alone |
| Frame pool/VAS | Existing serialized process/template context on kernel stack | Each owned frame returned once on teardown; template/supervisor pages not reclaimed as process pages |
| Cache/hash/clock/queue/pins/reservations | BKL for transitions; backing reserved before AP/process startup | Never recycle pinned/loading/reserved entries; generation tags prevent stale completion/ownership reuse |
| Worker PIO body | Holds storage claim; BKL released and interrupts off in that body | Clears claim before its DONE trap reacquires BKL; never retain/use a process buffer |
| Sync disk path | Existing kernel/BKL context | May wait on storage claim; worker can clear claim without BKL, avoiding that lock cycle |
| Display/session/palette/font references | Kernel calls/IRQ/poll paths serialize state; presenter owns hardware copies | Poll frees released objects only when no presentation copy/reference remains |
| Input/overlay/focus | IRQ/trap/System context with existing serialization | Wake makes a job runnable; it does not schedule in the IRQ callback itself |

Trap sleep/restart is part of the calling convention. Do not hold an extra lock or process-owned temporary across a scheduler transition unless its ownership/lifetime is explicitly supported. Never call a frame-table helper under a private CR3 that cannot identity-address the relevant pool memory; proc.c's template switch is mandatory.

No new lock hierarchy or device IRQ/DMA path is introduced by R3. Known hazards include fixed table/slot mismatches, changing cache pool membership after CR3 construction, and treating old comments about xchg or one CPU as the current implementation. The source functions and their selected current branches are authoritative; historical comments are not new guarantees.
