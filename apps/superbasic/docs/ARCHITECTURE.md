# Architecture

The interpreter is a set of plain C89 modules. Nothing depends on QDOS, the
68000, a platform service or POSIX. Channel I/O enters only through opaque
handles (`docs/SB3-CHANNEL-ABI.md`), and memory only through the allocator
abstraction.

## Module map

| Module | Responsibility |
|---|---|
| `sb_mem` | `SbAlloc`: a context plus `alloc` and `release` function pointers. `sb_alloc_default_init` binds `malloc`/`free` and a live-block counter for the leak check. The only file that calls `malloc` or `free` directly. |
| `sb_chan` | `SbChan`, an opaque channel handle built from an `SbChanOps` table and an adapter object. Capability flags, partial-write loop, end-of-input rule, status codes. No stdio. |
| `sb_console` | Host glue (not core): binds `#0` to `#2` to standard input and output, `#3` to standard output, and error reports to standard error. Uses stdio. |
| `sb_error` | Error codes (`SB_E_*`) and the message table. |
| `sb_format` | `sb_format_number` (output), `sb_parse_number` (strict whole-text numeric parse), `sb_parse_prefix` (leading-number parse used for string arithmetic), `sb_ql_quantise` (QL float rounding). |
| `sb_value` | `SbValue`: a number, a string or undefined. Conversions to number and to string. |
| `sb_lex` | Tokenizer (`sb_lex_start`, `sb_lex_next`), keyword table, and the canonical LIST form (`sb_lex_canonical`). |
| `sb_store` | Program line list (sorted by number), scalar variables, numeric arrays, and string arrays (`SbStrArray`). Names are stored upper-cased; string names keep their `$`. |
| `sb_check` | Statement grammar checks, including the forms of DATA, READ, RESTORE and LIST with `#n`. |
| `sb_expr` | Expression parser and evaluator, one function per precedence level (`docs/JS-SUPERBASIC-MAP.md`). |
| `sb_interp` | Statements, the run loop, `sb_execute` (one line of text), `sb_run`, `sb_init`, `sb_destroy`, and the DATA cursor. |
| `sb_state` | `SbState`, the complete interpreter state. Described in `INTERPRETER-STATE.md`. |
| `sb_main.c` | Hosted driver: reads each file (or stdin) line by line, of any length, and calls `sb_execute`. Attaches the file as the `INPUT` source. Not part of the core. |
| `tests/sbtest.c` | Hosted test runner: unit checks (including the channel checks), corpus comparison, known-difference report, leak check. |

## Data flow for one line

1. `sb_execute` strips the line terminator. A leading number means a program
   line: the number is parsed, and the rest is stored by `sb_prog_store`; an
   empty rest deletes the line.
2. Otherwise the text is a direct command. It runs through `run_loop` with
   `in_direct = 1`. Each statement is dispatched on its leading keyword or
   identifier.
3. Expressions go through `sb_expr_eval`. The evaluator reads tokens from
   `SbLexer` and returns an `SbValue`.
4. Output goes to the channel handle for the statement's channel number
   (default `#1` for `PRINT`), through `sb_chan_write`. Error reports go to
   the diagnostic handle as `message` or `message at line N`.
5. A statement returns an `SB_ST_*` status (EOL, END, STOP, JUMP, THEN, SKIP,
   or an error code). `run_loop` turns JUMP into a search for the target line
   and continues there.
6. `INPUT` reads from the input handle with `sb_chan_read`, one byte per call,
   until a newline or end of input.

## Memory

Every heap block is obtained through `st->mem`. Values own their string
storage and free it through the same allocator. Channel handles are allocated
through the allocator the host gives `sb_chan_new`. `sb_destroy` releases the
program, variables, arrays, string arrays, FOR frames and GOSUB frames; the
host releases its channel handles. The test runner counts live blocks after
each run, and a non-zero count fails the test.

## Numbers

Numeric values are `double`. Every operation result and every literal is passed
through `sb_ql_quantise` (a 31-bit magnitude, so the QL 6-byte float's
precision). Display is `sb_format_number`. Range is IEEE double, narrower than
the ROM's in places; see `KNOWN-LIMITATIONS.md` sections 3 and 4.

## Portability and style rules

- C89 only: no `//` comments, declarations at block starts, no C99 library
  functions (`snprintf`, `strcasecmp`, `frexpl`, ...), no `long long`.
- Compiled with `-std=c89 -pedantic-errors -Wall -Wextra -Werror` plus
  shadowing, prototype, cast and undefined-macro checks (`scripts/build.sh`).
  Verified with GCC 13 and Clang 18.
- No global mutable state: everything lives in `SbState` or in a handle, so
  two interpreters can run in one process.
- The core never calls stdio. Only `sb_console.c` and `sb_main.c` use
  `stdin`, `stdout` and `stderr`. The core formats numbers with `sprintf` into
  local buffers and parses with `strtod`; both are listed as replacements for
  a new target in `SB3-QDOS386-READINESS.md`.
- The core never calls `sb_chan_flush`. Flushing is the adapter's job, and
  stdio flushes at exit.

## SB4: call path

A procedure or function call goes through these functions in `src/sb_interp.c`:

- `stmt_ident` or `sb_call_function` (from the expression parser) find the
  name in the DEFine table (`sb_def_lookup`, `sb_def_seen`, `sb_def_kind`).
- `call_named` checks the kind (a function as a statement is -12), finds the
  DEFine line, scans its parameters, and collects the arguments (`collect_args`,
  `one_arg`: by reference for variables and numeric array elements, by value
  otherwise; extra arguments are evaluated and dropped).
- `run_def` pushes an `SbFrame`, binds the parameters (`bind_params`), runs the
  body (`run_body`), and unbinds the call's locals (`sb_frame_unbind`).
- `run_body` calls `exec_text` for each body line. A `RETurn` returns status
  `SB_ST_RETP`; a `GOSUB` inside a body is handled like a GOSUB at top level.

Each level of this path is a nested C call. The depth limit and the C stack
cost are in `docs/SB4-QDOS386-READINESS.md`.

The DEFine table is filled at RUN by `defs_scan` (every DEFine line in the
program) and by typed DEFine lines, which are noted as removed. `sb_def_drop`
runs when a program line is replaced or deleted. `NEW` clears the table.
