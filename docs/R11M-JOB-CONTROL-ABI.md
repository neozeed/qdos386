# R11M native job control

Applies to kernels built with `-DQDOS_JOBCTL` (`make r11m-kernels`; the console column needs `-DQDOS_VCON`, background starts with several copies need
`-DQDOS_PAGING`). Kernels built without the option contain none of this code and are byte-identical to R11L (checked by `scripts/r11l-default-check.sh`).

**This is job management, not isolation.** Kernel, heap and stacks are shared and writable by every job; a paged job has only a private image window
(`docs/R11K-PAGING-APP-ABI.md`). `KILL` does not check who the caller is.

## 1. Shell commands

| Command | Behaviour |
|---|---|
| `RUN path [args]` | unchanged: foreground, synchronous, slot 1 (shared window), prints `exit status N` |
| `START path [args] [/CON:n]` | launches the program and returns at once with `job <id> started on console <n>`. On a paging kernel the program runs in its **own paged image window** (so several copies linked at the standard origin run together); on a kernel without paging it uses the slot 1 loader (one at a time). `/CON:n` (0..3) selects the console of the new job; the default is the shell's console. Quoted arguments work as for `RUN`. Programs not linked for the standard origin are refused (`-61`) with a message. |
| `PS` | one line per job: `ID SLOT STATE PRI CON AGE NAME`, then the started jobs that are gone, with their recorded end (`status N`, `killed`, `no exit status`) |
| `WAIT id` | blocks until a started job has ended and its slot is released; prints `job <id> exit status N`, `was killed` or `ended without an exit status`. A job that ended earlier is answered from the shell's record. |
| `KILL id` | ends a started job and prints `job <id> killed` after its resources are released |

`WAIT` and `KILL` take the decimal id printed by `START` and `PS`. Ids are tagged: the low 16 bits are the slot, the high bits change every time a slot is
reused, so an id that no longer names the job it did fails with `no such job (unknown or stale id)`. `WAIT` refuses jobs that were not started (the shell,
the idle job); `KILL` refuses them and itself. The shell remembers its last 8 started jobs (`SJ_MAX`); it records the end of every one before each command, because the
kernel keeps an exit record only until the slot is reused.

## 2. Kernel traps (ABI version 1)

`int 0x30`, EAX = trap, EBX/ECX/EDX = arguments, EAX = result. A kernel without the option answers `-1`; the library maps it to `QD_ERR_NOJOBCTL` (-72).

### `QT_JOB_QUERY` 0x2D — `a` buffer, `b` buffer size in bytes, `c` version (must be 1)

Copies a 16-byte header and up to `(b-16)/40` job records into the caller's buffer and returns the record count. Errors: `-34` bad version; `-14` buffer smaller
than 16 bytes, outside the caller's arena (`chan_user_range_ok`), or wrapping the address space. Little endian.

    header (16): version=1, entry_size=40, count, job_max (6)
    entry (40):  u32 id | u8 slot, state, prio, console | u8 flags, pad[3] | u32 dispatches | u32 age_ticks | u32 stack_bytes | char name[16]

States: 1 RUN, 2 HELD (created inactive, never dispatched), 3 SLEEP (suspended, timed or not), 4 DEAD (ended, resources not yet released), 5 CONWAIT (sleeping in
`QT_CON_WAIT` on its console). Flags: 1 LAUNCHED (created through `QT_IMG_LOAD`, `QT_IMG_START` or `QT_PG_START`, i.e. by `RUN`, `START`, `qd_run`, `qd_start`,
`qd_pg_start`), 2 PAGED, 4 the calling job. Console is 0..3, or 0xFF on a kernel without virtual consoles. `name` is the file name of a launched job (volume
prefix and directory removed, at most 15 characters), the kernel's own name for other jobs. No kernel address is exposed.

### `QT_JOB_KILL` 0x2E — `a` tagged job id

Returns 0, or `-2` stale/unknown id; `-67` the job was not launched (SYSTEM, IDLE, the shell, ...); `-68` the caller itself; `-69` the job has already ended; `-62` the
caller is a paged job (refused, as for the launch traps).

