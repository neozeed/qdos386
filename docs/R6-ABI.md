# QDOS/386 R6 — file channel ABI

R6 adds read-only FAT16 files to the R4 channel manager. A job opens a file
with `QT_CH_OPEN` using a `WIN1_` name, reads it with `QT_CH_READ`, moves
with `QT_CH_SEEK`, and closes it with `QT_CH_CLOSE`. Everything from R4 and R5
is unchanged; the additions are listed here.

## 1. Traps

| Code | Name | Arguments | Result |
|------|------|-----------|--------|
| 0x14 | `QT_CH_OPEN` | a = name pointer (NUL-terminated, at most 15 characters) | channel id, or negative |
| 0x15 | `QT_CH_CLOSE` | a = channel id | 0, or negative |
| 0x16 | `QT_CH_WRITE` | a = id, b = buffer, c = length | `-15` on a file channel |
| 0x1A | `QT_CH_READ` (new) | a = id, b = buffer, c = length (0..256) | bytes read; 0 at end of file; negative on error |
| 0x1B | `QT_CH_SEEK` (new) | a = id, b = byte position | 0, or negative |

Trap gates are interrupt gates (type 0x8E), so a trap runs with interrupts
masked and cannot be preempted. The FAT16 reader and the IDE sequence rely on
this: a read is never interleaved with another read.

`QT_CH_READ` and `QT_CH_SEEK` operate only on channels the calling job owns.
Another job's id gives `-13`, as for R4.

## 2. Names

- `CONSOLE`: `"CON"`, exact match (R4, unchanged).
- Files: `"WIN1_"` followed by a root-directory name. The driver is a prefix
  driver: the whole name is passed on, and the driver reads the part after the
  prefix.
- File names are uppercase 8.3: base 1–8 characters, optional extension of
  1–3 characters, characters `A–Z`, `0–9`, `_`, one dot at most. Lowercase is
  refused with `-46`.
- `CHAN_NAME_MAX` rose from 8 to 16 (15 characters plus NUL), so
  `WIN1_HELLO.AOU` fits. R4's name checks are unchanged.
- Only the root directory is visible. Subdirectories, long names and writes are
  not supported. The channel API takes the full name, so directory traversal
  can later be added in the driver (`kernel/win1.c`, `fat16_open_root`) without
  changing any call above.

## 3. Errors

| Code | Meaning |
|------|---------|
| -3, -6, -7, -12, -13, -14 | as in R4 (no memory, no slot, no driver, stale id, owner, range) |
| -15 | `CHAN_ERR_NOTSUP`: write on a file channel, or read or seek on the console |
| -18 | `CHAN_ERR_NOVOL`: `WIN1_` volume not mounted |
| -40 | `FAT_ERR_IO`: sector read failed |
| -41 | `FAT_ERR_BPB`: boot sector or BPB invalid |
| -42 | `FAT_ERR_TYPE`: cluster count outside the FAT16 range |
| -43 | `FAT_ERR_CHAIN`: cluster out of range, cycle, bad-cluster mark, or chain longer than the file |
| -44 | `FAT_ERR_NOFILE`: no such root-directory entry |
| -45 | `FAT_ERR_SIZE`: chain shorter than the file size |
| -46 | `FAT_ERR_NAME`: not a valid uppercase 8.3 name |
| -47 | `FAT_ERR_RANGE`: sector or seek position outside the volume |

A file's chain is validated at open, so a damaged file fails at open (`-43`,
`-45`), not in the middle of a read.

## 4. Semantics

- Open: the file is found in the root directory, and its chain is walked once.
  The chain must have exactly `ceil(size / 512)` clusters, every entry in
  range, and no cycle.
- Read: returns `min(n, remaining)` bytes. At end of file it returns 0. The
  user buffer is range-checked as in R4 (`chan_user_range_ok`).
- Seek: the position may be from 0 to the file size inclusive. Past the end
  gives `-47`.
- Close: frees the channel. Closing on job exit is unchanged from R4: a job's
  file channels are closed by `chan_close_owner`.
- Each channel holds its own cursor (`struct fat16_file` inside
  `struct chan_object`). Two jobs can read the same file independently.

## 5. Loader

`app_load_bytes(img, len)` loads a.out bytes from a buffer. It runs the same
`aout_check`, slot-busy check, job creation with interrupts masked, copy and
BSS zeroing as R5's `app_load(index)`, which now calls it. The buffer is read
only during the call. The R6 phase stages `HELLO.AOU` in an 8 KiB kernel buffer,
so files larger than 8 KiB are not loaded.

## 6. Trap-level example

```c
int ch = qtrap(QT_CH_OPEN, (unsigned int) "WIN1_HELLO.AOU", 0, 0);
int n  = qtrap(QT_CH_READ, ch, (unsigned int) buf, 256);   /* bytes */
qtrap(QT_CH_SEEK, ch, 0, 0);                              /* back to 0 */
qtrap(QT_CH_CLOSE, ch, 0, 0);
```

An application using this ABI must link as in R5 and include the trap wrappers
in `apps/common/appcalls.h`. The `APP_QT_CH_READ` and `APP_QT_CH_SEEK` constants
are not yet defined there, as no R6 application uses them.
