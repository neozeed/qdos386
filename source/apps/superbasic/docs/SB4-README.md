# SuperBASIC/C89 (SB4)

A portable ISO C89 interpreter for a subset of Sinclair QL SuperBASIC, built
to match the JS ROM's observed behaviour. It runs as a hosted program on
Linux, macOS and Windows and has no dependency on QDOS or 68000 code.

**Private research. See `docs/PROVENANCE-LICENSING.md` before any use or
distribution.** This package is not freely redistributable, and nothing in it
is a licence grant.

## What it does

- Numeric and string literals, arithmetic with the ROM's precedence, string
  concatenation with `&`, comparisons, `AND`/`OR`/`XOR`/`NOT`, `DIV`/`MOD`.
- Numbers printed with the QL display rules and the QL float's precision.
- Variables (numeric and string), `LET`, `PRINT` with `;` and `,` tabs.
- Numbered program lines: store, replace, delete; `LIST` (with `#n`), `RUN`,
  `RUN n`, `GOTO`, `IF ... THEN`, `FOR`/`NEXT` with `STEP`, `STOP`,
  `CONTINUE`, `NEW`, `CLEAR`, `REM`.
- `LEN`, `ABS`, `ERNUM`; error codes as the ROM reports them.
- `INPUT` (numeric and string, with `#n` for channels 0 to 2) from the same
  stream as the program, so a file can supply its own input.
- `GOSUB` and `RETURN`, with a bounded return stack.
- `DATA`, `READ` and `RESTORE` for the verified subset: items are read in
  order across lines, numbers and strings are coerced as the ROM does, and
  `RESTORE` moves the cursor. The cursor survives `RUN`.
- `DIM` numeric arrays with any number of subscripts (0 to n); `DIM` string
  arrays with one subscript, which are fixed-length strings; string slices
  `s$(i)` and `s$(i TO j)`, read and assigned.
- Channels 0 to 3 for `PRINT #n`, `INPUT #n` and `LIST #n`, through opaque
  channel handles with explicit status and byte counts
  (`docs/SB3-CHANNEL-ABI.md`).
- SB4: `DEFine PROCedure` and `DEFine FuNction` with `END DEFine`, `RETurn`,
  `LOCal`, parameters (by reference for variables and numeric array elements,
  by value for expressions), nested calls and recursion to depth 5500 (the
  ROM's ceiling is between 5000 and 6000). Shallow dynamic scope. The verified
  cases are in `docs/JS-SB4-MAP.md`; the differences are in
  `docs/KNOWN-LIMITATIONS.md` section 13.

It does not do two-subscript string arrays (section 12 of
`docs/KNOWN-LIMITATIONS.md`), a DEFine nested inside another body, graphics,
files, most built-in functions, or a QDOS/386 backend. `docs/KNOWN-LIMITATIONS.md`
lists everything that differs from the ROM. Passing the corpus is not a claim
of full SuperBASIC compatibility.

## Build and test

```sh
scripts/build.sh                 # gcc and clang, strict C89, -Werror
scripts/test.sh                  # corpus and examples with gcc
CC_TEST=clang scripts/test.sh    # same with clang
```

Run a program from a file or from stdin:

```sh
build/gcc/sb < examples/times_table.bas
```

## Regenerating expectations from the ROM

The corpus `.out` files come from the ROM, through the QLBox emulator:

```sh
export QLBOX_DIR=/path/to/extracted/qlbox
python3 tools/gen_expected.py tests/corpus/*.bas
python3 tools/sweep.py build/gcc/sb 1 48     # random differential sweep
# tests/probes/*.json are the Phase 0 probe batches; tests/probes/runp.py runs them
# tests/probes/sb3/*.txt are the SB3 probe scripts; tools/runprobes.py runs them
# (for example: python3 tools/runprobes.py tests/probes/sb3/defproc_and_local.txt)
# tests/probes/sb4/*.txt are the SB4 probe scripts; their .out are the ROM output
```

QLBox is not part of this package. Without `QLBOX_DIR` the tools stop with a
message.

## Layout

```
src/        the C89 core and the hosted driver (sb_main.c)
tests/      sbtest.c (test runner), corpus/, known/, probes/ (SB3 probes in probes/sb3/)
examples/   example programs with ROM-generated output
scripts/    build.sh, test.sh
tools/      oracle harness, sweep, probe runner (need QLBOX_DIR)
docs/       map, architecture, state, channels, readiness, test results,
            limitations, provenance
```

## Documents

- `docs/JS-SB4-MAP.md`: the ROM behaviour SB4 implements (DEFine PROCedure and
  FuNction, RETurn, LOCal, parameters, scope, DEFine name errors), with the
  probe or corpus file behind each point.
- `docs/JS-SB3-MAP.md`: the ROM behaviour SB3 touches (DATA, READ, RESTORE,
  string DIM, channels), with source labels and whether each point was
  observed, traced or inferred.
- `docs/JS-SUPERBASIC-MAP.md`: the SB2 map, kept for the earlier features.
- `docs/ARCHITECTURE.md`: modules, data flow, memory and portability rules.
- `docs/INTERPRETER-STATE.md`: the state structure and its lifecycle.
- `docs/SB3-CHANNEL-ABI.md`: the opaque channel handle, status codes, the
  hosted adapter, and the prospective QDOS/386 contract (design only).
- `docs/SB4-QDOS386-READINESS.md`: the readiness audit for a native i386
  build: exact libc and libm use, memory, the C stack per call level, floating
  point and channels, with the build-only i386 probe.
- `docs/SB3-QDOS386-READINESS.md`: the SB3 audit, kept as the record it was.
- `docs/TEST-RESULTS.md`: what was run and what passed.
- `docs/KNOWN-LIMITATIONS.md`: every known difference.
- `docs/SB2-CHANNEL-ABI.md`: the SB2 callback interface, kept as the record of
  what SB3 replaced.
- `docs/SB2-RUNTIME-REPORT.md`: what SB2 adds, what it verifies, and what remains.
- `docs/PROVENANCE-LICENSING.md`: where the ROM-derived material comes from and
  its status.