How it ends a job safely: the target is not running (the caller is), so its frame is already saved. The kernel marks it inactive and dead and records that it was killed;
it never touches the target's stack or page directory. The ordinary reclaim pass (`reclaim_dead`, run at the start of every trap and yield, never in the timer ISR) then does
what it does for any exited job, in the same order: the address space (`pg_release_owner`, which keeps the job dead for a later pass if its directory were active),
the channels (`chan_close_owner`, so open files are closed and the open-file registry is released), application regions, heap blocks, the console session (waiter, user count,
focus) and the stack. `QT_JOB_EXIT` of a killed job returns `-66` (`JC_ERR_KILLED`); an exit status of exactly -66 is indistinguishable from it (the same documented
overlap as -16/-17). A job that was killed after calling `QT_EXIT` keeps its status.

## 3. libqdos (`sdk/lib/qdosjob.c`)

`qd_job_list(struct qd_jobinfo *out, int max)` (at most 8 records), `qd_job_find(id, &info)`, `qd_job_kill(id)`; constants `QD_JS_*`, `QD_JF_*`, `QD_ERR_KILLED`
(-66), `QD_ERR_PROTECT` (-67), `QD_ERR_SELF` (-68), `QD_ERR_ENDED` (-69), `QD_ERR_NOJOBCTL` (-72). Starting is `qd_pg_start`/`qd_pg_startv` (R11K) or `qd_run`.
`qd_con_child(ch, n)` sends the **next** job this job creates to console `n` (one-shot: it is consumed by the next creation, successful or not, so set it before every start).

## 4. Capacity and refusals

`JOB_MAX` 6 (SYSTEM, the shell, IDLE and three more); `PG_PROC_MAX` 3; frame windows 7 (i386VM block map), 2 (Tiny386/QEMU). Starting more programs than fit is
refused with a bounded code (`-29` no job slot, `-60` no frame run, `-63` no address-space record) and leaves nothing behind (`JOBT` checks the address-space count,
the paged byte count and the free heap bytes against their baseline).

## 5. Verification

| Evidence | Class |
|---|---|
| `JOBT.AOU` (69 checks; 65 plus an NA section on i386VM): query format, bad version/size/pointers, kill refusals (self, system job, stale tag, bad slot), kill of a **held** job before its first dispatch, two concurrent copies with output on console 2, kill of one while the other keeps running, table exhaustion and recovery, 8 launch/exit cycles (status 7 each), kill of a job **waiting for console input** (then a new waiter on that console works), kill of a job holding a **writable file** open (file readable afterwards, removable, not BUSY), baseline equality of address spaces, held bytes and heap after every section | guest executed on QEMU (69/69), Tiny386 (69/69), i386VM (65/65, the 4 console-wait checks NA: the harness has end-of-input pending, so a waiting job is released at once) |
| shell session on QEMU: `START` on consoles 1 and 2, `PS`, `CON 2`, Alt+F2/F3/F1, `KILL`, `WAIT` (running job, finished job, killed job, stale and unknown ids), refusals of the idle job and the shell itself, `RUN JOBT.AOU` at the same prompt, final image `fsck.fat` and `tools/r11m-fatwalk.py --strict` clean | guest executed, QEMU keyboard path |
| background `PHOON.AOU` (x87) on consoles 1 and 2 while the shell works; console 1 text equals the foreground `RUN` output | guest executed (i386VM) |
| `kernel/jobctl.c` strict C89, three configurations | host build |

## 6. Limits

Not tested here: kill of a job while another paged job is current on a PC map with both frame windows in use (covered only by the exhaustion section), kill during a
file write in the middle of a FAT update (traps are atomic with interrupts off, so a job is never parked inside one; not separately injected). Ownership is not
enforced (`KILL` works for any launched job), jobs are not isolated, a paged job cannot start or kill, `WAIT` blocks the shell (there is no Ctrl-C), the shell's record
holds 8 jobs.
