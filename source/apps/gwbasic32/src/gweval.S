# GWEVAL.ASM -> gweval.S
# Formula evaluator: FRMEQL, FRMPRN, FRMEVL, FRMCHK (operator-precedence loop), the
# operator table dispatch, and the push of temporary values on the stack.
#
# Stack temporaries (widened): RETAOP address (4 bytes), precedence (2 bytes), the
# value pushed from the FAC (2 or 4 or 8 bytes, see NUMREL), operator-application address
# (4 bytes). Each pointer push is 4 bytes; each value push keeps the original width.
#
# Notes:
#  - GETSTK's CL is in 2-byte words (GWMAIN FOR: CL=8 gives 16 bytes). A return address is
#    4 bytes, so FRMCHK asks for CL=2 (the original asked for CL=1).
#  - FRMCHK's "PUSH DX" (precedence) stays 2 bytes; RETAOP's "POP CX" matches it.
#  - Operator table OPTAB is 1 byte per entry (12 entries); the index math stays a byte offset.
#  - The F4-map routine PUSHF is called as PUSHF_ (see math1.S).
#  - Tokens are immediates via tokens.inc (an external bare symbol would be a memory operand).

	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"

	.section .text
	.globl FRMEQL, FRMPRN, FRMEVL, FRMCHK, TSTOP, RETAOP, DEF, FNINP, FNOUT, FNWAIT, LABBCK

# External routines used here (other stage 3 modules, math, stage 4 stubs):
#   SYNCHR, GETSTK, EVAL (below), CAT, TMERR, SNERR, CHRGTR, FRCSNG, PUSHF_, APPLOP (below),
#   FINREL, ANDORD, EXPSTK, VPUSHD, ... (see below). Data: VALTYP, FACLO, DFACLO, FLGOVC,
#   TEMP2, TEMP3, OPTAB (this file's table is in gwdata.S: OPTAB, 12 bytes).

# -------------------------------------------------------------------------------------
# FRMEQL / FRMPRN: check for "=" or "(" and then evaluate.
# -------------------------------------------------------------------------------------
FRMEQL:
	call SYNCHR
	.byte EQULTK			# check for equal sign
	jmp FRMEVL
FRMPRN:
	call SYNCHR
	.byte '('			# get paren before formula

# FRMEVL: evaluate the formula at BX. The result is left in the FAC.
FRMEVL:
	dec ebx				# back up character pointer
FRMCHK:
	mov dh, 0			# initial dummy precedence is 0
LPOPER:
	push dx				# save precedence
	mov cl, 2			# extra space needed for a 4-byte return address (words)
	call GETSTK			# make sure there is room for recursive calls
	call EVAL			# evaluate something
	xor al, al			# reset overflow printing back to normal
	mov BYTE PTR FLGOVC, al		# (set to 1 at FUNDSP to suppress multiple overflow messages)
TSTOP:
	mov TEMP2, ebx			# save text pointer
RETAOP:
	mov ebx, TEMP2			# restore text pointer
	pop cx				# pop off the precedence of OLDOP
NOTSTV:
	mov al, [ebx]			# get next character
	mov TEMP3, ebx			# save updated character pointer
	cmp al, GREATK			# is it an operator?
	jae Q_Q_NS_A
	ret				# no, all done (this can result in operator application
					# or an actual return)
Q_Q_NS_A:
	cmp al, LESSTK+1		# some kind of relational?
	jb DORELS			# yes, do it
	sub al, PLUSTK			# subtract offset for first arithmetic
	mov dl, al			# must multiply by 3 since OPTAB entries are 3 long (kept as the original)
	jnz NTPLUS			# not addition op
	mov al, BYTE PTR VALTYP		# see if left part is string
	cmp al, 3			# see if it's a string
	mov al, dl			# refetch op-value
	jnz Q_Q_NS_B
	jmp CAT				# must be CAT
Q_Q_NS_B:
NTPLUS:
	cmp al, LSTOPK			# higher than the last op?
	jb Q_Q_NS_C
	ret				# yes, must be terminator
Q_Q_NS_C:
	mov ebx, offset OPTAB		# create index into OPTAB
	mov dh, 0			# make high byte of offset = 0
	movzx edx, dx
	add ebx, edx			# add in calculated offset
	mov al, ch			# [A] gets old precedence
	mov dh, [ebx]			# remember new precedence
	cmp al, dh			# old - new
	jb Q_Q_NS_D
	ret				# must apply old op if it has greater or equal precedence
Q_Q_NS_D:
					# new operator
	push cx				# save the old precedence
	mov ecx, offset RETAOP		# put on the address of the place to return to
	push ecx			# after operator application
	mov al, dh			# see if the operator is exponentiation
	cmp al, 127			# which has precedence 127
	jz EXPSTK			# if so, FRCSNG and make a special stack entry
	cmp al, 81			# see if the operator is "AND" or "OR"
	jb ANDORD			# and if so FRCINT and make a special stack entry
	and al, 254			# make 123 and 122 both map to 122
	cmp al, 122			# special check for "MOD" and "IDIV"
	jz ANDORD			# if so, coerce arguments to integer

# THIS CODE PUSHES THE CURRENT VALUE IN THE FAC ONTO THE STACK, EXCEPT FOR STRINGS, IN WHICH
# CASE IT CALLS TYPE MISMATCH. [D] AND [E] ARE PRESERVED.
NUMREL:
	mov al, BYTE PTR VALTYP		# find out what type of value we are saving
	sub al, 3			# set the condition codes (zero for strings)
	jnz Q_Q_NR_A
	jmp TMERR
Q_Q_NR_A:
	push WORD PTR FACLO		# push FACLO
	jns Q_Q_NR_B
	jmp VPUSHD			# all done if the data was an integer
Q_Q_NR_B:
	push WORD PTR FACLO+2		# push FAC-1,0 on the stack
	jp Q_Q_NR_C
	jmp VPUSHD			# all done if we had a SNG
Q_Q_NR_C:
	push WORD PTR DFACLO		# push on low bytes of DP FAC
	push WORD PTR DFACLO+2		# push on next two bytes of DP FAC
VPUSHD:
	add al, 3			# fix [A] to be the VALTYP of the number just pushed
	mov cl, dl			# [C]=operator number
	mov ch, al			# [B]=type of value on the stack
	push cx				# save these things for APPLOP
	mov ecx, offset APPLOP		# general operator application routine -- does type conversions
FINTMP:
	push ecx			# save place to go
	mov ebx, TEMP3			# reget the text pointer
	jmp LPOPER

DORELS:
	mov dh, 0			# assume no relation ops; also set up high order of index into OPTAB
