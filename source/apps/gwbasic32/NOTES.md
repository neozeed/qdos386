# GW-BASIC 32-bit port: notes

Source: Microsoft GW-BASIC 1983 MASM sources (MIT), ~35k lines of 8086 real-mode assembly.
Target: i686 PE built with MinGW GAS (`.intel_syntax noprefix`), run under Wine 9.0.

## Conventions (apply to every stage)

- One GAS file per original module, same stem, lower case, in `src/` (e.g. `MATH1.ASM` -> `src/math1.S`).
- Intel syntax, so operand order matches MASM and line-by-line review is practical.
- Registers keep their 8086 roles: AX->EAX, BX->EBX, SI->ESI, DI->EDI, and so on. The
  internal register-based calling convention is kept so logic can be checked against the original.
- Segments are gone. CS, DS, ES and SS all share one flat address space. `SEGMENT`, `ASSUME`,
  segment overrides and `SEG`/`OFFSET` pairs are dropped. Far pointers become 32-bit near pointers.
- 16-bit stored addresses (line links, token and variable table pointers, string descriptors)
  are widened to 32 bits. This changes record layouts, so each one is documented as it is changed.
  Without it the 64 KB limit stays. Math (stage 2) stores no pointers. Its symbol-table search
  (LOPFND, LOPFD1) reads ARYTA2 and STREND as DWORD, so those two must be DWORD in GWRAM (stage 3).
- Workspace size is set at build time: `make MEMSIZE=<bytes>` (default 65536). The value is passed
  to `as` with `--defsym MEMSIZE=...`.
- Device layers (screen, keyboard, disk, cassette, printer, timer, BIOS/DOS interrupts) are replaced by
  thin Win32 console and file-API routines. They are not translated instruction by instruction.
- No features beyond the original. Where the original is ambiguous, the choice is noted here.

## Scope (chosen: language core + SAVE/LOAD, no graphics)

In scope:
| Stage | Original modules | Notes |
|---|---|---|
| 1 skeleton | (new) `src/main.S` | Done: builds, allocates workspace, runs under Wine. |
| 2 math | MATH1.ASM (3827), MATH2.ASM (1882) | Software floating point. Pointer-free. |
| 3 interpreter core | GWMAIN (3566), GWEVAL (1646), GWDATA (1565), GWSTS (2346), GWLIST (838), GWINIT (456), GWRAM (466), BIMISC (944), BIPTRG (693), BISTRS (1110), BIPRTU (469), FIVEO (821), NEXT86 (159), IBMRES.ASM (988) + IBMRES.H, BINTRP.H, OEM.H | Tokenizer, evaluator, statements, strings, arrays, PRINT USING. Pointer widening happens here. |
| 4 console I/O, editor, SAVE/LOAD | GIO86 (2277), GIOKYB (891), GIOSCN (327), GIOCON (119), GIOTBL (124), SCNEDT (462), GIODSK (1218), DSKCOM (867) | Replaced by Win32 console and file routines. SCNDRV (2465) is the video driver and is replaced, not translated. |

