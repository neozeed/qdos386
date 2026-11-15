# R7B2 compiler gate C0 — provenance established

Started from `bd6f55f51acfb75acfd728a74484f30323ba335e`. Safety ref:
`r7b2-safety-bd6f55f`; active investigation branch: `r7b2-compiler-hardening`.
Accepted refs and original working-tree inputs are preserved. Work directory:
`/tmp/qdos386-r7b2-hardening-urqeomc5/`.

The user-supplied archive
`/data/projects/qdos386.old/instructions/qdos386-R14B-GCC-ENV-SDK.tar.gz`
contains the accepted host cc1 (`1dd52eeb51191b0f1e6f92fcc9ed0be9e5fb80a97f28e41ddddb479f7399effb`),
the staged compiler sources, generated parser/backend inputs, original-input
manifest, R11L repair patch and host build recipe. All 289 compiler sources and
13 generated inputs match that archive's SHA256SUMS. Reverse-applying its patch
with zero fuzz restores the original LP64 host description and faulty expr.c.
The reconstructed source checkpoint is a separate Git repository:
`/tmp/qdos386-r7b2-hardening-urqeomc5/compiler-source`, baseline commit
`48929349e96e5401ef891d321f4958c64e46ed1f`.

The older ORIGINAL.SHA256 matches 21 recovered files directly and 249 after
restoring CRLF, including faulty expr.c. Nineteen do not match either uniform
line-ending form; their raw pre-normalization bytes are not claimed recovered.
Their normalized source provenance is pinned by the archive manifest and
staging/patch recipe. No legacy source Git history or stock-GCC distribution was
found. The archive's already repaired host binary is preserved as evidence,
not substituted for a freshly rebuilt compiler.

The accepted cc1 identifies GCC 1.40, i386 BSD syntax, built by GCC 13.3.
GCC 13.3 is unavailable here; a new unmodified reconstructed-host build with
GCC 15.3 GNU89 reproduces the same compiler SIGSEGV. This independently links
the recovered source to the fault, without claiming host executable identity.

The exact new-instruction C89 reproducer is permanently retained at
`validation/r7/compiler/aggregate-min.c`. The accepted compiler fails from both
normal C input and preprocessed input, without optimization and with `-O`:
wrapper exits 139; direct cc1 is killed by SIGSEGV. Direct cc1 never receives
modern `-O0`/`-O2` options. Debugger evidence shows BLKmode, size 12, alignment 4,
then an invalid source RTL pointer (`0x1a`) in move_by_pieces.

Root-cause evidence: expr.c:1305-1306 puts xinner, length and alignment inside
`gen_rtx(...)`, leaving only one argument to the four-argument
`move_by_pieces`. The earlier documented R11L guest patch corrects exactly that
parenthesis. This is a legacy source call-arity defect; stock-GCC versus legacy
edit provenance is not independently established. No pointer truncation repair
or target ABI change is indicated.

Raw provenance, archive inventory, source hashes, commands, debugger arguments,
RTL size and full disassembly are under `evidence/` and `logs/` in the work
directory. Subsequent compiler repair and qualification are still gated; FAT32
writes remain disabled and Part II has not started.