LOPREL:
	sub al, GREATK			# is this one relation?
	jb FINREL			# relations all through
	cmp al, NMREL			# is it really relational?
	jae FINREL			# no, just big
	cmp al, 1			# set up bits by mapping
	rcl al, 1			# 0 to 1, 1 to 2 and 2 to 4
	xor al, dh			# bring in the old bits
	cmp al, dh			# make sure result is bigger
	mov dh, al			# save the mask
	jae Q_Q_LR_A
	jmp SNERR			# don't allow two of the same
Q_Q_LR_A:
	mov TEMP3, ebx			# save character pointer
	call CHRGTR			# get the next candidate
	jmp LOPREL

# FOR EXPONENTIATION FORCE THE CURRENT VALUE IN THE FAC TO BE SINGLE PRECISION. AT APPLICATION
# TIME THE RIGHT-HAND OPERAND IS FORCED TO SINGLE PRECISION AS WELL.
	.extern FPWRQ
EXPSTK:
	call FRCSNG			# coerce left-hand operand
	call PUSHF_			# put it on the stack

# FOR EXPONENTIATION (continued): dispatch to FPWRQ with DH = 127 (restore the precedence)
	mov ecx, offset FPWRQ		# place to coerce the right-hand operand and do exponentiation
	mov dh, 127			# restore the precedence
	jmp FINTMP			# finish entry and evaluate more formula

# FOR "AND" AND "OR" AND "\" AND "MOD" FORCE THE CURRENT VALUE IN THE FAC TO BE AN INTEGER;
# AT APPLICATION TIME THE RIGHT-HAND OPERAND IS FORCED TO BE AN INTEGER.
ANDORD:
	push dx				# save the precedence
	call FRCINT
	pop dx				# [D]=precedence
	push bx				# push the left-hand operand (2-byte integer; DANDOR pops 2 bytes)
	mov ecx, offset DANDOR		# "AND" and "OR" doer
	jmp FINTMP			# push on this address, precedence, and continue evaluation

# HERE TO BUILD AN ENTRY FOR A RELATIONAL OPERATOR. Strings are treated specially.
# Numeric compares differ from most operator entries only in the bottom of the entry.
FINREL:
	mov al, ch			# [A]=old precedence
	cmp al, 100			# relationals have precedence 100
	jb Q_Q_FR1
	ret				# apply earlier operator if it has higher precedence
Q_Q_FR1:
	push cx				# save the old precedence
	push dx				# save [D]=relational bits
	mov dx, 256*100+OPCNT		# [D]=precedence 100, [E]=dispatch offset OPCNT for compares in APPLOP
	mov ebx, offset DOCMP		# routine that takes the compare result and relational bits
	push ebx			# and returns the answer (JMP to RETAOP when done)
	call GETYPR			# see if we have a numeric compare
	jz Q_Q_L005
	jmp NUMREL			# yes, build an APPLOP entry
Q_Q_L005:
	mov ebx, DWORD PTR FACLO	# get the pointer at the string descriptor (4 bytes)
	push ebx			# save it for STRCMP
	mov ecx, offset STRCMP		# string compare routine
	jmp FINTMP			# push the address, reget the text pointer, save the precedence,
					# and scan more of the formula

# APPLOP is returned to when it is time to apply an arithmetic or numeric comparison.
# The stack has a 2-byte entry with the operator number and the VALTYP of the value on the stack.
# APPLOP decides what value level the operation will occur at and converts the arguments.
# INTEGERS: left in [D,E], right in [H,L]. SINGLES: left in [B,C,D,E], right in the FAC.
# DOUBLES: left in the FAC, right in ARG.
APPLOP:
	pop cx				# [B]=stack operand value type, [C]=operator offset
	mov al, cl			# save in memory: the stack will be busy
	mov BYTE PTR OPRTYP, al		# a RAM location
	mov al, BYTE PTR VALTYP		# get VALTYP of the FAC
	cmp al, ch			# are the VALTYPs the same?
	jnz VALNSM			# no
	cmp al, 2			# integer?
	jz INTDPC			# yes, dispatch!
	cmp al, 4			# single?
	jnz Q_Q_L006
	jmp SNGDPC			# yes, dispatch!
Q_Q_L006:
	jae DBLDPC			# must be double, dispatch!
VALNSM:
	mov dh, al			# save in [D]
	mov al, ch			# check for double
	cmp al, 8			# precision entry on the stack
	jz STKDBL			# force FAC to double
	mov al, dh			# get VALTYPE of FAC
	cmp al, 8			# and if so, convert the stack operand
	jz FACDBL			# to double precision
	mov al, ch			# see if the stack entry is single
	cmp al, 4			# precision and if so, convert
	jz STKSNG			# the FAC to single precision
	mov al, dh			# see if the FAC is single precision
	cmp al, 3			# and if so convert the stack to single
	jnz Q_Q_L007
	jmp TMERR			# blow up on right-hand string operand
Q_Q_L007:
	jae FACSNG			# precision. NOTE: the stack must be integer at this point.
INTDPC:
	mov ebx, offset INTDSP		# integer-integer case
	mov ch, 0			# special dispatch for speed
	movzx ecx, cl
	shl ecx, 2			# 4-byte table entries
	add ebx, ecx			# [H,L] points to the address to go to
	mov ecx, [ebx]			# [B,C]=routine address
	pop dx				# [D,E]=left-hand operand
	movzx ebx, WORD PTR FACLO	# [H,L]=right-hand operand (integer)
	push ecx			# dispatch
	ret

# THE STACK OPERAND IS DOUBLE PRECISION, SO THE FAC MUST BE FORCED TO DOUBLE PRECISION, MOVED
# INTO ARG, AND THE STACK VALUE POPPED INTO THE FAC.
STKDBL:
	call FRCDBL			# make the FAC double precision
DBLDPC:
	call VMOVAF			# move the FAC into ARG
	pop bx				# pop off the stack operand into the FAC: its low four bytes are
	mov WORD PTR DFACLO+2, bx	# two words (NUMREL pushed them as words)
	pop bx
	mov WORD PTR DFACLO, bx		# store low bytes away
SNGDBL:
	pop cx				# POPR: pop off a four-byte value
	pop dx
	call MOVFR			# into the FAC
SETDBL:
	call FRCDBL			# make sure the left operand is double precision
	mov ebx, offset DBLDSP		# dispatch to a double precision routine
DODSP:
	movzx eax, BYTE PTR OPRTYP	# recall which operand it was
	shl eax, 2			# dispatch offset: 4-byte table entries
	add ebx, eax
	mov ebx, [ebx]			# fetch the address
	jmp ebx				# perform the operation, returning to RETAOP, except for
					# compares, which return to DOCMP

