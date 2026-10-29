# Known limitations and differences from the ROM

This interpreter implements a subset of SuperBASIC. Where it differs from the
JS ROM, the difference is listed here with the ROM's observed result. The
differences that have a fixture are also present as tests in `tests/known/`,
which the runner reports as KNOWN without failing. Items marked "no fixture"
are documented from probes in `tests/probes/`, or are not reproducible in a
finite test.

Nothing here is a claim of full SuperBASIC compatibility.

## 1. Language coverage

Implemented and checked against the ROM (see `TEST-RESULTS.md`):

- Statements: `PRINT` (with `#n`), `LET`, assignment, `IF ... THEN`,
  `GOTO`, `GOSUB`/`RETURN`, `FOR`/`NEXT` with `STEP`, `STOP`, `CONTINUE`,
  `RUN`, `LIST` (with `#n`), `NEW`, `CLEAR`, `REM`, `INPUT` (with `#n`), `DIM`,
  `DATA`, `READ`, `RESTORE` (the verified subset; section 12).
- Numeric arrays of any number of subscripts (the hosted limit is 8; see
  section 8), with zero-based subscripts.
- String slices `s$(i)` and `s$(i TO j)`, in expressions and as assignment
  targets.
- One-subscript string arrays `DIM a$(n)`, read and assigned (section 12).
- Channels 0 to 3 (see `SB3-CHANNEL-ABI.md`).

Not implemented. Each gives an error (-19 for the statements with a recognised
not-implemented path; otherwise -17, because an unknown word is read as an
identifier):

- String arrays with two or more subscripts (`DIM a$(3,4)` gives -19; see
  section 12).
- `DEF`, `PROC`/`FN` (the ROM has `DEFine PROCedure` and `LOCal`; section 12),
  multi-line `IF`, `WHILE`/`REPEAT`, `SELECT`.
- Graphics and the screen: `INK`, `PAPER`, `CLS`, `AT`, `BORDER`, `PRINT AT`.
- File I/O (`OPEN`, `CLOSE`) and any channel other than 0 to 3.
- Functions other than `LEN`, `ABS` and `ERNUM`.
- Editing commands, `SAVE`, `LOAD`, `MERGE`, `DELETE`, `RENUM`.
- Scalar and array with the same name (see section 11).

## 2. Syntax errors in direct mode

- **Fixed:** a typed line is checked for syntax before any of it runs
  (`src/sb_check.c`). `PRINT 1 : PRINT 3 4` prints nothing, as on the ROM
  (corpus 31). The check is grammar-only, so runtime errors are never reported
  as syntax errors.
- **Not implemented (observed):** the ROM discards the rest of the typed input
  after a syntax error. In a test with `PRINT 1`, `PRINT 3 4`, `PRINT 2` the
  ROM prints only `1`; the interpreter continues and prints `2`
  (`tests/known/syntax_swallow.bas`). The hosted driver treats each line on
  its own.
- **Same class, SB2 statements (observed):** the ROM swallows the following
  typed input for these malformed lines, where this interpreter reports -17
  and continues. Fixtures: `tests/known/gosub_alone_swallow.bas`,
  `dim_open_swallow.bas`, `input_chan_nocomma_swallow.bas`.
- **Statement order (observed):** the ROM runs `RETURN` before it notices a
  stray argument, so `RETURN 5` gives -7 there and -17 here
  (`tests/known/return_arg_order.bas`).
- **Incremental parsing (observed):** `INPUT a,` reads one line for `a` before
  the trailing comma is noticed, and the line it reads is then the one the
  program would have executed. The interpreter rejects the whole line first
  (`tests/known/input_comma_end.bas`).
- **Unexplained (observed):** `PRINT s$(1 TO)` prints nothing and leaves
  ERNUM at 0 on the ROM, where this interpreter gives -17
  (`tests/known/slice_open_range.bas`). The cause is not known.
- **Not checked:** numbered program lines are not syntax-checked when they are
  entered; their errors appear when they run. The ROM's behaviour here is not
  verified.
- **Unverified:** the checker accepts `ABS(1,2)` as a syntax error (-17); the
  ROM gives -15 for it at run time. This case is left out of the corpus.

## 3. Floating-point range (observed)

The ROM's exponent range is far wider than IEEE double.

