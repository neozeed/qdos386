# R3 native shell validation and remaining acceptance limits

## Result

All 6,666 lines of the supplied original shell have been audited. All original
instruction lines and 1,112 labels have native ownership, including the complete
data layout. No original source, frozen R2 routine, or kernel was changed.
The production game is native i386 assembly, assembled by genuine GAS 1.38 and
linked by binutils 1.94. The static source-lowering scripts run at development
time; the application contains no instruction decoder or emulated register bank.

`ELITE.AOU` has entry 0x240000, 54,816 text bytes, 133,392 initialized data bytes,
zero BSS bytes (reserved game/platform memory is explicitly zero-initialized),
and no undefined symbols. Runtime size is 188,208 bytes, below the slot's
argument area. The platform object is first and the SDK's 16-byte section
alignment is used. No post-link instruction patching is performed.

Image SHA256: `5c63e59814a8d9dac482fb52340252a06dd2ffb0b99c90996dc69a6459283dad`.
`R3-IMAGE.json` and `tools/check-r3-image.py` record/check the actual image.

## Differential evidence

`make r3-check` executes the full untouched 68000 shell as the behavioral oracle
and the genuine GAS 1.38 native instruction bytes, independently:

| Suite | Coverage and observed result |
|---|---|
| Setup | 64 scenarios; INWK, controls, commander/travel/market, RNG, dust, caches and full 32KB frame equality |
| Lifecycle | 64 remember/restore/title/death initializations; commander fittings included |
| Universe | 128 compact removal, target remapping, missile/cargo/pod spawn and full-pool cases |
| World | 128 existing cheat/warp scenarios, including successful and blocked warp; 2,048 encounter orchestration steps, pool admission, RNG and model-kind equality |
| Serialization | 64 complete save/load sequences; exact 148-byte saves, checksum and semantic rejection, I/O errors, original result rendering; accepted named-save path |
| Pages | 112 complete status/cargo/market/shop/system/local-chart/cheat/quantity pages; full frame equality |
| Original loop | 800 iterations through both complete title approaches, dock, launch/tunnel, flight, firing, pitch/roll, status overlays, quit/cancel, death animation and restart; state, ship fittings, normalized model pointers/cache fields and full frames |

The loop harness supplies only external input, time and unsupported IPC results.
It executes the original `asm_loop`, not a substitute game loop. Setting the
original death flag at iteration 700 is an explicit test input; the actual death
and restart paths execute on both CPUs. This test does not claim every combat
outcome has been reached through player input.

`R3-FULL-SHELL-TEST.log` contains exact deterministic hashes. The unchanged R2
suites are in `R3-FROZEN-REGRESSIONS.log`: all 33 fixture suites, platform tests,
movement/fixed-point, raster, rendering and effects oracle groups pass. The
existing shell leaf suites additionally check 2,048 complete text frames, 4,096
decimal cases, and 2,048 save copy/checksum cases. Original fixtures and R2 hashes
were preserved. Source coverage is not a substitute for this execution evidence
and none of these finite tests is a proof of all possible states.

## Existing OS integration

The unchanged KCTX-FIX2 normal VGA kernel is used. Its SHA256 remains
`f8e5d5e1f7e5c698ed13cc57159f7c9a7faeffb8fcf523e185625a57aec0d4c3`.
The test copies only the supplied public FAT16 volume; no OS assets are bundled.
The private 256x256 INDEX8 surface presents the original packed buffer and font.
The adapter uses existing keyboard events, CON queue, clock/yield, heap and file
traps. Optional suspend wake-flag/job arguments are zero, matching SDK qd_yield.

`test-qdos-adapter.py` checks actual assembled adapter instructions: keyboard
make/break and synthetic release, shared modifiers, 1,000 rational clock/wrap
samples, null suspend wake arguments despite dirty caller registers, counted
filenames and open flags, short/error transfers, all 65,536 packed pixel words.
The original sound sequencer runs, but QL IPC output returns -19 because the
supplied OS does not implement that output service. It is not reported as success.

`run-tiny-session.py` drives real PS/2 events in the supplied Tiny386, with a
read-only RAM/frame observer built from a separate copy. No CPU semantics or
guest/kernel memory is changed by the observer. The bounded session checks:
new commander, status, original cash cheat, market, equipment, named save,
launch, fire/steer, quit, fresh process, equipment-menu load and shutdown.
The cash change from 1,000 to 11,000 stored tenths is saved; the fresh process
starts at 1,000 and reloads 11,000 with original ship fittings. Both processes
return exit status 0. `R3-TINY-SESSION.log`, `evidence/session.json`, framebuffer
captures and the guest log preserve the observations. This is a bounded session,
not a long-duration soak or multiplayer/multitasking stress claim.

## Confirmed original title-load defect

This prevents an acceptance-complete release while preserving the supplied
source exactly. It is not untranslated work or an OS missing-service guess.

1. `flow_load_actual`, shell.s:3946, sets `asm_market_result` to 6 on valid load.
2. shell.s:3948 branches to `dock_page_refresh`. With `asm_shop=0`, that calls
   `asm_market_page`, including when loading from the title.
3. shell.s:2736-2741 indexes `asm_market_messages` with result times four and
   treats the fetched longword as a text pointer.
4. That table at shell.s:3217 has only four pointers. Entry 6 reads ASCII
   bytes instead of a pointer: original A4=0x2053544f; native ESI=0x4f545320.

`diff-shell-serialization.py` reproduces this on both original and native CPUs,
stopping at the bad pointer without masking it. A real title-load attempt on
KCTX-FIX2 faults while rendering after the successful load; its log is retained
in `R3-TITLE-LOAD-DEFECT.log`. `--title-load` reproduces it in the OS harness.
The equipment menu's existing nine-entry `shop_messages` table handles load
result 6 correctly; that existing path passes. Saving/loading from other
contexts that route a result greater than 3 through the market table has the
same underlying source risk. No bounds clamp, new message, fake success or
behavior change has been introduced. Repair needs an explicit decision to
change the original source behavior, outside this exact-port instruction.

## QEMU acceptance not completed

The supplied handoff requires meaningful QEMU validation as the primary gate.
No qemu-system-i386/qemu-system-x86_64 executable is installed here. Package
retrieval failed with HTTP 503 from the configured snapshot repository, so that
gate remains unperformed. Tiny386 evidence does not replace it. The remaining
acceptance work is a QEMU session on the supplied unchanged kernel and a decision
about the confirmed original load defect, not a new round of missing shell
translation. The native application, full source, history and reproducible tests
are preserved for that work.
