# R11D libc port map

No file from the historical C68/QDOS libc 4.24.5 tree is copied into the R11D
source. The new C code was written for this project against the C89 text; the
C68 library is the reference for intent only. Assembly is the project's own
(GAS 1.38 syntax for the historical toolchain).

| Area | Files | Origin | Notes |
|---|---|---|---|
| headers | `sdk/include/{assert,setjmp,locale,signal,time}.h`, additions to `stdio.h stdlib.h string.h math.h limits.h` | project | declarations and macros checked by the C89COR/MATHX builds |
| setjmp/longjmp | `sdk/libc/jmp.s` | project | saves ebx, esi, edi, ebp, esp (caller's, after return), eip; `longjmp(env,0)` returns 1 |
| strings, ctype, stdlib | `sdk/libc/string.c`, `stdlib.c` (R11A/R11D additions) | project | `strcoll` = `strcmp` (C locale) |
| scanf family | `sdk/libc/scanf.c` | project | one engine over a string or FILE source; one pushback character; `%n` counts characters consumed |
| printf precision | `sdk/libc/stdio.c` | project | R11D: integer precision now zero-pads (C89 7.9.6.1); `%.0d` of zero prints nothing |
| time | `sdk/libc/time.c` | project | `asctime`, `difftime` work; calendar functions fail |
| misc (assert, locale, signal, file-blocked APIs, gets, fgetpos/fsetpos) | `sdk/libc/misc.c` | project | see `docs/R11D-BLOCKED-APIS.md` |
| exit path | `sdk/lib/crt0_libc.s`, `sdk/lib/qdos.c` (`qd_exit_record`), `sdk/libc/stdlib.c` (`atexit`, `exit`) | project | returning from `main` runs atexit handlers and flushes stdio, then records the status with the kernel without the suspend loop |
| transcendental kernels | `sdk/libm/xtrans.s`, `sdk/libm/trans.c` | project | x87 kernels plus C wrappers; see `docs/R11D-LIBM-MAP.md` |

## Defects found and fixed during R11D

- Every program that returned from `main` stalled the shell (the exit path looped on the suspend trap). Fixed by `qd_exit_record`.
- `log` read its base argument from the wrong stack offset and computed log10 for most inputs. Fixed (`16(%ebp)`).
- `printf` ignored integer precision, so `asctime` printed `0:0:0`. Fixed.
- `fmod` via x87 `fprem` was not exact on the emulators (up to 1536 ulp). Replaced with an exact C method.
- `exp`, `sinh` and `tanh` lost accuracy for large or small arguments. Changed to Cody-Waite reduction and a Taylor series for small `sinh`.
- The volume script did not rebuild `sdk/libm`, leaving a stale `libm.a` (plan Phase 0, item 6). Fixed in `scripts/r11d-volume.sh`.
