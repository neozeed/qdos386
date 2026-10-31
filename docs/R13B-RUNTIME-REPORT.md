# R13B runtime report — dynamic shell, task launcher, CLS and clock

Base: frozen R13A `951352c73d20f4be2f593c974fbb47e6061154aa` (archive `6c966941…`, re-checked). Branch
`r13b-shell-usability`. The exact final commit and the archive SHA-256 are in `PACKAGE.md` and the delivery note.
Kernels under test: QEMU `build/r13b/pc-vga/r13b-vga.img` (`0199a72b…`), headless `build/r13b/pc/r13b.img`
(`d3e8a759…`), epoch test kernels `build/r13b/e2100|e2101|e2106`; boot shell `shell-f` (`b9ef7178…`), `SH.AOU`
(`0adeb529…`); all hashes in `logs/r13b/final/binaries.sha256`.

**Everything runs at CPL 0. Nothing in R13B is security isolation.**

Verdicts: **PASS**, **FAIL**, **NA** (cannot apply there), **NOT TESTED** (could apply, not run).

## 1. Result

| Item | Content | Verdict |
|---|---|---|
| Stage 0 | audit (`docs/R13B-AUDIT.md`), baseline smoke on the frozen R13A kernel (`logs/r13b/stage0`) | done |
| host | CMOS RTC decoder 26/26 against Python-computed vectors (regenerated identical), R11F clock arithmetic, Phoon host oracle build | **PASS** |
| Gate 1 | two `SH.AOU` at once (sessions 4, 5) beside the boot shell; own directory/PATH/variables; EDIT and SuperBASIC from each; focus returns to the launching shell; one shell killed, the other healthy, its children orphans; EXIT of SH ends only it; jobs, surfaces, memory to baseline | **PASS 23/23** |
| Gate 2 | one resolver through the command word, RUN, START, SH, SuperBASIC EXEC/EXEC_W: argv exact, PATH order, cwd first, case, literal before `.AOU`, explicit extension/path rules, quoted long names, built-ins first, every error message, job-table exhaustion | **PASS 33/33** |
| Gate 3 | CLS on VGA text and on rendered (mode 12h) consoles, boot shell and SH, only the caller's session, cursor home, attributes kept, then DIR /W, ECHO, EDIT; Task Manager F2 starts SH, BASIC, EDIT (argument) and a quoted long path with quoted arguments in new sessions with the boot shell's context; errors, Esc, unterminated quote; return to the previous session; no job slot for the Task Manager | **PASS 33/33** |
| Gate 4 | RTC anchor, DATE/TIME = libc time()/gmtime()/ctime() = SH; ticks separate from seconds; no-argument PHOON = host oracle at the guest's instant; 2100/2101/2106 boundaries; refused RTC value; clockless kernel | **PASS 24/24** |
| Stage 5 | integrated QEMU demo, one boot | **PASS 26/26**, IMAGE PASS |
| Stage 5 | Tiny386 smoke (headless kernel, R12C emulator, 0x8900 shutdown) | **PASS 14/14** (rendered text and screendumps **NA**, F2 **NOT TESTED** there) |
| Stage 5 | targeted regressions on the R13B kernel (§8) | **PASS** (all 10 suites) |
| Stage 5 | frozen images byte-identical (§9), package verification (§10) | §9, §10 |

## 2. Evidence classes

As in R13A: QEMU 8.2.2 full machine (`-machine pc -cpu pentium -m 16 -vga std`, `-rtc base=2026-10-29T12:00:00`
unless stated), monitor key input, captures of the text buffer or the presenter shadow, `%cells` (characters and
attributes), the CRTC cursor, QEMU screendumps checked pixel for pixel against the judge's rendering
(`tests/r13a/r13apics.py`), the programs' own verdicts (COLOUR `verify=PASS`), independent image checks (mtools,
`fsck.fat -n`, the VFAT walker, FAT copies). New: ARGV/WHICH probes print their exact argv, directory and PATH; TIMEQ
prints libc's view of the clock next to the tick count; the Phoon host oracle (`tests/r13b/phoon-host.sh`) is the R11F
patched source built on the host with `time()` returning a given instant. The judges re-run unchanged on
`logs/r13b/final` (screendumps kept as PNG; the oracle is rebuilt when absent).

