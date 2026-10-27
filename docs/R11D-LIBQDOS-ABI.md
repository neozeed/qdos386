# R11D libqdos ABI

Status: **R11D did not expand libqdos.a.** The only change is `_qd_exit_record`,
which records the exit status and returns to the caller's job teardown without
the suspend loop (used by `crt0_libc.s` so that every program returning from
`main` ends its job). The Phase 3 expansion to the C68 `qdos/` entry points is
not done and is recorded as R12 work in `docs/R11D-BLOCKED-APIS.md`.

## Exported entry points (libqdos.a, R11D)

| Symbol | Purpose | Status |
|---|---|---|
| `_qtrap` | raw trap entry | R11C |
| `_qd_exit` | exit, then suspend loop (never returns) | R11C |
| `_qd_exit_record` | record exit status and return (crt0 path) | R11D |
| `_qd_args` | command-line access | R11C |
| `_qd_ticks` | kernel tick counter (not used by `clock`; see blocked list) | R11C |
| `_qd_yield` | yield to the scheduler | R11C |
| `_qd_alloc`, `_qd_free`, `_qd_free_bytes`, `_qd_heap_ok` | heap support | R11C |
| `_qd_open`, `_qd_close`, `_qd_read`, `_qd_write`, `_qd_seek` | channel I/O | R11C |
| `_qd_putc`, `_qd_puts` | console output | R11C |

Trap numbers and register conventions are unchanged from R11C. Semantic
compatibility with the C68 `qdos/` interfaces is **not** established: the C68
code is 68000 and no mapping has been written.
