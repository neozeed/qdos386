# EliteQL i386 R2 — Complete 68000-to-i386 Gameplay Assembly Translation

## Autonomous builder handoff

### Mission

Continue from the **frozen EliteQL i386 R1** checkpoint: 38 native i386 routines assembled with **actual GNU GAS 1.38**, with 11 passing test suites. Finish translating the remaining original EliteQL 68000/JAS assembly into native i386 assembly, **without implementing QDOS/386 video, keyboard, audio, or a new game shell yet**.

The target is a **complete native assembly game core** whose original gameplay, state transitions, calculations and drawing algorithms are preserved, with external OS/hardware calls behind a small, documented platform boundary. This is a source translation, **not** a remake, feature redesign, 68000 emulator, or C rewrite.

### Inputs and frozen baselines

* User supplies the complete EliteQL RC6 original 68000/JAS source and the prior i386 R1 archive (including its 38 routines, tests, and coverage report).
* User also supplies the latest frozen QDOS/386 archive, preferably R14A or later, solely as an **ABI and future integration reference**. Do not modify its kernel, libraries, display subsystem, or images during this milestone.
* Branch `eliteql-i386-r2-assembly-completion` from R1; preserve R1 unchanged. Inventory archive contents, source versions, hashes and all original build dependencies before editing.
* All generated assembly must assemble with the **real GAS 1.38 i386 assembler** and link with the available historical i386 linker. Do not substitute modern GAS syntax or features without an explicit compatible fallback.
* Keep the original 68000 source and fixtures unchanged as an oracle. Do not invent missing gameplay logic or source assets.

### Phase 0 — Exhaustive assembly inventory

Build a source-to-source coverage manifest listing **every** original JAS file, routine, exported symbol, data table, global variable, interrupt/hardware dependency, and cross-file reference. For each item, record the i386 implementation location and status: `translated`, `tested`, `platform-boundary`, `not yet translated`, or `blocked`.

Confirm R1's 38 implementations against the actual source; do not assume they cover more than the existing tests prove. Identify which of the following are still missing from source: flight controls and motion, ship dynamics, orientation, projection, tactical AI, combat, lasers/missiles, shields/energy, docking, hyperspace, procedural generation, random number sequence, trade and commander progression, drawing primitives, status/dashboard calculations, and program initialization. Use only categories actually present in the original source.

Audit QL MODE 8 **as implemented by this EliteQL source**, rather than assuming the game uses every colour available to the hardware. The user reports a monochrome presentation, a 256×256 MODE 8 screen, a 256×192 flight viewport and a 64-line dashboard. Preserve the actual rendering and palette choices of the source.

**Gate 0:** a complete inventory and no unclassified original routines or data objects.

### Stage 1 — Complete the gameplay and simulation core

Translate all remaining gameplay and simulation routines from 68000/JAS to i386 GAS 1.38: flight, movement, controls, tactical decisions, combat, ship interactions, state progression and any other original core logic. Preserve algorithm order and exact observable behavior. Do not simplify because the 386 is faster.

Explicitly audit:

* 8/16/32-bit signed and unsigned operations, truncation, wraparound, sign extension and byte order.
* 68000 CCR carry/overflow/zero/negative behavior versus x86 EFLAGS, especially multiword arithmetic and branch conditions.
* Arithmetic shifts, rotates, multiply/divide, fixed-point scaling and division rounding.
* Register conventions, memory aliasing, stack frames and static data placement.
* Original PRNG seed, update order and call sequence; do not replace it with `rand()`.
* Original object, commander, equipment and universe layouts. Define explicit serialization where original on-disk formats depend on big-endian words.

Use native i386 instructions and a documented internal calling convention. Avoid an interpretive 68000 CPU loop or opaque 68k instruction wrappers masquerading as a port. Small flag-equivalence helpers/macros are acceptable when justified and covered by tests.

**Gate 1:** all non-rendering gameplay routines translated and individually linkable, with differential tests against original 68000 outputs for deterministic inputs.

### Stage 2 — Translate rendering *algorithms*, not the hardware backend

Port the original wireframe/projection, geometry cache, clipping, XOR line operations, screen-coordinate conversion, cockpit/dashboard state and any source-defined sprite or bitmap routines. Preserve their logic and calling relationships.

For hardware-facing drawing, use a minimal **memory-backed test surface** or original packed MODE 8 buffer and documented platform entry points; do not introduce QDOS/386 graphics APIs yet. Keep the original QL packed pixel representation and bit/colour decoding where needed. The game should be able to generate a frame into test memory without owning VGA or making QDOS calls.

