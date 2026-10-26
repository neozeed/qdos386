# QDOS/386 R4 - Channel Manager Design Note (not implemented)

Status: design only. No R4 code exists in this archive. The R3 kernel is
frozen. This note fixes the first R4 scope so that the firmware and memory
questions can close before any kernel functionality is added.

## 1. Scope of the first R4 objective

A native job opens a console channel through the channel API, writes
characters and a string through it, closes it, and the job exits cleanly.
Nothing else: no files, no directories, no keyboard input, no blocking I/O,
no SuperBASIC, no multiple devices.

Test checks for this objective:

- open returns a non-zero channel id (tag and index as in §3.3);
- write of a string returns its length; write of a single character works;
- close returns 0;
- a second close, or a write after close, is rejected with the invalid-id code;
- a channel left open at job exit is closed by the exit path, and its slot
  is free afterwards.

## 2. What the JS ROM does (source evidence)

Source: `scratchpad/x/qlbox/rom/js-rom-gas-r2/src/system_asm.s`.

Trap #2 (channel open/close/format/delete), `l032a2` at line 4747:

- `d0 = 1` open (`l032d0`), `2` close (`l03352`), `3` format (`l036b6`),
  `4` delete (`l03552`); any other value gives `-15` (bad parameter).
- Open: scans the channel table between `SV.CHBAS` and `SV.CHTOP` for a free
  entry (first byte negative means free). No free entry gives `-6`. It then
  walks the driver list `SV.DRLIST` and calls each driver's open entry
  (driver +8, `movea.l 0x0008(a2),a4`). A driver that does not know the name
  returns `-7` (`0xfff9`, the word compared at `l032f4`), and the walk continues. Other errors stop it.
- On success the channel block is filled: `+4` driver pointer, `+8` job id,
  `+0x0C` table slot address, `+0x10` tag word from `SV.CHTAG`, `+0x12` status
  (`CH.STAT`), `+0x14` job-waiting id (`CH.JOBW`).
- The returned channel id is `(tag << 16) | index`, built by `swap` and
  `move.w` in `l0331e`. The tag is incremented on each open. This is the same
  shape as R3 job ids (`(tag << 16) | slot`).
- Close: looks up the channel block (`l03476`), calls the driver's close entry
  (driver +0x0C via `0x000c(a4)`), then marks the table slot free with `st`.

Trap #3 (channel I/O), `l0337c` at line 4836:

- Looks up the channel block, then test-and-sets `CH.STAT` (`+0x12`). If the
  channel is busy, the job is put on hold (`JOB.HOLD`) with a timeout, and the
  trap is retried later (`l03414`). This is the blocking path. R4 does not
  need it, because the console is synchronous.
- Otherwise it stores the operation code in `CH.ACTN` (`+0x13`) and calls the
  driver's I/O entry (driver +4, `0x0004(a4)`) with `a3` set 0x18 bytes
  before the driver block. The driver returns `-1` for "not complete" when
  the call would block. Other results return directly.
- The operation codes that the trap dispatches to the driver are the ROM's
  own table. The screen-related trap #3 codes (`tr3disp`, line 2201) are
  window and screen operations and are out of R4 scope.

Not traced in this pass: the exact operation codes for write, and the
per-driver table layout beyond the offsets above. Both are needed before R4
can name its codes precisely. They are listed in §8.

## 3. Three distinct concepts

The request asks that three things stay separate. R4 keeps them separate in
code.

### 3.1 Internal channel object (kernel-private)

A kernel struct allocated from the heap, owned by the kernel
(`MEM_OWNER_KERNEL`) or by the opening job for bookkeeping. It is not
reachable from job code, which only ever sees the id. It holds:

- the owning job id (`owner`, the tagged job id from R3);
- a pointer to the backend driver object (§3.2);
- a state field (free, open, closing);
- the slot index and tag, for validation.

### 3.2 Platform backend (per platform)

The backend performs the actual output. It is selected at build time, as the
R3 platform code is:

- PC: the Tiny386 debug console, port `0x402`, byte writes.
- i386VM: the console MMIO byte at `0x800001`.

The backend interface is three functions, matching the JS ROM driver shape
(open, close, io), but as C function pointers, not 68000 entries:

```
struct chan_driver {
    int  (*open)(struct chan_object *ch, const char *name);
    void (*close)(struct chan_object *ch);
    int  (*write)(struct chan_object *ch, const unsigned char *buf, unsigned n);
};
```

