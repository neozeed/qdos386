# R12B Phase 0 audit — environment, PATH, startup, SuperBASIC file/process integration

Base: frozen R12A `6e30dd7046b4d09d77ec398bea38a76318b36c52` (package SHA-256
`979825fcfa8980547b5a3cbd04cbc85d88070ed7d4449ffe666f5364d53c4b4e`, both re-verified). Branch `r12b-system-environment`.
R12A kernel images: QEMU `257460b9…5b99`, headless `0b23ab9d…2ff3`. Baseline QEMU smoke: the R12A integrated demo re-run
(`logs/r12b/phase0/`): shell, EDIT save, two BASIC copies, SPAWNT spawn/wait, HEAPT, VFAT persistence across a reboot.

## 1. Process creation (R12A) — where an environment fits

* All loads go through `proc_load` (`kernel/proc.c`): `aout_check_src` (reads only) → `job_alloc_sized` (job record +
  16 KiB kernel stack owned by the new id) → `build_space` under the template (directory, image, argument page, stack) →
  bind → `prio_inc` set (or held). Every failure path ends in `vas_destroy` + `job_cancel`; `job_cancel` frees every
  kernel-heap block owned by the id (`mem_reclaim_owner`).
* **Safe copy point for an environment:** inside `proc_load`, after the job id exists and before the job can run
  (`prio_inc` is still 0, IF off): a kernel-heap block owned by the child id. A failure there is undone by the existing
  `job_cancel` (the block is owned by the id), so the rollback property is kept for free.
* The caller's arguments are copied by `img_*_request` into the static `img_packed` before anything is built (no parent
  pointer reaches the child). cwd is inherited by `win1_cwd_inherit` in `job_alloc_sized`; the console by
  `vcon_job_inherit`. Handles are never inherited (a child opens its own channels).
* `QT_PROC_*` contract: request block version 1 {version, packed, length, flags, console}; WAIT returns -73 to be
  repeated; child records 8 per parent / 32 system-wide; job table 16.
* libc: `getenv` is a stub returning NULL (`sdk/libc/stdlib.c`); `system` returns -1. No environment exists anywhere.

## 2. Shell (R11M/R12 source, `apps/shell/`)

* One line buffer (`line`, 160 bytes with `-DSH_LINE_MAX=160`), one dispatcher `exec_line(char *)` (command word folded
  to upper case, arguments keep case, quotes handled per command by `uq()`); `RUN` → QT_IMG_LOAD, `START` → QT_PG_START
  (both with the packed argument block); names are opened by the kernel **relative to the caller's cwd only** —
  `win1_open_img` → `fat16_open_path(cwd, name, FATP_CI)`. There is no search path: a program in `/` is not found from
  `/SRC` unless named `/X.AOU`.
* No script execution, no variables, no startup file. Boot: SYSTEM loads `SHELL.AOU` from the root and polls its exit.
* File API available to a guest: open/read/write/flush/truncate/close, `FS_OP` mkdir/rmdir/remove/rename/stat/getcwd/
  chdir. **Rename fails with -83 (FAT_ERR_EXIST) when the target exists** — a safe replacement is write-temp → rename the
  old file aside → rename temp → remove the old (the EDIT.AOU save procedure, R12).
* Console: one reader per console; queued-byte count (`CH_CTL 0x13/5`) lets the shell poll for a key without blocking.

## 3. SuperBASIC (SB4 core as integrated in R12, `apps/superbasic/`)

* **Program store:** a sorted singly linked list of `SbLine {number, text (canonical form), len}`
  (`sb_prog_store/find/delete/clear`). `LIST` prints `"<n> <text>\n"`; storing that text again gives the same canonical
  text, so LIST output is a stable save format (round trip at the canonical-listing level).
* **Keywords:** the lexer table (`sb_lex.c`) has no LOAD, SAVE, MERGE, EXEC, EXEC_W, DIR: they are identifiers, and a
  statement made of an unknown identifier gives -12 "bad name" (`stmt_ident`). The SB4 documentation lists SAVE, LOAD,
  MERGE as not implemented (`docs/KNOWN-LIMITATIONS.md` of the supplied package). The reconstructed ROM source is not part
  of this tree, so the ROM's own procedure table cannot be consulted; the syntax used is the QL User Guide form
  (`LOAD name`, `SAVE name`, `MERGE name`, `EXEC name`, `EXEC_W name`, `DIR [name]`, where name may be a string).
* **How the QL adds such words:** in QDOS the BASIC procedures LOAD/SAVE/EXEC are entries of the name table and
  extensions are added with `BP.INIT`. R12B adds the same mechanism to the core as a small, documented change: a host
  **extension name table** (procedures and functions) consulted only for names that are neither DEFine PROC/FN nor
  variables/arrays. With no table installed the core behaves exactly as before (corpus regression).
* **Checker:** `sb_check_line` already accepts `name expr, expr` statements (procedure calls), so no change is needed
  there.
* **Direct mode / program mode:** `run_loop` executes a program line or a typed line; LOAD/MERGE replacing the program
  while a program line is executing would free the line being run, so they are restricted to direct mode.
* **Channels:** `SbChan` handles #0..#3 for the console; files are not SuperBASIC channels in SB4 (OPEN/CLOSE are not
  implemented). Program files are read and written by the adapter with libqdos/libc file calls, relative to the
  interpreter's cwd.
* **Process API in BASIC:** none. The adapter links libqdos (`qd_spawn`, `qd_wait`, `qd_kill`, console focus calls).

## 4. Memory accounting (planned)

| Item | Where | Bound |
|---|---|---|
| environment of a job | one kernel-heap block owned by the job id, 256-byte steps | 2,048 bytes ("NAME=value\0" strings), 32 variables |
| 16 jobs × full environment | kernel heap | 32 KiB of the 806 KiB free heap |
| PATH candidates | kernel stack buffer | 128 bytes per candidate |
| shell script buffers | shell BSS | 4 levels × 4 KiB |
| BASIC LOAD staging | interpreter heap (private) | file ≤ 64 KiB, lines ≤ 255 characters |

## 5. Staged implementation map

| Stage | Kernel | SDK/libc | Shell | BASIC |
|---|---|---|---|---|
| 1 | `kernel/env.c` (store, rules), traps QT_ENV_GET/SET/LIST, copy in `proc_load` | `qd_env_*`, `getenv`, `setenv`, `unsetenv` | SET, UNSET, ECHO, `%NAME%` | — |
| 2 | shared resolver `img_resolve_open` for every load (cwd, PATH, root) | — | CALL/SOURCE, `/ETC/STARTUP.CMD` with ESC bypass | — |
| 3 | — | — | — | extension table; LOAD, SAVE, MERGE, DIR |
| 4 | — | — | — | EXEC, EXEC_W, EXEC_ID, EXEC_STATUS, JOB_WAIT, JOB_KILL |
| 5 | — | — | — | demo, regressions, package |
