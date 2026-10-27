# R11H multitasking and negative tests

Every test is a guest program run from the shell (`RUN`) or from another program, under scripted
keystrokes. Keys are injected by `tests/interactive/run_pc_multi.py` (Tiny386) or piped to the harness
(i386VM); nothing is operated by a person. Results: `docs/R11H-RUNTIME-REPORT.md`.

## 1. Programs

| Program | Slot / profile | Role |
|---|---|---|
| `MULTI.AOU` | 1 | Launcher: starts two children, checks refusals, reuses profiles |
| `MULTIA.AOU` | 2 (0x380000) | Child: ticks, checks a canary pattern, exits 3 |
| `MULTIB.AOU` | 3 (0x500000) | Child: ticks, checks a canary pattern, exits 7 |
| `COLLIDE.AOU` | 2 | Must never run: refused while MULTIA is live |
| `NEG.AOU` | 1 | Guest negative tests (section 3) |

`MULTI.AOU` and `NEG.AOU` run from the shell with `RUN`. The children run only because a program started
them with `qd_start`.

## 2. Concurrency test (`MULTI`)

1. Start MULTIA at profile 2 and MULTIB at profile 3. Both must return tagged ids.
2. Start COLLIDE at profile 2 while MULTIA is live. Must return -28 (`APP_ERR_SLOTBUSY`).
3. Start HELLO at profile 2 (wrong origin: HELLO is a slot 1 image), MULTIA at profile 3 (wrong origin),
   and NOSUCH.AOU at profile 2 (no such file). Must return -35, -35 and a negative code.
4. Wait for both. Exit codes must be 3 and 7. The children print `A tick` / `B tick` lines; their
   interleaving shows preemption on the timer tick.
5. Six reuse rounds on profile 3. Each round's id is kept. An older id must give `QD_ERR_NOJOB` only when
   a newer job has the same table index; otherwise its exit record must still read 7. At least one
   refusal must be seen, so the refusal path is exercised.
6. The program prints `MULTI: PASS` and exits 0.

After MULTI, the shell answers `DIR`. That checks the shell survived the concurrent load.

## 3. Negative tests (`NEG`)

The malformed images are built from `MULTIA.AOU` by `scripts/r11h-negative-volume.sh`, one field or byte
changed per file, on a separate test volume (`win1-r11h-neg.img`). The release volume does not contain them.

| Case | Expected | Check |
|---|---|---|
| BADMAG: OMAGIC replaced | -21 | magic |
| RELOC: trsize = 4 | -22 | relocation refusal |
| BADENT: entry 0x250000 | -25 | entry is no profile base |
| ENTOFF: entry 0x380010 | -25 | entry inside a profile, not its base |
| BADSYM: symbol count + 1 | -23 | symbol table not a record multiple |
| BIGTXT: text 0x200000 | -24 | larger than the profile |
| TRUNC: last byte removed | -20 | truncation |
| TRAIL: one trailing byte | -20 | trailing data |
| SHORT: 20 bytes | -20 | shorter than the header |
| EMPTYX: empty file | -20 | empty |
| MULTIB at profile 2 | -35 | wrong origin |
| MULTIA at profile 4 | -35 | profile does not exist |
| NOSUCH at profile 2 | -44 | no such file (`FAT_ERR_NOFILE`) |
| HELLO (slot 1) at profile 0 | -33 | wrong origin (`APP_ERR_NOTSLOT0`) |

After the refusals, the good pair (MULTIA, MULTIB) must still run and exit 3 and 7. Nothing was left
behind by the refusals.

Contention, with two children live: COLLIDE at 2 and MULTIB at 3 return -28; MULTI at profile 1 (owned by
NEG itself) returns -28. Then the good pair runs again.

Churn: 40 cycles of MULTIB at profile 3, each waited on. Every exit is 7. Every older id is checked against
the reuse rule, as in MULTI. Invalid ids (0, -1, 0x7FFF0001) give `QD_ERR_NOJOB`.

The program prints `NEG: checks N, failures F` and `NEG: PASS` when F is 0.

## 4. What is not tested in the guest, and why

- `APP_ERR_ADDROCC` (-36): the owner check runs before the region reservation, so with fixed profiles a
  conflicting range is always a busy profile first (-28). The code is defensive. The host allocator test
  covers `REGION_ERR_BUSY`.
- `APP_ERR_NOJOB` (-29): with four profiles and six jobs, a profile can be free only when the table has
  room. Not reached by the test set.
- Region exhaustion: fixed profiles, one region each. Covered by the host test (`REGION_ERR_FULL`,
  `REGION_ERR_NOMEM`).
- A failed copy after validation (the rollback in `app_load_src`): the copy source is the same validated
  file, so the path needs a read failure from the disk. Not injected. The rollback is reviewed in code.
- Phase 3 commands (`START`, `PS`): deferred (`docs/R11H-EXEC-ABI.md` section 7).
