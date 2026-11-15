# B2 writer counterexample — FAT32 writes NOT ACCEPTED

B0 native close/reopen/owner-exit and negative-callback tests passed. B1
read-only guest checks passed; Stage A utilities have their own accepted
checkpoint. The full B2/B3 moonshot is incomplete.

A bounded **host-only** experiment on a new disposable ordinary file demonstrated
that the historical `QDOS_APOLLO_FAT32_WRITE` switch does not implement a FAT32
writer. No accepted/reference image or guest filesystem was written by this
experiment. It is a negative result, not native write acceptance.

The current shared engine, compiled with that experimental switch, mounted a
valid independently constructed FAT32 volume, selected free cluster 70010,
created `/UNSAFE.DAT`, wrote 512 bytes and flushed. Every operation reported 0.
Independent parsing then found:

- Allocation was at cluster **70010**, but the directory entry contained
  **4474** (`70010 & 0xffff`), pointing at a free cluster.
- FAT entry 70010 contained **0x0000ffff**, a FAT16 end marker, instead of a
  valid FAT32 end marker in `0x0ffffff8..0x0fffffff`.
- `fsck.fat -n` returned nonzero: free-cluster chain, file size/chain mismatch,
  and a lost allocated cluster. It left the disposable evidence unchanged.

The cause spans the existing writer: `fat_set`/`chain_free` still store 16-bit
entries, allocation and truncation still pass `0xffff`, `dir_update`/`ent_new`
omit directory cluster high words, `fat_put_sector` assumes two consecutive
mirrored FATs, and fixed-root write/growth paths and FSInfo accounting have not
been ported. Toggling a flag or fixing only the observed directory word is
insufficient. The current header therefore deliberately rejects
`QDOS_APOLLO_FAT32_WRITE` at compile time. This safeguard does not alter the
accepted FAT16 writer or read-only FAT32 behavior.

Evidence retained under `/tmp/qdos386-r7-1wn7185l/`:

- `B2-WRITER-BLOCKER.json`: before/after hashes, exact outcomes, decoded fields.
- `logs/b2-write-counterexample.log` and `logs/b2-counterexample-fsck.log`.
- `products/UNSAFE-WRITER-COUNTEREXAMPLE.r7-disposable.img`: corrupt test output.
- `products/unsafe-host-writer`: the host experiment executable, never a guest
  program or release artifact.
- `stage-b-worktree/source/kernel/`: the exact pre-guard source used by it.

`validation/r7/fat32-write-counterexample.c` records the reproducer. Production
source now refuses its unsafe compiler flag, so reproducing the old negative
experiment requires the preserved temporary source. Do not remove the guard in
the canonical checkout merely to run it.

Next work is a complete FAT32 write/recovery port, followed by real native
mutation and independently judged failure-injection tests. It must preserve
reserved upper FAT bits, active/mirrored FAT semantics, high directory cluster
words, root growth, zeroing, advisory accounting, open-file exclusion and
physical-LBA cache coherence. Only after that gate may FAT32 become the system
volume with the existing WIN1 ABI. No B2/B3 tag, writable FAT32 candidate, or
claim of power-loss recovery is made here. The accepted fallback remains intact.
