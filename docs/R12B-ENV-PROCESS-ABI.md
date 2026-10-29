# R12B environment and process ABI

Applies to kernels built with `-DQDOS_ENV` (the R12B kernels `build/r12b/pc-vga/r12b-vga.img` and
`build/r12b/pc/r12b.img`; `QDOS_ENV` is added on top of every R12A option). Kernels without it answer the three new traps
like any unknown trap (a negative code); libc `getenv` then finds nothing, as in R12A.

**Not security isolation.** Every process still runs at CPL 0. The environment is kept by the kernel and copied with
pointer checks, but nothing prevents a ring-0 program from writing kernel memory.

## 1. Model

* Each job owns at most one environment: a block of entries `NAME=value\0` in name order, kept in the kernel heap,
  owned by the job id (the ordinary reclaim of a job's kernel-heap blocks frees it; there is no separate list to leak).
  Blocks are allocated in 256-byte steps; a change that stays within the step is made in place, so a SET followed by the
  matching UNSET leaves the kernel heap exactly as it was (no replace-then-free fragmentation).
* A process gets a **copy** (snapshot) of its creator's environment when it is created (`proc_load`, before the job can
  run: every load path — `RUN`/`QT_IMG_LOAD`, `START`/`QT_PG_START`, `QT_IMG_START`, `qd_spawn` — goes through it).
  Later changes on either side are invisible to the other. There is no way to change another job's environment.
* The shell started at boot has an empty environment; `/ETC/STARTUP.CMD` (Stage 2) fills it.
* Handles are still not inherited. A child inherits: current directory (R11M), console (R11N), environment (R12B).

## 2. Rules

| Item | Rule |
|---|---|
| Name | 1-31 characters `A-Z a-z 0-9 _`, the first not a digit. Anything else (including `=`, `-`, space, an empty name) is refused with -15. |
| Case | Names compare without regard to case and are stored in upper case (`setenv("path", …)` sets `PATH`; listing shows `PATH`). Values keep their case. |
| Value | 0-255 bytes, any byte except NUL. An **empty value is a set variable**: `getenv` returns `""`, not NULL. |
| Limits | 32 variables and 2,048 bytes of entries (`NAME=value\0` each) per job. |
| Order | `QT_ENV_LIST` returns entries in ascending byte order of the upper-case name (deterministic). |
| Atomicity | A change builds the new block aside and replaces the old one only when complete; on any error (bad name, too long, full, no kernel memory) the environment is unchanged. |

## 3. Traps (`int $0x80`, EAX = trap, EBX/ECX/EDX = arguments, result in EAX)

| Trap | Arguments | Result |
|---|---|---|
| `0x35` QT_ENV_GET | name, buf, size | value length (full, even if the copy was truncated to `size-1` bytes + NUL); -7 absent; -15 bad name; -14 name or buffer not in the caller's memory (or `size` 0) |
| `0x36` QT_ENV_SET | name, value (0 = remove) | 0; -15 bad name; -4 value over 255 bytes; -5 over 32 variables or 2,048 bytes; -3 kernel heap exhausted; -7 removing an absent name; -14 bad pointer |
| `0x37` QT_ENV_LIST | index, buf, size | length of entry `index` as `NAME=value` (copied like GET); -7 past the end |
| `0x37` with index `0xFFFFFFFF` | — | `(count << 16) | bytes` |
| `0x37` with index `0xFFFFFFFE` | n (≤ 16) | **diagnostic**: the next n environment copies at process creation fail with -3 (so the spawn rollback can be tested); returns the number of copies made so far |

Pointers are checked byte by byte against the caller's accessible memory (`chan_user_range_ok`); a name or value string
is read at most to its limit + 1 bytes, so an unterminated string is refused rather than over-read.

Process creation (`qd_spawn` and the older load traps) keeps its R12A ABI. A failed environment copy makes the creation
fail with **-30** (out of memory); the job record, kernel stack, environment block, frames and child record are all
released by the existing `job_cancel` path, so every count is as before the call (tested, §6).

## 4. SDK

`<qdos386.h>` (libqdos member `qdosenv.o`):

```c
int qd_env_get(const char *name, char *buf, unsigned int size);
int qd_env_set(const char *name, const char *value);
int qd_env_unset(const char *name);
int qd_env_list(unsigned int index, char *buf, unsigned int size);
int qd_env_summary(unsigned int *count, unsigned int *bytes);
```

`<stdlib.h>` (libc):

* `char *getenv(const char *name)` — C89. The value is copied into **one static 256-byte buffer**: the pointer stays
  valid until the next `getenv` call (from any caller in the process) or a change of the environment. NULL when absent
  or on an older kernel.
* `int setenv(const char *name, const char *value, int overwrite)` and `int unsetenv(const char *name)` — QDOS/386
  extensions with POSIX-like meaning (not C89; member `envx.o`). 0 on success, -1 on failure; `errno` is not set.
  `unsetenv` of an absent name succeeds.
* `system()` is unchanged (returns -1).

Rebuilding a program against this libc changes its bytes (`stdlib.o` now calls the trap); the R12/R12A regression runs
use the frozen R12A binaries (`build/frozen-r12a/`, checked against the R12A package `SHA256SUMS`).

## 5. Shell (`SHELL.AOU` built with `-DR12B_SHELL`)

| Command | Effect |
|---|---|
| `SET` | list every variable, one `NAME=value` per line, in name order; `(no environment variables)` when empty |
| `SET NAME` | `NAME=value`, or `NAME is not set` |
| `SET NAME=value` / `SET NAME = value` | set; spaces around `=` are skipped; the value runs to the end of the line (trailing spaces are dropped by the line reader) |
| `SET NAME=` | set NAME to the **empty** value (unlike MS-DOS, this does not remove it) |
| `SET NAME="value"` | the outer double quotes are removed; leading/trailing spaces, `;` and `=` inside are kept. An unterminated quote is an error. |
| `UNSET NAME` | remove; `NAME is not set` if absent |
| `ECHO text` | print text (after expansion) |

**Expansion.** Before a line is dispatched, `%NAME%` is replaced by the value of NAME, in one left-to-right pass:

* `NAME` is 1-31 name characters followed by `%`; an unset NAME expands to nothing;
* `%%` gives a single `%`;
* a `%` that does not start such a reference is kept as it is (`50% left`);
* nothing between double quotes is expanded (`"%PATH%"` stays as typed); the quotes themselves are kept for the command;
* an expanded value is not scanned again (no recursion);
* if the result exceeds the 159-character line, the line is **not run** and `line too long after %NAME% expansion` is
  printed (in a script this ends the script, Stage 2).

So `SET PATH=%PATH%;/GAMES` appends to PATH. Command names and arguments may come from variables.

## 6. Evidence

* Host: `tests/host/r12b/test_env.c` (strict C89) — name rules, case, order, limits, empty vs absent, refusal leaves
  the block unchanged.
* Guest: `ENVT.AOU` (`sdk/examples/r12b/envt/envt.c`) and the Gate 1 run (`tests/r12b/run-s1.sh`,
  `logs/r12b/stage1/`); results in `R12B-RUNTIME-REPORT.md`.
