# R13B known limitations

* **No security isolation.** Everything still runs at CPL 0; separate shells, sessions and address spaces are
  correctness and ownership properties, not protection.
* **Two shell images.** The boot shell `SHELL.AOU` keeps its historical 0x200000 link (R13B build `shell-f`); it cannot
  be started with `RUN`, `START` or F2 ("not a slot 1 program" / "not linked for the standard origin") — `SH`
  (`/BIN/SH.AOU`, the same source at 0x240000) is the shell to start. `SH` does not run `/ETC/STARTUP.CMD`.
* **Job table.** 16 jobs in all (SYSTEM, IDLE, the boot shell + 13); every dynamic shell and every program it starts
  uses one. The 14th start is refused ("no free job slot").
* **Orphans.** Programs started by a shell (`RUN`/`START`) or by F2 have no parent record: when their shell ends or is
  killed they keep running; nothing waits for them; their exit status can be read by id only until the slot is reused.
  `EXIT` of a dynamic shell warns once if its own started jobs still run.
* **Search order** keeps R12B's final fallback to `/` after `PATH`. Names over 64 characters are refused; a candidate
  path over 127 bytes is skipped. `.AOU` is the only implicit extension.
* **F2** uses the boot shell's directory, `PATH` and environment, not those of the session shown; its programs are owned
  by SYSTEM, not by a shell (no `WAIT`; `KILL`/F1 end them). The line holds 70 characters, ASCII only; no history, no
  completion. F2 programs always get a new session (no choice of console).
* **Focus return** goes to the launching session only while that session exists unchanged (same tag: a session that
  switched between text and graphics counts as changed); otherwise to console 0.
* **CLS** clears the whole session with its current attribute; there is no colour command in the shell (attributes
  come from programs).
* **Clock.** The RTC is read once at boot; the clock then follows the PIT (no drift correction, no NTP, no
  `DATE`/`TIME` setting, UTC only). The RTC is assumed to keep UTC. Two-digit years are never completed without the
  century byte. QEMU's `-rtc base` comes out wrapped for some dates after 2038 (2100-02-28 read back as 1964): those
  values are refused, so on QEMU dates after 2038 are tested with build-time epoch kernels. Tiny386 gives the host's
  UTC. Real-hardware RTCs: **NOT TESTED**.
* **Tiny386:** no display presenter: rendered consoles and screendumps are **NA**; the Task Manager's F2 there is
  **NOT TESTED** (the headless smoke has no Ctrl+Esc path; F2 is tested on QEMU).
* **Harness note:** the QEMU capture script treats a typed `EXIT` as the boot shell ending; the tests type `!EXIT` to
  leave a dynamic shell.
* **Not done (out of scope by the handoff):** serial, Elite, DOS V86, SMP, ring 3, DOOM, a graphics redesign.