# THE FAC IS DOUBLE PRECISION AND THE STACK IS EITHER INTEGER OR SINGLE PRECISION AND MUST BE
# CONVERTED.
FACDBL:
	mov al, ch			# get the value type into [A]
	push ax				# save the stack value type
	call VMOVAF			# move the FAC into ARG
	pop ax				# pop the stack value type into [A]
	mov BYTE PTR VALTYP, al		# put it in VALTYP for the force routine
	cmp al, 4			# see if it's single, so we know how to pop the value off
	jz SNGDBL			# it's single precision: do a POPR / call MOVFR
	pop bx				# pop off the integer value (2 bytes: the integer is pushed as a word)
	mov WORD PTR FACLO, bx		# save it for conversion
	jmp SETDBL			# set it up

# THE STACK IS SINGLE PRECISION AND THE FAC IS EITHER SINGLE OR INTEGER.
STKSNG:
	call FRCSNG			# convert the FAC if necessary
SNGDPC:
	pop cx				# POPR: left-hand operand into the registers
	pop dx
SNGDO:
	mov ebx, offset SNGDSP		# dispatch address for single precision operator routines
	jmp DODSP

# THE FAC IS SINGLE PRECISION AND THE STACK IS AN INTEGER.
FACSNG:
	pop bx				# pop off the integer on the stack (2 bytes: the integer is pushed as a word)
	call PUSHF_			# save the FAC on the stack (FACLO, then FAC-1: two words)
	call CONSIH			# convert [H,L] to a single precision number in the FAC
	call MOVRF			# put the left-hand operand in the registers
	pop bx				# restore the FAC from the stack: FAC-1 was pushed last
	mov WORD PTR FAC-1, bx
	pop bx				# then FACLO (two-byte pops, to match the two word pushes)
	mov WORD PTR FACLO, bx
	jmp SNGDO			# perform the operation

# INTEGER DIVISION. Both arguments are forced to single precision floating point and FDIV is
# used, so that 1/3 is .333333 and not zero.
	.globl INTDIV
INTDIV:
	push ebx				# save the right-hand argument
	xchg bx, dx			# [H,L]=left-hand argument
	call CONSIH			# convert [H,L] to a single-precision number in the FAC
	pop ebx				# get back the right-hand argument
	call PUSHF_			# push the converted left-hand argument onto the stack
	call CONSIH			# convert the right-hand argument to a single precision number in the FAC
	jmp FDIVT			# registers the left-hand argument

# -------------------------------------------------------------------------------------
# EVAL: evaluate a variable, constant, function call, or parenthesized formula.
# -------------------------------------------------------------------------------------
	.globl EVAL
EVAL:
	call CHRGTR
	jnz Q_Q_L008
	jmp MOERR			# test for missing operand: if none, give error
Q_Q_L008:
	jae Q_Q_L009
	jmp FIN				# if numeric, interpret constant
Q_Q_L009:
	call ISLET2			# variable name?
	jb Q_Q_L010
	jmp ISVAR			# an alphabetic character means yes
Q_Q_L010:
	cmp al, DBLCON+1		# is it an embedded constant
	jae Q_Q_L011
	jmp CONFAC			# rescan the token and restore the old text pointer
Q_Q_L011:
	call EVALX			# handle extended functions
	inc al				# is it a function call (preceded by 0FFh)?
	jnz Q_Q_L012
	jmp ISFUN			# yes, do it
Q_Q_L012:
	dec al				# fix A back
	cmp al, PLUSTK			# ignore "+"
	jz EVAL
	cmp al, MINUTK			# negation?
	jnz Q_Q_L013
	jmp DOMIN
Q_Q_L013:
	cmp al, 34			# string constant?
	jnz Q_Q_L014
	jmp STRLTI			# if so, build a descriptor in a temporary location and put a
					# pointer to the descriptor in FACLO
Q_Q_L014:
	cmp al, NOTTK			# check for "NOT" operator
	jnz Q_Q_L015
	jmp NOTER
Q_Q_L015:
	cmp al, '&'			# octal constant?
	jnz Q_Q_L016
	jmp OCTCNS
Q_Q_L016:
	cmp al, ERCTK
	jnz NTERC			# no, try other possibilities
	call CHRGTR			# grab following char
	mov al, BYTE PTR ERRFLG		# get the error code
NTDERC:
	push ebx			# save text pointer
	call SNGFLT			# return the value
	pop ebx				# restore text pointer
	ret				# all done.

NTERC:
	cmp al, ERLTK			# error line number variable
	jnz NTERL			# no, try more things
	call CHRGTR			# get following character
	push ebx			# save text pointer
	movzx ebx, WORD PTR ERRLIN	# get the offending line #
	call INEG2			# float 2-byte unsigned int
	pop ebx				# restore text pointer
	ret
NTERL:
	cmp al, _VARPTR			# VARPTR call?
	jnz NTVARP			# no
	call CHRGTR			# eat char after
	cmp al, '$'
	jnz Q_Q_L017
	jmp VARPT2			# branch if VARPTR$(x)
Q_Q_L017:
	call SYNCHR
	.byte '('			# eat left paren
	cmp al, '#'			# want pointer to file?
	jnz NVRFIL			# no, must be variable
	call GTBYTC			# read file #
	push ebx			# save text pointer
	call GETPTR			# get pointer to file
	pop ebx				# restore text pointer
	jmp VARRET
NVRFIL:
	call PTRGTN			# get address of variable
VARRET:
	call SYNCHR
	.byte ')'			# eat right paren
	push ebx			# save text pointer
	xchg ebx, edx			# get value to return in [H,L]
	or ebx, ebx			# make sure not an undefined var (set CCs; zero if undefined)
	jnz Q_Q_L018
	jmp FCERR			# all over if undefined (don't want the user poking into zero)
Q_Q_L018:
	call MAKINT			# make it an int
	pop ebx				# restore text pointer
	ret
NTVARP:
	cmp al, USRTK			# user assembly language routine?
	jnz Q_Q_L019
	jmp USRFN			# go handle it
Q_Q_L019:
	cmp al, INSRTK			# is it the INSTR function?
	jnz Q_Q_L020
	jmp INSTR			# dispatch
Q_Q_L020:
	cmp al, _SCREEN
	jnz Q_Q_L021
	jmp SCRENF
Q_Q_L021:
	cmp al, _POINT
	jnz Q_Q_L022
	jmp POINT
Q_Q_L022:
	cmp al, _INKEY_			# INKEY$ function?
	jnz Q_Q_L023
	jmp INKEY
