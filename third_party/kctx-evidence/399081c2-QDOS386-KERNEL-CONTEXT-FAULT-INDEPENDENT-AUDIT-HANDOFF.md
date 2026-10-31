# QDOS/386 — Independent Scheduler Context-Fault Audit and Corrective Release

## Handoff to the QDOS/386 OS builder (after Hack milestone)

**Objective:** Independently determine whether the current QDOS/386 kernel loses bits from an interrupted process's stack pointer, fix the OS if and only if confirmed, and provide a verified kernel/image back to the GW-BASIC builder. This is a **kernel ownership** task, not a request to alter GW-BASIC or its language semantics. Do not modify any frozen R13B/R14 release in place.

### Supplied evidence

- `FINDINGS-stack-corruption.md` — detailed analysis, source locations, controls and unresolved questions.
- `failing-checkpoint.zip` — frozen `GWBASIC.AOU`, FAT launch volume, QEMU harness, source hashes and failing kernel image if included; inspect actual archive inventory rather than assume every file is present.
- `gwbasic32-source-and-findings-2026-10-30.zip` — GW-BASIC translation and test reproducers. Use as evidence, not as the target of changes.
- Frozen R13B reference: commit `d9721897b4262a96a27e422e74c89cb5b428f06e`; current OS development baseline should be the latest frozen R14 or Hack successor, **not automatically R13B**.

Reported failure: QEMU timer preemption of an i386 process with ESP congruent to 2 mod 4 leads to `#DF`, sometimes EIP `0x00000024`, ESP `0x847a0000`. An independent `spin_misaligned.S` triggers it; `spin_aligned.S` exits cleanly. The report identifies `kernel/kctx.s` `resume_dispatch`, where format information is encoded in the low bits of a saved ESP and `andl $-4` discards bit 1. **Treat this as a hypothesis to verify against the actual linked kernel.** The report explicitly distinguishes `kctx.s` (R13B PC build) from `context.s` and `paging.s` (other build variants).

### Stage 0 — Freeze and verify provenance

1. Finish/freeze the current Hack milestone before beginning this branch. Create `kernel-context-fix` from the newest appropriate OS source; record exact commit, image hashes, build flags, link map and which context-switch object the PC/QEMU kernel actually links.
2. Read the three supplied artifacts without executing untrusted prebuilt programs outside the isolated guest. Verify hashes where available. Preserve the original failing checkpoint, baseline binaries and untouched reference archives.
3. Compare R13B and current kernel context-save/resume implementations to establish whether the defect was already corrected. Search **all** scheduler return paths, IRQ, voluntary yield, exception, initial context and task termination, not just one mask.
4. Record whether an alignment requirement is documented as an actual supported application ABI; distinguish ABI violation from a destructive kernel resume bug. Note that x86 permits unaligned stack pointers for many instructions, and the OS should not silently corrupt an interrupt frame.

### Stage 1 — Independent reproduction on the latest kernel

1. Run the **BASIC-independent** aligned and misaligned assembly controls on QEMU with the latest frozen kernel. Use bounded runs and retain console, QEMU interrupt logs and stack/context dumps. Verify that the controls really hold the intended ESP alignment during preemption.
2. If current kernel passes both, confirm by source/assembly inspection and repeated preemptions; document the precise earlier fix, commit and image identity. Do not make an unnecessary kernel change. Hand the already-correct kernel/image to GW-BASIC builder for its independent test.
3. If the misaligned control fails, locate the first corrupted saved/resumed value, with original SP, encoded context value, decoded SP, frame format, CR3, EIP and error path. Establish whether it occurs only on timer IRQ or also yield/other context transitions.
4. Do not infer the cause solely from `#DF`: preserve a before/after trace of the first divergence. Keep aligned control as a mandatory regression.

### Stage 2 — Fix the OS, not the interpreter (if confirmed)

Choose the smallest **correct and maintainable** kernel repair after auditing the full ABI:

- A mask change from `andl $-4` to `andl $-2` is a **candidate**, not an automatically approved permanent solution. It preserves ESP bit 1 but still requires even stack pointers and correct format encoding.
- Prefer a representation that preserves the **full stack pointer** separately from frame-format metadata if practical. However, a global `resume_fmt` may be unsafe across interrupts/reentrancy or context switches; audit ownership and all paths before adopting it. Per-context metadata or an explicit result structure may be safer.
- Preserve register/flags state and correct `popal`/`iret` and voluntary-yield restore paths. Audit stack guard/canary accounting, exception handling, CR3 transitions and the software kernel-entry stack switching.
- Update all **applicable** context implementations consistently, but don't change unrelated legacy images accidentally. Maintain binary reproducibility of frozen releases.
- Add a deterministic or high-frequency preemption test that covers ESP residues 0, 1, 2 and 3 mod 4. If some residues are intentionally unsupported, reject them **explicitly and safely**; never silently round the pointer down. Test both interrupt-frame and noninterrupt-frame resumes.

### Stage 3 — Verification and compatibility

Run in this order:

1. Aligned and misaligned standalone controls repeatedly; verify both exit normally for supported ESP values.
2. Original frozen `GWBASIC.AOU` on the same FAT volume: flat FOR/NEXT 16,000, 16,400, 20,000 and 200,000; nested 100×1,000; GOSUB 20,000; SAVE/LOAD; corrected `A$` string test. Record any remaining interpreter failures separately; **do not edit GW-BASIC**.
3. Existing OS scheduler/process tests: multiple concurrent apps, timer preemption, private heaps/stacks, fault isolation, kill/wait, dynamic shells, graphical sessions and Task Manager. Ensure a fault in one app does not halt the kernel.
4. Targeted R14 QLTerm serial/background transfer and Hack smoke, R13B RTC/SYNC, and R13A display regression. Run QEMU primary and one bounded full-machine Tiny386 smoke. No CPU-only i386VM or guest GCC endurance runs.
5. Verify FAT/VFAT integrity and frame/heap/job accounting. Report every FAIL, NA and NOT TESTED; do not weaken acceptance checks.

### Stage 4 — Deliver the corrected kernel to GW-BASIC builder

Produce a separate, clearly named corrective release, e.g. `qdos386-KCTX-FIX1.tar.gz`, with full source, minimal patch, image(s), link maps, `SHA256SUMS`, reproducible build commands, QEMU traces and independent controls. Record branch/commit and archive SHA256. Fresh-extract, verify and smoke-test. If latest kernel was already correct, deliver a **no-change audit** with the verified kernel image/hash instead of a fictitious fix.

Provide a small handback note containing: exact QEMU command, kernel and FAT image hashes, `GWBASIC.AOU` hash, tested controls, results and any remaining issue. The GW-BASIC builder then retests its **unchanged** executable against the corrected/current kernel and separately continues interpreter-specific work. Do not claim GW-BASIC is fully correct merely because the double fault disappears.

### Stop conditions and boundaries

Proceed autonomously after the Hack release is frozen. Stop and report a concrete blocker if an attempted context fix introduces corruption, changes frozen builds unexpectedly or cannot preserve all scheduler paths. Keep a runnable checkpoint. **No speculative gameplay, no GW-BASIC source fixes, no SMP, no ring-3 redesign and no unrelated feature additions.** The OS builder owns kernel faults; the GW-BASIC builder owns language/interpreter faults.
