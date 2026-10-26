# QDOS/386 R3 - Memory Service ABI (native i386)

This is the native register convention for memory services. It is **not**
the 68000 trap ABI of the JS ROM, and it is not binary compatible with QDOS.
Each function is classed as one of:

- **native**: a QDOS/386 extension with no JS ROM counterpart;
- **approximated**: close to a JS ROM service, with the differences listed.

## 1. Calling convention

- Instruction: `int $0x30` (vector `TRAP_VECTOR`, an interrupt gate, so IF is
  clear during the service).
- `EAX` = function code.
- `EBX`, `ECX`, `EDX` = arguments. Unused arguments are ignored.
- `EAX` = result on return. Other registers are preserved, as with any
  interrupt frame restored by `popal`/`iret`.
- Pointers are 32-bit flat addresses.
- Errors are negative values. A successful pointer result is always positive
  and below `0x80000000`, so the two cannot be confused.

C helper: `int qtrap(unsigned fn, unsigned a, unsigned b, unsigned c)`
(`kernel/context.s`) places the arguments in EAX, EBX, ECX, EDX and returns
EAX.

Pointers returned or passed are never checked by the trap layer beyond what
`kernel/memory.c` checks (`R3-MEMORY-DESIGN.md` §3.1). Owner is always the
calling job, so a job cannot act on another job's blocks.

## 2. Function codes

| Code | Name | Class | Status |
|---|---|---|---|
| `0x08`–`0x0F` | R2 job and scheduler traps | unchanged from R2 | — |
| `0x10` | `QT_MEM_ALLOC` | approximated (`MM.ALCHP`) | implemented |
| `0x11` | `QT_MEM_FREE` | approximated (`MT.RECHP`) | implemented |
| `0x12` | `QT_MEM_FREE_BYTES` | native | implemented |
| `0x13` | `QT_MEM_VALIDATE` | native | implemented |
| `0x14`–`0xFF` | — | — | not assigned |

Codes `0x10`–`0x13` do not overlap R2's `0x08`–`0x0F`.

### 2.1 `QT_MEM_ALLOC` (0x10): allocate

| | |
|---|---|
| Inputs | EBX = size in bytes |
| Output | EAX = pointer to a 16-byte-aligned, zero-filled block owned by the caller, or a negative error |
| Errors | `MEM_ERR_ZERO` (-4) size 0; `MEM_ERR_OVERFLOW` (-5) size > `0xFFFFFFE0`; `MEM_ERR_NOMEM` (-3) no free block large enough |
| Owner | the calling job. The block is freed when the job exits. |
| Size | rounded up to 16 bytes; the block actually reserved is `round16(size) + 16` bytes including the header, and the user region is at least `size` bytes |
| Preemption | the trap runs with IF clear; the allocation is atomic |

Deviations from `MM.ALCHP` (approximated):

- Rounding is 16 bytes, as in `MM.ALCHP`. The JS ROM's transient-area growth
  in 512-byte steps does not exist here.
- Zero-filling is the same as `MM.ALCHP`.
- The JS ROM returns `A0` and `D1` (the rounded size). R3 returns only the
  pointer. The size is not returned. A caller that needs it must record it.
- The JS ROM's out-of-memory path can move BASIC data. R3 cannot, and never
  does.
- The JS ROM allocates from a common heap that is also used by channels. R3
  has no channel ownership. Every allocation is job- or kernel-owned.

### 2.2 `QT_MEM_FREE` (0x11): release

| | |
|---|---|
| Inputs | EBX = pointer returned by `QT_MEM_ALLOC` |
| Output | EAX = 0, or a negative error |
| Errors | `MEM_ERR_BADPTR` (-6) not a block start in the arena, or not in use; `MEM_ERR_DOUBLE` (-7) already free; `MEM_ERR_OWNER` (-8) owned by another job; `MEM_ERR_CORRUPT` (-9) header invariant broken |
| Owner | must equal the calling job |

Deviations from `MT.RECHP` (approximated):

- The JS ROM takes the block size from the first longword at `A0`. R3 takes it
  from the native header, not from the caller.
- The JS ROM returns `D0 = 0` and does not check ownership. R3 checks both
  the pointer and the owner.
- The JS ROM may return the top of the heap to the transient gap. R3 does not.

### 2.3 `QT_MEM_FREE_BYTES` (0x12): free bytes

| | |
|---|---|
| Inputs | none |
| Output | EAX = free bytes in the arena (headers of free blocks included), or `MEM_ERR_CORRUPT` |

Native. Used by the guest tests to check that reclaim restores the baseline.

### 2.4 `QT_MEM_VALIDATE` (0x13): check the heap

| | |
|---|---|
| Inputs | none |
| Output | EAX = 0 if invariants I1–I4 hold, else `MEM_ERR_CORRUPT` |

Native. Walks the whole arena, so it is for test and diagnostic use only.

## 3. Changed R2 behaviour

- `QT_CREATE` (0x0D) can now also return `MEM_ERR_NOMEM` (-3), when the stack
  cannot be allocated. Previously only `-1` (no slot) was possible. `-1`
  still means no free slot. Callers that tested `< 0` are unaffected. Callers
  that tested `== -1` must also handle `-3`.
- Job stacks are heap-backed. The job id is the heap owner.
- Dead-job reclaim no longer runs in the timer ISR. It runs in trap and
  cooperative-yield context of another job.

## 4. Kernel-internal API

Used by the kernel and by boot-time tests, not by jobs. Declared in
`kernel/memory.h`:

```
int   mem_init(unsigned start, unsigned end);
void *mem_alloc(unsigned size, unsigned owner);   /* pointer or negative code */
int   mem_free(void *ptr, unsigned owner);
unsigned mem_reclaim_owner(unsigned owner);
int   mem_stats(struct mem_stats *out);
int   mem_validate(void);
unsigned mem_owner_of(void *ptr, int *ok);
```

These are **not** trap services. The trap services above are the only memory
interface available to job code. The kernel-internal API may be called only in
trap or boot context, with IF clear (`R3-MEMORY-DESIGN.md` §5).

## 5. Ownership rules, summarised

1. A block belongs to exactly one owner, fixed at allocation.
2. A job can free only blocks it owns.
3. Exit frees every block the job owns, including its stack.
4. A reused slot gets a new tag. Blocks of the old tag can never be freed by
   the new job.
5. Kernel-owned blocks are never reclaimed by job exit.
6. Freeing a block twice reports `MEM_ERR_DOUBLE`. Freeing a foreign or
   interior pointer reports `MEM_ERR_BADPTR`.

## 6. Unresolved and unsupported

- `l030e0` (the JS ROM largest-free query, trap 1) is unresolved. It has no
  R3 counterpart, and no code in R3 depends on it.
- The JS ROM's trap-1 and trap-2 dispatch tables, the channel services, and
  BASIC area services are not present in R3 and are not reserved here.
- Function codes `0x14`–`0xFF` are unassigned. Future services must keep the
  R2 and R3 numbers stable.
