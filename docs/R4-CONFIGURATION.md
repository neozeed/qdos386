# R4 independent configuration controls

`source/kernel/resource_config.h` owns compiler defaults: QDOS_CFG_MAX_JOBS=24, QDOS_CFG_MAX_CPUS=8 and QDOS_CFG_MAX_SESSIONS=20. Current Make recipes pass only explicit overrides; both current kernel and native-shell buffers consume the same header. Historical recipes retain their accepted configuration. Shell SJ_MAX=16, channel/child/session/surface quotas and cache policy do not derive from job capacity.

```sh
make production current-shell HOSTTOOLS=/data/projects/qdos386/hosttools
make production CURRENT_D=cpu1 QDOS_CFG_MAX_CPUS=1 HOSTTOOLS=/data/projects/qdos386/hosttools
make production CURRENT_D=cpu4 QDOS_CFG_MAX_CPUS=4 HOSTTOOLS=/data/projects/qdos386/hosttools
```

Use a distinct CURRENT_D for each profile; current shell outputs are source/build/CURRENT_D-shell. Kernel .pcflags and shell .flags force recompilation when options change. CPU configuration limits C context/TSS/BKL extents and startup, independently of firmware's discovered count. Discovery/failure lists remain sixteen; SMP wire formats/AUTO8 sentinel remain unchanged. One-context INFO avoids second-CPU reads; all context defaults initialize before AP startup. GDT retains spare descriptor space. Compile profiles1/4/8 have targeted runtime evidence;32 is rejected pending a separate SMP compatibility design.

Session configuration is bounded4..20 (four legacy consoles minimum, existing current capacity maximum), not implicitly expanded with jobs. The20-session default is exercised; other values are NOT runtime-qualified here. Job configurations24/64 fit current wire slot/capability encodings;256 and conflicting historical R17_JOB_MAX are rejected. Default 24 is accepted. Experimental64 requires two explicit controls:

```sh
make production current-shell CURRENT_D=jobs64 QDOS_CFG_MAX_JOBS=64 \
  R4_EXPERIMENTAL_64=1 HOSTTOOLS=/data/projects/qdos386/hosttools
```

Its wider cache ownership builds and high-slot tests run, but strict injected-accounting validation fails263 versus262; qualification is INCOMPLETE. The initial safety-guard failure and later runtime failure are both preserved, not bypassed or waived. See R4-KNOWN-LIMITATIONS.

B3 default 24 retains B2's complete slot0..23 coverage and exact resource recovery at 1/8 CPUs. Public layout probes match R3, diagnostics pass, and all 124 R18 outputs remain identical. No installed original disk is rewritten: the new test disk carries current shells and retains original shell bytes under /R3; all accepted reference images remain intact.


R4_TEST_FILL is deliberately not a production Make default: the test-only preprocessor override `R20_EXTRA=-DQDOS_R4_TEST_FILL=0` exercises all slots/deferred reclamation/tag wrap before AP startup and leaves no holders. Value31 on the 64 research build additionally reserves low slots to put real applications/cache readers above31. Default images omit this hook. Existing cache test overrides remain distinct named output profiles; no accepted thresholds/iteration counts change. Cache metadata is24 bytes at default 24 and 28 in64, both included in the original budget formula; public cache statistics stay40 words.

The source/header guard rejects64 without explicit experimental opt-in, invalid CPU/session/job bounds and conflicting historical definitions. This is a build control, not an approval flow. No accepted reference disk is rewritten by make; build a new R4 derivative volume to use the current native shells. Original apps remain installed and available.
