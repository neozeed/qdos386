# R7B2 representation investigation — 2026-11-15

**The demonstrated FAT32 corruption is caused by FAT16-specific source operations, not a demonstrated GCC 1.40 integer code-generation defect.** The historical compiler correctly executes all 110 representation checks at both supported optimization settings. A separate, reproducible compiler crash on struct-by-value calls was found; it does not explain the writer's scalar/pointer call paths. FAT32 production writes remain compile-time disabled. No writer implementation or acceptance claim is made.

Evidence directory: `/tmp/qdos386-r7b2-compiler-62ua6adw/` (abbreviated `$B` below). Raw sources, executable hashes, commands, assembly, symbols, logs and disposable images remain there. Temporary evidence is not durable across reboot; this report preserves the conclusions, essential reproducer and hashes. Earlier durable negative evidence remains in `reports/r7/B2-WRITER-COUNTEREXAMPLE.json` and `B2-WRITER-BLOCKER.md`.

## C0: provenance and measured ABI

Baseline and final HEAD: `bd6f55f51acfb75acfd728a74484f30323ba335e`, branch `r7-native-disk-tools`. Accepted tags remain `apollo-r7a-disk-tools` at `3e49154` and `apollo-r7b1-readonly` at `3aa5998905d6ed989ee4edd2c7bdbfb29c56c3f5`. All refs are unchanged. The two existing untracked instruction files were preserved.

The compiler was copied from `/tmp/qdos386-r7-1wn7185l/hosttools` to `$B/hosttools`. Its hashes match both the accepted R7 toolchain receipt and the currently present `/data/projects/qdos386.old/hosttools` executables. This location was verified, not assumed. `cc1 -version` identifies **GNU C 1.40, 80386 BSD syntax, built with GNU C 13.3.0**, default target switch `-m80387`. GAS reports **1.38**; LD reports **BFD 1.94**. The `i386-gcc` command is a shell wrapper, not the compiler executable.

| Historical component | SHA-256 |
| --- | --- |
| `bin/cc1` | `1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb` |
| `bin/cccp` | `a5d10c5c176cad35dae155194816e9633b923e8d3fafab100bdb6050dcf23cb5` |
| `bin/as` | `47369dd54851448080ce6836304829ea6ae98fbcb8efa944386c0ec0239d1009` |
| `bin/i386-ld` | `dfdb396514559958d9bd1ed651337abded3565f6665d8bad05dcaeec92d07c20` |
| `bin/i386-gcc` wrapper | `3567f3beab83e17d7f7b5ff5844ee1dd85fd452b84128bdc0a7d46bb103d7904` |

The original unsafe writer is an **x86-64 Linux executable built with host GCC 15.3.0 (Ubuntu 15.3.0-4ubuntu1)**, not guest-target GCC 1.40. Its ELF compiler comment identifies that version. Rebuilding the preserved pre-guard source with the recorded host command produces the **identical executable SHA-256** `fcf0f59de43d3aede39e022e7d8ac11a8b737d7001d55ef1b7cb6ff11d135679`. Modern comparisons used GCC 15.3.0 and Clang 22.1.8. Full tool paths/hashes and version results are in `evidence/compiler-provenance.json`, `evidence/host-rebuild.json` and `logs/*version.log`.

Actual GCC 1.40 executables ran under the accepted R7B1 kernel on a new disposable copy of its single-disk image, using four-CPU QEMU TCG. The SDK startup and `libqdos.a` were copied from the retained verified SDK solely for console/exit support; probe code was freshly compiled. No accepted kernel or shell was rebuilt or substituted.

| Measurement | Historical QDOS i386 | Modern Linux i386 | Modern Linux x86-64 |
| --- | --- | --- | --- |
| `sizeof(char, short, int, long, pointer)` | 1, 2, 4, 4, 4 | 1, 2, 4, 4, 4 | 1, 2, 4, 8, 8 |
| Plain `char` | signed | signed | signed |
| Member placement after a leading char: char/short/int/long/pointer | 1/2/4/4/4 | 1/2/4/4/4 | 1/2/4/8/8 |
| Inner `{ ushort s; uint i; ulong l; }` size | 12 | 12 | 16 |
| Nested `{ char c; inner in; uchar tail; }`: size/in/tail offsets | 20/4/16 | 20/4/16 | 32/8/24 |

