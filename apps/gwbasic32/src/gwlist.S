# GWLIST.ASM -> gwlist.S
# LIST, LLIST, DELETE, the line crunch/decrunch routines (BUFLIN, PLOOP2, RESSR*, NUMLIN),
# and the line-memory compactor DEL.
#
# Line record layout (widened, see NOTES.md):
#     [link: 4 bytes][line number: 2 bytes][text ... 0]
# The link is the address of the next record (0 at the end of the program).
# Pointers (BX, CX, DX when used as pointers, SI, DI) are 32-bit. Line numbers and
# character counts stay 16-bit / 8-bit. Stack widths follow the pushed values: a pointer
# is 4 bytes, a line number 2 bytes, a return address 4 bytes.
#
# Dropped: CS-segment prefixes (INS86 56), since code and data share one flat space.
# The CPU stores in the original (MOV DI,CX / STOSB) become MOV [ECX],AL, because
# STOSB only stores at DI and DI = CX here. "INC" that must keep the flags (LAHF/SAHF pairs)
# become LEA, which does not touch the flags.

	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"

	.section .text
	.globl LLIST, LIST, LISPRT, BUFLIN, PLOOP2, TSTANM, DELETE, DEL

# External routines (other stage 3 modules, and stage 4 for ISFLIO/OUTCH1/OUTDO):
#   SCNLIN, PROCHK, CHRGTR, SYNCHR, NAMSCN, NULOPM, POLKEY, READY, STPRDY, LINPRT,
#   OUTDO, OUTCH1, CRDO, ISFLIO, PPSWRT, ISLET2, CONFC1, FOUT, FOUTO, FOUTH, INXHRT,
#   CONFC1, DEPTR, FNDLIN, FCERR, EOSCHK, STROUT, FINI, REDDY (gwdata), LISTX (ibmres_tail),
#   FRQINT, CONLIN is local.
# Contract with SCNLIN (GWMAIN): on return ECX = pointer to the first line in the range,
# and the max line number (2 bytes) is left on the stack under the return address
# (as in the original).

# -------------------------------------------------------------------------------------
# LLIST: LIST to the printer. The LPT pseudo-FDB is 0FFFFh in the original (16-bit);
# here it is 0FFFFFFFFh.
# -------------------------------------------------------------------------------------
LLIST:
	push ebx
	mov ebx, 0xFFFFFFFF
	mov PTRFIL, ebx		# FDB pointer = LPT pseudo-FDB
	pop ebx
LIST:
	pop ecx			# get rid of the NEWSTT return address
	call SCNLIN		# scan line range
	push ecx		# save pointer to 1st line
	call PROCHK		# don't even list line #
	mov ebx, TEMP
	dec ebx
	call CHRGTR
	jz LIST4		# LIST to the screen
	call SYNCHR
	.byte 44		# else must be ,"dev:"
	call NAMSCN		# crack file spec.
	mov al, MD_SQO		# open file 0 for output
	call NULOPM
LIST4:
	mov WORD PTR CURLIN, 65535	# don't allow ^C to change continue parameters
	pop ebx			# pointer to the line
	pop dx			# max line number off the stack
	mov ecx, [ebx]		# ECX = link to the next line
	add ebx, 4		# BX -> line number
	test ecx, ecx		# end of chain?
	jnz Q_Q_L000
	jmp READY		# last line: stop
Q_Q_L000:
	call POLKEY		# check CTL-C, queue typeahead
	push ecx		# save link
	movzx ecx, WORD PTR [ebx]	# CX = line number
	add ebx, 2		# BX -> text
	push ebx		# save text pointer
	mov ebx, ecx		# BX = line number
	xchg bx, dx		# BX = max line, DX = line number
	cmp bx, dx		# past the last line in range? (max - line)
	pop ecx			# text pointer to CX (POP keeps the flags)
	jae Q_Q_L001
	jmp STPRDY		# past the range: done listing
Q_Q_L001:
	pop esi			# link
	push ebx			# max line on the bottom of the stack (2 bytes)
	push esi		# link
	push ecx		# text pointer
	movzx ebx, dx		# BX = line number
	mov WORD PTR DOT, bx	# save for later edit or list
	call LINPRT		# print as integer without leading space
	pop ebx			# text pointer
	mov al, [ebx]		# get byte from line
	cmp al, 9		# is it a tab?
	jz Q_Q_NOSPAL		# then don't print a space
	mov al, ' '
	call OUTDO		# print a space after the line #
Q_Q_NOSPAL:
	call BUFLIN		# unpack the line into BUF
	mov ebx, offset BUF
	call LISPRT		# print the line
	call CRDO		# print CRLF
	jmp LIST4		# go back for the next line

