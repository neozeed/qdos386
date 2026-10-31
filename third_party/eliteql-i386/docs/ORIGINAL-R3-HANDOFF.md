# EliteQL i386 R3 — Complete the Original Assembly and Integrate QDOS/386
## Autonomous builder handoff · 30 October 2026

### Mission

Resume **the actual EliteQL i386 R2 work-in-progress checkpoint**, finish its untranslated original 68000 `shell.s` in **native i386 GAS 1.38**, then connect the **complete, faithful game** to the already-existing QDOS/386 graphical surface and keyboard interfaces. No new gameplay, rewritten game loop, substitute C game engine, fictional screens, new operating-system APIs, or kernel changes.

**Critical sequencing:** complete and independently verify original assembly control flow first; only then connect video, input, time, file I/O and optional sound. A linkable headless executable that immediately returns is **not** a completed game.

### Inputs and frozen baselines

- `ELITEQL-I386-R2-WORK-IN-PROGRESS.tar.gz`: resume from its existing source, Makefile, fixture corpus, reference original 68000 sources, `CHECKPOINT.bundle`, `docs/R2-NEXT-WORK.md`, `docs/R2-PLATFORM-BOUNDARY.md`, `docs/R2-SOURCE-COVERAGE.md`, `docs/R2-DIFFERENTIAL-TESTS.md`, `docs/R2-KNOWN-LIMITATIONS.md`. Do not discard or regenerate the already verified R2 core.
- Latest corrected QDOS/386 OS: `qdos386-KCTX-FIX2-MAINLINE.tar.gz`, SHA256 `778a3d612d924a06f17bd8c73fb910406768264a46fdb7391bc82c36db3ed804`, tag `kctx-fix2-mainline`, source commit `b1db28a`, verification commit `5b6fc47`. Normal QEMU kernel `kernels/qdos386-vga.img` SHA256 `f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`.
- KCTX-FIX2 contains the R13A/R13B graphics, indexed surfaces, keyboard-event, process and filesystem APIs and R14 serial/QL compatibility facilities. **Read the actual headers, SDK and ABI docs**; do not infer API names or structures from prior conversations.
- The original EliteQL RC6 reference source in the R2 package is authoritative for mechanics and presentation. The source's MODE 8 display is **256×256, 32,768 bytes, 128 bytes/row**, with flight viewport **256×192** and dashboard below. The currently ported QL version may render monochrome despite MODE 8's palette capability; preserve actual source behavior.
- Branch separately from R2 (e.g. `elite386-r3-shell-qdos`), freeze the R2 checkpoint. Preserve the latest QDOS/386 kernel completely unchanged. Use actual GAS 1.38 and historical i386 linker. No guest GCC builds as routine tests.

### Phase 0 — Intake, audit and measurable inventory

1. Verify archives, source hashes, existing R2 `make check`, `make oracle-check`, `make platform-check`, and historical GAS 1.38 assembly/link.
2. Inventory **every** remaining routine/label in `reference/src/shell.s` (R2 notes say approximately 6,666 lines). This is substantial gameplay and UI control logic, **not merely a QDOS adapter**.
3. Map its entry points, data objects, cross-module references, startup, menus, flight loop, death/title flow, modal pause, save/restore sequencing, universe insertion/removal, cheats and developer behaviors. Produce `docs/R3-SHELL-COVERAGE.md` with every original label and translation status.
4. Identify exact R2 native ABI/cdecl rules and test adapters. Audit 68000 big-endian byte views of native words; preserve serialized commander CASH BE32, and the original packed QL screen words/pixel semantics.
5. Inspect KCTX-FIX2 actual display, keyboard, timing, file and process APIs. Record supported modes and their limitations. Never assume a native 256×256 physical VGA mode: it is a **logical game buffer**.
6. Preserve original resource licensing and source provenance. Do not bundle private SuperBASIC or other restricted components in a public demo image.

**Gate 0:** R2 regressions green, full original-shell inventory and an explicit platform-call map.

### Phase 1 — Translate all original shell control flow into native assembly

Translate the real original `shell.s` into i386 GAS 1.38 using the R2 established conventions. Preserve menu order, key-to-action mappings, startup sequence, timing-dependent game-state changes, title/death transitions, docking, launch, save/restore, text/bitmap routines, universe lifecycle and all original gameplay branches.

Do not use a generic 68k register-bank interpreter, C gameplay rewrite or fake shell. C89 is permitted only for host-side test adapters, reference comparisons and narrow OS glue. Maintain source-label traceability, with deliberate handling of carry/borrow/overflow, signed branches, 8/16/32-bit wraparound, shifts, fixed-point and random state. Compare original 68000 and i386 state transitions wherever possible.

The R2 boundary document identifies these external operations, which must remain **narrow interfaces** rather than replacements for internal shell logic: `platform_sound_ipc`, `platform_file_open`, `platform_file_transfer`, `platform_file_close`, `platform_keyboard_queue_pop`; plus the shell's QDOS screen allocation/mode, timer/poll, suspend, exit and memory-query operations. The original font/text/menu renderer is **internal assembly** and must be ported, not discarded.

**Gate 1:** complete translated shell links with R2 core and no unexplained missing original routines. A successful link alone is not a gameplay proof.

### Phase 2 — Deterministic headless full-game verification

Create a deterministic headless driver for the **actual translated game loop**. Feed recorded keyboard rows/events and ticks, capture original MODE 8 packed framebuffer, state and boundary calls. Provide explicit file/sound/frame adapters that record requests and return documented unsupported errors when unavailable; never claim a fake sound or save succeeded.

