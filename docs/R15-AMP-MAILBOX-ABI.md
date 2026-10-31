# R15 AMP0 — mailbox and service ABI

Two layers: the **mailbox** between CPU 0 and the worker CPU (kernel-internal, `kernel/amp.h`), and the **QT_AMP
trap** between applications and CPU 0 (`sdk/include/qdos386amp.h`). An application never touches the mailbox.

## 1. Mailbox (`struct amp_mbox`, static kernel BSS, version `AMP_ABI_VERSION` = 1)

One worker, **one outstanding request**. Buffers `amp_in`, `amp_out`: 16,384 bytes each.

### Field ownership (who may write; both may read)

| Fields | Writer |
| --- | --- |
| `boot_gen`, worker IDT, worker stack top, trampoline page | CPU 0, only while the worker is not running (before start-up / after a halt acknowledgement) |
| `ap_magic`, `ap_version`, `ap_apic_id`, `ap_cpuid_id`, `ap_lapic_base`, `ap_boot_gen`, `ap_stage`, `heartbeat`, `ap_completed`, `ap_errors`, `burn_*`, fault record (`f_*`) | worker |
| `ap_state` | worker (xchg); CPU 0 only while the worker is halted or has never run |
| `ap_ctl` (RUN / QUIESCE), `cancel_gen` | CPU 0 (xchg) |
| request: `gen`, `cmd`, `in_len`, `out_max`, `arg0`, `arg1` and `amp_in` | CPU 0, only while `state == IDLE` |
| result: `done_gen`, `status`, `out_len`, `r0`, `r1` and `amp_out` | worker, only while `state == RUNNING` |
| `state` | see transitions |

### Request state machine

```
          CPU 0: fill request, xchg            worker: lock cmpxchg QUEUED->RUNNING
  IDLE ----------------------------> QUEUED -------------------------------------> RUNNING
   ^                                                                                  |
   |  CPU 0: copy result out, xchg IDLE            worker: write result fields, xchg  |
   +-------------------------------- DONE / ERROR / CANCELLED <-----------------------+
```

* `DONE`: `status` 0. `ERROR`: `status` BADCMD (unknown command) or BOUNDS (a length over the buffer, XFORM output
  larger than `out_max`, NUMERIC count over `AMP_NUM_MAX`). `CANCELLED`: the worker saw `cancel_gen == gen` (NUMERIC
  checks every 65,536 iterations), stopped, and reports the iterations done in `r1` — a positive acknowledgement.
* The worker re-validates every length itself; CPU 0's checks are not trusted by the worker.

### Memory ordering (x86, two CPUs)

* Every state change is a LOCKed instruction (`xchg` or `lock cmpxchg`): a full barrier. Ordinary stores made before it
  (request fields, input bytes; result fields, output bytes) are globally visible before the new state.
* The reader of a state reads it first and the data afterwards; x86 does not reorder loads with older loads, and every
  shared access goes through an out-of-line assembly function, which GCC 1.40 cannot move other memory accesses
  across (it has no `memory` clobber — `volatile` alone is not relied on).
* Store→load reordering (the one x86 permits) cannot break the protocol: no side decides from its own unlocked store
  followed by a load of the other side's variable.
* Generations: 32-bit, `0` is never used (wrap from 0xFFFFFFFF goes to 1); the worker copies `gen` to `done_gen`;
  CPU 0 accepts a result only if `done_gen` equals the generation it published (else `AMP_ERR_STALE`, slot freed —
  the worker has published, so it no longer touches the request).

### Worker lifecycle (`ap_state`)

`OFF → ENTRY → READY`; from READY only the worker moves to `FAULTED` (its exception handler records vector, error
code, EIP, CR2, ESP, EFLAGS, APIC ID, then halts) or `QUIESCED` (it saw `ap_ctl = QUIESCE`). Both are written as the
worker's last store before `cli; hlt`, so they are positive acknowledgements that it has stopped. Only then may CPU 0
reuse the worker's memory or restart it. **A timeout is never treated as a stop.**

