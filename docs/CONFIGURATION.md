# Current configurations and independent bounds

> Preserved R3 baseline document. For current R4 resource changes and profile status, see [R4 configuration](R4-CONFIGURATION.md), [ABI review](R4-ABI-REVIEW.md) and [known limitations](R4-KNOWN-LIMITATIONS.md). The R3 text below is retained for provenance.

`source/mk/current.mk` explicitly lists current production flags and ordered extra objects. It no longer derives the production graph through the historical R11→R20 Make-variable ladder. Compatibility recipes remain preserved separately. Production main selects shell boot directly; R7_SHELL_MODE remains only where the old compatibility entry still needs it. Current options are described by behavior rather than assumed obsolete from their spelling.

| Choice | Meaning |
|---|---|
| production | Current protected/native shell, foreground-first SMP, storage worker/cache and display services; no legacy test harness |
| diagnostic | Original legacy nonpaged, one-BSP kernel tests and FAT fixture; trusted kernel-test bridge only; no normal shell/AP/IOW0 boot |
| QDOS_CON_VGA | VGA presenter versus serial/debug/headless build; both production images built by default |
| CURRENT_D | Separate production output directory, default canonical; use distinct directories for test variants |
| R20_EXTRA | Existing bounded test overrides such as R20C_TEST_ENT=8, R20_RSV_MAX=1, R20C_INJECT_EVERY=7 or R20C_TEST_INIT_FAIL |
| HOSTTOOLS / I386VM / QDOS386_TOOLS | Explicit paths to unchanged historical compiler/assembler/linker/converter inputs |

Cumulative feature markers still consumed by shared production/legacy code remain explicit. Genuine filesystem, paging, serial, title/session, font/input and test-injection decisions are not erased. The canonical conditional ledger records exactly which selected branches were resolved and which remain. An old spelling alone is not proof a flag is removable.

## Bounds are separate contracts

| Bound | Current production value | Domain |
|---|---:|---|
| JOB_MAX / R17_JOB_MAX | 24 | Entire job-slot namespace, including SYSTEM, shell, BSP/AP idles and IOW0; no 16+8 partition |
| KX_NCPU | 8 | CPU context capacity; runtime online count depends on discovery/startup |
| VCON_JOBS | 16 | Per-job console bookkeeping; known high-slot mismatch remains |
| JC_MAXJOBS | 16 | Existing rich-shell v1 query/kill collection |
| TASKS v2 records | 16 | Separate shell enumeration buffer |
| SJ_MAX | 16 | Rolling shell launch history, not live jobs |
| PROC_CHILD_LIMIT / CREC_MAX | 8 / 32 | Unreaped children per parent / global retained spawn records |
| VSESS_MAX / VCON_MAX | 20 / 4 | Session handles / original console subset |
| CHAN_MAX / SURF_GFX_MAX | 16 / 4 | Channels / simultaneous graphical surfaces |
| R20_NQ / R20_RSV_MAX / R20_MAX_MISS | 8 / 4 / 64 | Worker requests / per-job reserved sectors / consecutive restart bound |
| Cache physical backing | 2 MiB maximum | Payload + metadata + page rounding, zero general-heap array allocation |
| Physical ceiling / private tables | 256 MiB / 32 | Identity-mapped frame namespace / page tables per current process |
| Process heap / user stack / kernel stack | 64 MiB virtual / 64 KiB / 16 KiB | Separate per-process resources; physical frames remain the limit |

The diagnostic profile retains its historical JOB_MAX=6 and single BSP; it is not a change to production's 24-slot/eight-context limits. Its old backend declarations do not qualify multi-CPU diagnostic execution. No numeric bound is changed by R3.

Future 1/4/8/32-way models, 256 jobs, unified limits or an enforced slot partition are proposals only. Four-vCPU boot may be an ordinary runtime count within eight, but no expansion beyond the existing capacity is implemented. Do not derive one quota from another merely because numbers match.
