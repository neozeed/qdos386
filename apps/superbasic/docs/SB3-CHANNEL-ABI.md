# SB3 channel interface (opaque handles)

Status: implemented for the hosted build and tested with an injected adapter.
This document replaces the SB2 callback interface (`SbConsole`, described in
`docs/SB2-CHANNEL-ABI.md`, which is kept unchanged in the frozen SB2 archive).

The core never calls stdio, a system service, a platform trap or a POSIX call
for channel I/O. Every byte of channel I/O goes through a handle. The only
file that uses stdio for channel I/O is `src/sb_console.c`, which is host
glue.

## 1. The handle

Defined in `src/sb_chan.h`. The core sees an `SbChan *` and nothing else.

```c
typedef struct SbChanOps {
    int (*write)(void *obj, const char *s, size_t n, size_t *done);
    int (*read)(void *obj, char *buf, size_t n, size_t *got);
    int (*flush)(void *obj);      /* may be NULL */
    void (*close)(void *obj);     /* may be NULL */
} SbChanOps;

SbChan *sb_chan_new(const SbAlloc *mem, const SbChanOps *ops, void *obj,
                    unsigned flags);
void    sb_chan_free(SbChan *ch);
unsigned sb_chan_flags(const SbChan *ch);
int     sb_chan_write(SbChan *ch, const char *s, size_t n);
int     sb_chan_read(SbChan *ch, char *buf, size_t n, size_t *got);
int     sb_chan_flush(SbChan *ch);
```

Capabilities are set when the handle is made: `SB_CH_READ` (1) and
`SB_CH_WRITE` (2). An operation the handle lacks gives `SB_E_NOT_IMPLEMENTED`
(-19), as the ROM does for `INPUT #3`.

Ownership:

- The host creates a handle with `sb_chan_new` and installs it with
  `sb_set_channel(st, n, ch)` (or `sb_set_diag` for error reports).
- The core never frees a handle. `sb_destroy` does not close or free the
  handles; the host calls `sb_chan_free` after it has finished with the
  state or replaced the channel.
- `sb_chan_free` calls the adapter's `close` once, then frees the handle.
  `NULL` is a no-op.
- `ops` is kept by pointer and must outlive the handle.

## 2. Status and counts

Every function returns `SB_OK` (0) or an `SB_E_*` code from `sb_error.h`.
Counts are returned through pointers, never by a sign convention.

| Operation | Result | Code |
|---|---|---|
| `sb_chan_write` on a NULL handle | not open | -6 (`SB_E_NOT_OPEN`) |
| `sb_chan_write` without `SB_CH_WRITE` | not implemented | -19 (`SB_E_NOT_IMPLEMENTED`) |
| `sb_chan_write`, adapter reports no progress (`done == 0`) | I/O failure | -40 (`SB_E_IO`) |
| `sb_chan_write`, adapter reports more than it was given | I/O failure | -40 |
| `sb_chan_read` on a NULL handle | not open | -6 |
| `sb_chan_read` without `SB_CH_READ` | not implemented | -19 |
| `sb_chan_read` reports `*got > n` | I/O failure, `*got` set to 0 | -40 |
| `sb_chan_read`, end of input | success with `*got == 0` | 0 |
| `sb_chan_flush` on a handle without a flush operation | success | 0 |

Rules the core relies on:

- **Partial writes.** `sb_chan_write` loops until all `n` bytes are taken. An
  adapter may take fewer bytes per call; it sets `*done` to the number it
  accepted, which must be at least 1 unless it returns an error.
- **Partial reads.** `sb_chan_read` returns what the adapter provides; the core
  asks for one byte at a time when it reads a line, so a partial read is not
  an error.
- **End of input.** A read that gives 0 bytes with success is end of input.
  The core's line reader treats end of input before any byte of a line as
  "not implemented" (-19). That is the SB2 rule, kept because the ROM waits
  for the keyboard, which a finite input cannot reproduce. The ROM's behaviour
  is in `docs/KNOWN-LIMITATIONS.md` section 10.
- **Errors from the adapter** are returned as they are, so an adapter can use
  any `SB_E_*` code.
- **Error reports** go to the diagnostic handle. A failing diagnostic handle
  is ignored, so an error report cannot turn into a second error.

## 3. Channel numbers in the core

The core's routing is in `src/sb_interp.c` (`output_channel`,
`input_channel`). The numbers are the ROM's (see `docs/JS-SB3-MAP.md`
section 4):

| Number | `PRINT #n` | `INPUT #n` | Not installed by the host |
|---|---|---|---|
| 0 | writes `chan[0]` | reads `chan[0]`; the default for `INPUT` | -6 |
| 1 | writes `chan[1]`; the default for `PRINT` | reads `chan[1]` | -6 |
| 2 | writes `chan[2]` | reads `chan[2]` | -6 |
| 3 | writes `chan[3]` | -19 (output-only window) | -6 |
| other | -6 | -6 | -6 |