- `2^1100` prints `1.358299E331` in the ROM. Here it overflows (-18).
- `0.5^123456` is -18 in the ROM (underflow is an error there). Here it gives 0.
- `5^7 & 10` (`5^710`) prints `1.856535E496` in the ROM and overflows here.

Results between about 1.8E308 and the ROM's maximum (about 2^2047) are
therefore wrong here, and values below about 4.9E-324 disappear. Fixing this
needs an extended-range float, not a change to the formatter. The random sweep
shows this category (seeds 1, 2 and later; see `TEST-RESULTS.md`).

## 4. Concatenated text used in arithmetic (partly observed)

`&` produces text. The ROM's handling when that text is later used as a
number is not fully characterised:

- `"3x"+1` and `"3-5"+0` (literal strings) follow the leading-number rule and
  give 4 and 3.
- `-5 DIV 0.25 & -5` gives -17 in the ROM. The interpreter gets -18: it reads
  `0.25-5` as 0.25, which rounds to zero, so DIV reports a zero divisor.
- `1E6 OR 0.25 & 0.25 + -5 & 2147483648` gives -17 in the ROM and 1 here.

The ROM seems to reject some concatenated fractional text in arithmetic that
it accepts as a literal. The rule is not yet known. (`tests/known/concat_in_arith.bas`.)

## 5. NEXT without FOR

Fixed: `NEXT` with no matching `FOR` (whether the name differs or there is no
`FOR` at all) gives -7 (not found), as on the ROM (corpus 30).

## 6. Error reports on the screen

The oracle captures only channel #3, so the wording and placement of the
error report lines (`message at line N`) are not compared with the ROM. The
ROM's report wording (for example `error in expression at line 10`) is not
verified here.

## 7. Random-sweep differences

`tools/sweep.py` generates random expressions, runs them on the ROM and on
the interpreter, and reports differences. The sweeps recorded in
`TEST-RESULTS.md` found differences in the categories above (range, concatenated
text, and error code order for mixed expressions). The sweep covers only
expressions, not statements.

## 8. Hosted-only behaviour (not ROM behaviour)

- **Step limit:** a top-level command that makes more than
  `SB_DEFAULT_STEP_LIMIT` (1,000,000) control transfers stops with -4 (out of
  range). The ROM has no such limit. Unit check: `unit_step_limit` in
  `tests/sbtest.c`.
- **GOSUB depth:** `SB_MAX_GOSUB` (4096) nested calls; a deeper call gives -3
  (out of memory). The ROM accepted 3000 nested calls (corpus 37 uses 2000)
  and is limited only by free memory, so deeper calls are not compared.
- **Array size:** at most `SB_MAX_DIMS` (8) subscripts, and at most
  `SB_MAX_ARRAY_ELEMENTS` (1,000,000) elements in one array; a larger DIM gives
  -3. The ROM accepted four subscripts (corpus 43). Its memory limit was not
  probed.
- **Input end:** when a file ends while `INPUT` waits for a line, the
  interpreter gives -19. The ROM waits for the keyboard, which a finite input
  cannot reproduce. No corpus file depends on this.
- **Typed-line GOSUB frames:** a `GOSUB` made in a typed line is dropped when
  the next typed line starts, so a later `RETURN` gives -7. On the ROM such a
  `RETURN` resumes the old typed line (fixture: section 11). Frames made by
  program lines are kept across typed lines and across `STOP`.
- **RUN and GOSUB:** `RUN` clears pending GOSUB returns. The ROM was not probed
  for this; `RUN` does keep variables and arrays (verified).
- **Channels 0 to 2 output:** the hosted build sends `PRINT #0`, `PRINT #1` and
  `PRINT #2` to the same console as channel 3. On the ROM they go to the
  screen, which the oracle does not capture, so their text is not checked.

## 9. Harness artefacts (not interpreter behaviour)

- `NEW` closes channel #3 in the ROM, so output after `NEW` cannot be
  captured. The corpus test `20_new` therefore captures output only before
  `NEW`. The arrays-after-`NEW` case is consequently not checked.
- The harness rewrites `PRINT` and `LIST` to channel #3 in the typed copy.
  A `PRINT` that already names a channel (`PRINT #9,...`) is left as written.
  Program lines typed with `PRINT` are rewritten too, so a corpus test must
  not both store a `PRINT` line and `LIST` it (the listing would show `#3`).
