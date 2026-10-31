# EliteQL i386 R3 — native shell and KCTX-FIX2 integration

The complete original shell now has native i386 translation coverage, including
data, startup, menus, the original game loop, flight, universe lifetime,
death/restart, cheats and save/load. This is the existing RC6 assembly port,
based on frozen R2 commit `86bc95e5772e8da4b90b510b3dc3c450e256f0ed`.
The R2 core/reference implementations and QDOS386 kernel remain unchanged.

Genuine GAS 1.38 and binutils 1.94 build `build/aout/ELITE.AOU`. The native
platform adapter uses KCTX-FIX2's existing private logical INDEX8 surface,
keyboard events, clock, heap and channel APIs. The original packed screen/font
and gameplay stay in assembly. Unsupported QL sound returns -19 explicitly.

```sh
python3 -m pip install unicorn
make check platform-check oracle-check shell-check alternatives
make r3-check r3-game build/aout/r3-platform-test
ELITE_NATIVE_IMAGE=build/aout/r3-platform-test python3 tools/test-qdos-adapter.py
```

Source coverage is machine-readable in `docs/R3-SHELL-INVENTORY.json`; label
counts are not functional-completeness claims. Full original-shell differential
tests cover setup, actual loop execution, lifetime transitions, universe
management, save serialization and complete rendered pages. See
`docs/R3-SHELL-COVERAGE.md` and `docs/R3-NATIVE-SHELL-ABI.md`.

A bounded full-machine session on the supplied Tiny386 and unchanged VGA kernel
passes title/status/market/equipment/save/launch/fire/steer/quit/relaunch/load and
shutdown. Loading from the equipment menu works. **Title-screen loading retains
an oracle-confirmed original source defect**: result 6 indexes outside the four
market-message pointers. See `docs/R3-VALIDATION.md` before running. QEMU is
unavailable; no QEMU acceptance claim is made. No OS assets are redistributed.

The archive's `prebuilt/ELITE.AOU` is the application. Copy it to the supplied
public volume and run `RUN ELITE.AOU`. Press N, then X for a new commander;
6 opens the market, 8 equipment, L launches. In equipment, S saves and O loads.
Backslash opens the original quit confirmation; Y exits. The complete source
controls and menus remain as supplied. This is not an acceptance-complete release.

The following is retained R2 build/ABI documentation:

---

# EliteQL i386 R2 — assembly work in progress

Native i386 / GAS 1.38 translation of the supplied EliteQL RC6 JAS source.
**This is an incomplete checkpoint, not R2 complete and not a playable game.**
44 of the 45 original assembly files have native counterparts. The remaining
`shell.s` is 6,666 lines and contains startup, menus and main-loop sequencing;
its absence is substantial. File counts are not a feature-completion percentage.
No gameplay has been substituted with C or invented to fill that gap.

The frozen R1 remains unchanged outside this working tree. Original assembly
and fixture drivers under `reference/` are unchanged. Native gameplay, drawing,
compression and sound decisions are in `src/`. QDOS386 has not been modified.

## Reproduce

Requires a host C compiler, make, Python 3, Unicorn, modern GNU binutils and a
GCC capable of generating `-m32` objects (32-bit libc is not required).

```
python3 -m pip install unicorn
make check platform-check oracle-check
```

`make` builds actual bundled GAS 1.38 and native i386 a.out objects.
`make historical-link` builds the bundled host64 port of binutils 1.94's i386
linker and produces `build/aout/core-headless`. This is a test link image; its
entry returns immediately. It is not a headless game loop or QDOS executable.
External service adapters in test code explicitly return unsupported (-1).
They are not included in the assembly core's objects.

`make check` runs 33 fixture suites, including all 11 R1 suites. Their original
C drivers and test-only printing/memory runtime are compiled to i386 ELF. The
actual GAS 1.38 instruction bytes are retained by the a.out-to-ELF adapter.

`make oracle-check` executes the historical linker's final a.out image against
original 68000 routines assembled with bundled m68k GAS 1.38. Both CPUs run in
Unicorn, only in the test harness. The production native assembly contains no
68000 interpreter or CPU dispatch. See the differential report for exact scope.

`make platform-check` checks file/queue/sound decision sequences using explicit
recording test adapters. No OS driver or end-to-end integration is claimed.

## Coverage and limits

Read `docs/R2-SOURCE-COVERAGE.md`, `docs/R2-KNOWN-LIMITATIONS.md` and
`docs/R2-RUNTIME-REPORT.md` before integration. `translated` in the manifest
means source exists, not complete independent verification. Original benchmark
`render.s`/`models.s` and `development.s` are alternatives to the game variants.

The packed MODE 8 bitmap is retained byte-for-byte; numeric C shorts, pointers
and render-cache headers use i386 little-endian. Byte-encoded gameplay records
and commander file bytes retain their existing serialization. The ABI report
identifies the distinction. The original font/text shell is not yet ported.

Historical toolchain sources retain their upstream licensing notices. Legacy
R1 documents are retained as baseline evidence, not current coverage claims.
