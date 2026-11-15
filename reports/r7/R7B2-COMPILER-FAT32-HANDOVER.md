# Apollo R7B2 — compiler and FAT32 writer accepted

**C0–C4 and F0–F5 PASS.** Branch: `r7b2-fat32-writer`. The final freeze tag is
`apollo-r7b2-fat32`; code is frozen at F4 `a8c0706` / `apollo-r7b2-fat32-system`,
with F5 documentation and preservation receipts added afterward. No push or
publication occurred. All generated FAT32 artifacts remain under `/tmp` as
required by the latest instructions.

| Gate | Acceptance |
| --- | --- |
| C0–C4 | Previously accepted at `84cf893` / `apollo-r7b2-compiler`; not restarted. 131 aggregate/ABI plus 110 representation checks per setting passed; earlier 630-file production comparison had zero differences. |
| F0–F1 | Mutation audit and separately gated full-width writer: `bb35cd8`, `29e6464`; obsolete unsafe writer remains prohibited. |
| F2 | `90f5754` / `apollo-r7b2-fat32-host`: high clusters, fragmentation, exhaustion, malformed chains, one/two FATs, varied clusters, root growth, active FAT1, exact contents/ownership and 1320 failure cases. |
| F3 | `4ad51a7` / `apollo-r7b2-fat32-native`: native secondary-volume writes and reboot on 1/4/8 CPUs, 116 exact host/native injected-error comparisons, eight-reader contention and cleanup. |
| F4 | `a8c0706` / `apollo-r7b2-fat32-system`: one-disk writable FAT32 WIN1, read-only FAT16 WIN0, fallback on mount failure, GCC/HI, SH, Bounce/Hack/Elite, public channels/cache, recovery and independent checks. |
| F5 | Fresh Git extraction, sequential historical builds and 13 exact SHA-256 comparisons, including the entire 129 MiB disk. Existing read-only profile remains byte-identical. |

Compiler source repair: `9491134ab6696591fc726dc1d81446d65b8de502` in the
separate preserved compiler history. Two parentheses repair a malformed legacy
call passing one argument to a four-argument function; attribution to stock GCC
versus later editing remains unresolved. Target remains i386 a.out/C89.
Accepted repaired cc1 SHA-256:
`06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.
Original rollback cc1 SHA-256:
`1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb`.
Original and repaired binaries/source bundle remain in the existing private
compiler references; no compiler investigation or 630-file matrix was repeated.
[Compiler details](R7B2-COMPILER-REPAIR.md) preserve provenance and known unrelated
sanitizer findings.

The writer changes are confined to `source/kernel/fat16.c/.h`; the F4 mount
selection is in `source/kernel/win1.c`, selected by `make apollo-r7-rw`.
The audit covers 28-bit FAT writes, reserved upper bits, both directory cluster
words, allocation/free/truncate, growable roots, zero initialization, mirroring,
active FAT and unknown FSInfo counters. Public SDK/filesystem ABIs are unchanged.
The original 70010 → 4474 truncation and `0000ffff` EOC are corrected: the
retained active-FAT example has directory cluster 70010 and FAT1 `afffffff`,
including preserved upper nibble; inactive FAT0 stays `a0000000`.

`BPB_ExtFlags=0081` selects FAT1 with mirroring disabled. The specification and
independent decoder support the narrow checker compatibility exception; both
original `fsck.fat` results remain **exit 1**, unchanged. All five changed sectors
are explained. Ordinary valid mirrored volumes require Linux exit 0 and clean
independent allocation/content checks. See [active-FAT decision](R7B2-ACTIVE-FAT-DECISION.md),
[mutation audit](R7B2-FAT32-MUTATION-AUDIT.md), [F2](R7B2-F2-ACCEPTANCE.md),
[F3](R7B2-F3-ACCEPTANCE.md) and [F4](R7B2-F4-ACCEPTANCE.md).

The combined disk passed 1 CPU/16 MiB, 4 CPUs/32 MiB and 8 CPUs/256 MiB.
Native HI exactly matches accepted C4 output; reboot verifies it and all 700
process-exit-flushed bytes. Eight readers plus a mounted FAT32 writer recover
exact heap/frame/process counts and zero held cache/worker resources. FAT16 is
unchanged by normal FAT32 runs. With FAT32's signature deliberately invalid,
FAT16 recovery runs GCC/HI and channel tests without changing the bad partition.
Existing historical applications/assets are reused byte-for-byte. Fresh shells
match their payload copies. Elite's polled input requires sufficiently long
key holds in automation; exact observed prompts and successful quit are recorded.
No historical application sources were changed.

**Accepted pristine disk:**
`/tmp/qdos386-r7b2-fat32-glru8dtm/f5-freeze/combined/qdos386-r7b2-fat32.disposable.img`

SHA-256: `34ec7a9a0fc8dbb4f43bedf72de9fb2672d4193b3e5b725cdeab2ecde4d56074`.
This is the complete 129 MiB MBR disk, not the kernel package. Fresh VGA kernel
package SHA-256: `cc3466efd94699f1e61ccd7d6b5a99a10a0aeb55d9f1d2e5f95d088223305059`.
Use a new copy or QEMU `-snapshot` for manual testing. [F5 reproduction and
manifest details](R7B2-F5-FREEZE.md) give exact build/boot commands.

Evidence root: `/tmp/qdos386-r7b2-fat32-glru8dtm/`, with `f2-resume-jcqdsh61/`,
`f3-native/`, `f4-system/` and `f5-freeze/`. Source bundle, Git graph, accepted
artifact hashes and final preservation receipt are under `f5-freeze/`.
The original active-FAT counterexample/checker outputs and accepted R7A/R7B1/R6A
images are byte-identical. Existing accepted refs, private resources and the three
pre-existing untracked instruction files are preserved.

Limits: this is not a journaled filesystem. Injected torn writes can leave
structural damage (715 host cases explicitly recorded), and the implementation
returns/latches I/O failure instead of claiming success or repairing silently.
FSInfo free count/hint can remain unknown. Very large near-maximum 28-bit volumes
and physical hardware are not qualified. Earlier failed image assemblies and
script timeouts remain evidence. **No unresolved correctness blocker remains in
the defined gates. Stop here; no R8, export, installation, cleanup or push.**