# -------------------------------------------------------------------------------------
# LISPRT: output the NUL-terminated line at BX.
# -------------------------------------------------------------------------------------
LISPRT:
	mov al, [ebx]
	cmp al, 10
	jnz Q_Q_L002
	call LISEOL		# erase to end-of-line after LF and 0
Q_Q_L002:
	or al, al
	jnz Q_Q_L003
	call LISEOL
Q_Q_L003:
	jnz Q_Q_L004
	ret			# end of line
Q_Q_L004:
	call OUTCH1		# output char and check for LF
	inc ebx
	jmp LISPRT

# Clear to end of line if the current device is the screen. All registers and the flags
# are preserved.
LISEOL:
	lahf			# PUSH PSW
	xchg al, ah
	push ax
	xchg al, ah
	call ISFLIO
	jz Q_Q_L004E
	jmp LISELX		# return if listing to a file
Q_Q_L004E:
	mov al, BYTE PTR F_EDIT	# save current edit mode
	lahf			# PUSH PSW
	xchg al, ah
	push ax
	xchg al, ah
	mov al, 5		# 05 = erase to end-of-line for non-IBMLIK
	mov BYTE PTR F_EDIT, al	# enter edit mode (05 is a control function)
	call OUTDO		# output 05 to the current device
	pop ax			# POP PSW
	xchg al, ah
	sahf
	mov BYTE PTR F_EDIT, al	# restore edit mode
LISELX:
	pop ax			# POP PSW
	xchg al, ah
	sahf
	ret

# -------------------------------------------------------------------------------------
# BUFLIN: unpack the crunched line at BX into BUF (BUFLEN bytes max).
# -------------------------------------------------------------------------------------
BUFLIN:
	mov ecx, offset BUF	# start of the text buffer
	mov dh, BUFLEN		# its length in DH
	xor al, al		# decrunch mode
	mov BYTE PTR DORES, al	# bit 0 is quote, bit 1 is data, bit 2 is REM
	xor al, al		# special char for space insertion
	mov BYTE PTR TEMPA, al
	call PROCHK		# only proceed if OK
	jmp PLOOP2		# start here

PLOOP:
	inc ecx			# increment deposit pointer
	inc ebx			# advance text pointer
	dec dh			# bump down count
	jnz Q_Q_PL1
	ret			# buffer full
Q_Q_PL1:
PLOOP2:
	mov al, [ebx]		# get char from BUF
	or al, al		# set CCs
	mov [ecx], al		# save this char
	jnz Q_Q_PL2
	ret			# end of source buffer: all done
Q_Q_PL2:
	cmp al, OCTCON		# smaller than the smallest embedded constant?
	jb NTEMBL		# yes, don't treat as one
	cmp al, DBLCN1		# embedded constant?
	mov dl, al		# save char in DL
	jb PRTVAR		# print leading space if necessary
	cmp al, 34		# quotation mark?
	jnz BFCHKC		# if not, check for colon
	mov al, BYTE PTR DORES	# complement the quote bit
	xor al, 1
	mov BYTE PTR DORES, al
	mov al, 34		# restore the character
BFCHKC:
	cmp al, ':'		# colon ending data?
	jnz NTEMBL
	mov al, BYTE PTR DORES	# don't end if in quote
	rcr al, 1
	jb QTCOLN
	rcl al, 1
	and al, 253		# turn off bit 1 (data bit)
	mov BYTE PTR DORES, al
QTCOLN:
	mov al, ':'
NTEMBL:
	or al, al		# set CCs
	jns Q_Q_PL3
	jmp PLOOPR		# reserved word of some kind
Q_Q_PL3:
	mov dl, al		# save char in DL
	cmp al, '.'		# dot is part of a variable name
	jz PRTVAR
	call TSTANM		# is char alphanumeric?
	jae PRTVAR		# alphanumeric
	xor al, al		# make special
	jmp PLOOPH
PRTVAR:
	mov al, BYTE PTR TEMPA	# what did we do last?
	or al, al		# set CCs
	jz PLOOPG		# special, never insert space
	inc al			# in reserved word?
	jnz PLOOPG		# no
	mov al, ' '		# put out a space before the reserved word
	mov [ecx], al		# store in buffer
	inc ecx			# increment pointer into buffer
	dec dh			# space left?
	jnz Q_Q_PV1
	ret			# no, done
Q_Q_PV1:
PLOOPG:
	mov al, 1		# store flag saying in var
