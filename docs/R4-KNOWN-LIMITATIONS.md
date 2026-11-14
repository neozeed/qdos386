# R4 known limitations and acceptance boundaries

The accepted deliverable is the validated 24-slot consistency/configuration fix. Default remains24 jobs,8 CPU contexts and 20 sessions. CPU 1/4 have targeted boot/GCC tests; the complete default 1/8 workload coverage is broader. Other session values4..19 have compile-time bounds but are NOT runtime-qualified in this milestone. CPU 32 and 256 jobs are deliberately rejected and NOT TESTED.

## Experimental64 is incomplete

The first64 build correctly failed the original r20w reservation-mask guard. Allocator death ownership and cache reader ownership are now independently represented for64 in a strictly opt-in candidate. Build requires both QDOS_CFG_MAX_JOBS=64 and R4_EXPERIMENTAL_64=1. It is not the default and has no acceptance tag.

The candidate passes real slot63 session/input/exit/kill/reuse at 1/8 CPUs;64 short child jobs, child quota/ownership/rollback; original eight-reader200-operation functional tests at 1/8, all-eight kills at 1/8, four1000-operation survivors, and reservation fallback. Stopped-guest memory shows upper reservation bits while low-word bits remain zero. Exact heap/frame/session/surface/channel recovery holds in those accepted subchecks.

**Unresolved strict gate:** `r4-wide-injected-1` completes all eight readers ×200 operations with zero short/mismatch/giveup and CACHE8 I PASS, but totals263 failed fills versus262 reader-visible errors. The retained R3-derived strict equality judge fails. Root cause is not established; do not attribute it to the widened mask, allocator or prior cache behavior without evidence. No threshold is relaxed and no failure is relabelled PASS. Stop64 qualification here and investigate separately if authorized. `reports/r4/evidence/wide-cache.json` remains status FAIL with six completed cases and the exact failed record/hashes.

Minimal replay (new output path required; existing image/toolchain references are in the internal handoff):

```sh
make production CURRENT_D=slots64-injected QDOS_CFG_MAX_JOBS=64 \
  R4_EXPERIMENTAL_64=1 \
  R20_EXTRA='-DQDOS_R4_TEST_FILL=31 -DR20C_TEST_ENT=8 -DR20C_INJECT_EVERY=7' \
  HOSTTOOLS=/data/projects/qdos386/hosttools
python3 scripts/investigate-memory.py \
  source/build/slots64-injected/pc-vga/qdos386-vga.img \
  build/r4/wide-test-v2.img /tmp/qdos386-r4-injected-replay \
  --smp 1 --memory 16 --scenario A --steps tests/cache/r20-pages/injected.json \
  --aout source/build/slots64-injected/pc-vga/kernel.aout --timeout 300
```

This records a new attempt, not a promise that a scheduling-dependent counter discrepancy repeats identically every time. Rebuild the private volume using tests/resources/build-volume.py, existing cache fixture/SDK stage and current64 shells if the referenced local image is unavailable. Do not overwrite original disks or edit historical reader algorithms/counts.

## Other retained limits

- Twenty-four slots share SYSTEM, shell, BSP/AP idle and IOW0; there is no16+8 reservation partition. Actual free user slots depend on CPU count and live jobs.
- General heap still ends at 0x200000. Metadata/kernel-entry stacks consume it independently of guest RAM;64 table capacity never guarantees64 simultaneously resident protected processes. The64 coverage kernel reserves31 lightweight4 KiB holders to exercise high indices; it is not a production capacity benchmark.
- Channels16, per-parent children8/global retained records32, sessions20, surfaces4, shell history16 and SDK job-helper8 records remain separate limits. Old installed shells still have their old query buffers; use the explicit current-shell build/new derivative disk for full R4 visibility.
- Frame/paging/cache physical constraints and 5%/2 MiB policy remain. No runtime cache shrink, RAM hotplug, DMA, filesystem/display/API or scheduler redesign is included.
- Stale generation protection is finite. Forced boundary testing is test-only; no application ID bit allocation is changed.
- Firmware lists remain16 and execution capacity is at most8. Tested flat Pentium topology has no proven SMT siblings; physical HT parking on real SMT hardware/KVM is NOT TESTED. No hardware/soak/long-gameplay qualification is claimed.
- One eight-CPU cache-kill attempt typed a corrupted command under loaded TCG. Its original failure remains. Only that run was retried with 0.1-second host key pacing, unchanged workload and criteria, and passed. No kernel input policy changed.
- An early CPU sample preceded the original residency/sweep interval. V2 waits beyond the unchanged25/50 tick limits and records AP→CPU 0 foreground migration. Wide PS per-command slices ended early; the judge verifies all records/final marker in the complete captured command block, and future recipes wait for the final marker.

The original private laboratory, accepted R2/R3/R18 artifacts and histories remain intact. Source archives are tracked snapshots, not full workspace backups, and are not authorized public releases.
