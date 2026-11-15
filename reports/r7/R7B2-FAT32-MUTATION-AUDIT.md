# R7B2 Part II — F0 mutation contract

Starting checkpoint: `84cf893ecdc07c17f6327eb830fe50ad986349a2`
(`apollo-r7b2-compiler`). Work branch: `r7b2-fat32-writer`.
Workspace: `/tmp/qdos386-r7b2-fat32-glru8dtm`; baseline inventory is
`baseline.json`. The accepted compiler was verified before copying its tools:
`06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.
The original compiler remains unchanged for rollback.

The established negative experiment is authoritative: allocation 70010,
directory 4474, FAT end marker 0x0000ffff. Its sources, images and reports
remain preserved. Compiler C0–C4 and the 630-file comparison are not repeated.

## Mutation ledger

All shared-engine writes pass through `wr_sector`, except the legacy FAT16
write-protection probe. FAT32 must not rewrite the boot sector as a probe.

| Path | Required FAT32 semantics |
| --- | --- |
| `fat_set` / `fat_put_sector` | 28-bit entry replacement; preserve each copy's upper nibble; write only active FAT when mirroring is disabled; honor one/two copies |
| `fat_find_free`, `chain_free` | Scan full entries, exclude reserved/bad clusters, bounded traversal, full-width clearing; validate chain before freeing |
| `append_cluster`, `dir_grow`, `mkdir` | Initialize entire cluster before FAT publication; 0x0fffffff EOC; predecessor link after new cluster allocation |
| `dir_update`, `ent_new` | Both high and low cluster words; 32-bit file size independent of cluster address |
| `open_ex`, `lookup`, `remove`, `rename` | Legacy root APIs must dispatch to the FAT32 chain-backed root, retaining legacy name checks |
| `dir_sector`, `dir_capacity`, `dir_alloc_run` | Root sentinel zero maps to BPB root cluster; FAT32 root can grow; retain FAT16 fixed-root limits |
| `write`, `truncate`, `flush` | Checked file-size arithmetic, volume-bounded cluster counts; never mask metadata errors as successful short writes |
| `mkdir`, `rmdir`, `remove_path` | Full cluster words in dot/dotdot and parent entries; root parent remains sentinel zero; deletion precedes freeing |
| `vcommit`, `entry_delete`, `rename_path` | Preserve cluster words when copying entries; LFN/short-entry writes are not atomic; cross-directory file move may leave duplicate names after failure |
| FSInfo | Mark primary/backup free count and hint unknown before first mutation; never trust advisory counts as allocation state |
| `win1.c`, `disk.c`, `r20w.c` | Preserve ownership, physical-LBA cache keys, partition bounds and invalidation before/after failed writes; native integration remains a later gate |

## Representation and failure contract

Use scalar/pointer C89 interfaces, explicit byte loads/stores and a compile-time
32-bit unsigned-int check. No packed on-disk structures. Mount validates volume
geometry; cluster arithmetic is bounded by validated data-cluster counts. File
size is limited to 0xffffffff, with division/remainder ceiling calculations.

The old `QDOS_APOLLO_FAT32_WRITE` switch remains a compile-time error. A separate
qualification switch, `QDOS_APOLLO_FAT32_MUTATION`, is initially host-test-only;
existing production profiles remain read-only. No public filesystem ABI changes.

FAT is not journaled. Successful operations must pass exact content/ownership
checks and independent `fsck.fat -n`. An injected partial write may leave FAT
copies different, an allocated orphan, a size/chain disagreement, or duplicate
names during rename. Those images must remain unmodified as negative evidence;
tests must report their actual checker outcomes. A failed device write must
latch the volume against further writes and prevent close/flush from reporting
success. No automatic repair, no claim of power-loss atomicity. Ordinary
out-of-space must leave already published contents consistent.

F2 acceptance precedes native integration; F2/F3 acceptance precedes any combined
system disk. Raw evidence and all build artifacts remain under `/tmp`.
