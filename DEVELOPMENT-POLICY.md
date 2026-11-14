# QDOS/386 — Private Development Policy

**Repository-root policy for humans and development agents**

## Purpose and precedence

This repository is a **private research, development, compatibility and regression-testing environment** for QDOS/386 and related historical computing projects. The default activity is internal engineering, **not public redistribution**. Read this file before modifying source, generating packages or reorganizing directories.

This policy does **not** grant redistribution rights for third-party material. Specific task instructions may narrow the scope of a change; they must not be interpreted as permission to delete or publish private materials. Ask for explicit approval before destructive or external publication actions.

## Preserve the development laboratory

- Retain existing source, binaries, ROMs, reference images, historical tools, SDKs, application assets, fixtures and regression data, including SuperBASIC, Elite, Hack, GW-BASIC, Infocom interpreters/stories, QLTerm, GCC/GAS/LD, QLBox and related work where present.
- Never delete, disable, substitute, sanitize or exclude existing historical software merely because it is old, copyrighted, third-party or unsuitable for public distribution.
- Do not silently replace real application regressions with synthetic tests. Synthetic tests may supplement real workloads.
- A source-cleanup assignment does **not** authorize changes to historical application source or data.
- Tracked-source archives are not complete backups of ignored, untracked, external or mounted material. Record the difference and preserve the originals.

## Private versus public outputs

- Internal handoffs may retain the complete private development tree for local use. Do not publish or upload restricted third-party materials to public repositories, releases or distribution services without appropriate permission.
- Create a **separate public-export process only when explicitly requested**. Use a reviewed exclusion manifest in a new destination; never sanitize the original private working tree in place.
- Never assume an internal archive is authorized for public release. Label it **PRIVATE — INTERNAL DEVELOPMENT**.
- Keep secrets, credentials, private keys, personal data and tokens out of source archives and logs. Their presence in a private tree does not authorize replication.

## Source, history and provenance

- Git tags and commits are the authority for historical source identity. Do not rewrite, move or delete accepted release tags or branches.
- Before work, record current branch/commit, working-tree status, build tools, supported build command and relevant artifact hashes.
- Use a new branch for each milestone. Prefer small reviewable commits with explicit scope and evidence. Do not push or merge unless authorized.
- Per-file history/provenance belongs in documentation or comments, not generated revision strings embedded into executables. For uncertain introducing/last-functional-change history use `UNKNOWN` and `git log --follow` evidence.
- Keep a canonical production build and a separately runnable diagnostic build. Do not silently fold legacy test code into production or discard it.

## Compatibility and acceptance

- Preserve existing application ABI, syscall numbers, calling conventions, memory/protection invariants, process ownership and filesystem semantics unless a task explicitly authorizes a reviewed change.
- Source-only changes should aim for byte-identical outputs. When the task explicitly allows intentional composition/layout changes, record new hashes and demonstrate **functional equivalence**; byte identity is then not the sole acceptance criterion.
- Never alter test thresholds, expected failures or fixtures merely to manufacture a pass. Distinguish PASS, FAIL, EXPECTED FAILURE, NOT TESTED and INCOMPLETE.
- Test both normal operation and negative paths, including high-slot process lifecycle, resource exhaustion, cancellation, concurrent workloads and fault containment when relevant.
- Prefer short, representative QEMU checks and focused regressions. Do not automatically rerun multi-hour suites or generate enormous log archives. Retain concise reproducible evidence and first-failure traces.

## Resource and architecture boundaries

- Keep job slots, CPU contexts, sessions, consoles, channels, child records, surfaces and shell history as **separate resource domains**. Equal numeric constants do not imply shared ownership.
- The kernel is a **foreground-first, single-user personal-computer OS**. Avoid introducing an NT-style object manager, a general microkernel or a server-first architecture without explicit authorization.
- Favor clear subsystem ownership and bounded queues/worker contexts for naturally serialized hardware, while retaining proven fallbacks and preserving historical releases.
- No new feature, scheduler redesign, storage format, graphics mode or capacity increase is implicit in a cleanup request.

## Working procedures for agents

1. Read this policy and the active milestone handoff; inspect the current source before deciding how to change it.
2. Verify actual repository identities and file availability. Do not infer that a tarball, toolchain, ROM or disk image exists from a report alone.
3. Make the smallest auditable change and test it before expanding scope.
4. Continue through approved checkpoints autonomously; stop only for a genuine safety/ABI blocker, explicit acceptance boundary or exhausted execution resources.
5. Preserve failed attempts and explain them; do not disguise a defect as a test issue without evidence.
6. Deliver exact commit/tag/hash identities, build and test instructions, PASS/FAIL/NOT TESTED matrix, known limitations and a concise handover.
7. Never delete untracked/private resources or run destructive cleanup without explicit authorization.

## Standing instruction

**Preserve the private historical computing laboratory. Improve QDOS/386 without erasing its evidence, applications or provenance. Public distribution is a separate, explicitly authorized activity.**