PLOOPH:
	mov BYTE PTR TEMPA, al
	mov al, dl		# get back the char we had
	cmp al, OCTCON		# smaller than the smallest embedded constant?
	jb PLOOPZ		# yes, don't treat as one
	cmp al, DBLCN1		# embedded constant?
	jae Q_Q_PL4
	jmp NUMLIN		# yes, unpack it
Q_Q_PL4:
PLOOPZ:
	mov [ecx], al		# make sure byte stored after space
	jmp PLOOP		# store in buffer

PLOOPR:
	mov al, BYTE PTR DORES	# see what our uncrunch mode is
	rcr al, 1		# the LSB is the quote bit
	jb GPLOOP
	rcr al, 1		# get the REM bit
	rcr al, 1		# and see if set
	jae CHKDRS		# if not, just check the data bit
	mov al, [ebx]		# must see if it's SNGQTK
	cmp al, SNGQTK		# and preceded by ":REM"
	push ebx
	push ecx		# save buffer pointer
	mov ebx, offset NOSNGQ	# place to return on failure
	push ebx
	jz Q_Q_PR1
	ret
Q_Q_PR1:
	dec ecx
	mov al, [ecx]
	cmp al, 'M'
	jz Q_Q_PR2
	ret
Q_Q_PR2:
	dec ecx
	mov al, [ecx]
	cmp al, 'E'
	jz Q_Q_PR3
	ret
Q_Q_PR3:
	dec ecx
	mov al, [ecx]
	cmp al, 'R'
	jz Q_Q_PR4
	ret
Q_Q_PR4:
	dec ecx
	mov al, [ecx]
	cmp al, ':'
	jz Q_Q_PR5
	ret
Q_Q_PR5:
	pop eax			# get rid of the return-on-fail address
	pop eax			# get rid of the saved buffer pointer
	pop ebx			# get back the pointer into the line
	add dh, 4		# update the char count
	jmp RESEXP

NOSNGQ:
	pop ecx			# get back the buffer pointer
	pop ebx			# get back the source line pointer
	mov al, [ebx]		# get back the character
GPLOOP:
	jmp PLOOP

DATSET:
	mov al, BYTE PTR DORES	# bit indicating inside data
	or al, 2		# is bit 1
SETDRS:
	mov BYTE PTR DORES, al
	xor al, al
	ret

REMSET:
	mov al, BYTE PTR DORES
	or al, 4
	jmp SETDRS

CHKDRS:
	rcl al, 1		# get the data bit into carry
	jb GPLOOP
	mov al, [ebx]		# get back the character
	cmp al, _DATA		# need to set a bit
	jnz Q_Q_CD1
	call DATSET
Q_Q_CD1:
	cmp al, _REM
	jnz Q_Q_CD2
	call REMSET
Q_Q_CD2:
RESEXP:
	mov al, [ebx]
	inc al			# set zero if FN token
	mov al, [ebx]		# get char back
	jnz NTFNTK		# not a function: treat normally
	inc ebx			# bump pointer
	mov al, [ebx]		# get char
	and al, 0x7F		# turn off high bit
NTFNTK:
	inc ebx			# advance to point after
NTQTTK:
	cmp al, _ELSE		# ELSE?
	jnz NOTELS
	dec ecx
	inc dh
NOTELS:
	cmp al, _WHILE		# might have an extra "+" in WHILE formula
	jnz BFNTWH		# so skip over it if it's there
	mov al, [ebx]		# get char to see if it's PLUSTK
	inc ebx			# assume is PLUSTK
	cmp al, PLUSTK
	mov al, _WHILE		# restore token value
	jz BFNTWH		# version of BASIC before crunch changed
	dec ebx			# move pointer back
BFNTWH:
	push ebx		# save text pointer
	push ecx		# save deposit pointer
	push dx			# save char count
	call LISTX		# handle extended reserved words
	mov ebx, offset RESLST - 1	# start of the reserved-word list
	mov ch, al		# save this char in CH
	mov cl, 'A' - 1		# initial leading char value
RESSR3:
	inc cl			# bump leading char value
RESSR1:
	inc ebx			# bump pointer into RESLST
RESSRC:
	mov edx, ebx		# save pointer to the start of this reserved word
RESSR2:
	mov al, [ebx]		# get char from RESLST
	or al, al		# set CCs
	jz RESSR3		# end of this char's table: go back and bump CL
	lea ebx, [ebx+1]	# bump source pointer (flags kept)
	js Q_Q_RS1		# end of this word
	jmp RESSR2		# not end: keep looking