Generated target assembly confirms 32-bit stack arguments (`8(%ebp)`, `12(%ebp)`, etc.), scalar returns in EAX and caller stack cleanup. Separately compiled scalar/pointer functions execute successfully in the guest. Packed on-disk struct overlays are unnecessary and were not used. Historical a.out symbols were decoded directly into `evidence/historical-symbols.json`; assembly listings include complete function bodies.

## C1: probes and the separate compiler limitation

The same strict-C89 `probes/probe.c`, `probe.h` and separately compiled `ops.c` exercise all seven requested values: 65535, 65536, 70010, 131071, `0x0ffffff8`, `0x0fffffff`, `0xffffffff`. They check unsigned-short conversion and promotion; uint/ulong arguments, returns, globals, locals and pointers; nested struct assignment; unaligned bytewise LE32 serialization; 28-bit masks and reserved high bits; EOC; directory words; signed/unsigned comparisons; unsigned shifts/wrap and sector-bound arithmetic. Each executable reports **110 checks, zero failures, exit 0**.

| Execution | Settings | Result |
| --- | --- | --- |
| GCC 1.40, actual QDOS guest | wrapper `-O0`, normal `-O` | 2/2 pass |
| GCC 15.3, Linux i386 | `-O0`, `-O`, `-O2` | 3/3 pass; complete output identical to guest |
| Clang 22.1, Linux i386 | `-O0`, `-O`, `-O2` | 3/3 pass; complete output identical to guest |
| GCC 15.3 and Clang 22.1, Linux x86-64 | each `-O0`, `-O`, `-O2` | 6/6 pass; value results identical, ABI differences reported |

The historical wrapper defaults to `-O`. Its supported `-O0` means **omit `-O` when invoking cc1**; it does not pass a modern `-O0` switch to cc1. Wrapper `-O2` and `-v` are explicitly rejected with exit 2; no unsupported setting was silently used. `cc1 -version` provides its identity. Modern builds use `-std=c89 -pedantic-errors`; i386 comparisons are freestanding Linux executables with a small syscall adapter, requiring no installed 32-bit libc. Clang needed `memcpy` for struct assignment; an assembly implementation was added only to that temporary adapter. The initial link failure is retained. All shifts have unsigned operands and counts below their width; casts to unsigned short intentionally reduce modulo 65536. No signed overflow, negative signed shifts or unaligned typed dereferences are required for these results.

Probe sources and native outputs:

| File under `$B` | SHA-256 |
| --- | --- |
| `probes/probe.c` | `a05cbae38ad70152f5d69db43fad7f1f902a7fcd976f72d586b151a515c5cdbb` |
| `probes/ops.c` | `1a8a89677e499728eb2ca0c58b19561ec8fd8ae0b9a925f3e864f007440b96da` |
| `products/OLDO0.AOU` | `b76377c75499d1577351b27aef802835318ac778e65b770855408a9846088e99` |
| `products/OLDO.AOU` | `3d514411dd7988a2f1698253bf42149645149a521f60db98e41907b7fdcd4170` |

Exact argv and exits are in `evidence/commands.json`; reproducible build logic is in `run-builds.py`. In outline: historical `i386-gcc -O0|-O -S` per C file, historical GAS, then historical LD with copied SDK `crt0.s`, `ld/slot1.ld`, `libqdos.a`; modern `gcc|clang -std=c89 -pedantic-errors -O0|-O|-O2`, with `-m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -no-pie` and `probes/linux32.s` for i386. `target-test.py` records the QEMU command and two `RUN` commands. No full regression matrix ran. `evidence/probe-results.json` records exact output comparisons, and `evidence/SHA256.json` inventories retained artifacts.

**Additional finding: compile-time crash, not observed wrong machine code.** The initial broader ABI probe passed a struct by value and crashed cc1 at both historical settings. It was preserved as `probe-initial-struct-call.c`/`crashing.i`; the remaining representation probe uses struct assignment and pointer arguments. This independent three-line C89 reproducer also crashes at both settings:

```c
struct S { unsigned int a,b,c; };
unsigned int sum(struct S x) { return x.a+x.b+x.c; }
int main(void) { struct S x; x.a=1;x.b=2;x.c=3;return sum(x)!=6; }
```