## 2. Commands (`cmd`)

| Value | Name | Input | Result |
| --- | --- | --- | --- |
| 1 | CRC32 | `amp_in[0..in_len)` | `r0` = CRC-32 (IEEE 802.3, reflected 0xEDB88320, as zlib), `r1` = byte sum |
| 2 | XFORM | `amp_in`, `arg0` = seed | `out[len-1-i] = in[i] ^ ((s_i >> 16) & 0xFF)`, `s_i = s_{i-1}·1103515245 + 12345`; `r0` = last s |
| 3 | NUMERIC | `arg0` = n ≤ 0x40000000, `arg1` = seed (0 → 1) | xorshift32 (13, 17, 5) n times, `r0` = x, `r1` = Σ(x ⊕ i) mod 2³² |
| 0xF0 | T_STALE | — | test: answers with `done_gen = gen ^ 0x80000000` |
| 0xF1 | T_FAULT | — | test: executes an invalid opcode on the worker (#UD) |
| 0xF2 | T_SLOW | `arg0` = n | test: NUMERIC that never looks at `cancel_gen` |

## 3. Application ABI: `qtrap(QT_AMP = 0x3D, op, b, c)`

| op | b | c | returns |
| --- | --- | --- | --- |
| 0 INFO | — | 32-word buffer | 0 (words: version, service, reason, table, cpus, bsp, worker, reported ids, heartbeat, completed, errors, slot, gen, owner, next gen, fault record, boots, late, orphans, cancels, burn, ap_state, LAPIC base/version, ESR) |
| 1 SUBMIT | — | `struct amp_submit {cmd, in_ptr, in_len, out_max, arg0, arg1, gen}` | 0 and `gen` written back; `AMP_ERR_UNAVAIL`, `BUSY`, `RANGE` |
| 2 POLL | gen | — | `RQ_QUEUED/RUNNING/DONE/ERROR/CANCELLED`; `AMP_ERR_STALE`, `OWNER`, `FAULT` |
| 3 RESULT | gen | `struct amp_result {out_ptr, out_max, state, status, out_len, r0, r1, gen}` | 0 (output copied, slot freed); `NOTREADY`, `STALE`, `OWNER`, `RANGE`, `FAULT` |
| 4 CANCEL | gen | — | 0: cancellation **pending** until the worker acknowledges (POLL then shows CANCELLED, or DONE if it finished first) |
| 5 TEST | `AMP_T_*` | arg | diagnostics: SETGEN (next generation), MALFORMED (publish an over-long request bypassing CPU 0's check), QUIESCE, RESTART (only after FAULTED/QUIESCED), PROOF (`struct amp_proof`: CPU 0 spins with interrupts off until the worker answers) |

Rules enforced by CPU 0: every user pointer is checked with `chan_user_range_ok` (the caller's own memory, never
kernel memory) and **copied**; lengths ≤ 16,384; the slot belongs to the submitting job (`AMP_ERR_OWNER` for anyone
else); one request at a time (`AMP_ERR_BUSY`). Waiting is the caller's choice: poll and sleep (`qd_yield`) —
synchronous use is SUBMIT + POLL loop + RESULT; asynchronous use does its own work between POLLs.

**Process exit**: when the owning job ends (exit, KILL, fault), the kernel immediately sets `cancel_gen`
(`amp_job_ended`). The slot is freed by the next service call only after the worker has published DONE / ERROR /
CANCELLED (counted as an orphan); a request that ignores cancellation keeps the slot until it completes.

Errors: −80 UNAVAIL, −81 BUSY, −82 RANGE, −83 STALE, −84 OWNER, −85 NOTREADY, −86 STATE, −87 TIMEOUT (PROOF; nothing
is reclaimed), −88 FAULT.
