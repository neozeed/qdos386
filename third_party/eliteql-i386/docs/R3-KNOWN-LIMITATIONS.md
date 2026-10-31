# R3-WIP exact remaining work

The complete original shell is NOT translated: 1,091 of its 1,112 labels remain
explicitly unported in R3-SHELL-COVERAGE.md/JSON. Twenty-one are mapped to native
code/data, chiefly text/font, decimal and save-copy/checksum leaf routines.
The original asm_labels no-op is retained as a no-op, not a missing-function stub.

Startup, menus, key-action dispatch, actual flight loop, death/title/config flow,
universe lifecycle, original cheats/developer behaviors, text callers and complete
save/load validation/orchestration remain. No substitute main loop is supplied.

No headless full-game driver, native game .AOU, OS integration, palette converter,
key-row reconstruction, time adapter, QEMU play session, multitasking test or
Tiny386 machine smoke has been produced. Those stages remain NOT TESTED.
R2's link-test entry returns immediately and is not treated as a game.

The actual SDK supports the required logical 256x256 surface. Original 50Hz
poll timing must still be adapted to approximately 100Hz monotonic OS ticks.
QL IPC sound is explicitly unsupported by this OS's compatibility implementation.
No kernel changes or invented API have been made to address any of these items.

The handoff permits an honest WIP within the work budget. This is such a
checkpoint, not completion of R3. Continue the remaining original labels before
starting platform integration. Preserve R2 regressions and the new shell tests.