## 3. Stage 1 — dynamic shell (`logs/r13b/final/gate1`)

The shell source is built twice: `shell-f` (`-DR13B_SHELL`, 0x200000) is the boot `SHELL.AOU`; `sh`
(`-DR13B_DYN`, `apps/common/slot1.ld`, 0x240000) is `/BIN/SH.AOU`. One parser; the dynamic build differs only in not
running STARTUP.CMD, its banner, and an EXIT warning that counts only the jobs it started itself. `START SH /NEW`
twice: sessions 4 and 5, parent the boot shell. SH-A: `CD /HOME`, `PATH=/GAMES;/BIN`, `WHO=A`; SH-B: `/SDK`, `/BIN`,
`WHO=B`. Each started EDIT in a new session, typed and saved (`/HOME/ANOTE.TXT`, `/SDK/BNOTE.TXT`), quit: the display
returned to the shell that started it (kernel: a `/NEW` session records its creator's session and returns there when
it ends while shown). Each started SuperBASIC (`FROM A 25`, `FROM B 42`). `KILL /CON:5` from the boot shell ended SH-B
only; SH-A kept its directory and variables, ran HELLO and PHOON by name; B's BASIC ran on as an orphan. SH-A's first
`EXIT` warned about its running BASIC, the second ended it; the machine ran on (only the boot shell halts it). Both
orphans killed; boot shell's `/` and PATH untouched; `SURFT stats` 0 open before and after; `LEAK` heap, channels,
address spaces and held frames equal before and after (720,496 / 15 / 2 / 266,240 on the final kernel: the idle QEMU
baseline, 5,808 bytes below R13A's 726,304 for the R13B code and tables).

## 4. Stage 2 — programs by name (`logs/r13b/final/gate2`, `docs/R13B-EXEC-RESOLUTION-ABI.md`)

The kernel's R12B resolver already served every load; R13B makes the shell's command word use it. Results: `WHICH a
"b c" d` and `RUN WHICH a "b c" d` → WHICH-A with argv `[WHICH][a][b c][d]`, status 4; `START WHICH s1 s2` → the same
program, status 3 in PS; PATH reordered → WHICH-B; in `/HOME` → WHICH-CWD (current directory first); `which lower`,
`Which Mixed` → found, argv[0] as typed; `TOOL` → the literal extension-less file (HELLO) before `TOOL.AOU`; `TOOL.AOU t`
→ ARGV; `NOTE.TXT` → not found although `NOTE.TXT.AOU` exists (no `.AOU` after an explicit extension); `"NOTE.TXT.AOU"
n` → found; `DIR /HOME` is the built-in, `RUN DIR` runs `DIR.AOU`; `"/APPS/My Tools/Arg Probe.aou" "x y" z` and `RUN
"/APPS/My Tools/Arg Probe" 1` → argv exact; `/NOPE/HELLO` → not found (explicit path, PATH not searched); `/ORDB/WHICH`
→ WHICH-B whatever the PATH order; `README.TXT` → not a valid a.out (−21); `/HOME/README.TXT/X` → not a directory;
`NOSUCHPROG` → unknown command or program; a 68-character name → invalid name; `PHOON -l 3`, `EDIT HELLO.C` (file
saved), `START SH /NEW` by name; SuperBASIC `EXEC_W "ARGV", "one two"` (argv exact, `EXEC_STATUS` 3), `EXEC_W "WHICH"`
(inherited PATH), `EXEC_W "NOTE.TXT"` → not found, `EXEC "PHOON"`; 13 COUNTERs fill the job table, the 14th START →
"no free job slot", all 13 killed; memory to baseline.

## 5. Stage 3 — Task Manager F2 and CLS (`logs/r13b/final/gate3`, `docs/R13B-TASK-LAUNCHER.md`)

CLS (VGA text, boot shell): after `DIR /W /` the cells hold only the new prompt in row 0, cursor (CRTC) row 0 col 7,
all attributes 0x07; `DIR /W /BIN` then starts at row 0. In SH (session 4) the same; ECHO, EDIT (`CLS.TXT` saved) and
TYPE after it; session 0 untouched. Rendered consoles (`CONSOLE GFX /ALL`): CLS leaves only the prompt; the
640×480 screendumps after CLS, after ECHO/DIR, of SH's session after its own CLS and of the boot shell after switching
away and back are each pixel-identical to the cells rendered with the 8×16 font (plus one cursor); back in VGA text
the same cells. F2: the Run line shows the typed text; the selector lists 4 jobs (no slot for itself). `SH` → a new SH
shown at once in session 5 with the boot shell's `/HOME`, PATH and `ARGV_WAIT=1`; `BASIC` → session 6 (`F2 BASIC
42`); `EDIT F2NOTE.TXT` → the argument reached EDIT, the file was saved in `/HOME`; when EDIT ended the display
returned to BASIC (shown when the selector opened); `"/APPS/My Tools/Arg Probe.aou" "x y" z` → argv exact, cwd
`/HOME`, back to BASIC when it ended; `NOSUCH 1` → "NOSUCH: not found (here, in PATH or in /)" in the selector; Esc
→ "start cancelled"; `"/BIN/EDIT.AOU` → "unterminated quote". TASKS /L: the F2 programs have parent `-` (SYSTEM).
After killing them, `LEAK` back to baseline, only SYSTEM/SHELL/IDLE.

## 6. Stage 4 — clock (`logs/r13b/final/gate4`, `docs/R13B-CLOCK-ABI.md`)

k1 (RTC base 2026-10-29T12:00:00): boot line `CLOCK: CMOS RTC (UTC) read at boot [cc yy mm dd hh mi ss B = 20 26 10
29 12 00 ss 02]`, anchor = base + ss; `DATE: Thu 2026-10-29 (UTC)`; TIME before ≤ TIMEQ `time()` ≤ TIME after,
TIMEQ's gmtime/strftime/ctime consistent; 300 ticks slept = 300 ticks and 3 s; `SLEEP 500` moved the clock 5–15 s;
no-argument `PHOON` (exit 0) equals the host oracle for an instant between the TIME readings around it; SH sees the
same clock. k2/k6 (epoch kernels): `DATE: Sun 2100-02-28` → `Mon 2100-03-01`; `Fri 2100-12-31` → `Sat 2101-01-01`;
libc agrees. k3: 2106-02-07 06:28:0x valid (time() ≤ 0xFFFFFFFE); after the last representable second TIME, DATE,
time() and PHOON all report no clock (R11F OVERFLOW). k4: QEMU `-rtc base=2100-02-28T23:59:57` puts 1964 into the RTC;
the kernel refuses it (`CLOCK: unavailable - RTC date outside …`, raw registers shown) and everything reports
unavailable. k5: the clockless R13A kernel with the R13B shell: DATE/TIME/time()/PHOON unavailable. All images clean.

## 7. Stage 5 — integrated demo and Tiny386

QEMU demo (`logs/r13b/final/demo`, one boot): CLS + `DIR /W /BIN` at row 0; DATE/TIME; two SH (`/HOME` +
`/GAMES;/BIN`, `/SDK` + `/BIN;/GAMES`); BASIC from SH 4 (`DEMO A 2`), COLOUR from SH 5 (parents correct in TASKS);
F2 `EDIT DEMO.TXT` over COLOUR (saved in `/`); both COLOUR screendumps equal the model of their frames and COLOUR
advanced meanwhile; boot shell display preserved; `ECHO typed in shell A` only in SH 4 (BASIC's cells unchanged); COLOUR
quit → back to SH 5; no-argument PHOON in SH 5 = host oracle; SH 5 EXIT, BASIC and SH 4 killed → 3 jobs; COLOUR
`verify=PASS`; rendered console after CLS pixel-exact; LEAK baseline; SYNC; image clean.

Tiny386 (`logs/r13b/final/tiny386`): DATE/TIME from Tiny386's CMOS = the host's UTC during the run; TIMEQ consistent,
200 ticks = 2 s; PHOON = host oracle at the guest instant; CLS then `CONDUMP 0` starts with the DIR line; HELLO and
ARGV by name; `RUN SH` in the foreground (own directory, PATH, PHOON), EXIT back with the boot shell unchanged; `START
SH /NEW` and KILL; `CONSOLE GFX` refused (no presenter); SYNC; LEAK 735,936 before and after; 0x8900 shutdown, exit 0;
image clean.

## 8. Regressions on the R13B kernel (`logs/r13b/final/regressions`)

| Suite | Result |
|---|---|
| R13A Gates 1–4 (fonts, rendered text, logical surfaces, key events; R13A volume, shell-e) | 44/44, 26/26, 22/22, 17/17 **PASS** |
| R13 Gate 1 (sessions, surfaces, API matrix) | 93/93 **PASS** |
| R12C Stage 1 (Task Manager, titles), Stage 4 (SYNC) | 49/49, 15/15 **PASS** |
| R12B Stage 2 (STARTUP, PATH), Stage 4 (SuperBASIC EXEC/EXEC_W) | 61/61, 25/25 **PASS** |
| R12A Stage 3 (dynamic spawn/wait/kill) | 35/35 **PASS** |

The multi-hour historical matrices were not run (handoff); the NEG/MULTI/TWICE suite and the R12 corpus: **NOT TESTED**
for R13B.

## 9. Frozen images

During Stage 5 a rebuild showed that two R13B kernel edits were not guarded (a clock-source query in
`clock_boot.c` and a refactor of `jobctl_note_launch`) and changed every earlier kernel. Both are now under
`QDOS_R13B` (commit "guard the clock source query…"); the R13B kernel itself was confirmed byte-identical before and
after the fix (`0199a72b…`), so the official runs stand. Rebuilt after the fix: R13A `f802435e`/`10788251`, shell-e
`c5879378`; R13 `a7d86c21`/`c0546a47`/`569c863f`, shell-d `e7056b03`; R12C `50ab9a6e`/`fc4a1368`, shell-c `93b87d14`;
R12B `d22fd9f1`/`01ebca11`; R12A `257460b9`/`0b23ab9d`; R12 `1e00386b`/`dd11fe87`; shells b/a `dc50e084`/`9ed34a73` —
all as frozen.

## 10. Judge corrections (same evidence)

* Gates 1 and 2, first official runs: 22/23 and 32/33 — on those seconds Phoon drew its "Hubert" picture (phoon.c
  draws it on some seconds: `time % 13 == 3`, depending on the phase) instead of the moon with the phase text the judges looked for. The
  judges now accept either Phoon picture (phase text or Hubert's face) together with exit status 0; the same evidence
  passes 23/23 and 33/33, and both gates were run again: 23/23 and 33/33 (`logs/r13b/final/first-runs`).
* Tiny386: the first judge expected a CONDUMP layout without its `CONDUMP 0:` header and `|` row marks: 13/14; fixed,
  14/14 on the same evidence.

## 11. Package verification

`tests/r13b/verify-package.sh` on a fresh extraction of the candidate archive built from the commit before the final
one (`logs/r13b/package-verify.txt`; the final commit only adds this record, and the delivered archive was verified again
with the same script and the same results): manifest OK (every file against `SHA256SUMS`); QEMU smoke from the
extracted kernel and volume (CLOCK from the RTC, DATE/TIME, CLS then DIR /W at row 0, PHOON by name, a dynamic SH); host
tests pass (RTC vectors regenerate identical); rebuilt from the extracted source: both R13B kernels, the three epoch
kernels, shell-f, SH.AOU, ARGV, WHICHA/B/C and TIMEQ **byte-identical**, and the frozen R13A kernels and shell-e still
byte-identical. (The first candidate's smoke judge read the wrong capture lines and reported 4 false failures; fixed
before this candidate.)
