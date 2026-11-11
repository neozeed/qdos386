# Application and kernel ABI (canonical baseline)

Source of truth: `sdk/include/qdos386.h` (application side), `kernel/jobs.h` and the per-subsystem headers (service
numbers and error codes), `kernel/channel.h`, `kernel/surface.h`. This file summarises the rules; the headers are
authoritative.

## Entry

- Applications enter the kernel with `int 0x30`. The SDK trap entry (`sdk/include/qdos386.h`, `lib/qtrap.s`) loads
  `EAX` = service number, `EBX` = `a`, `ECX` = `b`, `EDX` = `c`. The result is read back as the SDK wrapper defines it;
  do not assume a register beyond what that header states.
- Service numbers are the `QT_*` constants. Ranges in use: `QT_CH_*` 0x14..0x17 (channels), `QT_SURFACE` 0x3B,
  `QT_SYNC` 0x39, `QT_SMP` 0x3E, the R20 storage traps 0x40..0x45 (internal, not an application ABI; `kernel/r20w.h`).
- Errors are negative integers; each subsystem header names its own codes (`CHAN_ERR_*`, `PROC_ERR_*`, `SURF_ERR_*`,
  `SMP_ERR_*`, `PATH_ERR_*`). Codes can overlap between subsystems, so a code is meaningful only with its service.

## Fixed values that applications may rely on

| value | where | meaning |
|---|---|---|
| `QD_READ_MAX` = 256 | `sdk/include/qdos386.h` | the largest single file read. Unchanged in R20; the cache must not change it. |
| `CHAN_MAX_WRITE` = 256 | `kernel/channel.h` | the largest single channel write. |
| `SURF_VERSION` = 1 | `kernel/surface.h` | the version of `QT_SURFACE`; `SURF_OP_CAPS` reports it. |
| `SMP_INFO_WORDS` = 24 | `kernel/smp.h` | size of the `SMP_OP_INFO` buffer. |
| channel id = `(tag << 16) | index` | `kernel/channel.h` | opaque to applications; the tag wraps to 1 at 0x7fff. |

## Stability rules

- Adding a service number, an error code or a field is an ABI change: it needs a version bump in the subsystem header and an
  entry in the subsystem's design document.
- The R20 traps are internal (kernel tests only). Applications must not call them.
- Changing `QD_READ_MAX`, `CHAN_MAX_WRITE`, the channel id layout, or the trap vector is out of scope for the canonical
  baseline. Such a change would need a new branch and a new release.
