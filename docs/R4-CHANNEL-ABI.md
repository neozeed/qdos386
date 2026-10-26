# QDOS/386 R4 - Channel Service ABI (native i386)

This is the native register convention for channel services. It is **not**
the 68000 TRAP #2 / TRAP #3 ABI of the JS ROM (see `docs/JS-ROM-CHANNEL-MAP.md`),
and it is not binary compatible with QDOS. The QDOS concepts that carry over
are the channel id as a tagged number, channel ownership by the opening job,
and channel-not-open as the basis for stale-id rejection.

## 1. Calling convention

Same as R3 (`docs/R3-MEMORY-ABI.md` §1):

- Instruction `int $0x30` (interrupt gate, IF clear during the service).
- EAX = function code. EBX, ECX, EDX = arguments. EAX = result.
- Other registers are preserved.
- C helper `int qtrap(unsigned fn, unsigned a, unsigned b, unsigned c)` places
  `a`, `b`, `c` in EBX, ECX, EDX.

## 2. Function codes

| Code | Name | Class | Status |
|---|---|---|---|
| `0x08`–`0x0F` | R2 job and scheduler traps | unchanged | — |
| `0x10`–`0x13` | R3 memory traps | unchanged | — |
| `0x14` | `QT_CH_OPEN` | native | implemented |
| `0x15` | `QT_CH_CLOSE` | native | implemented |
| `0x16` | `QT_CH_WRITE` | native | implemented |
| `0x17` | `QT_CH_PUTC` | native | implemented |
| `0x18`–`0xFF` | — | — | not assigned |

`0x17` is outside the `0x14`–`0x16` range proposed in the R4 design note. It is
added because the R4 requirements list "write one character" as a service in
its own right. Writing one byte through `QT_CH_WRITE` with length 1 is also
possible and gives the same result.

### 2.1 `QT_CH_OPEN` (0x14): open a channel

| | |
|---|---|
| Inputs | EBX = pointer to a NUL-terminated driver name, at most 7 characters |
| Output | EAX = channel id `(tag << 16) \| index` (positive), or a negative error |
| Errors | `-3` no memory for the channel object; `-6` no free channel slot; `-7` no driver with that name; `-14` name not in range or not terminated within 8 bytes |
| Owner | the calling job. The id is valid only for that job. |

The name `"CON"` is the only driver in R4. Name matching is exact.

### 2.2 `QT_CH_CLOSE` (0x15): close a channel

| | |
|---|---|
| Inputs | EBX = channel id |
| Output | EAX = 0, or a negative error |
| Errors | `-12` invalid, closed or stale id; `-13` owned by another job |

After a successful close the channel object is freed and the slot is free.
The slot's tag stays as it was, so a later open of the same slot gets a new tag.

### 2.3 `QT_CH_WRITE` (0x16): write a bounded string

| | |
|---|---|
| Inputs | EBX = channel id, ECX = buffer pointer, EDX = length in bytes (0 to 256) |
| Output | EAX = number of bytes written (equal to the length), or a negative error |
| Errors | `-12` invalid, closed or stale id; `-13` owned by another job; `-14` length above 256, or buffer not inside the flat range |

Length 0 returns 0 and does nothing. The output is raw: each byte goes to the
platform console, and no byte is interpreted (LF is not translated to CR-LF
and there is no cursor handling; see §5).

### 2.4 `QT_CH_PUTC` (0x17): write one character

| | |
|---|---|
| Inputs | EBX = channel id, ECX = byte in the low 8 bits (higher bits ignored) |
| Output | EAX = 1, or a negative error |
| Errors | `-12` invalid, closed or stale id; `-13` owned by another job |

## 3. Channel ids

- `index` is the slot number, 0 to 7.
- `tag` is a 15-bit number, incremented each time the slot is opened. Tags start
  at 1 and wrap to 1 before `0x7FFF`, so a tag is never 0 and an id stays
  positive.
- The lowest free slot is used for each open.
- Validation on every call, in order: index in range; slot in use; tag equal;
  owner equal. A failed check changes nothing and increments the rejection
  counter.

## 4. Ownership and cleanup

1. A channel belongs to the job that opened it. Only that job can use its id.
2. When a job exits, its open channels are closed by the kernel
   (`chan_close_owner`), before its heap blocks are reclaimed. Each channel
   object is freed, and the slot is free again.
3. The channel object lives in the kernel heap, owned by the opening job's
   tagged id. Cleanup frees it through the same owner, so reclaim finds nothing
   left for that channel.
4. A reused job slot has a new job tag, so a channel id from an earlier job can
   never match a channel opened by a later job.

## 5. Limits

- Eight channel slots, fixed at compile time (`CHAN_MAX`).
- One driver, `CON`. No file, directory, keyboard or serial driver.
- No blocking I/O. Every service completes at once. The 68000 "not complete"
  result (`-1`) does not occur.
- Writes are raw bytes. Cursor movement, scrolling and LF handling of the JS ROM
  screen driver are not reproduced.
- User buffers and names are checked for being inside the flat arena or the
  kernel image below it (`0..0x600000`). This is a range check, not a
  protection check. It does not stop a job from naming another job's memory
  inside that range. The R3 pointer policy applies (`R3-MEMORY-DESIGN.md` §3.1).
- Tags wrap after 32 767 opens of one slot. An id from exactly that many opens
  earlier could then match again. This is the only way a stale id can match, and
  it is not tested.
- Each write is limited to 256 bytes. Longer output is split by the caller.

## 6. Error codes

| Code | Meaning |
|---|---|
| `-3` | no memory (`MEM_ERR_NOMEM`, same value as the JS ROM) |
| `-6` | no free channel slot |
| `-7` | no driver with that name (the JS ROM uses the same value for "name not known") |
| `-12` | invalid, closed or stale channel id |
| `-13` | channel owned by another job |
| `-14` | buffer or name not in range, or length above 256 |

Codes `-1`, `-2`, `-4`, `-5` and `-8` to `-10` are used by R2 and R3 services with
the meanings in their own ABI documents.

## 7. Interrupt context

- All channel services are reached through `int $0x30`, so IF is clear for the
  whole service. A channel service cannot be preempted by the timer.
- The console write is synchronous. It does not wait for an interrupt.
- Channel services must not be called from the timer ISR. They are not.