`probes/struct-by-value.c` SHA-256: `ca91863937968caac02961d302c610ddcd20f53c11ee5b9126aee2921a04c729`. Historical compilation exits 139; modern GCC and Clang compile and execute it successfully at `-O0`/`-O` on both host x86-64 and i386. The original backtrace reaches `move_by_pieces` through `emit_push_insn` and `store_one_arg`. This establishes a limitation of **this historical compiler executable**; attribution to original GCC versus its modern-host port remains **UNRESOLVED**. It is not evidence of truncating scalar values incorrectly. Keep future writer APIs pointer/scalar based, as the relevant existing paths already are. No compiler replacement or integer-serialization workaround is justified by the FAT corruption.

## C2: source, assembly and on-disk attribution

Call path: `fat16_open_path` creates the empty directory entry; `fat16_write` → `append_cluster` → `fat_find_free` selects 70010 → data-sector write → `fat_set(nc, 0xffff)` → optional predecessor `fat_set(prev, nc)` → `f->first = nc` → `fat16_flush` → `dir_update` → sector write. `fat_set` calls `wr16` and `fat_put_sector`. Callback success means I/O succeeded, not that FAT32 metadata is valid.

The copied pre-guard `fat16.c` is byte-identical to canonical `source/kernel/fat16.c`; only the isolated historical header permits the old unsafe flag. Canonical `fat16.h:20-23` was never weakened. Full pre-guard engine assembly was generated with historical GCC and modern GCC at both no optimization and `-O`. Modern strict-prototype/implicit-function/incompatible-pointer checks pass. The functions below have visible matching prototypes/definitions, scalar parameters and pointer arguments; no struct-by-value call or implicit-int mismatch explains the two fields.

Types below describe the measured 32-bit target; uint/ulong/uchar abbreviate unsigned int/long/char. Modern x86-64 `unsigned long` is 64 bits, as explicitly measured above.

| Value and exact canonical location | Declaration / parameter / return | Intermediate expression | RAM width → disk width; first loss |
| --- | --- | --- | --- |
| Allocation hint/output, `fat16.h:92`, `fat16.c:632-656` | `uint next_free`, `uint c`, `uint *out`; status `int` | `c + 1` unsigned int | 32 → none; 70010 retained |
| Allocated cluster, `fat16.c:954-984`, `fat16.h:100` | `uint nc`, `uint first/cur`; append status `int` | assignment to `f->first` | 32 → none; 70010 retained |
| FAT address, `fat16.c:614-627` | `fat_set(v, uint c, uint val)` returns `int`; `ulong off`, `uint idx` | `(ulong)c * FAT_WIDTH(v)` is ulong; division/remainder unsigned | 32-bit cluster → 32-bit byte offset; 70010*4 = 280040, sector index 546, byte 488; no loss |
| FAT EOC, `fat16.c:972` | literal `0xffff` is 32-bit int, converted to uint parameter | value is already 65535 | Wrong constant chosen **at caller**, before code generation; FAT32 needs `0x0ffffff8..0x0fffffff` |
| FAT store, `fat16.c:626`, helper `:525-529` | `wr16(uchar *p, uint v)` returns void | uint `v & 0xff`, `(v >> 8) & 0xff`, then uchar casts | 32 → two 8-bit stores; ignores bits 16..31; FAT32 slot requires four bytes with reserved-bit handling |
| Directory first cluster, `fat16.c:751-761` | `dir_update(..., uint first, ulong size)` returns int | `wr16(sec + off + 26, first)` | 32 → low 16 only; high word at +20 untouched; 70010 becomes 4474 |
| New directory entry, `fat16.c:2455-2465` | `ent_new(..., uint first)` returns void | zero entry, `wr16(e+26, first)` | 32 → 16; high word remains zero |
| Directory size, `fat16.c:759`, `:531-537` | ulong size / `wr32(uchar *, ulong)` returns void | unsigned-long shifts by 0,8,16,24 | 32 → four bytes; correct for tested size 512 |
| Reader, `fat16.c:25-39` | `rd32` returns ulong; `entry_first` returns uint | unsigned casts before shifts; high word masked to 12 bits | Four bytes → 28-bit cluster; correctly reads what faulty writer stored |
| Metadata utility, `fatmeta.c:7-10`, `fatmeta.h:6` | `fm_u32` returns uint; `fm_p32(..., uint)` returns void | two 16-bit stores, second value shifted by 16 | Four-byte uint compile assertion, explicit four-byte encoding; separate from faulty writer |

