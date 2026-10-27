# R11B licence and provenance

Status: engineering record, not a legal opinion. Private development and
testing continue. Public redistribution is a separate decision for the project
owner, listed in the checklist at the end. This file records what is known; it
does not claim that anything is cleared for distribution.

## Source-level provenance

| Component | Where | Origin | Notice found | Status |
|---|---|---|---|---|
| R11B stdio, libqdos additions, guest tests | `sdk/libc/stdio.c`, `sdk/examples/r11b/` | Written for this project during R11A/R11B | none | Project code. The owner decides its licence. |
| R11A/R9 libc (`string`, `stdlib`, `ctype`, `malloc`) | `sdk/libc/` | Written for this project | none | As above. |
| C68 LIBC_A 4.24.5 | Reference only, not in the repository. Extracted from the attached `libc-4.24.5.tar.bz2` into a scratch tree | Historical 68000 C library | Mixed (see below) | **Reference only.** No C68 source file was copied or linked. Behaviour was compared with the C68 sources (`docs/R11B-LIBC-MAP.md`). |
| GCC 1.40, GAS 1.38, i386-ld | Historical toolchain tree, outside the repository (`QDOS386_TOOLS`) | GNU | GPL. GCC 1.40 predates the runtime-library exception. | Used as a compiler and linker. Its output (the a.out files) is not GNU code, but see the runtime note below. |
| compiler runtime (`gnulib`/libgcc helpers) | Historical tree; linked by the toolchain when needed | GNU | GPL with runtime exception only where the file carries it | Not replaced or modified in R11B. Keep separate from libc. |
| Tiny386 emulator and FPU patch | `fpu-diag/tiny/`, `fpu-round-nearest.patch` | Tiny386 upstream plus project patch | Upstream licence applies | External. Hash recorded in the package manifest. |
| LGTEST4.aout (3,576-check suite) | Owner's R10 FPU package; included in the R11B volume as `LGTEST.AOU` | Owner's work | To be confirmed by the owner | **Redistribution to be confirmed.** |
| FPJOB.aout (tested fixture) | `tests/r10/FPJOB.aout` | Owner's R10 work; no matching source (see the runtime report) | To be confirmed | External frozen fixture. |
| BIOS and VGA BIOS | `firmware/` in the package | SeaBIOS and VGA BIOS builds (known-good hashes) | Upstream licences | Included as tested. Check the upstream licence before redistributing. |

## C68 notices, as found

- `libc-4.24.5/stdio/stdiolib.h`: "(C) Copyright C E Chew". Conditions: do not claim
  authorship, and keep the copyright notice intact. This is a permissive notice
  with conditions. It applies to the file that carries it, and was not copied here.
- `libc-4.24.5/init/conqpac.c`: credits Eric Slagter for an original contribution
  and carries "(c)E.Slagter, 1994" in `_copyright`.
- Of 523 C and header files, 19 carry a notice-like line. The rest carry none.
- **No licence file** (`COPYING`, `LICENSE`, `NOTICE`) exists in the tree.
- Under the GNU policy, the GCC runtime-library exception applies only to files
  that carry its notice. It does not extend to GCC 1.40 files, and it does not
  apply to C68 code.

## What R11B relies on

- Compiling, linking and testing with the historical toolchain: no redistribution.
- Reading C68 sources to compare behaviour. No code was copied into the SDK; each
  R11B function was written from the C89 standard and the QDOS/386 trap ABI.
- Using the GCC 1.40 toolchain's output (the a.out executables) in the repository.
  This is the normal use of a compiler and is not, by itself, a distribution of
  GNU code.

## Distribution checklist (deferred)

Before the R11B archive is shared outside the project:

1. Owner decides the licence for project code (`sdk/`, `kernel/`, `apps/`, `tests/`).
2. Owner confirms LGTEST4.aout and FPJOB.aout may be redistributed, or they are
   removed and their hashes kept.
3. If the C68 reference tree is shipped, its notices stay in place and the
   relevant GNU and C68 licence terms are reproduced. Otherwise the tree is not shipped.
4. The historical GCC, GAS and i386-ld binaries are not shipped. Provide their
   hashes and the instructions to obtain them.
5. Firmware (SeaBIOS, VGA BIOS): confirm the upstream licence before shipping.
6. Relinkable form: an LGPL-type component in a statically linked program needs a
   relinkable object or source form. Confirm before shipping any binary that links
   a third-party library.

## Reference

- GCC runtime-library exception FAQ: https://www.gnu.org/licenses/gcc-exception-3.1-faq.html.en
- GCC runtime-library exception text: https://www.gnu.org/licenses/gcc-exception-3.1.en.html
- GPL FAQ: https://www.gnu.org/licenses/gpl-faq.en.html
