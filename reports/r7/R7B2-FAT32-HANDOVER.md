> Historical F2 stop record. Superseded by the accepted [final handover](R7B2-COMPILER-FAT32-HANDOVER.md); the counterexample and original checker logs remain unchanged.

# R7B2 Part II — stopped at F2, not accepted

The candidate writer is on `r7b2-fat32-writer`; source checkpoint
`29e64649de47b4f04353031a01e1dc031c166ed2`. F0 architecture is recorded in
`R7B2-FAT32-MUTATION-AUDIT.md` (commit `bb35cd8`). F1 is implemented as a
separately gated candidate. **F2 has not passed. F3–F5 were not started.**
There is no newly accepted writable FAT32 kernel or combined system disk.
The last accepted checkpoint remains `apollo-r7b2-compiler` at
`84cf893ecdc07c17f6327eb830fe50ad986349a2`.

## What passed

- The original high-cluster case now records directory cluster **70010** and
  FAT32 EOC, with exact 512-byte payload SHA-256
  `27655cd32576d8fa434a01666ec79058a040059e471ce12f5483e937107e22c4`.
  Independent ownership/content decoding and Linux `fsck.fat -n` both pass
  on the two mirrored FATs, root-cluster-7 fixture.
- The same fixture passed file growth, overwrite, append, shrink, zero extension,
  nested directories, move/rename, truncate-to-zero, rewrite, delete, rmdir and
  root growth. An initial harness assertion incorrectly treated the positive
  `getcwd` length as an error; it was corrected. Both runs remain under `/tmp`.
- Twelve subsequent successful-operation records passed exact content hashes,
  the independent decoder and unmodified `fsck.fat -n`: create, full mutation
  exercise and overflow rejection for SPC=1/2/8 with two FATs, and SPC=1 with
  one FAT. The SPC>1 cases also check that unused sectors of a new cluster
  contain zeros despite pre-seeded stale data.
- The accepted repaired GCC 1.40 compiled the candidate engine at no optimization
  and `-O`. These are compilation checks, **not native kernel/runtime acceptance**.

## Blocking independent check

The next case has two FATs, `BPB_ExtFlags=0x0081`: mirroring disabled, active
FAT1. The writer successfully creates `/HIGH.DAT` at cluster 70010. Independent
decoding finds no ownership, chain, size or content errors. Inactive FAT0 is
byte-identical to the input. Its entry remains `0xa0000000`; active FAT1 becomes
`0xafffffff`, preserving the reserved upper nibble.

Nevertheless, installed **fsck.fat 4.2 (2021-01-31)** returns **1**. By default
it reports differing FATs, selects the first FAT and describes `/HIGH.DAT` as
referencing a free cluster. `fsck.fat -n -F 2` reads the correct active FAT and
no longer reports a file-chain/content error, but still returns **1** for the
differing copies. The input passes with status 0.

This evidence points to a checker compatibility limitation for the active-FAT
policy; it does **not** establish the original narrow-store corruption or a new
writer corruption. However, the required clean independent filesystem check
has failed. No exit status was waived, no inactive FAT was synchronized, and no
image was repaired or normalized to obtain a pass. Under the strict gate, this
candidate cannot be accepted. The failure was reproduced byte-for-byte once.

All evidence is retained in `/tmp/qdos386-r7b2-fat32-glru8dtm`:

| Artifact | Relative path / SHA-256 |
| --- | --- |
| Exact input | `f2-blocker/active-fat1-before.disposable.img` |
| Input hash | `1214d800324a4c3697038629783d529a2b6b1b1fe294f708f39c09e550d9291e` |
| Actual output | `host-matrix/active-fat1.disposable.img` |
| Output/replay hash | `3f57dcaf8048d4a8905db26e0a5d31e55bccf84a2eabc92b8750115d9654cfcf` |
| Byte-identical replay | `f2-blocker/active-fat1-replay.disposable.img` |
| Full counterexample receipt | `f2-blocker/counterexample.json` |
| Checker logs | `f2-blocker/{before,after-default,after-active-fat1}.fsck.txt` |
| Independent decoding | `f2-blocker/independent-decoder.json` |
| Matrix receipts/logs | `host-matrix/results.json`, `logs/host-matrix.txt` |
| Host harness | `mutation-host`, SHA-256 `f155d5acbccb4847ab0f533cbbafd41c7b6cef170472f4e3020e82043273998a` |

Changed volume sectors are exactly **1, 7, 1839, 2554, 72562**: primary/backup
FSInfo, active FAT entry sector, root directory and file data. No inactive FAT,
boot-sector or unrelated-sector changes were observed.

Read-only reproduction of the failing judges:

```sh
fsck.fat -n /tmp/qdos386-r7b2-fat32-glru8dtm/host-matrix/active-fat1.disposable.img
fsck.fat -n -F 2 /tmp/qdos386-r7b2-fat32-glru8dtm/host-matrix/active-fat1.disposable.img
python3 validation/r7/fat32-decode.py /tmp/qdos386-r7b2-fat32-glru8dtm/host-matrix/active-fat1.disposable.img
```

The first two commands return 1; the decoder returns 0. To replay mutation, copy
the preserved input to a **new** `/tmp/*.disposable.img`, then run:

```sh
/tmp/qdos386-r7b2-fat32-glru8dtm/mutation-host --disposable /tmp/NEW.disposable.img create 70010 0 0 0
```

## Limits and continuation

The old unsafe `QDOS_APOLLO_FAT32_WRITE` remains a compile-time error. Existing
production targets remain read-only for FAT32. The new
`QDOS_APOLLO_FAT32_MUTATION` switch has only been used in isolated engine tests.
Public filesystem ABI and historical applications were not changed.

The complete failure-injection, exhaustion, malformed-chain, fragmented-mutation,
131071-boundary and native ownership/cache/resource suites are **not accepted or
complete**. Some have harness support but were not reached after the checker
failure. F1 error-latching code is therefore not claimed to have passed the full
failure matrix. FAT16 runtime regression and full candidate kernel builds remain
pending. The FAT filesystem remains non-journaled; no power-loss atomicity claim.

The next gate is still **F2**, specifically resolving the independent checker
compatibility problem without falsifying the active-FAT policy or weakening the
required checks. Resume from these retained results, not compiler C0–C4. Do not
build or freeze a writable system disk until F2/F3 pass.

Compiler used:
`/tmp/qdos386-r7b2-fat32-glru8dtm/hosttools/bin/cc1`, SHA-256
`06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.
The original compiler remains at `/data/projects/qdos386.old/hosttools/bin/cc1`,
SHA-256 `1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb`.
No compiler matrix was repeated. All raw evidence, generated images and build
products remain under `/tmp`; no packages, network actions or publication were
used. Existing acceptance refs and historical application material are preserved.