Out of scope (dropped unless you ask otherwise):
- Graphics: ADVGRP, GENGRP (and SCNDRV's graphics paths).
- Hardware: GIOCAS (cassette), GIOLPT (printer), GIOCOM (serial), ITSA86, BIBOOT.
- Kanji: KANJ86.
- Machine-code CALL: CALL86 (`CALL` enters 8086 machine code, so it has no meaning here).
- MACLNG (MASM macro driver used by the original build; not part of the language).

Open points for review at this milestone:
1. NEXT86 is assumed to be the NEXT statement's helper. Confirm when reading it in stage 3.
2. GIODSK's title line reads "GLOBAL TEMPS and DEFS", but the file contains disk I/O. Check in stage 4.
3. FIVEO includes WHILE/WEND, CALL, CHAIN and WRITE. CALL is dropped (above); the rest are kept.

## Stage 2 notes (math)
- `src/mathram.S`: math RAM layout copied byte-for-byte from GWDATA.ASM. `_DPADR` is 4 bytes (widened pointer).
- `src/math1.S`: MATH1.ASM translated in full (assembles). Its `PUBLIC` names are exported with `.globl`.
- `PUSHF` (F4 map routine) is written `PUSHF_` because `pushf` is an x86 mnemonic. Its callers use `PUSHF_`.
- Pointers are 4 bytes on the stack: pushes and RETs of BX/SI/DI pointers and return addresses are 4 bytes.
  Value pushes stay 2 bytes. FNDITV returns its pointer in EDX.
- Symbols that MATH1 uses but MATH2 must define: `_FMULS`, `_FMULD`, `_FS`, `_FI`, `_FLT`, `_FOUT2`, `_NORMS`,
  `_NORMD`, `_ROUND`, `_ROUNS`, `_ROUNX`, `_QINT`, `_POLY`, `_POLYX`, `_RND`, `_INT`, `_LOG`, `_FRCDBL`,
  `_FIDIG`, `_FIND`, `_FINEX`, `_FINS`, `_FOTAN`, `_FOTCV`, `_FOTED`, `_FOTNV`, `_SIGD`, `_CDS`, `_CSD`, `_CSI`.
- Symbols from the interpreter (stage 3 provides them): `ARYEXT`, `ARYTA2`, `STREND`, `NAMBUF`, `NAMCNT`,
  `VALTYP`, `NOTFNS`, `NOTFDD`, `TEMP3`, `ONELIN` (DWORD), `_CHRGTR`, `_TMERR`, `_BSERR`, `_OUTDO`, `_STROUT`,
  `_STROUI`, `_INTXT`, `_FRQINT`, `_REASON`, `_CRFIN`, `_CATTY`, `_CLROV`, `_DV0ERR`, `_DIV0M`, `_DINT`, `_OVERR`,
  `_OVMSG`, `_STPRN`, `_OHCNS`, `_FCERR`, `_FLGOC`, `_MAKINT`, `_NGBLDX`.

## Stage 2 verification
- `make mathtest` builds `build/mathtest.exe` (Win32 console), runs it under Wine and pipes the output to `tests/check_math.py`.
- `tests/gen_cases.py` writes `build/math_cases.S` with 545 random cases (seed 20260701): FLT, S.P. ADD/SUB/MUL/DIV, D.P. ADD/SUB/MUL/DIV, SQR, SIN, COS, TAN, ATN, INT, integer output.
- `tests/math_test.S` has 17 named cases (1*1e6, EXP(1), LOG(10), 1/3 in D.P., and others). `check_math.py` holds their reference values.
- Result: 545/545 random and 17/17 named cases pass under Wine. `make test` (stage 1) still passes.
- Not yet covered: `$FIN` (string-to-number), RND, error paths (overflow, divide by zero, SQR of a negative), `$FOUTO`/`$FOUTH`, PRINT USING, `$SIGD` edge cases, D.P. output outside the tested range.
- `CHRGTR` contract (used by `$FOTZS` and `$FIN`): EBX = text pointer. It increments EBX, loads AL, and sets ZF=1 on NUL. The harness uses the real routine.

Radix rule: MATH2 and most of MATH1 are `.RADIX 8`, so bare numbers are octal unless suffixed `D`. MATH1 lines 527-570 are decimal. `OFFSET $DFACL-10` means DFACL-8, and `MOV CX,10` means 8 words. Reading these as decimal gave a D.P. multiply error (about 0.99 scale; 1*1e6 printed as 999424). Fixed in `_FMULD` (math2: `DFACL-8`) and `_SETDB` (math1: EDI=`_DFACL-8`, ECX=8). `_RSUBM` uses EBP, as the original does. Every numeric literal in the remaining translation must be checked against the radix in effect.

Open points (stage 2, UNSURE):
1. `LOW OFFSET $FAC` in SIN30 is an assembler address, not an exponent. Translated as an exponent test. Check it.
2. `$FINCH` scan starts at `FINCH+11`, past the table end. Kept as written.
3. `DOINF` tests BH as the original does. Kept.
4. `POPHRT` pops a 4-byte text pointer. Confirm the caller pushes BX as 4 bytes.
5. `FA22` checks `CMP CX,31` (octal, so 25). Kept as the original.


## Stage 3 notes (interpreter core, in progress)

Files so far (all assemble):
- `src/oem.inc` (OEM.H effective switch values; last assignment wins), `src/gio86u.inc`, `src/msdosu.inc`.
  BINTRP.H is macros only; its uses are handled where they occur.
- `src/gwram.S`: DS block (GWRAM.ASM). Widened: MCLPTR, MCLTAB, SNDBAS, SNDOUT, TRMCUR, MAXUPD, MINUPD.
- `src/ibmres.S`: IBMRES.ASM. Data generated by `tools/gen_ibmres.py` from the original; the 8086
  code is hand-translated in `tools/ibmres_tail.S` and appended. Token values match the original
  (END=129 etc.). DW tables -> .long. Labels "??Lnnn" -> "Q_Q_Lnnn".
- `src/gwdata.S`: GWDATA.ASM. `src/gwdata_errors.inc` generated by `tools/gen_gwdata_errors.py`
  (error table; ERRNF=1 ... LSTERR=75; ERRFC=5, ERRTM=13, ERRFNF=53 match GW-BASIC).
- `src/gwinit.S`: GWINIT.ASM. DOS/CP/M parts dropped (segment setup, version check, /M /C /F /S,
  SEGOFF/MAPCLC). The constant copy CNSLEN+3 is omitted: CONSTR and RAMLOW share an address.
  The command line is empty, so TEMP8 points at the DS zero byte and no file is loaded.
- `src/gwtok.inc`: embedded-constant token values from GWMAIN.ASM 1728-1746 (OCTCON=11, HEXCON=12,
  DBLCON=31, DBLCN1=32). Shared by GWLIST and GWMAIN.
- `src/gwlist.S`: GWLIST.ASM (LIST, LLIST, DELETE, BUFLIN and the crunch/decrunch loop, NUMLIN, DEL).
  Assembles; external list checked. LPT pseudo-FDB is 0FFFFFFFFh (was 0FFFFh).

Layout decisions:
- User memory is one .bss block at TSTACK, sized MEMSIZ (build-time, `--defsym MEMSIZE=`). Program
  text, variables, strings and the stack live in it, as in the original 64K segment. Every pointer
  is a 32-bit flat address, so the block can be larger than 64K. The BASIC stack is ESP, set to the
  top of this block at init (the original set SP to TOPMEM).
- CONSTR (code-phase image) and RAMLOW (DS copy) are one image at the same address. GWINIT's copy
  is a self-copy.
- Line record: `[link: 4 bytes][line number: 2 bytes][text ... 0]`. The link is the address of the next
  record (widened from 2 bytes); link = 0 marks the end of the program.
- Stack contracts used by GWLIST (must match GWMAIN when it is translated): SCNLIN returns ECX = pointer
  to the first line and leaves the max line number (2 bytes) on the stack under the return address.
  DELETE relies on EOSCHK and STROUT preserving EDX.
- Pointer fields are 4 bytes; line numbers stay 2 bytes (DATLIN, ERRLIN, DOT, ONELIN, OLDLIN,
  AUTLIN, AUTINC, NXTLIN, CHNLIN, CURLIN). The full list is in the comment at the top of gwdata.S.
- String descriptor: STRSIZ 3 -> 5 (length + 4-byte address). TEMPST = 50 bytes, DSCTMP = 5 bytes.
- Dispatch and address tables: 2-byte entries -> 4-byte entries. Every index that scaled by 2 scales
  by 4 (and is computed in a 32-bit register so it cannot overflow 8 bits).
- Math scratch (_DBUFF..FBUFFR) stays in mathram.S. EXPAF and EXPTMP (used by FOUT) are in gwdata.S.
- Radix: the RNDX seed bytes in GWDATA are octal (122,307,117,200) because `.RADIX 8` is active there.
- NUMCON holds the fake tokens CONCON=30 and CONCN2=16 (the OFFSET in the source is only a value).

Open items found so far:
- FOR entry layout (GWMAIN FOR and FNDFOR, NEXT86): the three pointer fields (loop-variable pointer,
  NEXT text pointer, FOR text pointer) widen to 4 bytes, so the entry size and the FNDFOR skip
  constants (WHLSIZ, FORSZC-3) change. NEXT86 is translated after GWMAIN, with the layout in one
  shared include. Do not translate NEXT86 before then.
- PPSWRT (GWLIST MORLNZ/MORPUR paths) is entered with a 4-byte text pointer still on the stack, as in
  the original. Confirm PPSWRT's stack contract when GWMAIN is translated.
- USR and CALL (machine code) are part of the language core in OEM.H (NOUSR=0). Machine code cannot run
  in this port, so USR and CALL need a decision (drop with an error, or ask the user).
- Out-of-scope statements (graphics: PAINT, CIRCLE, DRAW, PUT/GET graphics, LINE, PSET, PRESET, SCREEN,
  COLOR, PALETTE, VIEW, WINDOW, PMAP, LCOPY) and hardware statements (cassette, printer details,
  COM, ERDEV, IOCTL) are referenced by the dispatch tables. Each needs a stub or a decision.

- Token and constant values used as immediates must be `.set` in a non-global include
  (`src/tokens.inc` for IBMRES tokens, `src/gwtok.inc` for GWMAIN/GWDATA constants). A bare external
  symbol in Intel syntax is a memory operand (`cmp al, EXTTOK` compares against address 0). Check the
  result with `objdump -dr -M intel` on the object: every absolute reloc must be a memory access or a
  `mov reg, offset X`. Current immediates in gwtok.inc: OCTCON, HEXCON, DBLCON, OPCNT, ONECON, PRMSIZ, ERRID.
- POPR (OEM.H, second definition wins) is `pop cx; pop dx`. `PUSHF` is `PUSHF_` (see math1.S).
- FACLO is DWORD-addressable for string and variable pointers (4 bytes at FACLO, overlapping FACM1 and
  FAC's low byte; safe because string values do not use FAC). Integers stay WORD at FACLO.
- Function dispatch (GWEVAL ISFUN): the function number is doubled (ROL AL,1, as the original) so the
  comparisons with NUMGFN, BOTCON and TOPCON stay as written. FUNDSP has 4-byte entries, so the byte
  offset is 2 * doubled index.

## Stage 3 GWEVAL (done, assembles, all externals checked)
- `src/gweval.S` (1229 lines) is the whole of GWEVAL.ASM. Stack widths: pointers 4 bytes, values 2 bytes.
  The function number pushed by ISFUN is 4 bytes (zero-extended), so it can be swapped with a 4-byte
  text pointer. The integer second argument of the "funny" functions is 2 bytes.
- DEF FN: the value slot holds a 4-byte text pointer. FNDOER's argument slots are 4-byte variable
  pointers, 2-byte types and an 8-byte value area. FINASG, FINVLS2, and DOCNVF use PRMSTK and PRMLEN as before.
- `DB 260O` after `CALL SYNCHR` in FINASG is an MVI AL, opcode trick that skips PUSH DX. It is now an
  explicit `jmp FINVLS2`.
- DECISION (reversible): machine-code and hardware statements parse their syntax as the original does,
  then report "Illegal function call": USRFN and DEFUSR (USR, DEF USR), FNINP (INP), FNOUT (OUT), FNWAIT (WAIT).
  DEF SEG parses its optional address and ignores it (no segments in the flat model). SAVSEG is kept.
- Source discrepancy: INS86 13,332 in DANDOR is documented as "OR BX,BX". Its encoding (0Bh DAh) is
  OR BX,DX, which is the only version that gives the OR operator's result. The code uses OR BX,DX.
- Symbols still undefined in gweval.o, for later modules: CAT, CONFAC, CONIA, CONSIH, DATA, FIXINP, INKEY,
  INSTR, POINT, SCRENF, STRCMP, STRNG_, EVALX, VARPT2, VNEG, PTRGTN, MOERR, MOVFR, MOVRF, OPRTYP, GETPTR,
  FPWRQ, FDIVT, FRQINT (GWMAIN 3084), FRMQNT, ERROR, LINGET, RNDMON, LHSMID, UFERR, IDIV, IMOD, INEG2.
  POINT, INKEY and SCRENF are graphics/keyboard dispatch targets: they need stubs or the out-of-scope decision.

## Stage status
- [x] Stage 1: skeleton builds with `make`, runs under Wine at 64 KB and 2 MB.
- [x] Stage 2: math (MATH1 + MATH2 translated; `make mathtest` passes 545 random + 17 named cases)
- [ ] Stage 3: interpreter core (in progress: headers, GWRAM, GWDATA, IBMRES, GWINIT, GWLIST, GWEVAL done;
  next GWSTS, GWMAIN, then NEXT86 after GWMAIN's FOR, then BI* and FIVEO)
- [ ] Stage 4: console I/O, editor, SAVE/LOAD

## Stage 3 GWSTS (done, assembles, all externals listed)
- `src/gwsts.S` (1416 lines) is the whole of GWSTS.ASM. The CP/M-only blocks and the dead `JMP DERDNA` are not assembled.
- Trap tables: NMCOMT=0, NMPENT=0, NMSTRT=0 (COM, light pen, STRIG, STICK out of scope), NUMTRP=14 (KEY only).
  Trap entries are 5 bytes (flag byte + 4-byte GOSUB pointer); EVADR computes 5*id + TRPTBL.
  ON COM/PEN/STRIG/STICK therefore find no device and report the usual error.
- ON PEN, PEN ON/OFF/STOP: no light pen. PENS jumps to FCERR; PENF calls ONESUB then FCERR.
- VARPTR$ returns a 5-byte string (type byte + 4-byte flat address). The original 3-byte layout is not kept.
  VARPT2 pops the type into [BX] and the address into [BX+1].
- LCOPY, COLOR, SCREEN (statement), PLAY (PLAYS) and graphics PUT/GET: FCERR. PLAY needs MACLNG, which is out of scope.
  The PLY tables are not translated.
- SOUND and BEEP stay (they call DONOTE, a Stage 4 stub).
- DATE$ and TIME$ use the device layer, not MS-DOS: DEVGDAT, DEVSDAT, DEVGTIM, DEVSTIM (contracts in the
  GWSTS.S comment block; Stage 4 implements them). DEVSDAT/DEVSTIM return AL=0 when the value is accepted.
- PALETTE reports Syntax error. TIMER, ERDEV, IOCTL, CHDIR, MKDIR, RMDIR, SHELL, ENVIRON, VIEW, WINDOW, PMAP
  all fall through to ADVERR ("Advanced feature"), as in the original. ERRADV is a `.set` in gwtok.inc.
- Undefined symbols in gwsts.o (for later modules or Stage 4): BEATS, CHRGTR, CLREOL, CLRSCN, CSRATR, CSRDSP,
  CSRTYP, CSRX, CSRY, CURLIN, DEV*, DONOTE, DPUTG, DSCPTR, ERROR, FKCNUM, FKYADV, FKYFMT, FRCSNG, FRESTR,
  FRETRP, FRMEVL, FRQINT, GETBYT, GETFBC, GETIN2, INTFR2, KEYSW, LINCNT, LINLEN, LINPRT, MAKINT, MMODE,
  MSCALE, MSDCCF, NOTE1L, NOTELN, OCTAVE, OFFTRP, ONTRP, OUTDO, POLCOM, POLKEY, POLLEV, PTRGTN, PUTNEW,
  RDSTIK, RDTRIG, REQTRP, SCNLOC, SCNPOS, SCRATR, SCRINP, SCROUT, SETCSR, SETFBC, SNERR, STPTRP, STRINI,
  STRTAB, SWIDTH, SYNCHR, TRPTBL, VALTYP, _FACLO, _FACM1, _FAC, _FMULS.
  The music variables (BEATS, MSCALE, MMODE, NOTELN, OCTAVE, NOTE1L) are data kept for the out-of-scope PLAY.
  They still need definitions so the link succeeds (Stage 3i).
- Verification: objdump shows every absolute memory operand in gwsts.o is a data variable. ADVERR is `mov dl,0x49`.

## Stage 3 FOR/NEXT stack layout (decided; shared by gwmain.S (FOR, FNDFOR) and next86.S (NEXT))
- Layout and offsets are in `src/forent.inc` (FE_* constants). Entry 26 bytes: pad, token, loop-variable
  pointer (4), then from E = FNDFOR's returned pointer: NEXT text pointer (4), sign (1), type (1, minus = integer),
  step (4), upper (4), FOR line (2), FOR text pointer (4). Next entry base = E+20.
- WHILE entry (fiveo.S) is 12 bytes: pad, token, WEND text ptr (4), WHILE text ptr (4), line (2). WHLSIZ=10.
- FNDFOR starts at SP+8 (two 4-byte return addresses: its own, and NEWSTT's). FOR's stand-in pushes above the
  entry are 4-byte words; NEXTS pops one of them.
- DX holds the variable pointer, so it is EDX here (pointer rule: any register holding an address is 32-bit).
- GETSTK count is in 2-byte words: CL = 13 for the 26-byte entry.

## Stage 3 integration (3i), in progress
- All 21 src/*.S modules assemble (gwdata, gwram, gwinit, gwlist, gweval, gwsts, gwfor, next86, gwmain_a/b1/b2,
  bimisc, bistrs, biprtu, biptrg, fiveo, math1, math2, mathram, ibmres, main).
- Fixed in integration: ONELIN is 4 bytes (line pointer), math1 uses ONELIN (not _ONELIN), NAMTMP 4 bytes,
  STRSIZ 5 in oem.inc, LOOP exported from gwmain_a (FNLOOP entry), LETCN2 and DATAS (=DATA) exported from gwmain_b1.
- GAS drops local labels that are used only inside their own file. A label used by another file must be .globl.
- tests/math_test.S defines ONELIN (the math test links no gwdata). make mathtest: 545 random + named cases pass.
- Unresolved after integration (about 120): Stage 4 device layer (GIO86, GIOCON, GIOKYB, GIOSCN, DSKCOM, GIODSK,
  console and file routines), and statements or functions out of scope (graphics, random-access files, kanji).

## Stage 3 integration status (3i, link)

- `make build/gwbasic.exe` links cleanly (PE32 i386 console). Remaining undefined symbols are only MinGW CRT/runtime hooks (`___crt_*`, `__pei386_runtime_relocator`, `__end__`); not program references.
- Token constants in ibmres.S are no longer `.globl` (276 lines). Exporting them collided with math routine labels (`_INT`, `_RND`, `_LOG`, `_EXP`).
- ibmres.S private statement helper renamed `GONE4` -> `IBMGONE4`. The GWMAIN `GONE4` (gwmain_b1.S) is exported.
- Exported for cross-file use: LOOP (gwmain_a), FNDLN1, ERRESM (gwmain_a), LETCN2 (gwmain_b1), DATAS (gwmain_b1), INPCOM, GONE4 (gwmain_b1), CONCH2 (gwmain_b2), CSRX/CSRY (gwram), LINCNT/LINLEN (gwram), STRIGF (gwsts).
- `src/stage3stubs.S` (generated): ADVERR stubs for out-of-scope and Stage 4 I/O routines (including SAVE, LOAD, PRINT, INKEY, file I/O, graphics); silent `ret` stubs for output/init (GWINI, GIOINI, SCRINP, etc.); zero-returning stubs for POLKEY/RDTRIG family; DEVGDAT/DEVGTIM/DEVSDAT/DEVSTIM device contract stubs; `_DVTBL`/`_DVPTR` zero.
- Makefile: `AS := i686-w64-mingw32-as -I src`; `SRC := $(wildcard src/*.S)`.
- RENUMBER/RESEQ remain blocked (RESEQ entry -> FCERR; body unreachable). The 5-byte PTRCON mismatch in gwmain_b2.S is therefore not executable yet.
- STKINI writes no stopper. FNDFOR walks pad+type bytes from SP+8 and stops on a non-FOR/WHILE type. The stopper relies on the byte at TOPMEM-1 being non-token (zero high byte of the bottom return address). Explicit stopper write is an open decision.
- Only `make mathtest` has run. Everything else is link-level verification; no interpreter behaviour is tested until Stage 4 console I/O exists.
- Math test harness stub renamed `_DV0ERR` -> `DV0ERR` (tests/math_test.S) to match math1.S and gwmain_a.S. `make mathtest`: 545 random + named cases pass.
- `build/gwbasic.exe` is still the Stage 1 skeleton entry (src/main.S): it allocates a heap block, prints a banner, and exits. It does not call GWINIT/GWMAIN yet. Wiring the interpreter entry is an open step.
- Wine smoke test: `wine build/gwbasic.exe` starts and exits 0 within 15 s.

## Stage 4 notes (Win32/Wine checkpoint: the interpreter runs real programs)

Status: builds at 64 KB (default) and 2 MB (`make MEMSIZE=2097152`). `make test` (tests/run_gw.py) passes
23 of 23 regression cases on both builds. Logs: `logs/stage4_tests_64k.txt`, `logs/stage4_tests_2mb.txt`.
This is not full GW-BASIC compatibility. No original binary was available, so every expected output in
tests/cases was written from the GW-BASIC semantics in the original sources and was not compared with the
original. Each item below marked "unverified" is a judgement, not a checked fact.

### Toolchain: GAS 1.38 requirement (conflict, left open)
The Stage 4 directive asks for GAS 1.38. The only assembler here is `i686-w64-mingw32-as` from binutils 2.41.
Every build and test in this checkpoint used 2.41. No 1.38 build was attempted, so that requirement is not
met as written. Decide whether to supply 1.38 or accept 2.41.

### Platform layer (src/platform_win32.S)
- Console routines unchanged (PLAT_INIT, CHOUT, FLUSH, CHIN, CHAVAIL, EXIT).
- File routines added: PLAT_FOPEN (read: OPEN_EXISTING; write: CREATE_ALWAYS), PLAT_FSIZE, PLAT_FREAD,
  PLAT_FWRITE, PLAT_FCLOSE, over CreateFileA, GetFileSize, ReadFile, WriteFile, CloseHandle. Errors are
  returned as CF=1 with the Win32 error code in EAX (2/3 = not found, 38 = short transfer, 223 = too large).
- Names are relative to the process's current directory. tests/run_gw.py runs Wine in the scratchpad test
  directory, so SAVE/LOAD files never land in the repository.

### SAVE and LOAD (src/bifile.S)
- File format: this port's own. A file is the stored program image exactly as it is in memory from TXTTAB to
  VARTAB: every line with its 4-byte link and 2-byte line number, then the 4-byte zero end link. It is not the
  GW-BASIC binary or ASCII format, and files do not move between the original and this port.
- LOAD rewrites the links (CHEAD) and clears variables and the stack, as NEW does. Without `,R` it returns to
  "Ok". With `,R` it runs the program (as RUN). LOAD does not garbage collect.
- Error order: name, open, size and room checks come before the program is changed. A file shorter than the
  end link is refused with the program intact ("Bad file mode"). A file of the right size is read in place and
  then checked for line structure (CHKIMG); if that check fails the program is left empty ("Bad file mode").
- Errors: missing file = "File not found" (53); empty name = "Bad file name" (64); other open, read or write
  failure = "Device I/O Error" (57); image too big for the space below FRETOP = "Out of memory" (7); non-string
  name = "Type mismatch". Text after the name (other than `,R`) is a "Syntax error" before any file work.
- The structure check does not parse the text of lines. A file with valid structure but invalid token bytes
  is loaded unchecked, and may misbehave when listed or run (not tested).
- LOAD from inside a running program: not tested.

### CLEAR (src/bimisc.S)
- Form: `CLEAR [string-space][,[top][,stack]]`. The first argument is parsed and ignored (string space is the
  rest of the workspace, fixed at build time). The second sets the top of memory as an offset from the start of
  the workspace (TSTACK). The third sets the stack size in bytes. Both are 16-bit values.
- Checked: `CLEAR` (default); `CLEAR ,30000` (free space drops to about 29 K, variables cleared, programs and
  GOSUB/FOR still run); `CLEAR ,,500`; `CLEAR ,100` = "Out of memory" with the program intact;
  `CLEAR ,100000` = "Overflow" (16-bit parse).
- Limit: the top of memory cannot be set above 65535 bytes, so on the 2 MB build CLEAR can only lower it to
  within the first 64 KB.
- Deviation: `CLEAR 1 2 3` is accepted; the original gives "Syntax error" (unverified; not fixed).

### Unsupported or blocked (what the user sees)
- CALL (machine code): "Syntax error" (dispatch removed; see Stage 2 decision on machine code).
- RENUMBER/RESEQ: blocked. RESEQ is routed to FCERR and its body is unreachable, so `RENUM` gives
  "Illegal function call". Known blocker: the 5-byte PTRCON mismatch in gwmain_b2.S.
- CHAIN: "Syntax error". MERGE, LRUN, BLOAD, BSAVE: "Advanced Feature" stubs (stage3stubs.S).
- OPEN and other sequential/random file statements: "Advanced Feature" (Win32 file adapter exists; the
  statements do not yet use it).
- Deferred to later stages: graphics, SOUND/PLAY, INP/OUT, printer/LPRINT, DOS machine code, QDOS/386 port.

### 4-byte end-of-program marker
Every stored program ends with a 4-byte zero link. SCRTCH (NEW, startup) writes it. CRUNCH writes five zero
bytes after a direct line's text so NEWSTT's 4-byte link read after the terminator is zero (this fixed the
stale-line failures). CHEAD/LINKER test the full 4 bytes. The shadow screen was checked as a cause of the stale
line and is not one (see the comment in src/scedit.S, INLIN0).

### 5-byte string descriptor audit
- Descriptor: [length byte][4-byte address], STRSIZ = 5 (oem.inc). TEMPST is 50 bytes (10 temporaries), DSCTMP
  is 5 bytes. VARPTR$ returns a 5-byte string (type byte + 4-byte flat address).
- Fixed: SMKVAR stored the value size (5) in the variable record's type byte instead of VALTYP (3). String
  variables then read back empty. string_concat, string_gc and vars_num_str now pass.
- LEN, ASC, MID and the file-name copy (FILNAM) read the address at descriptor+1 as 4 bytes.

### 16-bit array indexing and 64 KB limits
- Subscripts are popped as 2-byte words and read as unsigned 0..65535 (`movzx`). The array table holds a 2-byte
  size and a 2-byte maximum per dimension, so an array's byte size is limited to 65535.
- DIM A(70000) gives "Overflow" (unverified against the original). DIM A(20000) (80 KB)
  gives "Subscript out of range". The original would say "Out of memory" for an array that does not fit. The
  message is wrong here; the cause is the 16-bit size field. Not fixed.
- String length is one byte (255), as in the original.

### Bugs fixed in this stage (beyond the items above)
- FOR + GOSUB second iteration crash: GOTO converted line constants to 4-byte PTRCON in place (two bytes
  written). Line-pointer conversion is now disabled, so each GOTO/GOSUB searches for its line.
  Known effect: correct but slower.
- EDIT after a syntax error printed "Advanced Feature": ERRED2 now returns. Lines are not loaded for editing.
- DIM index pushed as 4 bytes but popped as 2 (push si now); GETDEF rewritten for 2-byte subscripts.
- FRE(0) on the 2 MB build returned the free count modulo 65536 (GIVDBL subtracts in 16 bits). FRE now uses
  GIVFRE, which floats the 32-bit count. Exact up to 8 MB free (FLOATR reads the top mantissa bit as a sign).
  2 MB build: `PRINT FRE(0)` = 2096631 (2097152 - 521). The 64 KB result is unchanged (65015).
- CLEAR's top-of-memory argument was an absolute address (an unrebased GETMPM value); it is now an offset
  from TSTACK. Verified with programs and variables after CLEAR (see CLEAR above).
- Makefile: objects depend on MEMSIZE (a stamp file), so `make MEMSIZE=...` rebuilds. Before, a size change
  reused the old objects and the 2 MB "build" was a 64 KB one.
- Shared constants moved into src/gwconst.inc (plain .set values), included from gwmain_a.S, gwmain_b1.S,
  gwmain_b2.S, biptrg.S and bifile.S. The error messages stay in gwdata_errors.inc.

### Regression cases (tests/cases, 23 at the Stage 4 checkpoint; 42 at Gate 4)
Directive acceptance list and the case that covers it (the cases added later are listed in
`docs/GWBASIC-RUNTIME-REPORT.md`):
- PRINT 2+2: print_arith. PRINT "HELLO": print_string.
- 10 PRINT "HELLO":20 END: stored_run. Result: HELLO, then "Syntax error in 10" (the `20 END` part is not a
  statement; unverified against the original).
- FOR I=1 TO 10:PRINT I:NEXT I: for_loop. Nested FOR: nested_for. Next without FOR: next_nofor.
- GOSUB/RETURN: gosub_return, for_gosub. RETURN without GOSUB: return_nogosub.
- Numeric and string variables: vars_num_str. Concatenation and GC: string_concat, string_gc.
- DIM and arrays: dim_array (1-D numeric), dim_more (2-D numeric and string).
- FRE and CLEAR: fre_clear (checks signs and ranges only, so it passes on either workspace size).
- LIST/RUN/NEW and error recovery: list_program, new_clears, div_zero, div_zero_run, syntax_error,
  stored_two_lines.
- SAVE/NEW/LOAD/RUN: save_load. Failure paths: load_errors (uses fixtures written by the runner).

### Test harness
- tests/run_gw.py: runs `wine build/gwbasic.exe` in the scratchpad test directory with the case's typed input.
  Before each case it removes *.BAS there and writes any fixtures for that case (bytes that cannot be typed).
- `make test` builds and runs it. `make MEMSIZE=2097152 test` runs the 2 MB build.
- Expected outputs are hand-written. A case that fails shows the diff.

### Known limitations (summary)
- EDIT of a program line is not supported.
- GOTO/GOSUB line-pointer optimisation disabled (correct, slower).
- The RUN-mode division warning is printed as "Division by zero" without " in 10". Unverified against the original.
- `CLEAR 1 2 3` is accepted.
- Arrays larger than 65535 bytes give the wrong error message (see above).
- CLEAR top of memory limited to 65535 bytes.
- LOAD does not check token bytes; LOAD from a running program is untested.
- Stage 4 note: the Win32 reference build uses the MinGW binutils. The GAS 1.38 requirement applies to the QDOS
  build (`Makefile.qdos`), which uses GNU as 1.38 and i386-ld 1.94.
- Not compared with the original GW-BASIC. No original binary was available.

## Stage 5: QDOS/386 build (Gates 0–4 complete on the KCTX-FIX2 kernel)

### Current status (supersedes the older status text below)

- Kernel: the normal KCTX-FIX2 kernel (`kernels/qdos386-vga.img`, SHA256 `f8e5d5e1…c3`), unmodified. The
  kernel fix for the misaligned resume (the investigation below) is in this kernel. The port changes proposed
  in "Fix options" below were **not** applied and are not needed on this kernel.
- Long FOR/NEXT loops, nested loops and GOSUB stress run without a double fault (`long_loops` in
  `tests/cases`, Win32 and QDOS). The "Status on QDOS" paragraph below is the record from before the fix.
- QDOS suite: see `docs/GWBASIC-RUNTIME-REPORT.md` (counts from the final full run). Expected failure:
  `save_write_error` (kernel finding, `evidence/save_write_error/`). Skip: `save_readonly` (harness cannot
  attach read-only media under QEMU 8.2.2).
- Documents: `docs/GWBASIC-KNOWN-LIMITATIONS.md` (current limitations), `docs/GWBASIC-LANGUAGE-COVERAGE.md`
  (statement and function matrix), `docs/GWBASIC-KCTX-FIX2-HANDBACK.md` (hand-back, kernel finding),
  `HANDOVER.md` (package and how to verify it).

### Build details (unchanged)


Build: `make -f Makefile.qdos R13B=<R13B tree> QDTOOLS=<dir with as and i386-ld>`. The QDOS build uses the
historical toolchain (GAS 1.38, i386-ld 1.94, a.out OMAGIC 0407). The R11L archive ships these in
`toolchain/host-bin`. `make image` copies the R13B system volume and adds GWBASIC.AOU to the copy;
`make run` starts QEMU with the R13B kernel. The OS and the R13B volume are not modified.
Default MEMSIZE is 64 KB. The app slot is 256 KiB (0x240000-0x27FFFF), so larger workspaces do not fit.

Conversion (tools/intel2att.py, checked by tools/verify_att.py: 0 of 32 files differ from the Intel build):
- A comment line whose text starts with a digit (`#<blank>123...`) is a line-number marker in GAS 1.38 and
  silently drops the section code after it. It is emitted as `#-`.
- `.rept` is expanded (not supported by GAS 1.38).
- Symbols beginning with `L` are local to GAS 1.38 and never reach the object even with `.globl`. They are
  exported as `XL<name>` (the converter renames all uses and fails on a clash).
- An alias to a later label (`=` or `.set`) becomes an undefined external. The aliases in gwdata.S
  (FILDEV, FILNM, FILEXT) and gwmain_b1.S (DATAS) are moved after their targets, in the Intel sources too.
- i386-ld 1.94 resolves undefined absolute references to 0 without an error. Only call and pc-relative
  references are reported. An audit of absolute references is still needed.

Source fix found in this stage (also in the Win32 build): stage3stubs.S used `mov dl, ERRMMM` without the
constant, which made it a load from address 65. It now includes gwconst.inc. Win32 `make test`: 23/23 pass
after the alias moves and this fix.

Status on QDOS (before the KCTX-FIX2 kernel; historical): banner, PRINT, FOR/NEXT, RUN, LIST, and SAVE/LOAD
(write and read back) work on short programs. Long FOR/NEXT loops fail with a kernel double fault: `PGFAULT: #DF double fault eip 00000024`
with ESP outside the app slot. The failure needs a loop of more than about 8100 iterations on the current
build (1100, 2100, 4100, 8100 pass; 16000, 16400, 20000, 200000 fail; nested 100x1000 passes; GOSUB 20000 fails).

Root cause (established; reproducers in qdos/repro/):
- The FOR entry is 26 bytes (FE_SIZE, forent.inc). Its base is 2 mod 4, so while a FOR entry is on top of the stack
  the program's ESP is 2 mod 4 (the loop body and the path back to the NEXT chop). The 2-byte pushes elsewhere in
  the port (`push ax`, `push cx`, `push si`) can leave ESP misaligned in the same way; this is not yet tested.
- Reproducers: qdos/repro/spin_misaligned.S (ESP 2 mod 4, expected double fault on preemption) and
  qdos/repro/spin_aligned.S (control, ESP 0 mod 4, expected exit 0). The BASIC loop programs are in
  qdos/repro/basic/ with the observed results in basic/README.txt.
- The kernel saves a preempted job as `saved_sp | fmt` (kernel/jobs.c schedule()), with fmt in bit 0 and the
  comment "Stacks are 4-byte aligned, so bits 0-1 are free" (context.s lines 9-10). On resume, resume_dispatch does
  `andl $-4,%eax` at kernel/kctx.s line 46. kctx.s is the file the R13B image links (Makefile: PCCTX=kctx.o for
  QDOS_DYNPROC kernels, R12A and later); context.s line 48 holds the same instruction but is not linked into R13B.
  The andl discards bit 1 of a misaligned ESP. The popal/iret frame is then read two bytes low, and
  iret loads EIP=0x24 and a wrong ESP.
- Evidence: the job record for job 0x10003 (found in a RAM dump at the fault) holds saved_sp=0x0025b922 (2 mod 4)
  with saved_fmt=1; the app's timer tick was taken at SP=0x0025b94e (2 mod 4); the iret frame at the fault has
  EIP=0x24 and ESP=0x847a0000.
- Control: a program that spins with ESP 2 mod 4 faults with a double fault as soon as the kernel preempts it
  (no BASIC code involved); the same spin with ESP 0 mod 4 exits with status 0.
- Earlier hypotheses are withdrawn: the 16-bit BX/DX operations in FADDS/FCOMP and the widened 2-byte stores
  checked by the width audit do not change ESP. The ESP range checks inside NEXT, FNDFOR, NXTCON, NEWSTT, _FADDS
  and _FCOMP never fired.

Fix options (historical: the KCTX-FIX2 kernel corrects the resume path, and none of these was applied):
- Kernel, option A (smallest change): kctx.s line 46 uses `andl $-2,%eax` (clear only the format bit), so a
  2-byte-misaligned ESP is restored intact. This widens the documented stack-alignment contract (context.s
  lines 9-10) and needs a rebuild. It requires saved_sp to be even, which the port's stack adjustments satisfy.
  Details and the proposed patch are in the findings document delivered with this build.
- Kernel, option B: pass the format flag in a global `resume_fmt` set by schedule() and read by resume_dispatch,
  so the ESP value is not altered at all. Touches jobs.c in several places, including the `return sp | 1u` sites.
- Port: keep ESP 4-byte aligned at every point where the kernel can preempt. This means a 4-byte FOR entry
  (26 -> 28 bytes with the offsets in forent.inc updated) and every 2-byte push/pop in the port converted to a
  4-byte one, with the stack consumers (FNDFOR, NEXT, GOSUB/RETURN, WHILE/WEND) updated. Large (several hundred
  sites are 16-bit register saves), and verifiable on Win32 only for layout consistency.

Other audit results (from the same milestone):
- WHLSIZ: forent.inc defines 10, fiveo.S redefines it as 11 and uses WHLSIZ-1 = 10 at its one consumer. Both give
  the same WHILE skip (12-byte WHILE entry, skip 10 from the entry's second byte), so the values agree in effect,
  but the name means different things in the two files.
- FNDFOR's WEND search (fiveo.S FNDWND) skips a FOR entry with FORSZC-1 = 25. The next entry starts 24 bytes
  after that point, so the skip should be FE_SIZE-2 = 24. Latent (WEND with a FOR still on the stack); not changed.
- The 5 PTR accesses wider than their declared size (FACLO/_FACM1 overlap, DIMFLG/VALTYP, FSTCOL/LSTLIN) are
  intentional overlaps in the FAC block or adjacent flag bytes; none touches a stack-restore variable.

Corrections to earlier notes: the double fault reports eip 00000024 (not 0x240024); the esp in the report is the
real ESP at the fault.

Not done (as of the Gate 0–4 pass): file I/O with `#` file numbers (OPEN, PRINT #, INPUT #, …) is still
deferred to ADVERR; FILGET now handles `PRINT A#` correctly and refuses a real `#` file. Graphics and the other
unsupported statements are unchanged. The keyword gap and the tokenizer divergence are in
`docs/GWBASIC-KNOWN-LIMITATIONS.md`, sections 3 and 4. SuperBASIC (BASIC.AOU in the R13B archive) is not
redistributable and is not part of any artifact from this build.