Historical engine `fat16-old-O0.s:2185-2198` implements `wr16` exactly as written:

```asm
movl 8(%ebp),%eax
movb 12(%ebp),%cl
movb %cl,(%eax)
movl 8(%ebp),%eax
incl %eax
movl 12(%ebp),%edx
shrl $8,%edx
movb %dl,(%eax)
```

`_dir_update` at assembly lines 2912-2917 pushes the full first-cluster argument, adds 26 to the entry address and calls `_wr16`. `_append_cluster` explicitly emits `pushl $65535` before `_fat_set`. No earlier 16-bit parameter narrowing occurred. Modern i386 `source_bad_dir` likewise emits `movb %dl,26(%eax)` and `movb %dh,27(%eax)`. Both compilers faithfully implement the same incomplete serialization. Full numbered excerpts are in `evidence/assembly-excerpts.txt`.

The original unsafe executable was copied and run only against a new ordinary disposable FAT32 file. Independent decoding and read-only Linux fsck reproduce the original evidence exactly:

| Observation | Result |
| --- | --- |
| Before image SHA-256 | `191dfbc8b156548e909084e5d361e063025208311c9d400c6c07f190447bc5cb` |
| After image SHA-256 | `079dc0c41d79e0c4fcb61ec30f22e828499207fb10629f97c8c30098198516ff` — identical to preserved negative output |
| Program result | mount/open/write/flush 0; 512 bytes; first 70010; six sector-write callbacks |
| Directory cluster bytes | high `00 00`, low `7a 11` → 4474; correct high word would be `01 00` |
| Both FAT copies, entry 70010 | `ff ff 00 00` → `0x0000ffff`; not a FAT32 EOC |
| Changed 512-byte volume sectors | **578, 1587, 2055, 72058**; exact changed byte offsets retained in `evidence/counterexample.json` |
| `fsck.fat -n` | before exit 0; after exit 1: free-cluster reference, chain/size mismatch and lost cluster; fsck did not change the file |

The guest probe disk's two partitions passed `fsck.fat -n` before execution; the entire image hash was identical after execution, so those same partition results still apply. `decode-fat32.py` independently walks chains/directories, hashes file contents and checks allocation ownership using Python byte offsets, without the QDOS engine. It finds no errors in the pristine negative fixture or guest FAT32 partition, and detects the free-cluster reference, size mismatch and lost cluster 70010 in the corrupt output (`evidence/independent-fat32-decoder.json`). No guest filesystem mutation was used to attribute the unsafe host-only result.

## Remaining writer inventory and classification

| Finding | Classification and next verification |
| --- | --- |
| 16-bit FAT writes and missing directory high words | **SOURCE FAT16 ASSUMPTION**, proven by source, both compilers and exact bytes. Fix all serialization sites, including mkdir/dot/dotdot and update paths. |
| FAT16 EOC callers | **SOURCE FAT16 ASSUMPTION**, at `fat16.c:972,1164,1484,2658` (append, truncate, directory growth, mkdir). Correcting stores alone still writes an invalid marker. |
| Free-chain writes | **SOURCE FAT16 ASSUMPTION**, `:726` clears only low 16 bits; must clear all 28 payload bits while retaining reserved high bits. |
| Mirroring/active FAT | **SOURCE FAT16 ASSUMPTION**, `:605-611` always writes `fat_start` and `fat_start + fat_sectors`. Mount already distinguishes one/two FATs and active FAT at `:112-114`; active FAT1 would make the second write fall beyond the FAT area. |
| Root growth | **SOURCE FAT16 ASSUMPTION**, `:1462-1463` and `:1542-1543` reject root identifier zero. FAT32's root is a growable chain; fixed-root APIs also require an inventory before enabling writes. |
| FSInfo | **SOURCE FAT16 ASSUMPTION / missing writer integration**. `:122-130` validates signatures; allocation/free update only in-memory `next_free`. Primary/backup advisory counts/hints need defined update or unknown-value policy. Existing `fatmeta` advisory repair is not integrated file mutation. |
| Zero-fill/publication | **UNRESOLVED acceptance**. `append_cluster` writes only the first sector before linking; `cluster_zero` does cover all sectors for directory allocation. Require explicit multi-sector file-cluster zeroing/order policy and stale-data/failure tests; no new leak claim was established here. |
| Failure handling | **UNRESOLVED acceptance**. Multi-step writes have no transactional rollback; write/flush success is not semantic validation. Partial mirrored writes, link/directory publication, short-write reporting, truncate/delete failures need injection and independent checks. |
| Cache coherence | **UNRESOLVED for a future FAT32 writer**, not proven broken: existing `win1.c:142-153` translates to physical LBA, `platform/pc/disk.c:265-267` routes writes through `r20c_write`, and `r20w.c:1052-1065` invalidates before/after writes even on error. Preserve that route and test remount/open-file/cache behavior. |
| Width/layout assumptions and bounds | **C89/ABI PORTABILITY**, not the observed cause. LP64 host size/offset arithmetic can conceal 32-bit unsigned wrap; e.g. `fat16.c:1009` size rounding needs checked bounds. `:1018` retains a 65536-cluster file limit. Keep i386 tests, explicit widths and subtraction-based range checks; no signed-shift UB explains this counterexample. |
| Struct-by-value compiler crash | **UNRESOLVED** original-compiler versus host-port cause; reproduced compile failure. Scalar/pointer APIs avoid it. Separate from FAT field corruption. |
| GCC 1.40 wrong integer machine code | **No PROVEN GCC 1.40 CODEGEN finding** in these tests. This is bounded evidence, not a claim that the compiler is universally defect-free. |

