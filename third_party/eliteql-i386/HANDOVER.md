# EliteQL R3 native shell handover

The complete original shell has native translation ownership. Continue from this
code; do not restart the project. Frozen R2 core and reference source are intact.

`prebuilt/ELITE.AOU` is assembled by genuine GAS 1.38 and linked by binutils 1.94.
Rebuild with `make r3-game`; run differential suites with `make r3-check` and the
frozen suites with `make check platform-check oracle-check shell-check alternatives`.
The production image has no unresolved symbols or headless INT3 adapters.

Use the unchanged KCTX-FIX2 VGA kernel identified in `docs/R3-PLATFORM-ABI.md`.
Only a disposable copy of the public volume is used for integration. No OS
assets are bundled. `tools/make-r3-volume.py OSROOT` prepares this test volume;
`tools/build-tiny-observer.py OSROOT` builds a test observer without changing
CPU semantics, guest memory, or kernel. `tools/run-tiny-session.py` sends real
PS/2 events and records frames/state. Its optional `--title-load` reproduces the
known original-source fault instead of the passing equipment-menu load path.

Read `docs/R3-VALIDATION.md`: full translation is not proof of every possible
execution. The major outstanding acceptance issues are the original title-load
message-table defect and the unavailable QEMU run. Do not repair the source
bug silently: preserving exact original behavior and making that path work
require an explicit upstream behavior decision. QL IPC sound remains an
existing OS limitation, returned as -19. Do not modify the kernel.

Machine-readable source ownership and data layout are in
`docs/R3-SHELL-INVENTORY.json` and `docs/R3-SHELL-DATA.json`. ABI/byte-order notes
are in `docs/R3-NATIVE-SHELL-ABI.md`. Test-only C fixture drivers are inherited;
production game logic, menus, rendering, serialization and platform adapter
are native assembly. `CHECKPOINT.bundle` preserves the commit history.