Compare with original 68000 implementation or recorded QLBox reference using identical initial state and input sequence. Test title/menus, flight movement, orientation, combat, enemies, trade, docking, death, save/restore sequencing, and representative original cheats/edge cases as source permits. Compare internal states, RNG sequence, geometry caches, clipped lines, dashboard and framebuffer hashes. Do not require all graphics to match if the oracle has an independently documented difference; report exact mismatch and cause.

Keep R2 oracles unchanged and green. Add new differential tests rather than weakening earlier fixtures.

**Gate 2:** the real game loop advances through several reproducible gameplay sequences; original state and drawing behavior are independently verified. No video device required yet.

### Phase 3 — QDOS/386 video, keyboard, timer and files

Implement a **small platform adapter** using KCTX-FIX2's existing userspace SDK and graphical display ABI. Do not modify the kernel.

**Video:** retain EliteQL's packed 32 KB MODE 8 buffer and original XOR drawing semantics. Decode its actual bitplane/colour layout into a process-private indexed-pixel surface, initially using R13A's available logical sizes or a documented 256×256 indexed surface if supported by the *actual* SDK. If unsupported, adapt presentation via a userspace scaling/copy layer, not by changing game geometry. Flight viewport 256×192 and dashboard 64 lines must be maintained. Define exact palette mapping; monochrome game graphics remain monochrome. Never let the game write physical VGA memory or map `$20000` as a shared physical address. Compare displayed frame hashes to the headless packed buffer conversion.

**Keyboard:** use the existing foreground-only key-event API. Reconstruct original QL key rows/modifiers/repeat behavior **only as actually used by EliteQL**. No invented controls; focus changes must not inject held keys or leak input to another session. Confirm gameplay controls against source and QL reference.

**Time:** use existing monotonic scheduler ticks and bounded wait/yield; preserve game timing semantics. Avoid busy polling and do not tie game logic to the wall-clock RTC.

**Files:** map original commander save/load to existing QDOS/386 channels, preserving documented 144-byte legacy read, optional 4-byte EQA2 tail, 148-byte write, short-transfer error -10, and transfer/close error precedence. Preserve original big-endian serialization; compare saved bytes with original oracle. Do not invent a new save format.

**Sound:** optional if source-backed and feasible through existing APIs. Preserve original IPC packet formation even if the OS cannot play it; report unavailable sound honestly. Do not block the graphical game over sound.

Build a normal `.AOU` at the standard dynamic-process origin, using historical GAS 1.38. Include a reproducible Makefile and link map. Program title should appear in Task Manager via supported `_prog_name` convention or fallback.

**Gate 3:** Elite starts, shows title and original gameplay graphics in its own virtual session, accepts input, and advances actual gameplay on QEMU. No kernel patch required.

### Phase 4 — Playability and multitasking acceptance

Run the actual game under QEMU and perform a meaningful session: launch, fly, interact, trade/dock if reachable, fight, die/restart, save/quit/relaunch/load. Capture screenshots and deterministic state/frame hashes. The game must remain responsive after Task Manager switching and when backgrounded; only foreground Elite receives keyboard events. Run alongside shell, BASIC or Hack, QLTerm and another graphics demo where the packaged legal assets permit. Verify resource reclamation on exit/kill and no corruption of other surfaces.

Test error cases: missing commander file, short read/write, malformed data, disk full, low memory, invalid display mode, unsupported sound and unexpected input. Preserve R2 tests and run targeted KCTX-FIX2 process/display/keyboard regressions only. QEMU is primary; one bounded Tiny386 full-machine smoke where display capabilities permit. CPU-only i386VM retired.

**Gate 4:** a reproducible native gameplay session and stable coexistence with other tasks, no kernel regression.

### Phase 5 — Source release and handover

Package `ELITEQL-I386-R3-QDOS386-NATIVE.tar.gz` with complete i386 GAS 1.38 source, original source/reference fixtures (subject to redistribution rights), narrow platform adapter, native `.AOU`, Makefile, tests, state/frame oracles, screenshots, logs, link maps, docs and `SHA256SUMS`. Required docs:
- `R3-SHELL-COVERAGE.md`: label/routine-by-routine original translation completeness.
- `R3-PLATFORM-ABI.md`: exact video/input/file/timing/sound calls and limitations.
- `R3-DIFFERENTIAL-RESULTS.md`: original vs i386 state/bitmap comparisons.
- `R3-RUNTIME-REPORT.md`: QEMU evidence, test results, hashes and commands.
- `R3-KNOWN-LIMITATIONS.md`: unimplemented functionality and NOT TESTED.
- `HANDOVER.md`: how to build and launch using **normal KCTX-FIX2** kernel, paths, binaries and hashes.

Fresh-extract, verify manifest, rebuild native binary deterministically where feasible and run a bounded QEMU smoke from extracted artifacts. Distinguish compile/link proof from game-loop proof, and actual live playability from headless oracle checks.

### Autonomy, limits and stop conditions

Proceed autonomously with runnable checkpoints, but **do not declare R3 complete merely because `shell.s` assembles**. If the original shell translation cannot be finished within the work budget, deliver an honest R3-WIP with exact coverage and remaining original labels. If platform integration is blocked by a missing QDOS/386 API, document a minimal source-grounded OS request and keep the kernel unchanged; do not invent new hardware interfaces.

**Do not start** SMP, DOS V86, DOOM, GW-BASIC work, QDOS kernel modifications or a new Elite implementation. Preserve the game feature-for-feature.
