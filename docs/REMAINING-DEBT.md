# Deferred kernel work and qualification limits

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

R3 establishes a current composition and removes selected obsolete branches. It does not finish every historical conditional. The exact selected groups and retained configurations are in CANONICAL-CONDITIONALS; original audit/report evidence remains under reports/r3.

- JOB_MAX=24 shares one allocator among ordinary jobs, SYSTEM, shell, BSP/AP idles and IOW0. There is no reserved sixteen-plus-eight partition. Increasing CPUs consumes job slots.
- VCON_JOBS=16 still disagrees with the job namespace. Jobs allocated above the console bookkeeping bound can fail to receive expected console state. This needs a dedicated functional fix and high-slot tests, not a changed constant during cleanup.
- JC_MAXJOBS, TASKS v2 records and SJ_MAX are independent sixteen-entry collections with different purposes. Shell enumeration/history and child limits are not a global job limit. PROC_CHILD_LIMIT=8, CREC_MAX=32, sessions=20, graphics=4 and channels=16 remain unchanged.
- The general heap still ends at 0x200000 and depends on linked BSS. Page-backed cache fixes its previous dominant allocation but does not remove fragmentation, kernel-stack pressure or other heap constraints.
- Cache backing is contiguous physical memory above 8 MiB reserved before ordinary CR3 construction/AP startup. It is capped at 2 MiB including metadata/page rounding; fragmentation/firmware holes can force reduction or synchronous fallback. No runtime shrink, memory hotplug or remapping is provided.
- Cache retry, reservation and request bounds remain 64/4/8. Injected returned errors are not real IDE timing qualification; cold streaming and the previously recorded superseded-retry observations remain separate performance/liveness research.
- Remaining release flags often select real paging, loader, console, surface, trap/context and shell behavior across compatibility builds. They require per-consumer evidence before further canonicalization. Historical release recipes are preserved, but only current production, the explicit legacy diagnostic profile and frozen R18 were rebuilt in this continuation; arbitrary older recipes may need their tagged sources.
- The diagnostic profile intentionally uses the original nonpaged single-BSP six-slot harness. Its isolated trusted-test buffer bridge does not qualify protected/SMP userland. Historical test sources/criteria remain unchanged; old stalled/failed diagnostic runs are retained as counterexamples.
- Some historical comments still describe an older backend (for example proc.h's CPL0 wording and bkl.h's xchg acquisition). Current implementation and contract docs supersede those descriptions; broad comment churn was avoided.
- Provenance not established by existing Git evidence is UNKNOWN in SOURCE-INVENTORY. Importer lists describe linked global-symbol dependencies, not every runtime callback edge. No invented original author/license/history is supplied.
- Verification is bounded QEMU TCG on 1/8 CPUs, not KVM, physical hardware, long gameplay/soak or arbitrary firmware. The context capacity remains eight. Proposed 32-way/256-job, new APIs/storage/display and scheduler redesign are future work.

Private application sources, binaries, ROMs, assets, fixtures and reference archives are retained. Their presence supports internal research and regression testing; it does not grant public redistribution permission. No application rewrite or sanitization is part of R3.