Q_Q_L023:
	cmp al, _STRING_		# STRING function?
	jnz Q_Q_L024
	jmp STRNG_			# yes, go do it
Q_Q_L024:
	cmp al, _INPUT			# fixed length input?
	jnz Q_Q_L025
	jmp FIXINP			# yes
Q_Q_L025:
	cmp al, CLINTK			# want to know current line on screen?
	jnz Q_Q_L026
	jmp GETLIN			# yes
Q_Q_L026:
	cmp al, FNTK			# user-defined function?
	jnz Q_Q_L027
	jmp FNDOER
Q_Q_L027:
# ONLY POSSIBILITY LEFT IS A FORMULA IN PARENTHESES
	.globl PARCHK
PARCHK:
	call FRMPRN			# recursively evaluate the formula
	call SYNCHR
	.byte ')'
	ret
DOMIN:
	mov dh, 125			# a precedence below ^ but above all else
	call LPOPER			# so ^ greater than unary minus
	mov ebx, TEMP2			# get text pointer
	push ebx
	call VNEG
LABBCK:					# functions that don't return string values come back here
	pop ebx
	ret

	.globl ISVAR, RETVAR
ISVAR:
	call PTRGET			# get a pointer to the variable in [D,E]
RETVAR:
	push ebx			# save the text pointer
	xchg ebx, edx			# put the pointer to the variable value into [H,L]. For a string
					# this is a pointer to a descriptor, not an actual value
	mov DWORD PTR FACLO, ebx	# in case it's a string, store the pointer to the descriptor
					# in FACLO (4-byte pointer)
	call GETYPR			# for strings we just leave
	jz Q_Q_L028
	call VMOVFM			# a pointer in the FAC using [H,L] as the pointer
Q_Q_L028:
	pop ebx				# restore the text pointer
	ret

	.globl MAKUPL, MAKUPS
MAKUPL:
	mov al, BYTE PTR [ebx]		# get char from memory
MAKUPS:
	cmp al, 'A'+32			# is it lower case range? ('A'+40 octal)
	jae Q_Q_MK1
	ret				# less
Q_Q_MK1:
	cmp al, 'Z'+33			# greater ('Z'+41 octal)
	jb Q_Q_MK2
	ret				# test
Q_Q_MK2:
	and al, 95			# make upper case (137 octal)
	ret				# done

	.globl CNSGET, _OHCNS, OCTCNS
CNSGET:
	cmp al, '&'			# octal perhaps?
	jz Q_Q_L029
	jmp LINGET
Q_Q_L029:
_OHCNS:
OCTCNS:
	xor edx, edx			# initialize to zero and ignore overflow
	call CHRGTR			# get first char
	call MAKUPS			# make upper if necessary
	cmp al, 'O'			# octal?
	jz LOPOCT			# if so, do it
	cmp al, 'H'			# hex?
	jnz LOPOC2			# then do it
	mov ch, 5			# init digit count
LOPHEX:
	inc ebx				# bump pointer
	mov al, BYTE PTR [ebx]		# get char
	call MAKUPS			# make upper case
	call ISLET2			# fetch char, see if alpha
	xchg ebx, edx			# save [H,L]
	jae ALPTST			# yes, make sure legal hex
	cmp al, '9'+1			# is it bigger than the largest digit?
	jae OCTFIN			# yes, be forgiving and return
	sub al, '0'			# convert digit, make binary
	jb OCTFIN			# be forgiving if not hex digit
	jmp NXTHEX			# add in offset
ALPTST:
	cmp al, 'F'+1			# is it legal hex?
	jae HEXFIN			# yes, terminate
	sub al, 'A'-10			# make binary value
NXTHEX:
	add bx, bx			# shift left four bits (accumulator in BX, 16-bit)
	add bx, bx
	add bx, bx
	add bx, bx
	or al, bl			# or on new digit
	mov bl, al			# save back
	xchg ebx, edx			# get text pointer back in [H,L]
	dec ch
	jnz LOPHEX			# keep eating if not too many digits
#CKOVER follows in the next part.

# -------------------------------------------------------------------------------------
# Overflow during an octal/hex constant: the return address CKOVER was pushed by LOPOCT
# (4 bytes) and "RET" in the overflow case jumps here.
# -------------------------------------------------------------------------------------
CKOVER:
	mov al, BYTE PTR FLGSCN
	or al, al
	jnz Q_Q_L030
	jmp OVERR			# if not INPUT statement, this is an error
Q_Q_L030:
	push ebx
	mov ebx, offset _OVMSG
	call _STPRN			# print overflow message
	call CRDO
	pop ebx
CONER2:
	mov al, BYTE PTR FLGOVC
	inc al
	mov BYTE PTR FLGOVC, al		# tell input code that an error occurred
	ret
CONERR:
	mov al, BYTE PTR FLGSCN
	or al, al
	jz Q_Q_L031
	jmp CONER2			# if input code error, return error code
Q_Q_L031:
	jmp SNERR			# else goto error from here

# Octal digits (LOPOC2 re-reads the last character; LOPOCT scans digits)
LOPOC2:
	dec ebx				# reget last character
LOPOCT:
	call CHRGTR			# read a digit
	xchg ebx, edx			# result into [H,L]
	jae OCTFIN			# out of digits means done
	cmp al, '8'			# is this an octal digit
	jnae Q_Q_L032
	jmp CONERR			# no, too bad, you will lose
Q_Q_L032:
	mov ecx, offset CKOVER		# where to go on overflow error
	push ecx			# save addr on stack (4 bytes)
	add bx, bx			# multiply by eight (16-bit accumulator)
	jnb OCTOK1
	ret				# overflow error
OCTOK1:
	add bx, bx
	jnb OCTOK2
	ret				# overflow error
OCTOK2:
	add bx, bx
	jnb OCTOK3
	ret				# overflow error
OCTOK3:
	pop ecx				# get rid of overr address
	mov ch, 0			# setup to add [B,C]
	sub al, '0'
	mov cl, al
	add bx, cx			# add in the digit
	xchg ebx, edx			# put text pointer back in [H,L]
	jmp LOPOCT			# scan more digits

HEXFIN:
OCTFIN:
	.globl HOCFIN
HOCFIN:
	call MAKINT			# save as an integer
	xchg ebx, edx			# [H,L]=text pointer
	ret

