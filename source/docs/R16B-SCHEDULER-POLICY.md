# R16B — foreground-first SMP scheduling policy

Opt-in kernel `make qdos386-r16b-pc` (`-DQDOS_R16 -DQDOS_R16B`). The Big Kernel Lock, CPL3, context switching and the
QDOS accumulated-priority scheduler are unchanged; R16B changes only *where* a protected job may run, and reports what
happened. Default kernels, R15, R16 and R16A builds are byte-identical.

## Roles

* **CPU 0** keeps every device IRQ, SYSTEM/SYSTASK and the idle job, all legacy ring-0 processes, and first preference
  for the **foreground** session's protected jobs.
* **CPU 1** has first preference for **background** protected jobs. If CPU 1 is absent everything runs on CPU 0 and the
  policy does nothing.
* Only **CPL3 (protected) jobs** ever change CPU. Kernel, idle and ring-0 jobs never move; a ring-0 process started by a
  CPU-1 job is placed on CPU 0 when it is loaded.

## Foreground

The foreground session is `vcon_fg()` — the session that currently has keyboard and display focus. Every focus change
goes through `vcon_switch()` (Alt+F1…F4, the console channel control, the surface layer, the Task Manager's "go to
it"), which bumps `smp_focus_gen`. It is **not** the shell's parent, launch order or session number. While the Task
Manager overlay is open the foreground is still the underlying session (SYSTASK is a kernel job on CPU 0 anyway). All
eligible jobs whose session is the foreground one are favoured: if a foreground session holds several protected jobs,
they all prefer CPU 0 and share it by the normal QDOS priorities.

Keyboard focus and CPU placement are independent: input is queued for the foreground session by CPU 0's keyboard IRQ,
whichever CPU the receiving job last ran on, and a background session never receives direct input.

## Decision rule

For a live protected job with no manual pin (`pin = AUTO`):

```
preferred CPU = 0  if its session is the foreground session (or the policy is OFF, or CPU 1 is absent)
                1  otherwise
```

A manual pin (`CPUSET <id> 0|1`) always wins; `CPUSET <id> AUTO` returns the job to the policy.

## When it acts (event-driven, at safe points)

The policy runs on CPU 0's timer tick with the BKL held. It returns at once unless the focus generation changed, or
a 50-tick sweep is due (to pick up newly started jobs and session changes), or a move it deferred is pending. A pass only
rewrites `aff`; each move is then honoured by the owning CPU's own `schedule()` pass, which saves the frame and x87 state
and releases ownership (`running_on = -1`) before the other CPU can pick the job (docs/R16B-MIGRATION-ABI.md).

**Hysteresis.** A job is not moved again until it has stayed 25 ticks where it is; a move that is due too soon is
deferred and retried a few ticks later. Rapid focus switching therefore cannot bounce a job between CPUs faster than
four times a second, and in the Gate 3 timeline 12 focus-driven requests produced 10 completed migrations in 25 s.

**Fairness.** Placement never blocks a job from running: each CPU round-robins its own runnable jobs by the usual
accumulated priorities, so background jobs share CPU 1 and are not starved by the foreground. Blocking I/O is
unaffected: a blocked job holds no CPU, and its new preference applies when it next becomes runnable.

## Idle job

R16B also stops the idle job from competing. In R16/R16A the idle job was an ordinary priority-1 candidate, and under the
accumulated-priority rule it out-bid a CPU-bound priority-2 job about half the time (measured: one CPL3 job alone on a
CPU got 50% of it). In R16B the idle job is only the fallback when nothing else is runnable; the same job now gets 100%.

## Idle CPU 0 lending (policy `LEND`, optional)

With policy FG, CPU 0 is idle whenever the foreground session has no runnable protected job (for example while the
shell has focus) even if CPU 1 has several background jobs. Policy `LEND` (`CPUSET POLICY LEND`) adds work conservation:
when CPU 0 has no candidate of its own, it may borrow a READY background job while CPU 1 is busy with another one. The
job keeps `aff = 1` and is marked lent; CPU 0 keeps re-dispatching it while it stays otherwise idle, and CPU 1 leaves a
lent job alone for 50 ticks after CPU 0 last ran it, taking it back earlier only if CPU 1 would otherwise idle. This
recovers most of the throughput the strict policy leaves on the table but migrates far more (459 completed migrations
vs 10 in the same 25 s), so it is opt-in; FG is the default.

## Measured effect (Gate 3)

Three CPU-bound protected jobs in text sessions 1–3; focus: shell 4 s, then sessions 1, 2, 3, 1 for 5 s each, then the
shell; 25 s; fixed work units; logs read back from disk. Two independent runs (run 1 in parentheses; ~10–15 % host
variance on a 2-core TCG host):

| policy | foreground ticks / unit | fg wake latency mean / max | background ticks / unit | units completed | completed migrations |
| --- | --- | --- | --- | --- | --- |
| OFF (R16A placement) | 21.8 (22.0) | 2.17 / 3 | 22.0 | 311 (321) | 0 |
| manual pins 0/1/1 | 10.3 (9.5) | 1.09 / 3 | 12.3 | 589 (646) | 1 |
| **FG** (default) | **9.0** (7.3) | **1.00 / 1** | 18.0 | 491 (572) | 10 |
| LEND | 8.3 | 1.00 / 1 | 14.3 | 577 | 459 |

The foreground job runs ~2.4× faster than with R16A placement and at least as fast as the hand-pinned baseline, with a
hard one-tick wake bound. Pins beat strict FG on total throughput because CPU 0 idles while the shell has focus; LEND
closes most of that gap.

## Not done

No per-CPU run queues, no load balancing beyond the above, no N > 2 CPUs, no hard real-time guarantees, no per-application
rules (nothing is keyed to a program name).
