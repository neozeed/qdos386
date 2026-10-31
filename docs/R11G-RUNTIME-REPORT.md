# R11G runtime report: Task Force '87 Z-machine interpreter running Zork I on QDOS/386

Status: runtime verification complete on Tiny386, i386VM and graphical QEMU VGA. All keyboard
input in these runs is scripted (xdotool, or piped/typed text in the emulator harnesses). None of it
is human-operated. Packaging and fresh-extraction verification results are in the release folder
(`VERIFY-FRESH.txt`), not here, so that this report does not have to name its own commit.

Related documents: `docs/R11G-INTERPRETER-AUDIT.md` (audit, frozen base hashes, first failure
boundary), `docs/R11G-ZORK1-PORT.md` (port description, memory, build, behaviour), `third_party/infocom/PROVENANCE.md`
(licence and provenance).

## 1. Identity

| Item | Value |
|---|---|
| Frozen base | R11F, commit `d614fa6` on branch `r11f` |
| R11G branch | `r11g`; the source revision is recorded in the release `MANIFEST.txt` |
| Interpreter a.out | `build/r11g/infocom.aout`, 40,566 bytes, SHA-256 `4204fcf9043d0580f04489c0eab41353b0661144e6eec357fb17fb5554ca2e4b` (build v5, final) |
| Story | `zork1_z3`, 86,838 bytes, SHA-256 `37084966477dff679282de42974b2077156b1bd68fad92a65d4ea94d8eb64d79`. Z-code version 3, release 119, serial 880429. Not included in the release. |
| Production volume (tested) | `build/fat16/win1-r11g.img`, SHA-256 `e47ddf039cb81c3cc2c356f93db27c4c90da63b1185e844b287c1e20e604d7ea`. 51 root entries: the R11F set, `INFOCOM.AOU` and `ZORK1_Z3`. Contains the story, so it is not included in the release. |
| Shipped volume (story-free) | `win1-r11g-missing.img`, SHA-256 `1d4451af01695c401274b4fbbf84ad502be6eaab30158bb639bfa96feea0d56d`. The R11F set plus `INFOCOM.AOU`, without `ZORK1_Z3`. Root directory checked by parsing the FAT16 entries: the only difference from the production volume is `ZORK1_Z3`. |

The filename is the hardcoded `WIN1_ZORK1_Z3`. It maps to the FAT16 root entry `ZORK1_Z3` (8-character
base, blank extension, underscore kept). The open succeeds with the story present.

## 2. Full-story runs (scripted)

Input: `tests/r11g/seq-zork-game.txt` (21 commands: look, inventory, open mailbox, take leaflet,
read leaflet, north, look, east, open window, enter window, look, take lunch, inventory, west,
wait, xyzzy, score, south, look, quit, y). Each boot runs the game twice: `RUN INFOCOM.AOU`, then
`RUN` again after QUIT, with no reboot. Each transcript is compared with the host reference build
of the same patched source by `tests/r11g/compare-zork.py`. That script aligns both runs on the
`ZORK I: The Great` banner and ends at the quit prompt, strips echoes, prompts and the shell's exit
status text, and checks for anomalies.

| Platform | Round 1 | Round 2 | Round 3 |
|---|---|---|---|
| Tiny386 | PASS (games=2, both match, anomalies 0) | PASS | PASS |
| i386VM | PASS (games=2, both match, anomalies 0) | PASS | PASS |

Overall result: `ZORK COMPARE RESULT: PASS` (`scratchpad/r11g/matrix-final2/zork-compare.txt`).
The FAT16 production volume hash was `e47ddf03…` before and after the matrix (`VOLUME UNCHANGED`,
`matrix-final2/matrix.txt`). Logs and transcripts: `scratchpad/r11g/matrix-final2/`.

The host reference is the same patched source built with the host gcc (`-std=gnu89 -w`, no
`TIMESEED`), run with `WIN1_ZORK1_Z3` in its working directory (`scratchpad/r11g/host-ref/`). Its
two runs match the Tiny386 and i386VM transcripts after normalisation.

## 3. Regression against R11F

Run: the R11F regression and pinned-clock sequences (`tests/r11f/seq-regression.txt`,
`seq-pinned.txt`) on the R11G production volume, on both emulators. Compared with the logged R11F
results for the same sequence (`logs/r11f/`).

| Run | Result |
|---|---|
| i386VM, pinned | identical to the R11F log |
| Tiny386, pinned | identical to the R11F log |
| i386VM, regression | identical except the two new DIR entries (`INFOCOM.AOU`, `ZORK1_Z3`) and the file count (49 instead of 47) |
| Tiny386, regression | the same DIR differences, plus FPMULT timing counters (`ticks`, `seen`) |

The FPMULT differences are timing only. In every job, `clean`, `exact`, `exit`, `cw`, `sw`, `tw` and
`bits` are identical to R11F. The volume hash was unchanged across the runs (`regression2.log`).

## 4. Negative and damaged-input tests