The console backend is the only one in R4. Nothing outside the backend knows
about ports or MMIO addresses.

### 3.3 QDOS channel id (job-visible)

A 32-bit id `(tag << 16) | index`:

- `index` selects the slot in the kernel channel table (fixed size, see §5);
- `tag` is incremented on each open (wraps before `0x8000`, skipping `0`), so
  an id from a closed channel cannot match a reused slot.

Validation on every call: index in range, slot in use, tag equal, and the
caller's job id equal to the channel owner. Any failure returns the invalid-id
code without touching the channel.

## 4. Native ABI (proposed)

R4 uses the R3 native convention (`int $0x30`, EAX = function, EBX/ECX/EDX =
arguments, EAX = result; see `R3-MEMORY-ABI.md` §1). It is not the 68000 trap
ABI and not QDOS-compatible.

| Code | Name | Inputs | Output |
|---|---|---|---|
| `0x14` | `QT_CH_OPEN` | EBX = pointer to NUL-terminated name (`"CON"` in R4) | channel id, or negative error |
| `0x15` | `QT_CH_CLOSE` | EBX = channel id | 0 or negative error |
| `0x16` | `QT_CH_WRITE` | EBX = channel id, ECX = buffer pointer, EDX = length | bytes written, or negative error |

Codes `0x17`–`0x1F` are reserved for R4 channel services. `0x20`–`0xFF`
remain unassigned, consistent with `R3-MEMORY-ABI.md` §2.

Single-character output is `QT_CH_WRITE` with length 1. No separate call.

Error codes (new, R4 range, negative):

| Code | Meaning |
|---|---|
| `-6` | no free channel slot (same as the JS ROM's table-full code) |
| `-7` | name not known to any driver (same value as the JS ROM's not-found code) |
| `-12` | invalid channel id (index, slot free, or tag mismatch) |
| `-13` | channel not owned by the caller |
| `-14` | buffer or length rejected (zero length is a valid no-op; length over the cap is rejected) |
| `-15` | bad parameter (same code as the JS ROM) |

## 5. Storage and limits

- Channel table: fixed at 8 entries, a static array in the kernel. It is not
  on the heap, so it cannot be starved by the heap tests.
- Channel objects: allocated from the heap with owner `MEM_OWNER_KERNEL`.
  The table slot holds the pointer, so the object is freed by the close path,
  not by job reclaim. This avoids a double free and keeps reclaim unchanged.
- Write length cap: 256 bytes per call. Longer writes are the caller's job to
  split.

## 6. Job exit and reclaim

R3 reclaim frees a job's heap blocks by owner. R4 adds one step ahead of it:
before `mem_reclaim_owner`, job exit calls `chan_close_owner(job id)`, which
closes every channel open for that job and frees its objects. The order is
fixed: channels first, then heap blocks. The reclaim path stays as it is in
R3.

This is the only change to the job exit path. It runs in the same context as
R3 reclaim (trap or cooperative-yield context of another job), so it is not
in the timer ISR.

## 7. Interrupt context

- Channel traps run with IF clear, as all R3 traps do (`int $0x30`, interrupt
  gate). No channel operation is preempted.
- The console write is synchronous. It does not wait on an interrupt.
- No channel call is allowed from the timer ISR.

## 8. Open items before R4 code

1. Trace the JS ROM trap #3 operation codes for write (and the driver table
   entry offsets) in `system_asm.s` from line 4836, and record them in
   `docs/JS-ROM-CHANNEL-MAP.md`. R4 does not depend on the exact values, only
   on the native codes in §4, but the note should cite the source.
2. Decide whether `QT_CH_OPEN` takes a name in the job's memory with a bounds
   check only (best effort, as R3 pointer checks are), or a fixed selector.
   The first objective can use a fixed selector to avoid the question.
3. Confirm that the exit path can call `chan_close_owner` before reclaim in
   every case, including a job that exits while blocked. The first objective
   has no blocking I/O, so this is a later item.

## 9. Relation to the R3 freeze

R4 starts only after the R3 validation package is accepted. Nothing in this
note changes R3 code, R3 ABI or R3 tests. The R3 open item about the `0x2F`
EOI (`R3-FW-VALIDATION.md` §5.3) is not part of R4, and it should be settled
in its own small revision.