- Typed input for `INPUT` is the next line of the same file, in both the ROM
  harness and the corpus runner.

## 10. Unknown and unverified behaviour

- The exact rule for assignment of a string to a numeric variable name (the
  corpus covers the cases in `24_assign_mismatch.bas`).
- String ordering for characters that are not letters.
- Fractional line numbers in `GOSUB`. In a typed line the ROM gives -7, which
  is what this interpreter does. In a program line the ROM printed nothing at
  all (no fixture); this interpreter ends the run with -7.
- `PRINT #-1` and fractional channels beyond the rounding shown in
  `SB2-CHANNEL-ABI.md`.
- DIM memory exhaustion on the ROM (not probed).
- `READ`/`DATA`/`RESTORE`: not probed; not implemented.

## 11. SB2 observations kept as fixtures or probe records

These came out of the Phase 0 probes, kept as JSON batches in `tests/probes/`
(run with `tests/probes/runp.py`, which needs the oracle module). Where a
result is not reproduced, the behaviour is recorded here rather than matched.

- **Typed RETURN into a finished typed line (observed).** `GOSUB 10` in a
  typed line whose subroutine line runs off the program, followed by a later
  typed `RETURN`, gives no error on the ROM and resumes the old line (it
  printed `s`, `after`, `e0` in the probe). The interpreter gives -7 (section 8).
- **Typed RETURN after STOP in a later-line subroutine (observed, erratic).**
  With the `GOSUB` on program line 20 and a `PRINT` on line 10 before it, a
  typed `RETURN` after the subroutine's `STOP` makes the ROM repeat output
  from line 10 instead of resuming at line 30. The `GOSUB` on the first line
  behaves as expected. Not reproduced; the probe is recorded in the development
  fixtures (`tests/probes/batchN.json`, case `prog_gosub_stop_then_return`).
- **Wrong subscript count (observed).** With `DIM a(2,2)`, the ROM prints three
  zeros for `PRINT a(1)`. The interpreter gives -17.
- **Scalar and array with one name (observed).** `DIM a(2)`, then `a=5`,
  prints `000 0` and gives -19 on the ROM. Not modelled; the two are
  independent here.
- **String arrays (observed).** See section 1.
- **`INPUT` alone (observed).** A no-op on the ROM (no error, no line read).
  Matched.
- **`PRINT #n` alone (observed).** Prints a blank line. Matched.
- **`a(1 TO 2)` on a numeric array (observed).** -19. Matched.

## 12. SB3 additions: differences and gaps

Each item is a difference that the SB3 build keeps on purpose. A fixture in
`tests/known/` is reported by the runner without failing the build. Items
marked "no fixture" are documented from probes only.

### DATA, READ and RESTORE

- Only a DATA statement that begins its line is read by READ. The ROM's
  behaviour for a DATA statement after another statement on the same line is
  not established. (no fixture)
- Bare words in DATA: the ROM swallows the statements after the DATA line, so
  the program prints only `start`. SB3 does not implement the swallow
  (fixture `data_bare_words_swallow`).
- `READ 1` (a number where a variable belongs): the ROM gives -10 and assigns
  nothing; SB3 gives -17 at entry (fixture `read_number_item`).
- READ with no positioned cursor and no DATA: the ROM's results were not
  consistent across probes (one probe assigned a value and then gave -10,
  another stopped silently). SB3 returns -10 without assigning. Unresolved.
  (no fixture)
- `NEW` clears the DATA cursor in SB3. The ROM was not probed for this because
  its effect cannot be seen on channel #3. (no fixture)

### String arrays

- Two or more subscripts: SB3 gives -19 at `DIM` and at element use. The ROM
  gives a value for the case in fixture `str_dim_two_subscripts` (the ROM
  prints `hell`).
- A one-subscript element read out of range: the ROM returns bytes outside
  the string; SB3 gives -4. (no fixture)
- A `DIM` length above `SB_MAX_ARRAY_ELEMENTS` gives -3 in SB3. This is a
  hosted bound and not ROM behaviour. (no fixture)

### Channels

- An input that ends before any byte of a line gives -19. This is the SB2
  rule, kept: the ROM waits for the keyboard, which a finite input cannot
  reproduce. (SB2 record)
- No `OPEN`, `CLOSE`, or channel other than 0 to 3. The core never calls
  `sb_chan_flush`; output is flushed by stdio at exit, as in SB2.