- `PRINT` with no channel writes to `chan[1]`. `INPUT` with no channel reads
  `chan[0]`.
- `LIST` and `LIST #n` use the output rule above.
- A channel number is an expression rounded half up. A number outside
  ±1000000 is treated as not open (-6).

## 4. The hosted adapter

`src/sb_console.c` (host glue, uses stdio):

- `sb_console_attach(st, in)` installs the standard channels. Handles #0, #1
  and #2 read from `in` and write to standard output. #3 writes to standard
  output and cannot be read. The error-report handle writes to standard error.
- Each read asks for one byte. `fread` therefore returns as soon as a byte is
  available, so an interactive terminal does not block on a larger request.
- `sb_console_detach(st)` frees the handles and clears the state's slots. It
  is safe to call twice.
- `sb_main.c` attaches the file being run as the input for `INPUT`, and
  re-attaches standard input afterwards. It detaches before `sb_destroy`.
- The adapter's `flush` calls `fflush`. The core does not call `sb_chan_flush`
  itself, so output is flushed by stdio at exit, as in SB2.

The adapter does not allocate beyond its handles, and it frees each handle's
own object in `close`.

## 5. How the tests exercise the interface

`tests/sbtest.c`, `unit_channels()` (in the unit checks) covers:

- partial writes (an adapter that takes at most a few bytes per call), with
  the full text arriving in order;
- `close` runs exactly once for each handle;
- zero progress from an adapter (-40) and over-reporting (-40);
- an injected operation failure, passed through unchanged, both at the handle
  and at `PRINT` (the interpreter's `ERNUM` shows the same code);
- direction: a write-only handle cannot be read, and the reverse;
- `NULL` handle gives -6; a flush with no flush operation succeeds;
- end of input (0 bytes with success);
- routing: `PRINT` goes to `#1` and `PRINT #3` to `#3`, each to its own
  adapter; `PRINT #2` with no handle installed gives -6;
- no leak of handles or of the state after `sb_destroy` (live-block count).

Not covered by a test: a failing diagnostic handle (it is written to, its
errors are ignored in `report`, but no check exercises that path).

The corpus files `41_input_channels` and `48_channels_print` check the ROM
behaviour of the hosted adapter end to end.

## 6. Prospective QDOS/386 adapter contract (design only)

This section is a design. Nothing here is built or tested, and nothing in the
core depends on QDOS, the R8 kernel or a 386 target. The aim is that a future
adapter can be written without changing `src/` outside the adapter file.

Channel identity. The core addresses channels by the same small numbers
(0 to 3). A QDOS adapter installs, for each number, a handle whose object holds
the QDOS channel ID the job opened for that number. The core never sees a
QDOS ID. The mapping from number to ID is the adapter's job; it can be empty
(channel not open, -6) or can map #1 to a window the job created.

Ownership and lifetime. The job owns the channel IDs. The adapter takes a
handle's lifetime from the state's: it creates handles before `sb_run` and
frees them after `sb_destroy`. `close` releases the QDOS channel. A channel
the job did not open is never closed by the adapter.

Reads and writes. `write` takes at most the bytes the caller gave, and reports
how many it accepted. A driver that accepts only part of a write reports that
count; a driver failure returns an error code and `*done` as 0. `read` returns
what is available; `*got == 0` with success means end of input. Blocking
behaviour is the driver's: the core never polls, and it asks for one byte at
a time for line input, so a blocking read waits for a keystroke.

Memory. The handle's allocation goes through the `SbAlloc` the adapter passes
to `sb_chan_new`. The adapter supplies the allocator; the core never calls a
platform allocator directly, so a QDOS build can route it to the job's heap.

Errors. The adapter returns `SB_E_*` codes. A QDOS driver error that has no
equivalent is mapped to `SB_E_IO` (-40), and the message text stays the
adapter's concern. The ROM's own codes (-6, -19) are kept by the core's routing,
not by the adapter.

Flush. `flush` maps to the driver's flush, if it has one. The core calls
`sb_chan_flush` only when a future feature needs it; SB3 does not.

Not covered: `OPEN` and `CLOSE` from the program, file channels, window
parameters, and any QDOS trap. Those need their own design, and nothing in
this document implies they are complete.

## 7. What SB3 does not claim

- It does not claim QDOS window or file-channel compatibility.
- The channel set is the SB2 set, #0 to #3, with the SB2 routing rules.
- The hosted adapter is the only adapter. The QDOS contract above is a plan.
