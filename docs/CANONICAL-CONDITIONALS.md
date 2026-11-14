# Canonical conditional ledger

This ledger covers the continuation after accepted Gate 1. The original 800-site/221-configuration audit remains in reports/r3 and is not a claim that every site was removed. Each group below was reviewed independently, built in production serial/VGA and explicit legacy diagnostics, and exercised before its source commit. All 53 selected C decisions preserve the post-Gate-2 images byte for byte. Semantic token comparisons ignore line markers/whitespace while retaining literals and punctuators.

| Group / old selector | Selected current behavior and location | Configurations and proof | Commit |
|---|---|---|---|
| R7_SHELL_MODE / entry | kernel/main.c now calls shell_boot unconditionally; intact original harness is diagnostics/legacy/main.c, explicitly selected by Make | Production excludes test objects and includes; diagnostic executes all original criteria. Service object equality and removed symbols in gate2.json | 3c60e1b |
| R11→R20 Make ladder | source/mk/current.mk lists the exact current flags, extra objects and kctx17 backend in original order; compatibility ladder retained | Both current images and diagnostic identical to Gate 2, focused GCC/HI and diagnostics pass; group-config.json | f6b6cd5 |
| bkl: QDOS_R16B, QDOS_R17 (5 decisions) | Recursive owner-checked eight-context CAS acquisition; omit obsolete two-CPU lending; source/kernel/bkl.c | Current serial/VGA token equality; full builds and focused production/full diagnostic execution; evidence/group-bkl.json; R18 also rebuilt identically | 0cc93a2 |
| graphics: QDOS_R13A (4 decisions) | INDEX8 operations and current VGA planar/session-view presenter; source/kernel/gsurf.c | Current serial/VGA token equality; full builds and focused production/full diagnostic execution; evidence/group-graphics.json; R18 also rebuilt identically | bcfdc0a |
| graphics: QDOS_R18 (9 decisions) | INDEX8 operations and current VGA planar/session-view presenter; source/platform/pc/vgapres13a.c | Current serial/VGA token equality; full builds and focused production/full diagnostic execution; evidence/group-graphics.json; R18 also rebuilt identically | bcfdc0a |
| process: QDOS_R16, QDOS_R16B, QDOS_R17 (17 decisions) | Protected private address spaces, guarded user/kernel stacks, eight-context CR3/entry handling; source/kernel/proc.c | Current serial/VGA token equality; full builds and focused production/full diagnostic execution; evidence/group-process.json; R18 also rebuilt identically | 176952b |
| taskmgr: QDOS_R13B, QDOS_R16, QDOS_R17, QDOS_R18 (18 decisions) | Current protected-process/memory rows, eight-CPU scheduler information and foreground/session overlay; source/kernel/taskmgr.c | Current serial/VGA token equality; full builds and focused production/full diagnostic execution; evidence/group-taskmgr.json; R18 also rebuilt identically | a553f77 |

The exact select/omit decision sequence is preserved in `reports/r3-canonical/evidence/selected-decisions.json`. No global macro substitution was performed. The five C files are current service owners; the legacy diagnostic composition does not link them. Frozen R18 selects the same resolved implementations and all 124 outputs remain identical.

## Retained choices and scope

| Class | Retained examples / reason |
|---|---|
| Genuine current platform/feature | QDOS_CON_VGA and platform paths; filesystem writes/VFAT, paging/dynamic process, console/job-control/environment/title/sync/surface/input support are selected in current.mk and still consumed by shared source |
| Diagnostic-only | PC_COMPOSITION selects entry, objects, headers and QDOS_LEGACY_DIAGNOSTIC bridge. Direct user-range refusal checks remain strict; no production pointer-check change |
| Hardware/CPU/context | CPU discovery/startup/APIC and kctx17 are real mechanisms; current KX_NCPU=8. Meaningful UART/platform selection and noncurrent backends are preserved |
| Test injection | R20C_TEST_ENT, R20_RSV_MAX override, R20C_INJECT_EVERY and R20C_TEST_INIT_FAIL remain isolated explicit test variants. No bounds changed in production |
| Shared historical selectors | Remaining release-named guards in jobs, trap/context, channel, loader, paging, console and other services still differentiate genuine compatibility/test paths. Canonicalization is deferred pending equally narrow per-consumer proof |

R11M_SHELL/R11M_JOBS/R12A_SHELL/R13B_DYN and related historical SDK/shell recipes are not casually deleted: current production uses its established userland and current feature flags; the compatibility recipes and original sources remain. R7 semantics were accounted for at the composition boundary, not removed from the legacy entry. No language modernization, global relocation, application edit, trap/record change or numeric-bound expansion is included.

## Configuration capture

`reports/r3-canonical/evidence/compile-configurations.json` captures exact commands, flags, compiler effective macros and ordered link commands from the fresh build. ABI comparison records identical public constants and seven layout probes against Gate 1; the new internal CHAN_SERVICE_RANGE_OK alias is separately identified. Earlier Gate 1 preprocessed-byte identity remains its own historical acceptance, not the criterion for this phase.

Remaining work is explicitly tracked in REMAINING-DEBT. This is a completed selected cleanup, not a claim that all historical scaffolding or arbitrary old release recipes are now canonical.
