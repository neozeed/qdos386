# QDOS/386 R3 - Memory Design

Scope: the native kernel heap, heap-backed job stacks, job-owned allocations,
and the interrupt rules that protect them. The trap interface is specified
separately in `docs/R3-MEMORY-ABI.md`. Historical JS ROM behaviour and its
classification are in `docs/JS-ROM-MEMORY-MAP.md`.

## 1. Arena

| Item | Value | Source |
|---|---|---|
| Start | `bss_end` rounded up to 16 bytes | linker symbol `_bss_end` (`platform/pc/kernel.ld`; i386VM driver) |
| End | `MEM_ARENA_END = 0x600000` | `kernel/memory.h` |
| PC, measured | `0x1064E0` .. `0x600000` (5,217,056 bytes) | `logs/pc-run*.txt`, line "Heap: arena" |
| i386VM, measured | `0x006430` .. `0x600000` (6,265,808 bytes) | `logs/i386vm-run*.log` |

Why `0x600000`. Both targets have 8 MiB of RAM. The boot stack starts at
`0x7FFFF0` and grows down, and SeaBIOS keeps its data near the top of RAM. The
top 2 MiB is reserved, so the arena never reaches it. This is a conservative
bound. It is not a measured SeaBIOS memory map. The guest results show no
overlap with the boot stack or the canaries, but SeaBIOS's own use of
`0x600000..0x7FFFFF` is not verified.

On PC the kernel image (`0x100000`) and the staging area (`0x10000`) are below
the arena, and the arena starts after BSS.

## 2. Blocks

Every block is `16-byte header + payload`. The payload is 16-byte aligned.

```
+0   size    total block bytes, header included (multiple of 16, >= 32)
+4   magic   MEM_USED 0x55534421 or MEM_FREE 0x46524545
+8   owner   allocation owner (used blocks)
+12  next    next free block by address (free blocks; 0 ends the list)
```

- Requests round up to 16 bytes (`need = round16(size) + 16`).
- The free list is sorted by address. It is singly linked with absolute
  addresses. The arena is flat and never moves, so absolute links are safe.
- A free block is split only if the remainder is at least `MEM_MIN_BLOCK`
  (32 bytes). Otherwise the whole block is handed out, and its recorded size
  includes the slack.
- Blocks tile the arena exactly, in address order. Free and used blocks
  alternate freely, but two free blocks are never adjacent.

## 3. Operations

| Operation | Behaviour |
|---|---|
| `mem_alloc(size, owner)` | Reject 0 (`MEM_ERR_ZERO`) and sizes above `0xFFFFFFE0` (`MEM_ERR_OVERFLOW`). First fit in address order. Split when the remainder is at least 32 bytes. Zero-fill the payload. No free block large enough: `MEM_ERR_NOMEM`. |
| `mem_free(ptr, owner)` | Check pointer range, 16-byte alignment, `magic == MEM_USED`, size sanity, and owner. Insert in the sorted list and merge with both neighbours. |
| `mem_reclaim_owner(owner)` | Free every used block with this owner. Rescans from the arena start after each free, since merging changes the layout. Terminates because the arena has a bounded number of blocks. Never frees kernel-owned blocks. |
| `mem_stats(out)` | Walk the arena and fill the counters. Returns `MEM_ERR_CORRUPT` if the walk fails. |
| `mem_validate()` | Full invariant check (below). |

Error codes are in `kernel/memory.h`. `MEM_ERR_NOMEM` is `-3`, the same
value as the JS ROM out-of-memory code.

### 3.1 Pointer checks

`mem_free` accepts a pointer only if it is inside the arena, 16-byte aligned,
and its header holds `MEM_USED` with the caller's owner.

- A header holding `MEM_FREE` is `MEM_ERR_DOUBLE`.
- Anything else is `MEM_ERR_BADPTR`. This covers misaligned, out-of-range,
  and interior pointers, since their "header" is payload data.
- A used block whose owner differs is `MEM_ERR_OWNER`.

Coalescing leaves the absorbed header's magic as `MEM_FREE`. So a double free
of a block that has since merged is still reported as `MEM_ERR_DOUBLE`, until
that memory is reused.

Limit: pointer checks are best effort. A stale pointer into memory that has
been reallocated is caught by owner mismatch or magic, not by a generation
counter. An owner that writes outside its block can corrupt headers. The
canary tests detect overruns in the guest tests. The allocator does not
prevent them.

### 3.2 Invariants (`mem_validate`)

- **I1** blocks tile the arena: sizes are multiples of 16, at least 32, and the
  walk ends exactly at `arena_end`.
- **I2** every magic is `MEM_USED` or `MEM_FREE`.
- **I3** no two physically adjacent blocks are both free.
- **I4** the free list contains exactly the free blocks, once each, in
  strictly increasing address order.

## 4. Ownership

Each allocation has an owner value:

- **Kernel**: `MEM_OWNER_KERNEL` (`0xFFFFFFFF`). The kernel API uses this.
  Kernel blocks are not reclaimed by job exit.
- **Job**: the job's tagged id `(tag << 16) | slot`. Job 0 (SYSTEM) has id 0,
  which is a valid owner. Tags start at 1, skip `0xFFFF`, and wrap to 1 before
  reaching `0x7FFF`, so ids stay positive.

