# F2 active-FAT compatibility decision

The user approved a narrow compatibility classification for mirroring-disabled
volumes. This resolves the checker blocker in `R7B2-FAT32-HANDOVER.md`; it does
not waive corruption or complete F2 by itself. Existing evidence is unchanged.

## Specification and checker implementation

Microsoft's **FAT32 File System Specification, version 1.03**, page 12,
`BPB_ExtFlags` at offset 40, defines bit 7 as the mirroring selector. With bit 7
set, bits 0–3 select the zero-based active FAT; with bit 7 clear, FAT updates are
mirrored. Thus `0x0081` selects FAT1 alone. An inactive copy may retain older
allocation entries. Making it match solely to appease a checker would violate
the intended active-only update policy.

[Microsoft specification, hosted by PCjs](https://www.pcjs.org/documents/papers/microsoft/MS_FAT_OVERVIEW_103-2000-12-06.pdf#page=12).
An unmodified local reference copy and extracted text are retained under
`/tmp/qdos386-r7b2-fat32-glru8dtm/f2-resume-jcqdsh61/`.

The upstream **dosfstools v4.2** `read_fat()` implementation compares the copies,
chooses the first by default, and queues synchronization of the other copy.
Explicit `-F 2` selects the second but still queues a first-copy replacement.
This explains the nonzero result even with a correct active chain. `-n` prevents
these proposed changes from being written.

[dosfstools v4.2, src/fat.c, read_fat](https://github.com/dosfstools/dosfstools/blob/v4.2/src/fat.c#L149).
The checker itself was neither patched nor replaced.

## Exact retained counterexample

Input SHA-256: `1214d800324a4c3697038629783d529a2b6b1b1fe294f708f39c09e550d9291e`.
Output SHA-256: `3f57dcaf8048d4a8905db26e0a5d31e55bccf84a2eabc92b8750115d9654cfcf`.

Independent decoding selects FAT1 from the BPB, checks every allocated cluster's
ownership and chain, traverses directories, checks file sizes and hashes, and
checks FSInfo. No structural errors or orphan allocations occur. `/HIGH.DAT`
owns cluster 70010, is 512 bytes, and hashes to
`27655cd32576d8fa434a01666ec79058a040059e471ce12f5483e937107e22c4`.

Inactive FAT0 is byte-identical to the input, including cluster 70010's
`0xa0000000`. Active FAT1 contains `0xafffffff`: FAT32 EOC with preserved upper
nibble. The directory contains the full cluster number 70010.

| Changed sector | Explanation |
| --- | --- |
| 1, 7 | Primary/backup FSInfo counts and hints marked unknown |
| 1839 | Active FAT1 sector containing cluster 70010 |
| 2554 | Root directory entry publication |
| 72562 | Initialized file cluster and exact payload |

Every changed sector is in the retained callback write trace. All other sectors,
including both boot records and the entire inactive FAT, are unchanged.

Both original Linux checks remain recorded as **exit 1**, not relabeled as clean:
default selection reports an invalid chain through inactive FAT0; `-F 2` reports
no chain error but proposes copy synchronization. This case is now
**PASS WITH DOCUMENTED CHECKER COMPATIBILITY LIMITATION**, under the user's seven
conditions. The independent decoder passes, and the specification establishes
which FAT must supply the allocation truth.

## Scope of subsequent qualification

`validation/r7/fat32-resume-f2.py` retains both Linux statuses for active-only
cases, requires exact inactive-copy preservation, accounts for all changed
sectors, and requires independent allocation/content checks. Selected-FAT checker
output is restricted to the observed synchronization/unknown-FSInfo diagnostics;
any other diagnostic fails. Ordinary mirrored cases still require Linux exit 0.
Malformed inputs and injected failures are recorded separately as negative cases;
their damaged filesystems are never described as clean or repaired before judging.

The original counterexample, both original checker logs and the passing first-run
matrix remain in place. Resume evidence lives in the new `f2-resume-jcqdsh61`
subdirectory. No compiler investigation or completed mutation matrix was restarted.
