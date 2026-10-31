# Translation conventions (8086 MASM -> 32-bit GAS, intel syntax)

Target assembler: i686-w64-mingw32-as, `.intel_syntax noprefix`, 32-bit flat address space.
Output is one `.S` file per original module, same stem, lower case (MATH2.ASM -> math2.S).

## Symbols
- Replace every `$` with `_` (GAS symbols cannot start with `$`): `$FAC` -> `_FAC`, `$FOTZS` -> `_FOTZS`.
- Replace `?` with `Q_`: `?CSLAB` -> `Q_CSLAB`.
- All other names unchanged. Original names are the link interface between modules.
- Every PUBLIC name gets `.globl`. Names referenced but not defined in your file need nothing
  (GAS treats them as external).
- Equates (`X=5`) -> `.set X, 5` (or `.equ`). Keep original names.
- `OFFSET sym` -> `offset sym`. `LOW expr` -> just the expression (8-bit value).
- Radix: `167O` -> `0167`; `3277D` or `3277` -> `3277`; `0FFH` -> `0xFF`; `'0'` char literals stay.
- Local labels in MASM (`FZ10:`) keep their names. Forward/backward refs like `2f` are fine.
- Drop: `SEGMENT`, `ENDS`, `ASSUME`, `SUBTTL`, `PAGE`, `TITLE`, `.SALL`, `.LIST`, `.XLIST`,
  `INCLUDE` (values are already in the translated equates), and segment overrides like `CS:`, `DS:`, `ES:`.
- `PUBLIC`/`EXTRN` lines are not translated as directives. Use `.globl` for PUBLIC, nothing for EXTRN.

## Register width rule (the important one)
- VALUE registers keep 16-bit operations: AX, DX, CX/CL when used as a shift count, mantissa words,
  digits, characters, flags words. Write them as `ax`, `dx`, `cx`, and `WORD PTR` memory operands.
- POINTER registers become 32-bit: any register holding an ADDRESS (BX, SI, DI, BP used with [ ] or
  as a pointer) becomes `ebx`, `esi`, `edi`, `ebp`. Memory operands `BYTE PTR 0[BX]` become
  `BYTE PTR [ebx]`. Constants loaded into pointers use `mov ebx, offset X`.
- Pointer arithmetic is 32-bit: `INC BX` -> `inc ebx`, `ADD SI,DX` -> use `movzx edx, dx` first if DX
  holds an unsigned 16-bit offset, then `add esi, edx`. `SUB DI,BX` (difference of pointers) -> 32-bit
  `sub edi, ebx`; the zero/sign tests keep their meaning for small differences.
- Pointers saved on the stack are 4 bytes: `PUSH BX` (pointer) -> `push ebx`; `POP BX` -> `pop ebx`.
  Value pushes stay 16-bit: `PUSH AX` -> `push ax` (2 bytes in 32-bit mode, same as the original).
- EVERY stack adjustment must match the pushed width: `ADD SP,2` after a pointer push becomes `add esp, 4`.
  Audit all `ADD SP,n` / `RET n` against the widths actually pushed.

## Counts (CRITICAL)
- LOOP, REP*, JCXZ use the full ECX/ECX in 32-bit mode. If the original sets CX (16-bit), set ECX with
  `mov ecx, value` (32-bit) so the upper half is zero. Never leave an old upper half in ECX.
- Shift counts in CL stay as CL.

## Flags and stack
- `PUSHF` / `POPF` -> `pushfd` / `popfd` (4 bytes). Pairs stay paired.
- `CMP/JB/JNB/JA/JBE` etc. keep their meaning: the ops are identical in 32-bit mode.
- `CBW` (AL->AX) stays `cbw`. Use `cwde` only where the original wants EAX.
- `XLAT` uses (E)BX as the table base: load `mov ebx, offset TABLE` (32-bit) before `xlat`.
- `MOVSB/MOVSW/STOSB/STOSW/LODSB/LODSW` use ESI/EDI. Use 32-bit pointers in ESI/EDI.

## Tables of addresses (CRITICAL)
- `DW OFFSET label` tables become `.long label` (4-byte entries).
- Index math that scaled by 2 for 16-bit entries scales by 4 for 4-byte entries
  (`SHL BP,1` for jump tables becomes `SHL EBP,2`).
- `JMP WORD PTR CS:TABLE[BP]` -> `jmp DWORD PTR TABLE[ebp]`.
- Byte tables (`$FINCH`, `$FOITB`, `$NUMB`, etc.) stay byte tables; only their index math changes
  with the pointer rule above (e.g., `CMP BP,OFFSET $FINCH` -> `cmp ebp, offset _FINCH`).

## Data and memory
- Math variables (FAC, ARG, FBUFF, FMTAL, ...) are defined in `src/mathram.S` with the original
  byte layout. Use the original names (`_FAC`, `_FACLO`, `_FBUFF`, `_FMTAL`, ...). Do NOT redefine them.
- A `WORD PTR _FACLO` access is a 16-bit access to the same bytes as in the original.

## Things that are not part of the math layer
- Calls into the interpreter (e.g., `$CHRGT`, `CHRGTR`, error routines `$FCERR`, `OVERR`, `DV0ERR`,
  `BSERR`, `$STPRN`, `TMERR`) stay as plain `call` to the original name (after the `$` -> `_` rule).
  They are defined later in the interpreter port.
- `FAR` calls and `RETF` do not appear in the math layer; if you find one, report it.

## Fidelity
- Keep the algorithm identical. Do not optimise or change the arithmetic.
- Keep the original comments as `#` comments for traceability (shortening is fine).
- Anything you could not map cleanly: write a `# UNSURE:` comment at that spot and list it in your report.

## Constants from other files (CRITICAL)
- In Intel syntax, a bare symbol used as an operand is a MEMORY reference, even when the symbol is a
  constant defined elsewhere. `cmp al, _DATA` with `_DATA` external compares against the byte at
  address 0. Token and constant values used as immediates must be known at assembly time: put their
  `.set` lines in a non-global include (`src/tokens.inc` for IBMRES tokens, `src/gwtok.inc` for the
  GWMAIN embedded-constant tokens) and include it.
- Addresses used as immediates are written `mov ebx, offset X`.
- `PUSHF` (the math routine) is called as `PUSHF_`.
- GETSTK's count register CL is in 2-byte words. A 4-byte return address is therefore CL=2.