### DEFine PROCedure and LOCal

- Both exist in the ROM. Oracle probes show a procedure defined in a program
  runs when called, and `LOCal` works inside it. SB3 does not implement them;
  it gives -17 at the DEFine line (fixtures `defproc_program` and
  `defproc_local`).
- A DEFine typed in direct mode: the ROM gives -7 at the later call. Which
  statement produced the -7 was not separated. SB3 does not implement it.
  (no fixture)

## 13. SB4 additions: procedures, functions and LOCal

Probe names refer to `tests/probes/sb4/*.txt`; corpus and known names refer
to `tests/corpus` and `tests/known`. "Verified" means the ROM output was
captured under QLBox and SB4 matches it.

### Implemented and verified (summary)

- `DEFine PROCedure` and `DEFine FuNction` with `END DEFine`; `RETurn`;
  `LOCal`; parameters by reference (variables and numeric array elements) and
  by value (expressions); missing arguments undefined; recursion to depth
  5000 (corpus 68 to 99, examples unchanged).
- Shallow dynamic scope: a called procedure sees its caller's LOCal names.
- FOR, GOSUB and GOTO interactions in the verified cases (corpus 81, 82, 89 to
  95).
- DEFine name errors: -12 for a name never defined, -7 for a DEFine whose
  header line was deleted, -12 for a function used as a statement, -17 for a
  procedure used as a value (probes p16 to p18).

### Gaps and differences

1. **Nested DEFine inside a body** gives -19 (not implemented). The ROM's
   result for the nested case (`nest_inner_not_called`) printed `outer end`
   and then -7; that was not separated into its causes, so SB4 does not copy
   it.
2. **GOTO out of a procedure**: the ROM keeps the procedure's LOCal visible
   to the main program (`goto_out_local_visible`, `out3`); SB4 drops the frame
   and the variable reads as undefined (`out*`). Fixture:
   `tests/known/defproc_goto_out_local`. The ROM rule for when the frame ends
   is not known.
3. **Bare END before a DEFine line in a program**: the ROM prints nothing
   (`call_before_define_end_line`, fixture `tests/known/defproc_bare_end_before_define`);
   SB4 runs the procedure and prints `late p`, E0. Not traced.
4. **Extra arguments to a call**: the ROM evaluates them. With `p 1,g` where
   `g` is a FuNction, the ROM ran `g` (printed `g ran`) before `a=1`
   (`extra_arg_fn_side`). SB4 evaluates expression arguments but treats a bare
   name as a variable, so it does not call `g`. An extra argument that is not
   an expression (`p 1,PRINT ...`) stops the ROM run (`extra_arg_evaluated`,
   nothing printed); SB4 gives -17 there.
5. **A DEFine without END DEFine**: SB4 gives -17; not probed.
6. **Replacing a DEFine header line with another DEFine**: SB4 keeps the
   name's last registration; not probed.
7. **Bare END in a program** ends the run in SB4 (as the SB1 to SB3 notes say).
   Its effect on DEFine calls is item 3.
8. **CLEAR or NEW inside a call**: SB4 gives -7 when the call returns. The ROM
   output for CLEAR inside a call (corpus 85) shows the run stopping after the
   CLEAR; the -7 is from earlier notes and is not printed in that file.
9. **STOP inside a function**: not resumable with CONTINUE in SB4.
10. **Recursion depth**: the ROM accepts depth 5000 and gives -3 at 6000
    (`p11`). SB4 stops at depth 5500 with -3 (`SB_MAX_CALL_DEPTH`), a hosted
    limit. Each level uses C stack; see `docs/SB4-QDOS386-READINESS.md`.
11. **Hosted C stack**: a 6 MB C stack segfaults at depth 5499; 8 MB runs it.
    Hosted builds use the system default stack.
12. **Probe budget**: some ROM probes give blank output with a small frame
    budget (a recursion probe at depth 5000 with a smaller run). The probe
    runner uses one fixed budget; blank output is not treated as a result.
13. **Sweep**: the random-expression sweep gives the same count as SB3 for the
    same seeds (see `docs/TEST-RESULTS.md`); the differences are the two
    known categories (sections 3 and 4).
14. **Unknown bare-name statements in direct mode**: the ROM and SB4 agree
    (-12); no difference recorded.