Q_Q_RS1:
	mov al, [ebx]		# get ptr to reserved word value
	cmp al, ch		# same as the one we search for?
	jnz RESSR1		# no, keep looking
	xchg ebx, edx		# save found pointer in BX
	cmp al, _USR		# USR function token?
	jz NOISPA		# don't insert space
	cmp al, _FN		# is it a function token?
NOISPA:
	mov al, cl		# get leading char
	pop dx			# restore line char count
	pop ecx			# restore deposit pointer
	mov dl, al		# save leading char
	jnz NTFNEX		# not "FN" expansion
	mov al, BYTE PTR TEMPA	# set CCs on TEMPA
	or al, al
	mov al, 0		# clear reserved-word flag: mark as special
	mov BYTE PTR TEMPA, al	# set flag
	jmp MORLNZ		# do expansion
NTFNEX:
	cmp al, 'Z'+1		# was it a special char?
	jnz NTSPCH		# non-special char
	xor al, al		# set non-special
	mov BYTE PTR TEMPA, al
	jmp MORPUR		# print it
NTSPCH:
	mov al, BYTE PTR TEMPA	# what did we do last?
	or al, al		# special?
	mov al, 255		# flag in reserved word
	mov BYTE PTR TEMPA, al	# clear flag
MORLNZ:
	jz MORLN0		# get char and proceed
	mov al, ' '		# put space in buffer
	mov [ecx], al
	inc ecx
	dec dh			# any space left in buffer?
	jnz Q_Q_ML1
	jmp PPSWRT		# no, return
Q_Q_ML1:
MORLN0:
	mov al, dl
	jmp MORLN1		# continue
MORPUR:
	mov al, [ebx]		# get byte from reserved word
	inc ebx			# bump pointer
MORLNP:
	mov dl, al		# save char
MORLN1:
	and al, 0x7F		# turn off high-order bit for disk and edit
	mov [ecx], al		# store this char
	inc ecx			# bump pointer
	dec dh			# bump down remaining char count
	jnz Q_Q_ML2
	jmp PPSWRT		# end of line: just return
Q_Q_ML2:
	or al, dl		# set CCs
	js Q_Q_ML3
	jmp MORPUR		# end of reserved word?
Q_Q_ML3:
	cmp al, '(' + 128	# SPC( or TAB( ?
	jnz NTSPCT
	xor al, al		# clear flag
	mov BYTE PTR TEMPA, al	# to insert a space afterwards
NTSPCT:
	pop ebx			# restore source pointer
	jmp PLOOP2		# get next char from the line

# TSTANM: CY=0 if AL is a letter or a digit. (Digits: AL in 0..9, with CMC as in the original.)
TSTANM:
	call ISLET2		# letter?
	jb Q_Q_TA1
	ret			# yes
Q_Q_TA1:
	cmp al, '0'		# digit?
	jae Q_Q_TA2
	ret			# too small
Q_Q_TA2:
	cmp al, '9'+1		# last digit
	cmc			# make carry right
	ret			# no carry = digit

# NUMLIN: unpack an embedded constant (token in AL) at BX.
NUMLIN:
	dec ebx			# move pointer back as CHRGTR increments
	call CHRGTR		# scan the constant
	push dx			# save char count
	push ecx		# save deposit pointer
	push ax			# save constant type
	call CONFC1		# move constant into FAC
	pop ax			# restore constant type
	mov ecx, offset CONLIN	# put return address on stack
	push ecx
	cmp al, OCTCON		# octal constant?
	jnz Q_Q_NL1
	jmp FOUTO		# print it
Q_Q_NL1:
	cmp al, HEXCON		# hex constant?
	jnz Q_Q_NL2
	jmp FOUTH		# print in hex
Q_Q_NL2:
	movzx ebx, WORD PTR CONLO	# get line # value if one
	jmp FOUT		# print remaining possibilities

CONLIN:
	pop ecx			# restore deposit pointer
	pop dx			# restore char count
	mov al, BYTE PTR CONSAV	# get saved constant token
	mov dl, 'O'		# assume octal constant
	cmp al, OCTCON		# octal constant?
	jz SAVBAS		# yes, print it
	cmp al, HEXCON		# hex constant?
	mov dl, 'H'		# assume so
	jnz NUMSLN		# not base constant
SAVBAS:
	mov al, '&'		# print leading base indicator
	mov [ecx], al		# save it
	inc ecx			# bump pointer
	dec dh			# bump down char count
	jnz Q_Q_CL1
	ret			# return if end of buffer
Q_Q_CL1:
	mov al, dl		# get base char
	mov [ecx], al		# save it
	inc ecx			# bump pointer
	dec dh			# bump down base count
	jnz Q_Q_CL2
	ret			# end of buffer, done
