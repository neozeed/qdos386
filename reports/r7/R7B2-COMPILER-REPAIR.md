# R7B2 C1/C2/C3 — bounded compiler repair verified

The fault is a malformed legacy-source call, not demonstrated pointer-width
truncation or target i386 ABI failure. `move_by_pieces` needs four arguments;
misplaced parentheses put the last three inside `gen_rtx`, leaving the callee
reading unspecified host argument registers/stack slots. Restoring the two
parentheses fixes that call without any target or host-width change.

The same recovered legacy source also builds on the available 32-bit hosted
GCC environment after changing only the host long-width description/assertion
and using `-m32`; it reproduces SIGSEGV at both settings. Thus this defect is
not restricted to LP64. This is a configured legacy-source comparison, not a
reconstructed stock GCC distribution; stock-versus-legacy-edit attribution
remains unproven. The archived R11L guest source already has the correction.

Source baseline: `48929349e96e5401ef891d321f4958c64e46ed1f`.
Repair source: `9491134ab6696591fc726dc1d81446d65b8de502`, separate repository
`/tmp/qdos386-r7b2-hardening-urqeomc5/compiler-source`.
Permanent minimal patch: `toolchain/provenance/r7b2/aggregate-call-repair.patch`.
Fresh repaired host cc1 SHA-256:
`06371b4e8765b296c0082f89eecea73df441a831550dd7e294c8780981abfafc`.
The accepted compiler is retained unchanged for rollback. The source patch
was not applied to the kernel tree or existing external toolchain.

Build: GCC 15.3 GNU89, `-O0 -g -DLEGACY_HOST64 -DTARGET_CPU_I386`, archived
R11L object list and pinned generated parsers/backend tables; GAS/LD and SDK
ABI unchanged. Exact commands are `evidence/build-repaired.json` in the work
directory above. The accepted binary was built with GCC 13.3, unavailable on
this host; new host executable byte identity is not claimed.

C2 PASS: both freshly built minimal target executables ran under the accepted
R7B1 kernel with exit zero, without optimization and with historical `-O`.
C3 PASS for representation/aggregate matrix: each guest setting passes 131
aggregate/ABI checks and 110 representation checks. Aggregate output matches
GCC 15.3 and Clang 22.1 i386 at O0/O exactly. Representation output matches the
previous actual historical-target output exactly. No guest fault; image hash
unchanged. Native GCC/HI path has a separate evidence receipt. C4 production
compatibility and subsequent manual Elite acceptance are recorded separately
in `R7B2-C4-RESULTS.json` and `R7B2-ELITE-MANUAL-ACCEPTANCE.json`; the accepted
compiler-only checkpoint is `apollo-r7b2-compiler`.

Sanitizer comparison: the reconstructed broken compiler reports invalid RTL
member access at the known call and exits nonzero; repaired sanitizer build
compiles the reproducer and exits zero. UBSan also reports existing signed
integer overflow in `tree.c:582` identifier hashing and `stor-layout.c:1025`
integer bounds. Those diagnostics occur before the affected call in both
builds and remain documented, unfixed limitations. Sanitizers are evidence,
not a blanket clean-toolchain claim. No broad modernization was attempted.

All builds and raw evidence remain in the unique temporary workspace. Native
minimal/aggregate tests link actual SDK support; no synthetic result or
archive compiler binary was substituted. Permanent tests are under
`validation/r7/compiler`; machine-readable results are in
`reports/r7/R7B2-AGGREGATE-RESULTS.json`.