Compare line endpoints, clipped geometry, XOR draw/erase reversibility, projected object coordinates and full framebuffer hashes against the original QL execution where feasible. A screen capture comparison must account for the original MODE 8 packing and presentation; do not confuse different display scalers with algorithmic differences.

**Gate 2:** all original drawing-related assembly translated, with pixel/geometry tests, while physical video remains intentionally unimplemented.

### Stage 3 — Finish the game control flow and isolate platform services

Translate the remaining original startup, main loop, menus, title, status updates, game transitions and shutdown logic. Replace only **actual external dependencies** with narrow, named platform calls such as input polling, time/tick, present frame, sound output, filesystem, and program exit. Derive the required calls from the original source, not from speculative features.

Use a deterministic headless driver to supply recorded inputs and ticks and collect game state and frames. It may be written in host C89 for testing; **gameplay logic must stay in translated i386 assembly**. Any temporarily unavailable platform call must have an explicit unsupported result or test-only mock, never a silent success stub that makes the game appear functional.

Preserve original game sequencing. In particular, do not add new ships, HUD features, difficulty settings, graphics, controls, audio behavior or physics.

**Gate 3:** the entire original game control flow is represented in native i386 assembly and links with a headless platform adapter. Every original symbol is either translated or deliberately classified as a hardware/OS boundary.

### Stage 4 — Differential behavioral verification

Build an independent original-68000 reference oracle using the existing QL build/QLBox when available. Compare deterministic test scenarios with the translated i386 core: initialization, commander data, galaxy generation, movement, hyperspace, docking, trade, equipment, combat, AI, damage/death, drawing and return to title. Use fixed seeds, controlled inputs and controlled tick sequences; record the exact fixtures.

Where an original run is not possible, use static source-derived invariants and mark the test `NOT VERIFIED`, not `PASS`. Preserve R1's 11 suites, add targeted suites for every new subsystem, and use a coverage report linking tests back to original routine names.

Include adversarial numerical boundaries, signedness cases, overflow, PRNG progression and XOR draw/erase. Check repeated runs and deterministic output hashes. No gameplay feature may be declared complete merely because the source assembles.

**Gate 4:** all available differential suites pass; any remaining source or runtime gaps are explicitly named and evidenced.

### Stage 5 — Assembly-only release and next-stage interface contract

Package a reproducible R2 archive with complete original and translated sources (subject to redistribution rights), Makefile, GAS 1.38 build instructions, linker script, headless runner, test fixtures, oracle comparisons, symbol/coverage inventory, ABI contract and exact SHA256SUMS. Build from fresh extraction, verify deterministic output where possible and retain all R1 tests.

Required documents:

* `docs/R2-SOURCE-COVERAGE.md` — original files, routines, symbols and port status.
* `docs/R2-I386-CALLING-CONVENTION.md` — registers, stack, flags, data layout and exported entry points.
* `docs/R2-PLATFORM-BOUNDARY.md` — **only** the external services actually required for subsequent QDOS/386 integration.
* `docs/R2-DIFFERENTIAL-TESTS.md` — fixtures, oracles, hashes and observed differences.
* `docs/R2-KNOWN-LIMITATIONS.md` — exact unfinished or unverified behavior.
* `docs/R2-RUNTIME-REPORT.md` — build evidence and truthful gate results.

Suggested package: `ELITEQL-I386-R2-COMPLETE-ASSEMBLY.tar.gz` with SHA256 and a frozen source commit.

### Explicitly out of scope for R2

* No QDOS/386 kernel changes, native graphics calls, keyboard integration, sound driver or OS launch packaging.
* No new rendering mode, palette enhancements, mouse controls or gameplay features.
* No reimplementation in C, no 68000 emulation, no new game engine and no port from a different Elite source tree.
* Do not claim a fully playable QDOS/386 application until the later integration stage actually runs it.

### Follow-on R3 (only after R2 is frozen)

Connect the verified assembly core to QDOS/386's existing R13A/R13B/R14 graphics, keyboard, clock, file and process APIs. Prefer an original 256×256 packed QL MODE 8 compatibility buffer, converted by a thin presentation adapter to a private QDOS/386 indexed-colour surface. Preserve monochrome presentation if that is what the source draws. Let Task Manager own focus and switching. Implement no new gameplay. The R2 platform-boundary document should make R3 a bounded integration effort rather than a second assembly translation.

### Autonomy

Proceed through the phases without routine approval checkpoints. Commit runnable, testable checkpoints. If a reference routine or dependency is missing, investigate once, record the precise blocker and continue independent work; **do not fabricate replacement behavior**. Prefer a complete, honestly verified assembly core over a superficially playable approximation. Do not run multi-hour OS regressions or native guest GCC builds for this assembly-only milestone.