Q_Q_CL2:
NUMSLN:
	mov al, BYTE PTR CONTYP	# get type of constant we are
	cmp al, 4		# is it single or double precision?
	mov dl, 0		# no, never print trailing type indicator
	jb TYPSET
	mov dl, '!'		# assume single precision
	jz TYPSET		# CONTYP=4 was single
	mov dl, '#'		# double precision indicator
TYPSET:
	mov al, [ebx]		# get leading char
	cmp al, ' '		# leading space
	jnz Q_Q_NS1
	call INXHRT		# go by it
Q_Q_NS1:
NUMSL2:
	mov al, [ebx]		# get char from number buffer
	inc ebx			# bump pointer
	or al, al		# set CCs
	jz NUMDN		# if zero, all done
	mov [ecx], al		# save char in buf
	inc ecx			# bump pointer
	dec dh			# see if end of buffer
	jnz Q_Q_NS2
	ret			# if end of buffer, return
Q_Q_NS2:
	mov al, BYTE PTR CONTYP	# get type of constant to be printed
	cmp al, 4		# test for single or double precision
	jb NUMSL2		# no, was integer
	lea ecx, [ecx-1]	# pick up saved char (flags kept)
	mov al, [ecx]
	lea ecx, [ecx+1]	# restore to point where it should be (flags kept)
	jnz DBLSCN		# if double, don't test for embedded "."
	cmp al, '.'		# test for fraction
	jz ZERE			# if single & embedded ., don't print !
DBLSCN:
	cmp al, 'D'		# double precision exponent?
	jz ZERE			# yes, mark no value type indicator needed
	cmp al, 'E'		# single precision exponent?
	jnz NUMSL2		# no, proceed
ZERE:
	mov dl, 0		# mark no printing of type indicator
	jmp NUMSL2		# keep moving number chars into buf

NUMDN:
	mov al, dl		# get flag to indicate whether to insert a "D"
	or al, al		# after double precision number
	jz NOD			# no, don't insert it
	mov [ecx], al		# save in buffer
	inc ecx			# bump pointer
	dec dh			# decrement count of chars left in buffer
	jnz Q_Q_ND1
	ret			# =0, must truncate list of this line
Q_Q_ND1:
NOD:
	mov ebx, CONTXT		# get back text pointer after constant
	jmp PLOOP2		# get next char

# -------------------------------------------------------------------------------------
# DELETE: remove the lines in the range (BEFORE the lines are deleted, OK is printed).
# -------------------------------------------------------------------------------------
DELETE:
	call SCNLIN		# scan line range
	push ecx
	call DEPTR		# change pointers back to numbers
	pop ecx
	pop dx			# pop max line off the stack
	push ecx		# save pointer to start of deletion (for CHEAD after FINI)
	push ecx		# save pointer to start of 1st line
	call FNDLIN		# find the last line
	jae FCERRG		# must have a match on the upper bound
	mov edx, ebx		# [DX] = pointer at the start of the line beyond the range
	pop esi			# XTHL
	xchg esi, ebx
	push esi		# save the pointer to the next line
	push ebx		# save the pointer to the start of the first line in range
	cmp ebx, edx		# make sure the start comes before the end
FCERRG:
	jnae Q_Q_DL1
	jmp FCERR		# "ILLEGAL FUNCTION CALL"
Q_Q_DL1:
	# DELETE must now be at EOL
	mov ebx, TEMP		# get text pointer
	call EOSCHK		# check for end of statement
	mov ebx, offset REDDY	# print "OK" prematurely
	call STROUT
	pop ecx			# get pointer to first in [CX]
	mov ebx, offset FINI	# go back to FINI when done
	pop esi			# XTHL
	xchg esi, ebx
	push esi		# [BX]=pointer to the next line

# -------------------------------------------------------------------------------------
# DEL: erase one line from memory.
#   [CX] = start of the line being deleted
#   [DX] = start of the next line
# -------------------------------------------------------------------------------------
DEL:
	xchg ebx, edx		# [DX] now has the pointer to the line beyond this one
	mov ebx, VARTAB		# compactifying to VARTAB
MLOOP:
	mov al, [edx]
	mov [ecx], al		# shoving down to eliminate a line
	inc ecx
	inc edx
	cmp ebx, edx		# done compactifying?
	jnz MLOOP		# no
	mov ebx, ecx
	# Clear SCALARS and ARRAYS in case we are replacing a line that may force
	# garbage collection in BLTU.
	mov VARTAB, ebx
	mov ARYTAB, ebx
	mov STREND, ebx
	ret
