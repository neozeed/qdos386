# GCC 1.40 aggregate regression inputs

`aggregate-min.c` is the exact three-line reproducer from the R7B2 mission.
`aggregate.c` and `aggregate-ops.c` must be compiled as separate translation
units; `aggregate.h` declares every cross-unit call. Supply a console adapter
implementing `probe_write(const char *, unsigned int)` and link the real SDK
startup/libc/libqdos for native runs. Large copies legitimately require memcpy.

The 131-check suite covers all required sizes (1,2,3,4,5,8,12,16,20,32,64),
scalar/aggregate mixed arguments, returned structures, nested returns,
recursion, function pointers, nested struct arrays, integer/pointer arguments,
SDK-supported unsigned varargs and volatile stack guards. Assignments cover
separate objects and exactly overlapping self-assignment. Partially overlapping
struct objects are deliberately excluded because that is not a defined C89
aggregate-assignment test. Padding is never used as a value oracle.

Build without optimization and with historical `-O`. Modern i386 GCC/Clang
use `-std=c89 -pedantic-errors` at `-O0` and `-O`. Exact output must match,
including `CHECKS=00000083 FAILURES=00000000`, with process exit zero.
Compiling successfully alone is not acceptance. The existing independent
110-check representation suite is separately retained in the R7B2 evidence.
See `reports/r7/R7B2-COMPILER-REPAIR.md` for source provenance and gate status.