# -------------------------------------------------------------------------------------
# ISFUN: a function token (preceded by 0FFh). The function index is doubled (ROL AL,1,
# as in the original) so the comparisons with NUMGFN/BOTCON/TOPCON are unchanged. The
# dispatch table FUNDSP has 4-byte entries, so the byte offset is 2 * doubled index.
# Stack: the function number is pushed as a 4-byte zero-extended value (NTMRND); the
# text pointer and the string pointer are 4 bytes; the integer second argument is 2 bytes.
# -------------------------------------------------------------------------------------
ISFUN:
	inc ebx				# bump source text pointer
	mov al, BYTE PTR [ebx]		# get the actual token for FN
	sub al, ONEFUN			# make into offset
	cmp al, _RNDFN			# is it RND?
	jnz NTMRND			# if not, no need to check monadic
	push ebx			# save text pointer
	call CHRGTR			# see if next char is "("
	cmp al, '('
	pop ebx				# get back the old text pointer
	jz Q_Q_L033
	jmp RNDMON			# handle monadic case
Q_Q_L033:
	mov al, _RNDFN
NTMRND:
	rol al, 1			# multiply by 2 (doubled index)
	movzx ecx, al
	push ecx			# save the function # on the stack (4 bytes)
	call CHRGTR
	mov al, cl			# look at function #
	cmp al, NUMGFN			# is it past LASNUM?
	jae OKNORM			# no, must be a normal function
#
# MOST FUNCTIONS TAKE A SINGLE ARGUMENT (see the original comments). The "funny" functions
# take a string first argument and an integer second argument; their text pointer is in
# [D,E] while the arguments are read, and the string pointer is left on the stack.
#
	call FRMPRN			# eat open paren and first arg
	call SYNCHR
	.byte ','			# two args, so comma must delimit
	call CHKSTR			# make sure the first one was string
	xchg ebx, edx			# [D,E]=TXTPTR
	mov ebx, DWORD PTR FACLO	# get ptr at string descriptor (4 bytes)
	pop esi				# XTHL: get function #
	xchg esi, ebx
	push esi			# save the string ptr in the function's slot
	push ebx			# put the function # back on the stack
	xchg ebx, edx			# [H,L]=TXTPTR
	call GETBYT			# [E]=value of formula
	xchg ebx, edx			# text pointer into [D,E]; [H,L]=int value of second argument
	pop esi				# XTHL: get function #
	xchg esi, ebx
	push si					# save int value of second arg (2 bytes, as PREAM2 pops it)
	jmp FINGO			# [H,L]=function number, dispatch
OKNORM:
	call PARCHK			# check out the argument and make sure it is followed by ")"
	pop esi				# XTHL: [H,L]=function #
	xchg esi, ebx
	push esi			# save text pointer (4 bytes)
#
# Special coercion for the transcendental functions (RND, SQR, COS, SIN, TAN, ATN, LOG, EXP):
# they assume a single-precision argument in the FAC, so FRCSNG is called first.
#
	mov al, bl			# [A]=function number
	cmp al, BOTCON			# less than square root?
	jb NOTFRF			# don't force the argument
	cmp al, TOPCON			# bigger than arc-tangent?
	push ebx			# save the function number
	jnb Q_Q_L034
	call FRCSNG			# if not, force FAC to single-precision
Q_Q_L034:
	pop ebx				# restore the function number
NOTFRF:
	mov edx, offset LABBCK		# return address
	push edx			# make them really come back
	mov al, 1			# function should only print overflow once
	mov BYTE PTR FLGOVC, al
FINGO:
	movzx ebx, bx			# function number (zero-extended)
	add ebx, ebx			# 4-byte table entries: byte offset = 2 * doubled index
	mov ecx, offset FUNDSP		# function dispatch table
DISPAT:
	add ebx, ecx			# add on the offset
	jmp DWORD PTR [ebx]		# dispatch

# -------------------------------------------------------------------------------------
# MINPLS: scan leading signs for numbers (called from FIN; moved here to avoid byte externals).
# -------------------------------------------------------------------------------------
	.globl MINPLS
MINPLS:
	dec dh				# set sign of exponent flag
	cmp al, MINUTK			# negative exponent?
	jnz MNP1
	ret
MNP1:
	cmp al, '-'
	jnz MNP2
	ret
MNP2:
	inc dh				# no, reset flag
	cmp al, '+'
	jnz MNP3
	ret
MNP3:
	cmp al, PLUSTK			# ignore "+"
	jnz MNP4
	ret
MNP4:
	lahf
	dec ebx				# check if last character was a digit
	sahf
	ret				# return with non-zero set

# -------------------------------------------------------------------------------------
# Logical and relational operators.
# -------------------------------------------------------------------------------------
DOCMP:
	inc al				# setup bits
	adc al, al			# 4=less 2=equal 1=greater
	pop cx				# what did he want?
	and al, ch			# any bits match?
	add al, 255			# map 0 to 0
	sbb al, al			# and all others to 255
	call CONIA			# convert [A] to an integer signed
	jmp RETAPG			# return from operator application
NOTER:
	mov dh, 90			# "NOT" has precedence 90, so
	call LPOPER			# formula evaluation is entered with a dummy entry of 90 on the stack
	call FRCINT			# coerce the argument to integer
	not bx				# NOT [H,L]
	mov WORD PTR FACLO, bx		# update the FAC
	pop cx				# FRMEVL, after seeing the precedence of 90, thinks it is applying an operator
RETAPG:
	jmp RETAOP			# return to refetch it (the text pointer is in TEMP2)

# -------------------------------------------------------------------------------------
# GETYPR: replacement for the GETYPE RST. CGETYP sets the other condition codes correctly.
# -------------------------------------------------------------------------------------
	.globl GETYPR
GETYPR:
	mov al, BYTE PTR VALTYP		# (replacement for "GETYPE" RST)
	cmp al, 8
CGETYP:
	dec al				# set the other condition codes correctly
	dec al				# without affecting carry
	dec al
	ret

# -------------------------------------------------------------------------------------
# DANDOR applies AND, OR, XOR, EQV, IMP (and MOD, IDIV). The precedence is in [CH].
# The right hand argument is coerced to integer, as the left one was when pushed.
# -------------------------------------------------------------------------------------
DANDOR:
	mov al, ch			# get the precedence
	push ax				# save the precedence (2 bytes)
	call FRCINT			# coerce right hand argument to integer
	pop ax				# get back the precedence to distinguish AND and OR
	pop dx				# pop off the left hand argument (integer, 2 bytes)
	cmp al, 122			# is the operator MOD?
	jnz Q_Q_L035
	jmp IMOD			# if so, use the special routine
Q_Q_L035:
	cmp al, 123			# is the operator IDIV?
	jnz Q_Q_L036
	jmp IDIV			# let the special routine handle it