The trap layer (`QT_MEM_ALLOC`) uses the calling job's id. A job cannot choose
its owner.

### 4.1 Reclaim policy (decided)

User allocations do **not** survive job exit. When a job exits, every block it
owns is freed. This includes its stack. The policy is simple and gives the
exit rule in §6.

Reclaim happens after the dead job's frame is saved and it can no longer run.
It runs only in trap or cooperative-yield context of another job (§5). A
reused slot gets a new tag, so an old id can never reach the new job's
blocks (guest tests: "stale owner cannot free a live block", "stale job id
cannot reactivate reused slot").

## 5. Interrupt rules

The rules are enforced in code, not just stated.

| Context | IF | Allowed |
|---|---|---|
| Timer ISR (`irq0_stub` → `sched_irq_tick`) | 0 (interrupt gate) | Scheduling only: poll list, schedule list, pick, switch. **No** allocation, free, reclaim, logging, or unbounded list walk. The canary check is a fixed four-word compare that increments a counter. |
| Trap (`trap_stub` → `trap_dispatch`) | 0 (interrupt gate) | All memory operations, job creation, and `reclaim_dead`. Cannot be preempted by the timer. |
| Cooperative yield (`yield_asm` → `sched_yield_sp`) | 0 (`cli` at entry) | `reclaim_dead` and scheduling. Resumed with `sti` for fmt-0 frames. |
| Boot (`kstart` → `main`) | 0 (`cli` at entry) | Heap init and heap-only tests. |

Every heap mutator also saves and restores IF with `irq_save`/`irq_restore`
(`kernel/context.s`). `irq_restore` sets IF only if it was set on entry. This
makes the mutators safe even if a future caller forgets the rule above.

### 5.1 Why `yield_asm` masks interrupts

R2 `yield_asm` ran the scheduler with IF set. A timer tick could then switch
jobs while the cooperative frame was being built. R3 masks interrupts for the
whole pass. `resume_ctx` re-enables them only for the job being resumed.

### 5.2 Why the PC kernel clears IF at entry

Tiny386 starts the kernel with IF inherited from the boot path, and SeaBIOS's
PIT is still ticking. R2 was unaffected only because it enabled the scheduler
at once. R3's heap tests run before the scheduler and leave the tick pending
across the PIC reprogram in `timer_start`. The PIC then returns spurious IRQ7
(vector `0x27`), which had no IDT gate, and the emulator aborted. Two changes
fix this. `kstart` clears IF, and vectors `0x27` and `0x2F` now point to a
handler that returns with `iret`. A spurious IRQ7 needs no EOI.

## 6. Job stacks

- Each job's stack is `JOB_STACK_BYTES = 4096` from the heap, owned by the
  job id. The job record has `stack_base`, `stack_top`, and `stack_hwm`.
- **Allocation before the slot changes.** `job_alloc` allocates the stack
  first. On failure it returns the memory error code (`MEM_ERR_NOMEM`) and
  leaves the slot, the tag, and the heap unchanged. If no slot is free it
  returns `-1`. The guest test "job creation fails cleanly under memory
  pressure" checks both.
- **Canary.** Four words of `JOB_STACK_CANARY` (`0xC0DEC0DE`) at the stack
  base. `schedule()` checks the outgoing job's canary on every switch-out and
  increments `stack_faults` on failure. The ISR does not log or halt. The
  SYSTEM job reports the count.
- **High-water mark.** Recorded at reclaim: distance from the top to the
  lowest non-zero word above the canary. Scan bounded by 4096 bytes. Run in
  job context, not in the ISR.
- **Exit.** `job_exit` masks interrupts, marks the job dead, and yields.
  `schedule()` saves the frame and then sets the slot's bit in `dead_mask`.
  So the stack is never freed while its job could still run. The job is never
  resumed.
- **Never free the current stack.** `reclaim_dead` skips `job_current`.
- **Reuse.** The next `job_alloc` for that slot gets a fresh stack and a new tag.

## 7. Exhaustion and failure behaviour

- `mem_alloc` returns `MEM_ERR_NOMEM` and changes nothing.
- `QT_CREATE` returns `-1` (no slot) or `MEM_ERR_NOMEM` (no memory). No slot,
  tag, or block is leaked on either failure.
- A failed `mem_free` changes nothing. The caller gets the error code.
- A corrupt header found during reclaim stops the walk. The bytes already
  freed are reported. The heap is not repaired. `mem_validate` reports the
  corruption.

## 8. Limits and known gaps

- Single arena, fixed at boot. No growth and no relocation.
- The heap is not compacted. Fragmentation is handled only by coalescing.
- Block contents are zeroed on allocation, so the zeroing cost is linear in
  block size. The guest tests do not measure it.
- The 8-byte allocation path of `MM.ALLOC` is not exposed (see
  `docs/JS-ROM-MEMORY-MAP.md`).
- Interrupt-context allocation is not supported.
- Pointer validation is best effort (§3.1). It is not a capability system.
- Stack size is fixed at 4 KiB. Overflow is detected by the canary at the next
  switch-out, not at the moment of the overflow, so a corrupted canary can be
  reported late.
