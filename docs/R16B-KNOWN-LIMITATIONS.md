# R16B — known limitations, defects found, and application support

## Application modes

| Mode | R16B behaviour | Evidence |
| --- | --- | --- |
| Protected (CPL3-marked or `QD_SPAWN_PROT`) text-session program | **Supported.** Placed by the policy (foreground → CPU 0, background → CPU 1), pinnable with CPUSET, migrates at safe points | LOOPW, MIGW, R16T (Gates 2–4) |
| Protected graphics-surface program | **Supported.** Its surface session follows focus like a text session; frames keep running while hidden and on either CPU | CPL3-marked BOUNCE: 1500 frames, 482 shown / 1018 hidden, `verify=PASS` across migrations |
| Protected program blocked on console input | **Supported.** Input is queued for the foreground session by CPU 0's keyboard IRQ; the job wakes on whichever CPU it is placed on | `R16T ASK` pinned to CPU 1 read a line typed on the keyboard |
| Protected program doing file I/O, spawn/wait, kill | **Supported** from either CPU (all under the BKL) | LOOPW writes its log from CPU 0/1; STRESS, KILLW, MIGKILL |
| Legacy ring-0 program (any unmarked `.AOU`, the shell, LEAK, …) | **Unchanged: CPU 0 only, never migrates.** A ring-0 process started by a CPU-1 job is placed on CPU 0 at load | Gate 1 R16TL; LEAK runs in every test |
| Program that needs direct port I/O or other CPL0 privilege | Must stay a ring-0 program (CPU 0). R16B never marks binaries protected and never grants I/O privilege | — |
| Elite, Hack, QLTerm, GW-BASIC, GCC, SuperBASIC, Infocom | **Not tested in R16B.** They are external application ports, not rebuilt or bundled; unmarked they run on CPU 0 exactly as on R16A. Marking one protected is the user's choice and was not exercised here | — |

## Defects found in the frozen R16 / R16A kernels (fixed only in R16B)

1. **Unlocked `schedule()` on ring-0 process exit (correctness).** A legacy ring-0 process ends in job context
   (`proc_entry` returns → `job_run` → `job_exit` → `yield_asm`), not inside an entry stub, so R16/R16A ran
   `schedule()` without the BKL — racing the other CPU on the job table, `dead_mask` and the `running_on` hand-off —
   and `resume_dispatch` then released a lock it did not own. This is the source of every nonzero `bkl_badrel` seen in
   R16/R16A (each `LEAK.AOU` exit adds one). The R16 documentation wrongly described that counter as benign. R16B's
   `job_exit` takes the BKL first; three ring-0 exits plus migration then leave `badrel = 0`, and every R16B run reports 0.
2. **Idle job competes for the CPU (performance).** The idle job was an ordinary priority-1 candidate and took ~50% of a
   CPU from a CPU-bound priority-2 job. R16B makes it a pure fallback (one job alone on a CPU: 50% → 100%).
3. **`SETAFF` to CPU 0 had no protection check (latent double-run).** R16 accepted CPU 0 for any job, so the CPU-1 idle
   job could be placed on CPU 0 while CPU 1's idle fallback still returned it. R16B accepts only live CPL3 jobs.
4. A CPU-1 job spawning a ring-0 program gave that program `aff = 1` (inherited); R16B places it on CPU 0 at load.
   (Found by code audit; the fix is one assignment in `proc_load` and was not exercised by a dedicated test.)

R16 and R16A remain frozen and byte-reproducible as released; they keep these behaviours.

## Scope limits

* **Two CPUs, one Big Kernel Lock.** All kernel work is serialized; user code runs in parallel.
* **Migration latency.** A CPU change is honoured at the owning CPU's next `schedule()` pass — at most one timer tick
  (10 ms at the 100 Hz tick), sooner if the job blocks. No reschedule IPI is sent.
* **Placement is per session.** All protected jobs of the foreground session prefer CPU 0 and share it; the policy does
  not pick one "main" job within a session.
* **Strict FG idles CPU 0** while the foreground session has no runnable protected job (e.g. the shell has focus), even
  if CPU 1 has queued background work. `LEND` fixes that at the cost of many more migrations; it is opt-in.
* **No per-CPU run queues.** `pick_next` still scans the job table on each pass, per CPU.
* **Sessions ≥ 4 have no Alt+Fn key** (an existing UI limit, unchanged): graphics sessions created with `/NEW` are
  focused through the Task Manager ("go to it").
* **Default policy.** The R16B kernel boots with FG, so background protected jobs run on CPU 1 automatically — a visible
  difference from R16A. `CPUSET POLICY OFF` restores R16A placement.

## Measurement caveats

* Utilisation is **tick-sampled** by each CPU's timer: it classifies what each tick interrupted. It is an estimate at
  tick granularity, not exact time, and a job that always sleeps across ticks can be under-counted.
* **BKL wait time uses the TSC.** Under QEMU TCG the guest TSC follows host time; on a host with fewer free cores than
  vCPUs (two emulators at once on a 2-core host) a descheduled lock holder inflates a waiter's measured time by orders of
  magnitude. Even with a single emulator, two vCPU threads plus QEMU's own threads and the capture harness share the
  two host cores, so wait times stay inflated. The contended-acquisition count is the robust figure. Gate 3, Stage 4
  and the soak ran one emulator at a time.
* Throughput comparisons vary ~10–15 % between runs on the 2-core TCG host; conclusions are drawn only from orderings
  that held across independent runs.
* Wake latency is measured in whole ticks.

## Not attempted

Fine-grained locking, per-CPU run queues and load balancing beyond LEND, more than two CPUs, device interrupts on CPU 1,
per-application rules, CPU usage percentages per job, and any change to graphics, console, filesystem or application code.