Q_Q_L036:
	mov ecx, offset MAKINT		# place to return when done
	push ecx			# save on stack (4 bytes)
	cmp al, 70			# set zero for OR
	jnz NOTOR
	# INS86 13,332 encodes OR BX,DX (opcode 0Bh, modrm DAh). The source comment says
	# "OR BX,BX"; the encoding and the semantics are BX := BX OR DX.
	or bx, dx
	ret				# return the integer in [H,L]
NOTOR:
	cmp al, 80			# AND?
	jnz NOTAND
	and bx, dx			# AND BX,DX
	ret
NOTAND:
	cmp al, 60			# XOR?
	jnz NOTXOR
	xor bx, dx			# XOR BX,DX
	ret
NOTXOR:
	cmp al, 50			# EQV?
	jnz NOTEQV
	xor bx, dx			# XOR BX,DX
	not bx				# NOT BX
	ret
# For IMP use A IMP B = NOT(A AND NOT(B))
NOTEQV:
	not bx				# NOT BX
	and bx, dx			# AND BX,DX
	not bx				# NOT BX
	ret

# -------------------------------------------------------------------------------------
# GIVDBL subtracts [D,E] from [H,L] and floats the result into the FAC.
# -------------------------------------------------------------------------------------
	.globl GIVDBL
GIVDBL:
	sub bx, dx			# [H,L]=[H,L]-[D,E]
	jmp INEG2			# float 2-byte unsigned int

# GIVFRE: FRE's result. [H,L]-[D,E] is a 32-bit byte count, which can exceed 64 K on a larger
# workspace. FLOATR takes a 24-bit unsigned mantissa from AL (high byte) and DX (low 16 bits), so the
# count is floated from those. Exact up to 8 MB (above that the top mantissa bit reads as a sign).
	.globl GIVFRE
GIVFRE:
	sub ebx, edx			# EBX = free bytes (32-bit)
	mov edx, ebx			# DX = bits 0..15
	shr ebx, 16
	mov al, bl			# AL = bits 16..23
	call VALSNG			# single precision
	jmp INEGAD			# FLOATR with a 24-bit integer (exponent 0x98)

	.globl SNGFLT, GIVINT
SNGFLT:
	mov bl, al			# make [A] an unsigned integer
	xor al, al
GIVINT:
	mov bh, al
	jmp MAKINT

# -------------------------------------------------------------------------------------
# USR (user machine code) and DEF USR. Machine-code calls are out of scope for this port
# (see NOTES.md). The argument syntax is parsed as in the original, then the statement or
# function reports "Illegal function call".
# -------------------------------------------------------------------------------------
USRFN:
	call SCNUSR			# scan the USR#
	call PARCHK			# eat left paren and formula
	jmp FCERR			# machine-code calls are not supported

SCNUSR:
	call CHRGTR			# get a char
	mov ecx, 0			# assume USR0
	cmp al, ONECON+10		# single byte int expected
	jae NOARGU			# no, must be defaulting to USR0
	cmp al, ONECON			# is it smaller than ONECON
	jb NOARGU			# yes, assume trying to default to USR0
USERCN:
	call CHRGTR			# scan past next char
	mov al, BYTE PTR CONLO		# get value of 1 byter
	or al, al			# make sure carry is off
	rcl al, 1			# multiply by 2
	mov cl, al			# save offset in [C]
NOARGU:
	xchg ebx, edx			# save text pointer in [D,E]
	mov ebx, offset USRTAB		# get start of table
	add ebx, ecx			# add on offset
	xchg ebx, edx			# restore text pointer, address to [D,E]
	ret

DEFUSR:
	call SCNUSR			# scan the USR name
	call SYNCHR
	.byte EQULTK			# must have equal sign
	call FRMQNT			# get the address (parsed; machine code not supported)
	jmp FCERR

# -------------------------------------------------------------------------------------
# DEF: DEF USR (parsed, then illegal function call), DEF FN, and DEF SEG (parsed; no
# segments in the flat model, so the address is evaluated and ignored).
# -------------------------------------------------------------------------------------
DEF:
	cmp al, USRTK			# defining the call address of USR routine?
	jz DEFUSR			# yes, do it
	cmp al, FNTK			# "FN"?
	jz DEFNC3			# then go do function name
	call SYNCHR
	.byte 'S'			# else must be "SEG" or error
	call SYNCHR
	.byte 'E'
	call SYNCHR
	.byte 'G'
	mov al, BYTE PTR [ebx]		# no assignment means no change of segment
	cmp al, EQULTK
	jnz DEFNC2
	call SYNCHR
	.byte EQULTK			# else must have equal sign
	call FRMQNT			# get the segment (parsed and discarded)
DEFNC2:
	ret				# return to NEWSTT
DEFNC3:
	call GETFNM			# get a pointer to the function name
	call ERRDIR			# DEF is "illegal direct"
	xchg ebx, edx			# [D,E]=text pointer after the function name; [H,L]=place for the value
	mov DWORD PTR [ebx], edx	# save the text pointer as the value (4 bytes)
	xchg ebx, edx			# restore the text pointer to [H,L]
	mov al, BYTE PTR [ebx]		# get next char
	cmp al, '('			# does this function have args?
	jz Q_Q_L039
	jmp DATA			# no
Q_Q_L039:
	call CHRGTR
SCNLIS:
	call PTRGET			# get pointer to dummy var (create var)
	mov al, BYTE PTR [ebx]		# get terminator
	cmp al, ')'			# end of arg list?
	jnz Q_Q_L040
	jmp DATA			# yes
Q_Q_L040:
	call SYNCHR
	.byte ','			# "," must follow then
	jmp SCNLIS

# -------------------------------------------------------------------------------------
# FNDOER: call a user-defined function. Stack: [type (2)] [0 marker (2)] [caller text
# pointer (4)] while the arguments are bound; each bound argument leaves a variable
# pointer (4), a type (2), and an 8-byte value area on the stack.
# -------------------------------------------------------------------------------------
FNDOER:
	call GETFNM			# get a pointer to the function name
	mov al, BYTE PTR VALTYP		# find out what kind of function it is
	or al, al			# push this [A] (carry off) so the values can be popped and restored
	push ax				# save so the final result is coerced to the function type (2 bytes)
	mov TEMP2, ebx			# save the text pointer that points past the function name
	xchg ebx, edx			# [H,L]=pointer to the value of the function
	mov ebx, [ebx]			# [H,L]=value of function: text pointer at the formal parameter list
	or ebx, ebx			# a zero text pointer means the function was never defined
	jnz Q_Q_L041
	jmp UFERR			# "undefined function"
Q_Q_L041:
	mov al, BYTE PTR [ebx]		# see if there are any parameters
	cmp al, '('			# parameter list starts with "("
	jz Q_Q_L042
	jmp FINVLS			# skip over parameter setup