## C3: bounded implementation and acceptance recommendation

1. Keep accepted R7A/B1 and the production write guard intact. Introduce an explicitly 32-bit, masked 28-bit cluster interface with compile-time width checks, scalar/pointer arguments, explicit LE accessors, separate FAT16/FAT32 EOC/bad/reserved rules and checked geometry/size arithmetic. Avoid on-disk struct casts and by-value aggregates.
2. Implement allocation/link/free/truncate and all directory high/low-word updates together. Preserve reserved FAT bits. Honor FAT count, mirror mode and selected active FAT. Handle FAT32 root growth, dot/dotdot semantics, whole-cluster initialization and bounded directory/file growth.
3. Define primary/backup FSInfo advisory handling, write ordering, flush boundaries and recovery outcomes. Propagate partial-I/O failures; never silently declare a corrupt result accepted. Preserve exclusive-writer and open-file rules and physical-LBA cache invalidation.
4. First use disposable host-engine differential tests against an independent decoder/Linux tools, including high clusters and boundary values, one/two FATs, active FAT1, root not at cluster 2, multi-sector clusters, growth/shrink/delete/mkdir/rename, fragmented/full volumes and reserved bits. Add targeted failure injection at each mutation boundary, including mirror and directory publication failures.
5. Then run native GCC 1.40 guest mutation tests on disposable secondary volumes. Close/reopen and reboot, independently check fsck/content/hashes, exercise I/O failures and cache/owner-exit behavior, and compare against the same defined host outcomes. Preserve negative evidence; do not relax oracles.
6. Only after these writer/recovery gates pass, consider FAT32 WIN1 system-volume boot and application tests. Full matrix requalification belongs to that later acceptance decision, not this investigation. No compiler replacement is required by the demonstrated serialization faults; isolate compiler-port repair separately if by-value aggregate support becomes necessary.

## Preservation and stop point

`evidence/preservation.json` verifies all **2348 original checkout files**, including untracked inputs, by contents/symlink target and mode: unchanged. HEAD, branch and Git refs are unchanged; the existing two instruction-file status entries are preserved. The sole checkout addition is this report, left uncommitted for review. No source, Makefile, toolchain, SDK, application, ROM, fixture or historical resource was modified. No package installation, network operation, publication or push occurred.

The accepted R7B1 image remains `49b211df1686e7f4f07a8b24f331e5c9e80b95997ead97cfab39b117fc4b888c`; the fallback remains `3b3ceb41210c9726af2cd65c55225a5571f9df7193c96d722ce017506560396a`. The original fixture and negative image retain the before/after hashes above. All generated probes, builds, logs and new images remain under `$B`. The compile-time FAT32 write guard is byte-identical to baseline. Investigation complete; no FAT32 write implementation started.
