#!/usr/bin/env python3
"""R11E C89 coverage matrix.  One row per required C89 function or macro/type.

Status comes from the EXPORTED symbols of the built libraries (scripts/r11d-symbols.py),
not from header declarations.  Usage (from the repo root, after building the SDK):
    python3 scripts/r11e-coverage.py sdk/lib docs/R11E-C89-COVERAGE
writes DIR.csv and DIR.md.
"""
import csv, os, subprocess, sys

LIBDIR, OUT = sys.argv[1], sys.argv[2]
exported = {}
for lib in ("libc", "libm", "libqdos", "libgcc-qd"):
    out = subprocess.run([sys.executable, "scripts/r11d-symbols.py", os.path.join(LIBDIR, lib + ".a")],
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        kind, name, member = line.split(" ", 2)
        if kind != "U":
            exported[name.lstrip("_")] = lib

# (header, name, kind, notes)  kind: f = function, m = macro, t = type.
# Notes give a blocker or limitation when status is not plain "implemented".
C89 = {
 "assert.h": [("assert", "m", "")],
 "ctype.h": [(n, "f", "") for n in "isalnum isalpha iscntrl isdigit isgraph islower isprint ispunct isspace isupper isxdigit tolower toupper".split()],
 "errno.h": [("errno", "m", ""), ("EDOM", "m", ""), ("ERANGE", "m", "")],
 "float.h": [(n, "m", "") for n in "FLT_RADIX FLT_ROUNDS FLT_MANT_DIG FLT_DIG FLT_MIN_EXP FLT_MIN_10_EXP FLT_MAX_EXP FLT_MAX_10_EXP FLT_MAX FLT_MIN FLT_EPSILON DBL_MANT_DIG DBL_DIG DBL_MIN_EXP DBL_MIN_10_EXP DBL_MAX_EXP DBL_MAX_10_EXP DBL_MAX DBL_MIN DBL_EPSILON LDBL_DIG".split()],
 "limits.h": [(n, "m", "") for n in "CHAR_BIT SCHAR_MIN SCHAR_MAX UCHAR_MAX CHAR_MIN CHAR_MAX SHRT_MIN SHRT_MAX USHRT_MAX INT_MIN INT_MAX UINT_MAX LONG_MIN LONG_MAX ULONG_MAX".split()],
 "locale.h": [("setlocale", "f", ""), ("localeconv", "f", ""), ("LC_ALL", "m", "")],
 "math.h": [(n, "f", "") for n in "acos asin atan atan2 ceil cos cosh exp fabs floor fmod frexp ldexp log log10 modf pow sin sinh sqrt tan tanh".split()],
 "setjmp.h": [("setjmp", "f", ""), ("longjmp", "f", ""), ("jmp_buf", "t", "")],
 "signal.h": [(n, "f", "") for n in "signal raise".split()] + [("SIGABRT", "m", "")],
 "stdarg.h": [("va_list", "t", ""), ("va_start", "m", ""), ("va_arg", "m", ""), ("va_end", "m", "")],
 "stddef.h": [("size_t", "t", ""), ("ptrdiff_t", "t", ""), ("wchar_t", "t", ""), ("NULL", "m", ""), ("offsetof", "m", "")],
 "stdio.h": [(n, "f", "") for n in "clearerr fclose feof ferror fflush fgetc fgetpos fgets fopen fprintf fputc fputs fread freopen fscanf fseek fsetpos ftell fwrite getc getchar gets perror printf putc putchar puts remove rename rewind scanf setbuf setvbuf sprintf sscanf tmpfile tmpnam ungetc vfprintf vprintf vsprintf".split()] + [("BUFSIZ", "m", ""), ("EOF", "m", ""), ("FILENAME_MAX", "m", ""), ("TMP_MAX", "m", ""), ("SEEK_SET", "m", "")],
 "stdlib.h": [(n, "f", "") for n in "abort abs atexit atof atoi atol bsearch calloc div exit free getenv labs ldiv malloc mblen mbstowcs mbtowc qsort rand realloc srand strtod strtol strtoul system wcstombs wctomb".split()],
 "string.h": [(n, "f", "") for n in "memchr memcmp memcpy memmove memset strcat strchr strcmp strcoll strcpy strcspn strerror strlen strncat strncmp strncpy strpbrk strrchr strspn strstr strtok".split()],
 "time.h": [(n, "f", "") for n in "asctime clock ctime difftime gmtime localtime mktime strftime time".split()] + [("time_t", "t", ""), ("clock_t", "t", ""), ("CLOCKS_PER_SEC", "m", "")],
}

NOTES = {
 "gets": "Deprecated in C11, required by C89; reads stdin without a size limit.",
 "tmpfile": "Needs a writable file; FAT16 is read-only. Kernel/filesystem-blocked.",
 "tmpnam": "Needs a writable file namespace. Kernel/filesystem-blocked.",
 "remove": "FAT16 is read-only. Kernel/filesystem-blocked.",
 "rename": "FAT16 is read-only. Kernel/filesystem-blocked.",
 "fopen": "Read modes only (FAT16 read-only); write modes fail.",
 "freopen": "Same stream model as fopen.",
 "mblen": "Locale is the C locale only (one byte = one character).",
 "mbtowc": "C locale only.", "mbstowcs": "C locale only.", "wctomb": "C locale only.", "wcstombs": "C locale only.",
 "wchar_t": "No wide-character library; the type is declared only if the header provides it.",
 "signal": "No delivery semantics in QDOS/386; no fake handlers.",
 "raise": "No delivery semantics in QDOS/386.",
 "SIGABRT": "Signal numbers are declared only if signal.h exists; no delivery.",
 "clock": "Needs the QDOS tick rate; see time notes.",
 "time": "No calendar clock in QDOS/386 (no RTC service in the trap ABI).",
 "ctime": "Depends on time().", "asctime": "Pure formatting; depends on struct tm.",
 "gmtime": "Needs time().", "localtime": "C locale = UTC; needs time().", "mktime": "Needs calendar support.",
 "difftime": "Pure arithmetic on time_t.", "strftime": "C locale only, depends on struct tm.",
 "system": "No command processor. system(NULL) returns 0; a command returns -1.",
 "getenv": "No environment. Always returns NULL.",
 "abort": "Terminates the job with status 3 without flushing stdio (C89 requirement is termination, not flushing).",
}

# Functions that are exported but always fail explicitly (kernel or filesystem
# service absent).  They are not counted as implemented.
BLOCKED = {"remove", "rename", "tmpfile", "tmpnam", "freopen", "signal", "raise",
           "time", "clock", "mktime", "gmtime", "localtime", "ctime", "strftime"}
# Exported and working, with a documented restriction.
PARTIAL = {"fopen", "gets", "mblen", "setlocale", "localeconv", "getenv", "system",
           "abort", "fgetpos", "fsetpos", "perror", "strerror"}

# Guest-test evidence: which example programs name the symbol, and whether that
# program is in the R11E run list (scripts/r11e-run-all.sh).
def program_of(path):
    return os.path.splitext(os.path.basename(path))[0].upper()
sources = {}
for root, _, files in os.walk("sdk/examples"):
    for fn in files:
        if fn.endswith(".c") and fn != "check.h":
            sources[program_of(os.path.join(root, fn))] = open(os.path.join(root, fn), errors="replace").read()
import re
run_list = set()
for m in re.finditer(r'"!?RUN ([A-Z0-9]+)\.AOU"', open("scripts/r11e-run-all.sh").read()):
    run_list.add(m.group(1))

def test_refs(name, kind):
    pat = re.compile(r"\b%s\s*\(" % re.escape(name)) if kind == "f" else re.compile(r"\b%s\b" % re.escape(name))
    hits = sorted(prog for prog, text in sources.items() if pat.search(text))
    return hits

rows = []
for header, items in C89.items():
    for name, kind, note in items:
        lib = exported.get(name)
        if kind == "f":
            if not lib:
                status = "missing"
            elif name in BLOCKED:
                status = "blocked"
            elif name in PARTIAL:
                status = "partial"
            else:
                status = "implemented"
        else:
            # macros and types: defined in the installed header (text search, not a compile test)
            path = os.path.join("sdk/include", header)
            text = open(path).read() if os.path.exists(path) else ""
            status = "defined" if name in text else "missing"
        hits = test_refs(name, kind) if kind == "f" else []
        guest = "yes" if any(h in run_list for h in hits) else ("no" if hits else "none")
        rows.append([header, name, {"f": "function", "m": "macro/constant", "t": "type"}[kind],
                     status, lib or "", guest, ";".join(hits), NOTES.get(name, note)])
os.makedirs(os.path.dirname(OUT) or ".", exist_ok=True)
with open(OUT + ".csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["header", "name", "kind", "status", "library", "guest_tested", "test_sources", "notes"])
    w.writerows(rows)
counts = {}
for r in rows:
    counts[r[3]] = counts.get(r[3], 0) + 1
with open(OUT + ".md", "w") as f:
    f.write("# R11E C89 coverage (generated by scripts/r11e-coverage.py)\n\n")
    f.write("Status is taken from the exported symbols of the built libraries, not from headers.\n")
    f.write("Macros, constants and types are listed for completeness and are not counted as implemented here.\n\n")
    f.write("Status: implemented = exported and working; partial = working with a documented restriction;\n")
    f.write("blocked = exported but always fails explicitly (kernel or filesystem service absent);\n")
    f.write("missing = not exported; defined = macro or type present in the installed header.\n")
    f.write("guest_tested: yes = a program in the R11E run list names the symbol; no = named only by a program outside the run list; none = no test names it.\n\n")
    f.write("| status | rows |\n|---|---|\n")
    for k in sorted(counts):
        f.write(f"| {k} | {counts[k]} |\n")
    for header in C89:
        f.write(f"\n## {header}\n\n| name | kind | status | library | guest_tested | test sources | notes |\n|---|---|---|---|---|---|---|\n")
        for r in rows:
            if r[0] == header:
                f.write(f"| {r[1]} | {r[2]} | {r[3]} | {r[4]} | {r[5]} | {r[6]} | {r[7]} |\n")
print("rows", len(rows), counts)
