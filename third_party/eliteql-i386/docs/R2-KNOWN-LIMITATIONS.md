# Exact unfinished work

1. `reference/src/shell.s` (6,666 lines) has no native counterpart. Startup,
   original game-loop sequencing, menus, text/font drawing, complete state
   transitions, configuration, save orchestration, title/death flow and shell
   universe management are therefore NOT VERIFIED and not runnable as a game.
2. No deterministic whole-game headless input/tick driver exists. The final
   historical link entry only returns; it is a link-validation image.
3. Forty-four original assembly files have native counterparts; this is NOT
   44/45 gameplay completion. The shell represents substantial behavior.
4. Independently executed m68k comparisons cover movement, line rasterization,
   ship rendering, circles, dust, cloud points and galaxy charts. Other fixture
   suites are source-derived/upstream evidence, not independent proof of the
   complete m68k behavior. Full per-routine differential coverage is unfinished.
5. Save/keyboard/sound platform calls have source-derived sequence tests only;
   actual OS operations and timing are NOT VERIFIED. No QDOS386 runtime test,
   video, keyboard, audio, task-management or launch integration is claimed.
6. Benchmark render/models and development-mode alternatives assemble, but the
   benchmark fallback orientation path is NOT VERIFIED by the game-render oracle.
7. Data byte-order boundaries are documented. Shell integration must explicitly
   preserve serialized gameplay bytes while using native numeric model/cache
   words. No blanket memory-layout conversion is safe.
8. Oracle JAS-to-m68k-GAS conversion changes syntax and replaces label(pc)
   addressing with equivalent absolute relocation to avoid historical GAS's
   two-byte displacement ambiguity. It does not prove identity to an original
   shipped QL executable. CPU execution is in Unicorn, not physical 68000/386.

No missing game routine is supplied as a success stub. Original source remains
available and unmodified. R1 is frozen separately. This archive is deliberately
named WORK-IN-PROGRESS, not COMPLETE-ASSEMBLY.