Q_Q_L042:
	call CHRGTR			# go past the "("
	mov TEMP3, ebx			# save the text pointer to the start of the parameter list
	xchg ebx, edx			# [D,E]=text pointer of the parameter list
	mov ebx, TEMP2			# get the text pointer from the call (past the function name)
	call SYNCHR
	.byte '('			# make sure the argument list is there
	xor al, al			# indicate end of values to assign
	push ax				# (2 bytes)
	push ebx			# save the caller's text pointer (4 bytes)
	xchg ebx, edx			# get the pointer to the beginning of the parameter list
ASGMOR:
	mov al, 128			# outlaw arrays when scanning
	mov BYTE PTR SUBFLG, al		# parameters
	call PTRGET			# read a parameter
	xchg ebx, edx			# [D,E]=parameter list text, [H,L]=variable pointer
	pop esi				# XTHL: save the variable position, get the pointer at the arg list (4)
	xchg esi, ebx
	push esi			# (4 bytes)
	mov al, BYTE PTR VALTYP		# and its type (for coercion)
	push ax				# (2 bytes)
	push edx			# save the text pointer into the parameter list (4 bytes)
	call FRMEVL			# evaluate the argument
	mov TEMP2, ebx			# save the argument list pointer
	pop ebx				# and the parameter list pointer (4 bytes)
	mov TEMP3, ebx
	pop ax				# get the value type (2 bytes)
	call DOCNVF			# coerce the argument
	mov cl, 4			# make sure there is room for the value (4 words = 8 bytes)
	call GETSTK
	sub esp, 8			# save eight places (BX = SP-8)
	mov ebx, esp
	call VMOVMF			# put value into the reserved place in the stack
	mov al, BYTE PTR VALTYP		# save the type for assignment
	push ax				# (2 bytes)
	mov ebx, TEMP2			# reget the argument list pointer
	mov al, BYTE PTR [ebx]		# see what comes after the argument formula
	cmp al, ')'			# is the argument list ending?
	jz POPASG			# make sure the argument list also ended
	call SYNCHR
	.byte ','			# skip over argument comma
	push ebx			# save the argument list text pointer (4 bytes)
	mov ebx, TEMP3			# get the text pointer into the definition's parameter list
	call SYNCHR
	.byte ','			# skip over the parameter list comma
	jmp ASGMOR			# and bind the rest of the parameters
POPAS2:
	pop ax				# if assignment is successful update PRMLN2 (2 bytes)
	mov BYTE PTR PRMLN2, al		# indicate new variable is in place
POPASG:
	pop ax				# get the value type (2 bytes)
	or al, al
	jz FINASG			# zero means no more left to pop and assign
	mov BYTE PTR VALTYP, al
	mov ebx, esp			# point into the stack to get the saved value (BX = SP)
	call VMOVFM			# put value into FAC
	add esp, 8			# free up stack area
	pop edx				# get place to store to (4 bytes)
	mov bl, 3			# calculate the size of the name (length of the look)
LPSIZL:
	inc bl				# increment size
	dec edx				# point at previous character
	mov esi, edx
	mov al, BYTE PTR [esi]		# see if it is the length or another character
	or al, al
	jns Q_Q_L043
	jmp LPSIZL			# high bit indicates still part of name
Q_Q_L043:
	dec edx				# back up over looks
	dec edx
	dec edx
	mov al, BYTE PTR VALTYP		# get size of value
	add al, bl			# add on size of name
	mov ch, al			# save total length in [B]
	mov al, BYTE PTR PRMLN2		# get current size of block
	mov cl, al			# save in [C]
	add al, ch			# get potential new size
	cmp al, PRMSIZ			# can't exceed allocated storage
	jnae Q_Q_L044
	jmp FCERR
Q_Q_L044:
	push ax				# save new size (2 bytes)
	mov al, bl			# [A]=size of name
	movzx ecx, cl			# [B,C]=size of PARM2 so far (B=0)
	mov ebx, offset PARM2		# base of place to store into
	add ebx, ecx			# [H,L]=place to start the new variable
	movzx ecx, al			# [B,C]=length of name of variable
	call BCTRAN			# put in the new name
	mov ecx, offset POPAS2		# place to return after assignment
	push ecx			# (4 bytes) return address for LETCN4's RET
	push ebx			# (4 bytes) the slot COPNUM pops into EBX after its "pop edx"
	jmp LETCN4			# perform assignment on [H,L]
FINASG:
	mov ebx, TEMP2			# get argument list pointer
	call CHRGTR			# skip over the closing parenthesis
	push ebx			# save the argument text pointer (4 bytes)
	mov ebx, TEMP3			# get the parameter list text pointer
	call SYNCHR
	.byte ')'			# make sure the parameter list ended at the same time
	jmp FINVLS2			# (the original skipped the next byte, the PUSH DX, with an MVI AL,)
FINVLS:
	push edx			# here when there were no arguments or parameters: save the caller's text pointer
FINVLS2:
	mov TEMP3, ebx			# save the text pointer of the function
	mov al, BYTE PTR PRMLEN		# push PARM1 stuff onto the stack
	add al, 6			# with PRMLEN (2 bytes) and PRMSTK (4-byte link): 6 bytes extra
	push ax				# save the number of bytes (2 bytes)
	ror al, 1			# number of 2-byte entries in [A]
	mov cl, al
	call GETSTK			# is there room on the stack?
	pop ax				# [A]=amount to put onto the stack
	movzx ecx, al			# [B,C]=transfer count
	movzx ebx, al
	sub esp, ebx			# SET UP NEW STACK
	mov ebx, esp
	push ebx			# save the new value for PRMSTK (4 bytes)
	mov edx, offset PRMSTK		# fetch data from here
	call BCTRAN
	pop ebx
	mov DWORD PTR PRMSTK, ebx	# link parameter block for garbage collection
	movzx ebx, WORD PTR PRMLN2	# now put PARM2 into PARM1
	mov WORD PTR PRMLEN, bx		# set up length
	movzx ecx, bx			# [B,C]=transfer count
	mov ebx, offset PARM1
	mov edx, offset PARM2
	call BCTRAN
	mov bh, al			# clear out PARM2
	mov bl, al
	mov WORD PTR PRMLN2, bx
	mov bx, WORD PTR FUNACT		# increment function count
	inc bx
	mov WORD PTR FUNACT, bx
	mov al, bh
	or al, bl			# set up active flag non-zero
	mov BYTE PTR NOFUNS, al
	mov ebx, TEMP3			# get back the function definition text pointer
					# skip over the "=" in the definition
	call FRMEQL			# and evaluate the definition formula (can recurse here)
	dec ebx
	call CHRGTR			# see if the statement ended right
	jz Q_Q_L045
	jmp SNERR			# (a known shortcut: the line number reported is the caller's)
