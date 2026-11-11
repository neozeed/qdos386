# APOLLO 16 / R20 — Phase 1: Ring 0 worker context and request table (checkpoint)

Opt-in: `make qdos386-r20-pc` (QDOS_R20). The R18 images are byte-identical to the frozen baseline hashes (checked after
all kernel changes). Kernel source: kernel/r20w.c, kernel/r20w.h; hooks in kernel/jobs.c (trap cases, reclaim),
kernel/shellboot.c (boot), Makefile (target). Internal trap numbers 0x40-0x45 (not a public API).

## Design (Option a, approved)
- The worker is one dedicated Ring 0 job (job_alloc, own supervisor stack from the kernel heap), scheduled by the existing
  scheduler and its context save/restore. Pinned to CPU 1 when two or more CPUs exist (background CPU), else CPU 0.
- Request table: 8 fixed slots, FIFO, tickets are (generation << 8) | slot.
- Worker states: STOPPED, READY, RUNNING, BLOCKED. Request states: FREE, QUEUED, ACTIVE, DONE, ABANDONED.
- Every shared-state transition is a trap (under the BKL, interrupts off). The worker body (the work) runs with the BKL released.
- Caller: SUBMIT never sleeps; WAIT collects a DONE result or sleeps and returns R20_RETRY (the caller retries, as QT_CON_WAIT does).
- Worker: TAKE sleeps when the queue is empty (checked under the BKL, so no lost wakeup); DONE wakes the submitter only if it waits
  for that request, identified by tagged job id (safe against slot reuse).
- Process exit: QUEUED and DONE requests are freed; ACTIVE requests become ABANDONED and are freed by the worker's DONE.

## Test results (tests/a16/r20t.c, sequence tests/a16/p3.seq, volume = copy of the Apollo 15 test volume + R20T/R20B/R20X)
Evidence: docs/evidence/a16-phase1/*.summary.txt

- Functional (2 CPUs and 1 CPU): 40 synthetic requests, all results correct (fails=0); queue-full backpressure
  (8 accepted, 4 refused of 12); a stale ticket is rejected (-41).
- Completion racing the sleep: 300 zero-work submit/wait pairs, all correct.
- Concurrency (2 CPUs): background process pinned to CPU 0 made 3,000,000 kernel calls over ticks 753-2147; the worker's
  3,000,000-iteration request ran over ticks 1314-1319, inside that window (overlap shown at tick granularity only).
- Concurrency (1 CPU): same workload, background 772-1956, request 1340-1345: both complete, no hang.
- Exit while active: the abandoned request is counted (abandoned=1) and its slot is freed (later runs get all 8 slots).
- Leak: heap before and after the Phase 1 runs is unchanged within each run (LEAK heap=628608 at 2 CPUs).

## Not yet tested or not measured
- That the worker body runs with the BKL released is by construction, not measured.
- Two-CPU run exclusivity of the worker (pinned, pick_next running_on check) is by construction, not measured.
- 8-CPU and -m 32/128/256 runs not run in this checkpoint.
- Caller's zero-CPU use while asleep is inferred from the trap sleep, not measured.

## Phase 2 blocker (not started)
Storage calls run inside FAT16 inside a trap. Sleeping the caller there means resuming mid-FAT16 with the BKL released, which the
scheduler does not provide (sleep is only at trap exit). Options: (1) make the FS trap restartable from its start with the
sector cache already filled by the worker; (2) a sleepable filesystem lock with a mid-trap resume primitive. Decision needed
before Phase 2.