Each variant is a separate volume built by `scripts/r11g-negative-volumes.sh` from the story, then
run through `scripts/r11g-negative-tests.sh` on Tiny386. Each run is followed by `look`, `quit`,
`y` and `DIR`, to confirm the shell survives.

| Variant | Input | Output (verbatim) | Interpreter exit | Shell after |
|---|---|---|---|---|
| missing | no `ZORK1_Z3` | `Failed to Open File WIN1_ZORK1_Z3` | 34 | survives |
| trunc | first 40,000 bytes of the story | `WIN1_ZORK1_Z3: story file is truncated: 40000 bytes, header needs 86838.` | 0 | survives |
| corrupt | byte 0 = 9, length field 0xFFFF | `WIN1_ZORK1_Z3: unsupported Z-code version 9 (this interpreter runs version 3).` | 0 | survives |
| oversized | story + 204,800 bytes of trailing data | plays normally (the header length is satisfied, extra bytes are ignored) | normal | survives |
| empty | 0 bytes | `WIN1_ZORK1_Z3: not a Z-code story file (0 bytes).` | 0 | survives |
| badcode | valid header, seeded random bytes from the initial PC | `Internal Error #21`, once | 1 | survives |

The `badcode` case is the fatal-error fix. Before the fix, an internal error printed `Internal Error
#21` in an endless loop until the harness pipe broke. Now `error()` ends in `quit()` and then
`exit(1)`, so the message is printed once and the program returns to the shell.

Exit statuses: the rejection messages for trunc, corrupt and empty return 0, because `main` returns 0
on that path. This is the documented behaviour, not a silent failure: the message is printed and the
interpreter does not start the game. The shipped volume lacks the story and reports the missing file
with exit 34, which is what the shell shows.

Oversized is the one variant that is not an error. The story file is larger than its header length
field; the extra bytes are ignored and the game runs. This matches the rule that a file needs to be
at least as large as the header requires, not exactly that size.

## 5. Save and restore

Out of scope for R11G. Both commands print `Failed.` and the game continues:

```
> save
Failed.
> restore
Failed.
```

Output: `scratchpad/r11g/negative-run4/saverestore.txt`. The libc has no `fopen` for writing, so the
interpreter's existing error path runs; nothing is written to the volume (the volume hash is unchanged).

## 6. Graphical QEMU VGA (scripted)

`scripts/r11g-qemu-smoke.sh`, on the `r9shell-vga` image with the R11G production volume. Scripted
xdotool keystrokes, not human-operated. The sequence runs the interpreter, `look`, `inventory`,
`open mailbox`, `take leaflet`, `read leaflet`, `north`, `look`, `quit`, `y`, `dir`, then `phchk.aou`
and `phoon.aou`.

Screenshots (`scratchpad/r11g/qemu/`):
- `qemu-01-opening.png`: the opening text, West of House, score 0/0.
- `qemu-02-after-commands.png`: the commands processed.
- `qemu-03-shell-after-quit.png`: captured after the QUIT sequence (not described further here).
- `qemu-04-phchk.png`: `PHCHK: 2 checks passed, 0 failed`, `QDOS/386 R11F PHCHK PASS`.
- `qemu-05-phoon.png`: Phoon renders and the prompt returns.

The log reports `volume unchanged by the run` with hash `e47ddf03…`.

## 7. Known limits and what is not checked

- **Licences are not established.** The story's MIT release claim has not been matched to this file's
  hash. The interpreter has no licence statement; this package does not grant one. `third_party/infocom/PROVENANCE.md`
  records both. The release is for private compatibility testing only, until those checks are done.
- **Story text in the release.** The captured transcripts and the QEMU screenshots contain the game's
  own text. They are included as test evidence, as the plan asks, and are subject to the licence
  check above.
- **Story not in the release.** `ZORK1_Z3` and any image containing it are excluded. The negative
  variants are derived from the story bytes, so they are excluded too. The scripts rebuild them from
  a local story file (`ZORK_STORY`), checked against the SHA-256 above.
- **Slot size.** The SDK README says slot 1 holds "about 1 MiB". The linker script and `docs/R5-APP-ABI.md`
  say 256 KiB (`0x240000`..`0x27FFFF`). The interpreter fits under the smaller figure, with 226,176
  bytes of headroom (`docs/R11G-ZORK1-PORT.md`). The discrepancy is recorded, not resolved.
- **Emulator identity.** The R11G logs do not record the SHA-256 of the Tiny386 or i386VM binaries that
  produced them. The fresh-extraction check records the hash of the Tiny386 binary it runs.
- **Boot disk and firmware** come from the R11F release (`build/pc-r11f-none`, `build/pc-r11f-none-run`).
  The R11G release does not contain them.
- **`clock()` is not supported,** as before in R11F.
- **Exit status** is not defined by the K&R `main`; it is documented per case above.
- **Not checked:** the R11F tarball hash (not present in this scratch area); the Phoon and
  date/clock matrices, which are R11F results and were not rerun for R11G (R11G does not change them,
  and the R11F regression above is identical apart from the DIR entries and FPMULT timing).