Q_Q_L045:
	call GETYPR			# see if the result is a string
	jnz NOCPRS			# (a string descriptor in PARM1 would be wiped out otherwise)
	mov edx, offset DSCTMP		# DSCTMP is past all the temp area
	mov ebx, DWORD PTR FACLO	# get the address of the descriptor
	cmp ebx, edx
	jb NOCPRS			# result is a temp: no copy necessary
	call STRCPY			# make a copy in DSCTMP
	call PUTTMP			# put result in a temp and make FACLO point at it
NOCPRS:
	mov ebx, DWORD PTR PRMSTK	# get place to restore PARM1 from stack
	mov edx, ebx
	movzx ecx, WORD PTR [ebx+4]	# [B,C]=length: PRMLEN follows the 4-byte link in the block
	add ecx, 6			# include extra bytes (4-byte link + 2-byte PRMLEN)
	mov ebx, offset PRMSTK		# place to store into
	call BCTRAN
	xchg ebx, edx			# [D,E]=place to restore stack to
	mov esp, ebx
	mov bx, WORD PTR FUNACT		# decrease active function count
	dec bx
	mov WORD PTR FUNACT, bx
	mov al, bh
	or al, bl			# set up function flag
	mov BYTE PTR NOFUNS, al
	pop ebx				# get back the caller's text pointer (4 bytes)
	pop ax				# get back the type of the function (2 bytes)
	.globl DOCNVF
DOCNVF:
	push ebx			# save the text pointer
	and al, 7			# setup dispatch to force formula type to conform to the variable
	mov ebx, offset FRCTBL		# table of force routines (4-byte entries)
	movzx ecx, al
	shl ecx, 2			# dispatch byte offset = 4 * type
	call DISPAT			# dispatch
	pop ebx				# get back the text pointer
	ret

# -------------------------------------------------------------------------------------
# BCTRAN: block transfer with source in [D,E], destination in [H,L], count in [B,C].
# The transfer is forward (BCTRAL). The count is 16-bit, as in the original.
# -------------------------------------------------------------------------------------
	.globl BCTRAN
BCTRAL:
	mov esi, edx
	mov al, BYTE PTR [esi]
	mov BYTE PTR [ebx], al
	inc ebx
	inc edx
	dec cx
BCTRAN:
	mov al, ch
	or al, cl
	jnz BCTRAL
	ret

# -------------------------------------------------------------------------------------
# ERRDIR: if in direct mode (CURLIN = 65535, so CURLIN+1 = 0), give "Illegal direct".
# -------------------------------------------------------------------------------------
	.globl ERRDIR
ERRDIR:
	push ebx			# save their [H,L]
	movzx ebx, WORD PTR CURLIN	# see what the current line is
	inc bx				# direct is 65535, so now 0
	or bx, bx			# is it zero now?
	pop ebx
	jnz ERRDR1			# return if not direct
	mov dl, ERRID			# "ILLEGAL DIRECT" error
	jmp ERROR
ERRDR1:
	ret

# -------------------------------------------------------------------------------------
# GETFNM: a pointer to a function name ("FN" followed by the name). The name is not an array.
# -------------------------------------------------------------------------------------
	.globl GETFNM
GETFNM:
	call SYNCHR
	.byte FNTK			# must start with "FN"
	mov al, 128			# don't allow an array
	mov BYTE PTR SUBFLG, al		# don't recognize the "(" as the start of an array reference
	or al, BYTE PTR [ebx]		# put function bit on
	mov cl, al			# get first character into [C]
	jmp PTRGT2

# -------------------------------------------------------------------------------------
# ISMID_ (the original ISMID$): left-hand side MID$ statement.
# -------------------------------------------------------------------------------------
	.globl ISMID_
ISMID_:
	cmp al, 126			# function? (FF - $END)
	jz Q_Q_L046
	jmp SNERR			# no, error
Q_Q_L046:
	inc ebx				# point to next char
	mov al, BYTE PTR [ebx]		# get FN descriptor
	inc ebx				# point to char after
	cmp al, MIDTK			# is it MID?
	jnz Q_Q_L047
	jmp LHSMID			# code is in BISTRS
Q_Q_L047:
	jmp SNERR

# -------------------------------------------------------------------------------------
# INP, OUT, WAIT (hardware ports). Out of scope for this port: the syntax is parsed as in
# the original, then the function or statement reports "Illegal function call".
# -------------------------------------------------------------------------------------
FNINP:
	call FRQINT			# make argument an integer channel number
	jmp FCERR			# port I/O is not supported
SETIO:
	call FRMQNT			# read a 16-bit port
	push dx				# save for use by WAIT and OUT
	call SYNCHR
	.byte 44			# ","
	call GETBYT			# read the data byte to [A] and [E]
	pop dx				# return the port in DX
	ret
FNOUT:
	call SETIO			# get port and data (parsed)
	jmp FCERR			# port I/O is not supported
FNWAIT:
	call SETIO			# set up for WAIT (parsed)
	dec ebx
	call CHRGTR			# see if the statement ended
	jz Q_Q_WAIT1			# if no third argument, skip it
	call SYNCHR
	.byte 44			# make sure there is a ","
	call GETBYT			# (third argument, parsed)
Q_Q_WAIT1:
	jmp FCERR			# port I/O is not supported
CONSOL:
	jmp SNERR

# -------------------------------------------------------------------------------------
# Integer and byte argument helpers.
# -------------------------------------------------------------------------------------
	.globl GETINT, GETIN2, INTFR2, GTBYTC, GETBYT, CONINT
GETINT:
	call CHRGTR
GETIN2:
	call FRMEVL			# evaluate a formula
INTFR2:
	push ebx			# save the text pointer
	call FRCINT			# convert the formula to an integer in [H,L]
	xchg ebx, edx			# put the integer into [D,E]
	pop ebx				# restore the text pointer
	mov al, dh			# set the condition codes on the high order
	or al, al
	ret
GTBYTC:
	call CHRGTR
GETBYT:
	call FRMEVL			# evaluate a formula
CONINT:
	call INTFR2			# convert the FAC to an integer in [D,E] and set the condition codes
	jz Q_Q_L048
	jmp FCERR			# wasn't an error
Q_Q_L048:
	dec ebx				# functions can get here with a bad [H,L], not serious
	call CHRGTR			# set condition codes on terminator
	mov al, dl			# return the result in [A] and [E]
	ret
