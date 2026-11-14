# R4 local test results and profile status

Source implementation checkpoint:3aabae5 (explicit64 research guard and retained counterexample); accepted24 code path is byte-identical to B3's tested default after the 64-only changes. Final archived source commit/receipt are recorded in the internal artifact provenance and HANDOVER. All verification is bounded local QEMU TCG; no external reviewer, KVM, real hardware or soak claim is made.

| Profile/check | Status | Actual evidence |
|---|---|---|
| Frozen R3 reproduction | PASS, newly executed | Fresh export of 3900b20; production serial/VGA + diagnostic + R18 build; four smokes, all 124 R18 outputs and separate250 historical inventory match; docs/releases/R3-FREEZE.md |
| Broader prior R3 matrix | PREVIOUSLY RECORDED |18 runs/1007 checks/79628 PTE checks, not repeated as R4 freeze evidence |
| Original R3 high-slot reproducer | EXPECTED FAILURE | All24 slots fill, exactly8 wrong console associations at 16..23. Baseline trace preserved; no reference source altered |
| Default 24,8 contexts, runtime1/8 | PASS | B2/B3 normal production: every slot0..23 including SYSTEM/AP/worker, v1/v2/TASKS/PS visibility, slot23 session4 input, KILL/exit/reuse/stale IDs, full-table rejection and exact recovery;100 checks per checkpoint pair |
| Default 24 parent/lifecycle test build | PASS | Test-only fill0 hook covers24 allocator/death/wrap states before normal boot; actual protected slot23,64 short children, eight-child quota, parent identity, nonchild refusals, environment startup rollback;144 independent checks |
| CPU 1 and CPU 4 compile profiles | PASS targeted; broader matrix NOT TESTED | Actual1/4 CPU boot/idle/IOW0, existing CCCP/CC1/AS/LD and HI at 16 MiB. Default 8-context build separately tested at runtime1/8 |
| Remaining default compatibility | PASS | Seven new runs:32 MiB/1 CPU,128 MiB/8 and 256 MiB/8 with two original Elites, SH, Hack and GCC/HI twice;16 MiB1/8 original R16T registers/#GP/#PF/bad-pointer, Task Manager/focus/Bounce/kill; original cache functional1 and kill8.346 independent checks and 56667 cache PTE checks |
| Legacy diagnostics | PASS | Original R4 CHANNEL, R5 A.OUT, R6 FAT16, R7 CONSOLE and R8 SHELL criteria, no individual FAIL. B3 diagnostic bytes equal final/R3; no unnecessary replay after unchanged64-only code |
| Fresh final build | PASS | Current default/kernel/native shells/diagnostics/R18; all 124 R18 outputs still identical. Default disks equal B3. Explicit64 rebuild reproduces tested experimental images; hashes in evidence/final-build.json |
|64 initial build | FAIL, retained | Original32-bit cache reservation guard blocked it; not removed without reviewing/updating every mask consumer |
|64 high-slot test build 1/8 | PASS subgate | Boot hook covers0..63/deferred reclaim/generation wrap, then reserves31 lightweight low slots. Real protected jobs use high slots, slot63 keyboard/exit/kill/reuse, complete query rows,64 short child jobs and rollback.146 independent checks; AP→CPU 0 foreground migration observed |
|64 cache functional1/8, kill1/8, survivors1, fallback1 | PASS subgates | Original eight×200, all-eight kills, four surviving×1000, original thresholds. Actual upper ownership bits6/7/11/4 observed in stopped memory, zero low bits and exact recovery |
|64 injected-error accounting | FAIL |263 failed fills versus262 reader-visible errors, despite completed readers/no content errors. Strict judge remains FAIL; qualification INCOMPLETE |
|64 production qualification | INCOMPLETE / NOT ACCEPTED | Explicit R4_EXPERIMENTAL_64=1 required; source and artifacts retained for research. No64 release acceptance tag |
|32 CPU /256 jobs | REJECTED / NOT TESTED | Require separate ABI/topology/budget review, not enabled |
| Nondefault session capacities | NOT TESTED at runtime | Bounds4..20 checked; only 20-session default qualified here |

## Behavioral judges and retained failure evidence

`tests/resources/judge-high.py` validates complete captured command blocks, every expected slot, actual statuses, channel/heap/frame/session/surface recovery, cache mapping/zero-quiescence, and native compiler stages. It can independently read existing results:

```sh
python3 tests/resources/judge-high.py ../test-runs/qdos386 /tmp/default-parent.json \
  --prefix r4-default-parent- --parent
python3 tests/resources/judge-default.py ../test-runs/qdos386 /tmp/default-compatibility.json
python3 tests/resources/judge-high.py ../test-runs/qdos386 /tmp/wide-high.json \
  --prefix r4-wide-high-v2- --jobs 64 --parent
python3 tests/resources/judge-cache.py ../test-runs/qdos386 \
  build/r4/b3-export/source/build /tmp/wide-cache.json --paced-retry
```

The last command intentionally returns failure on the retained263/262 discrepancy and records the six completed subcases. It is not an accepted64 gate. No expected count, workload iteration or historical helper source was changed to manufacture a pass. All raw attempts remain under ../test-runs/qdos386/r4-*; concise hashes/snapshots are in reports/r4/evidence.

The failed eight-CPU key-transport run is retained separately from its paced retry. The received command was visibly corrupted and never executed R20K; only that failed check was retried after host pacing changed. Earlier wide PS slices ended before the final marker; the complete transcript contains all 63 rows, now checked individually. V2 CPU sampling waits beyond unchanged scheduler residency/sweep bounds. These corrections concern observation/delivery, not kernel acceptance thresholds. Other passing runs were preserved through interruptions and not restarted.

Exact normal-default 16 MiB quiescent free heap is722864 bytes at one CPU and 694080 at eight, with 2875 free application frames; all recorded fault/kill/GCC recovery snapshots return exactly. Test-only fill0 has a slightly larger linked image (722064/693280 free), and 31 holders reduce the 64 test baseline (552672/523888 free). These are distinct builds, not hidden numeric heap-limit changes. General heap still ends at 0x200000.

Private preservation checks verify unchanged R2/R3 tag/archive/manifests,124 R18 outputs, original SDK/toolchains/apps/tests, all 70 original FIX1 payloads in derivative disks (old shell bytes saved under /R3), and no tracked deletion. Only current native-shell query capacity is modified in source/apps; third-party historical application source is untouched. The private handoff's later fresh-extract smoke/patch/manifest receipt is recorded separately from its source snapshot to avoid self-referential hashes.
