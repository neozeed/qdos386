# R14A known limitations

Status words: **build verified**, **host tested**, **guest executed** (QEMU unless stated), **differentially
verified** (against the original QL binary under QLBox), **not tested**, **blocked**.

## 1. Differences from the QL build that are kept on purpose

| Topic | QL build (QLBox R20, measured) | QDOS/386 | Why |
| --- | --- | --- | --- |
| Pet dog | Never appears: the `struct monst` exists with `mx = my = 0` (RAM dump, `tests/r14a/oracle-dog.sh` → `logs/r14a/oracle-dog/dog.txt`); `mnexto()` receives (0,0) from `enexto()`, a function returning a 2-byte `coord` struct | Placed next to you, as the source says | a code-generation fault of the QL toolchain (struct return), not Hack's behaviour; not reproduced |
| Object-letter lists in prompts | "What do you want to drop [¿xoHx or ?*]?" — the letter list is garbage (OCR: unknown glyph, "xoHx") | "[$a-f or ?*]" etc., as the source builds them | same class of QL toolchain fault (not investigated further); not reproduced |
| Save (`S`) | "Cannot open save file. (Continue or Quit)": C68 passes `save/0root` unchanged and the QLBox host filesystem (flat `WIN1_save_x` names) has no `save/` directory form | save/restore works (`save/` is a VFAT subdirectory) | QDOS/386 follows the source's path; the QL result depends on the QL filing system (a level-2 directory driver would need `save_0root`) — **the QL save path is not differentially verified** |
| Data directory | Without Toolkit II's `DATA_USE WIN1_` every file open fails ("help: No such file or directory"); the oracle sets it | files are relative to the job's current directory; start Hack in `/GAMES/HACK` | QDOS/386 has per-job cwd (R13B) |
| Window | `_condetails`: 512×256 window, red border, 85 columns | the whole 80×25 session, no border | QDOS/386 sessions have no QL windows; Hack lays out 80×24 with CM in both |
| Colours | green on black (LibVT) | green on black (0x02), SO = reverse | measured on the QL build; LibVT itself not supplied |
| `` ` `` (0x60, boulder) | £ in the QL font | `` ` `` (CP437) | no translation (documented fallback) |
| Random dungeons | C68 `rand()`, QL clock seed | QDOS/386 `rand()` (C89 LCG), CMOS clock seed | same game logic; maps differ for the same seed — comparisons are of texts, formats and outcomes |
| End of the program | C68 `_endmsg` "Press a key to exit" when it owns its window | the same in a private session (`START … /NEW`); nothing in the shell's own session | C68 shows it only for a console the program owns |

## 2. Limits of the port

* **Save, level and bones files are not portable** between the QL and QDOS/386 builds (raw structs: byte order,
  padding, bitfield order). Hack's own checks are unchanged; there is no converter. Native save → quit → relaunch →
  restore: guest executed (Gate 3). Old QL saves: blocked (would need explicit serialization, i.e. source changes).
* **No crash atomicity**: Hack writes `save/0root` in place (`creat` + `write`); a power loss or a full disk during
  the save leaves a partial file. Hack refuses it on the next start ("Read N instead of M bytes." / "Error restoring
  old game.") and deletes it — guest executed for a truncated file and for a disk-full save (Gate 4). The previous save
  is not kept by Hack (it deletes the save after every restore), so there is nothing to preserve.
* **Year 2000 in `getdate()`** (Hack 1.0.3): `sprintf("%2d%2d%2d", tm_year, …)` writes 7 digits plus NUL into
  `char datestr[7]` for 2000 and later (one byte past the array), and `record` lines start `1261030` instead of a
  6-character date; `fscanf("%6s")` then reads the record back misaligned (the date field swallows a digit of the
  next field). The QL build shows exactly the same (its `record`: `1261030 0 2 2 12 12 105 SM root,quit`). Not fixed:
  the source stays unchanged; the 1-byte overflow lands in the static data of `unix.o` in both builds.
* `!` (shell escape), `^Z` (suspend), mail, `MULTIUSER_LOCK`, `CHDIR`, `HACKDIR` are compiled out by the QL
  configuration, as on the QL.
* `signal()` delivers nothing (as `x68k.c` on the QL); Ctrl+C does not interrupt Hack.
* LibVT was not supplied: `ANSI_conwrite` implements what Hack emits plus common neighbours; VT52 mode alone is not
  emulated (bytes are written unchanged) — Hack never selects it.
* The console write vector, end message and full-count file reads live in `libc68.a` and apply only to programs
  linked with it (C68 ports); `libc.a` and every R14 binary are unchanged.

## 3. Not tested

* Bones files (needs a death on a level with no previous bones; deterministic death not scripted).
* Hack on Tiny386 in a private session (`START … /NEW`) — the Tiny386 smoke runs it in the shell's session.
* Long menus that reach `docorner()`'s `CD` (lone ESC) path inside Hack itself — covered by the host test and the
  ANSIT fixture (CD + CM), not by a Hack screen.
* The QL build's save/restore (blocked by its file name, §1).
