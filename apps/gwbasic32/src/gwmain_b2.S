# GWMAIN.ASM lines 2636-3566 -> gwmain_b2.S (second part of the interpreter's main module).
# Covers: LINE INPUT, INPUT (incl. the two-pass type scan), READ/INPUT data handling,
# PEEK/POKE, RENUMBER, ANSI helpers, the text scanners NXTSCN/WNDSCN/FORTRM etc. and FINOVC/CLROVC.
#
# Width rules: pointers (BX, DX/CX when holding a variable or text pointer, SI) are 32-bit;
# pushed/popped pointers are 4 bytes; line numbers and counts are 2/1 bytes.
#   gwmain_a.S: 1-299, 372-1422   gwmain.S: 300-371, 1423-1622   gwmain_b1.S: 1623-2635
	.intel_syntax noprefix
	.include "gwconst.inc"			# shared constants (tokens, error numbers)
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"
	.include "forent.inc"
	# Error codes used here (copied from gwdata_errors.inc; that file also holds message data, so it is not included)
	.set ERROD, 4		# copy of gwdata_errors.inc (keep in sync)
	.set ERRFN, 26		# copy of gwdata_errors.inc (keep in sync)
	.set ERRWH, 29		# copy of gwdata_errors.inc (keep in sync)

	.section .text
	.globl LINE, INPUT, RDOINP, SCNSTR, QTINP, INPBAK, TRMNOK, RDOIN2, FILSTI
	.globl PEEK, POKE, RESEQ, WNDSCN, NXTSCN, FORTRM, FORINC, DECNXT, TRMNXT
	.globl FINOVC, CLROVC, _FLGOC, _CLROV
	.globl READ, RANDOM, OPTION, SCNEXT, SCCLIN, STRPRN, _STPRN, DATBK

# Dispatch names that the reserved-word tables reference, and the DO_EXT list, are not needed here.
# Undefined symbols (other stage 3 modules or stage 4) are external by default.

# ---------------------------------------------------------------------------------------
# LINE INPUT. LINE with any other keyword is a graphics statement: out of scope, FCERR.
# ---------------------------------------------------------------------------------------
LINE:
	cmp al, _INPUT
	jz Q_Q_L064
	jmp FCERR			# graphics LINE (out of scope)
Q_Q_L064:
	call SYNCHR
	.byte _INPUT
	cmp al, '#'			# see if there is a file number
	jnz Q_Q_L065
	jmp DLINE			# disk line input (Stage 4 file layer)
Q_Q_L065:
	call SCNSEM			# scan semicolon for no-CR
	call QTINP			# print quoted string if one
	call PTRGET			# read string to store into
	call CHKSTR			# make sure its a string
	push edx			# save pointer at variable
	push ebx			# save text pointer
	call SINLIN			# read a line of input
	pop edx				# get text pointer
	pop ecx				# get pointer at variable
	jae Q_Q_L066
	jmp STPEND			# if control-C, stop
Q_Q_L066:
RELINE:
	push ecx			# save back variable pointer
	push edx			# save text pointer
	mov ch, 0			# setup zero as only terminator
	call STRLT3			# literalize the input
	pop ebx				# restore text pointer
	mov al, 3			# set three for string
	jmp LETCN2			# do the assignment

TRYAGN:
	.ascii "?Redo from start"
	.byte 13, 10			# ACRLF
	.byte 0

# Here when passing over a string literal in a subscript of a variable in INPUT list,
# on the first pass of INPUT checking for type match and number.
SCNSTR:
	inc ebx				# look at the next character
	mov al, BYTE PTR [ebx]		# fetch it
	or al, al			# end of line?
	jnz Q_Q_L067
	jmp SNERR			# ending in string in subscript is bad syntax
Q_Q_L067:
	cmp al, 34			# only other terminator is quote
	jnz SCNSTR			# continue until quote or 0 is found
	jmp SCNCON			# continue matching parens since string ended

INPBAK:
	pop ebx				# get rid of pass1 data pointer
	pop ebx				# get rid of pass2 data pointer
	jmp RDOIN2			# get rid of pass2 varlst pointer and retry

# Here when the data typed in or in DATA statements is improperly formatted.
# For INPUT we start again. For READ we give a syntax error at the data line.
TRMNOK:
	mov al, BYTE PTR FLGINP		# was it READ or INPUT?
	or al, al			# zero = INPUT
	jz Q_Q_L068
	jmp DATSNE			# give error at data line
Q_Q_L068:
RDOIN2:
	pop ecx				# get rid of the pointer into the variable list
RDOINP:
	mov ebx, offset TRYAGN
	call STROUT			# print "?REDO FROM START"
	mov ebx, DWORD PTR SAVTXT	# get saved text pointer
	ret				# go back to NEWSTT of the INPUT statement

FILSTI:
	call FILINP
	push ebx			# put the text pointer on the stack
	mov ebx, offset BUFMIN		# point at a comma
	jmp INPCN3

INPUT:
	cmp al, '#'
	jz FILSTI
	call SCNSEM			# scan semicolon for no-CR
	mov ecx, offset NOTQTI		# where to go
	push ecx			# when done with quoted string
QTINP:
	cmp al, 34			# is it a quote?
	mov al, 0			# be talkative
	mov BYTE PTR CNTOFL, al		# force output
	mov al, 255			# make non-zero value
	mov BYTE PTR TEMPA+1, al	# flag to do "? "
	jz Q_QTINP_OK			# (MASM: JZ $+3, skip the RET)
	ret				# just return
Q_QTINP_OK:
	call STRLTI			# make the message a string
	mov al, BYTE PTR [ebx]		# get char
	cmp al, 44			# comma? (54 octal)
	jnz NTICMA			# no
	xor al, al			# flag not to do it
	mov BYTE PTR TEMPA+1, al
	call CHRGTR			# fetch next char
	jmp INPCMA			# continue
NTICMA:
	call SYNCHR
	.byte 59			# must end with semi-colon
INPCMA:
	push ebx			# remember where it ended
	call STRPRT			# print it out
	pop ebx				# get back saved text ptr
	ret				# all done
NOTQTI:
	push ebx
GETAGN:
	mov al, BYTE PTR TEMPA+1	# do "? "
	or al, al
	jz SUPPRS			# then suppress "?"
	mov al, '?'			# type "?" and input a line of text
	call OUTDO
	mov al, ' '
	call OUTDO
SUPPRS:
	call SINLIN
	pop ecx				# take off since maybe leaving
	jae Q_Q_L069
	jmp STPEND			# if empty leave
Q_Q_L069:
	push ecx			# put back since didn't leave

# First pass, as ANSI requires: no values are assigned before checking type and number.
# The variable list is scanned without evaluating subscripts, and the input is scanned to
# get its type. No assignment is done.
	xor al, al			# flag as INPUT so SCNVAL will work
	mov BYTE PTR FLGINP, al		# when scanning strings with ":"
	mov BYTE PTR [ebx], 44		# put a comma in front of BUF
	xchg ebx, edx			# save data pointer in edx
	pop ebx				# get the varlst pointer into ebx
	push ebx			# resave the varlst pointer
	push edx			# save a copy of the data pointer for pass 2
	push edx			# save the data pointer for pass 1
	dec ebx				# read the first variable name
VARLOP:
	mov al, 128			# don't allow subscripts -- return pointing to "("
	mov BYTE PTR SUBFLG, al
	call CHRGTR			# advance text pointer
	call PTRGTN			# scan name and return pointer in edx
	mov al, BYTE PTR [ebx]		# see if it ended on "("
	dec ebx				# rescan the terminator
	cmp al, '['			# allow brackets also
	jz SCNBKT
	cmp al, '('			# array or not?
	jnz ENDSCN			# if not, variable name is done
SCNBKT:
	inc ebx				# now scan the subscript expression
	mov ch, 0			# initialize the paren count
SCNOPN:
	inc ch				# up the count for every "("
SCNCON:
	call CHRGTR			# get the next character
	jnz Q_Q_L070
	jmp SNERR			# shouldn't end statement in expression
Q_Q_L070:
	cmp al, 34			# is there a quoted string constant
	jnz Q_Q_L071
	jmp SCNSTR			# go scan the entire constant (may contain parens)
Q_Q_L071:
	cmp al, '('			# another level of nesting?
	jz SCNOPN			# increment count and keep scanning
	cmp al, '['			# left bracket?
	jz SCNCON			# yes, ok
	cmp al, ']'			# right bracket?
	jz LEFPRN			# yes
	cmp al, ')'			# one less level of parens?
	jnz SCNCON			# no, keep scanning
LEFPRN:
	dec ch				# decrement paren count. out of subscript?
	jnz SCNCON			# if not at zero level, keep scanning
ENDSCN:
	call CHRGTR			# get terminating character
	jz OKVLST			# last variable in input list
	cmp al, 44			# otherwise it must be a comma
	jz Q_Q_L072
	jmp SNERR			# badly formed input -- syntax error
Q_Q_L072:
OKVLST:
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save the varlst pointer; get the data pointer into ebx
	mov al, BYTE PTR [ebx]		# data should always have a leading comma
	cmp al, 44			# is it properly formed?
	jz Q_Q_L073
	jmp INPBAK			# no, ask for complete reinput
Q_Q_L073:
	mov al, 1			# set OVCSTR=1
	mov BYTE PTR OVCSTR, al
	mov BYTE PTR FLGSCN, al		# flag to SCNVAL that call is from INPUT
	call SCNVAL			# go into pass 2 code and scan a value
	mov al, BYTE PTR FLGSCN
	dec al				# restore flag
	mov BYTE PTR FLGSCN, al
	mov al, BYTE PTR OVCSTR		# see if it was too big
	dec al
	jz Q_Q_L074
	jmp INPBAK
Q_Q_L074:
	push ebx			# save the returned data pointer
	call GETYPR			# release string
	jnz Q_Q_L075
	call FREFAC
Q_Q_L075:
	pop ebx
	dec ebx				# skip over spaces left after value scan
	call CHRGTR

# NOTE: check for overflow of input value here.
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save the data pointer; [ebx] = data list pointer
	mov al, BYTE PTR [ebx]		# did variable list continue?
	cmp al, 44			# must have had a comma
	jnz Q_Q_L076
	jmp VARLOP			# go check another
Q_Q_L076:
	pop ebx				# get final data pointer
	dec ebx				# skip over any trailing spaces
	call CHRGTR
	or al, al			# is it a true end?
	pop ebx				# get the start of data pointer for pass 2
	jz Q_Q_L077
	jmp RDOIN2			# if data ended badly ask for reinput
Q_Q_L077:
INPCN3:
	mov BYTE PTR [ebx], 44		# setup comma at BUFMIN
	jmp INPCON

# READ: the original's "DB 15O" skips the XOR AL,AL (as an OR AX,imm16) so AL stays nonzero.
READ:
	push ebx			# save the text pointer
	mov ebx, DWORD PTR DATPTR	# get last data location
	mov al, 1			# nonzero: this is a READ (replaces the DB 15O skip)
	jmp INPSTF
INPCON:
	xor al, al			# set flag that this is an input
INPSTF:
	mov BYTE PTR FLGINP, al		# store the flag

# In the processing of DATA and READ statements: one pointer points to the data (the numbers
# being fetched) and another points to the list of variables. The pointer into the data always
# starts pointing to a terminator: , : or end-of-line.
	pop esi				# XTHL (4-byte text pointer)
	xchg esi, ebx
	push esi			# [ebx]=variable list pointer; data pointer goes on the stack
	jmp LOPDAT
LOPDT2:
	call SYNCHR
	.byte 44			# make sure there is a ","
LOPDAT:
	call PTRGET			# read the variable list and get the pointer to a variable into edx
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# put the variable list pointer onto the stack; take the data pointer off

# NOTE: at this point we have a variable which wants data, so we must get data or complain.
	push edx			# save the pointer to the variable we are about to set
	mov al, BYTE PTR [ebx]		# the data pointer always points at a terminator
	cmp al, 44
	jz DATBK			# a comma, so a value must follow
	mov al, BYTE PTR FLGINP		# see what type of statement this was
	or al, al
	jz Q_Q_L078			# search for another data statement
	jmp DATLOP
Q_Q_L078:
DATBK:
	# The original "DB 15O" here is an OR AX,imm16 that swallows the XOR AL,AL of SCNVAL and
	# leaves AL nonzero; the net effect is AL |= 30h. Reproduced directly:
	or al, 0x30
	jmp SCNVAL_STORE

SCNVAL:
	xor al, al			# set zero flag in [A]
SCNVAL_STORE:
	mov BYTE PTR INPPAS, al		# store so early return check works
	call ISFLIO			# see if a file read
	jz Q_Q_L079
	jmp FILIND			# if so, special handling
Q_Q_L079:
	call GETYPR			# is it a string?
	push ax			# save the type information (value: 16-bit)
	jnz NUMINS			# if numeric, use FIN to get it
	# Only the variable type is checked, so an unquoted string can be all digits.
	call CHRGTR
	mov dh, al			# assume quoted string
	mov ch, al			# setup terminators
	cmp al, 34			# quote?
	jz NOWGET			# terminators ok
	mov al, BYTE PTR FLGINP		# input shouldn't terminate on ":"
	or al, al			# see if READ or INPUT
	mov dh, al			# set D to zero for INPUT
	jz NCOLST
	mov dh, ':'			# unquoted string terminators
NCOLST:
	mov ch, 44			# are colon and comma
	# NOTE: ANSI uses [B]=44 as a flag to trigger trailing space suppression.
	dec ebx				# backup since start character must be included in the unquoted case;
	# in the quoted case we don't want to include the starting or ending quote.
NOWGET:
	call STRLT2			# make a string descriptor for the value and copy if necessary

	.globl DOASIG
DOASIG:
	pop ax				# pop off the type information
	add al, 3			# make VALTYPE correct
	mov cl, al			# save value type in [C]
	mov al, BYTE PTR INPPAS		# see if scanning values for pass 1
	or al, al			# zero for pass 1
	jnz Q_DOASIG_OK
	ret				# go back to pass 1
Q_DOASIG_OK:
	mov al, cl			# recover VALTYP
	xchg ebx, edx			# [edx]=text pointer
	mov ebx, offset STRDN2		# return location
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# [ebx]=place to store variable value
	push edx			# text pointer goes on
	jmp INPCOM			# do assignment
NUMINS:
	call CHRGTR
	pop ax				# get back VALTYPE of source
	push ax			# save back
	cmp al, 5			# regen cc's
	mov ecx, offset DOASIG		# assignment is complicated even for numerics, so use the LET code
	push ecx			# save on stack
	jae Q_Q_L080
	jmp FIN				# if not double, call usual number inputter
Q_Q_L080:
	jmp FINDBL			# else call special routine which expects doubles

STRDN2:
	dec ebx
	call CHRGTR
	jz TRMOK
	cmp al, 44
	jz Q_Q_L081
	jmp TRMNOK			# ended properly?
Q_Q_L081:
TRMOK:
	pop esi				# XTHL
	xchg esi, ebx
	push esi
	dec ebx				# look at terminator
	call CHRGTR			# and set up condition codes
	jz Q_Q_L082
	jmp LOPDT2			# not ending, check for comma
Q_Q_L082:
	# and get another variable to fill with data
	pop edx				# pop off the pointer into data
	mov al, BYTE PTR FLGINP		# fetch the statement type flag
	or al, al			# if zero, INPUT statement
	xchg ebx, edx
	jz Q_Q_L083
	jmp RESFIN			# update DATPTR
Q_Q_L083:
	push edx			# save the text pointer
FINPRG:
	pop ebx				# get back the text pointer
	jmp FINPRT

# The search for DATA statements is made by using the execution code for DATA to skip over
# statements. The start word of each statement is compared with DATA. Each new line number
# is stored in DATLIN so that an error reading data gives the line number of the bad data.
DATLOP:
	# DATPTR starts at TXTTAB-1, which holds CR (NEWSTT takes it as an end of line). DATA's skip
	# stops at 0 or ":" only, so it would run on into the first line's link and line number. The
	# start sentinel is the end of an imaginary line before the program, so the search begins at
	# the first line as it would from a 0 terminator.
	cmp BYTE PTR [ebx], 13
	jnz DATSKP
	xor al, al
	jmp DATFND
DATSKP:
	call DATA
DATFND:
	# EBX is on the 0 that ends a line. A line record is [link 4][line 2][text ... 0], so the
	# next line's link is at EBX+1 (4 bytes; zero at the end of the program) and its number at
	# EBX+5. Then EBX+6 is put in front of its text, for NOWLIN's CHRGTR.
	or al, al
	jnz NOWLIN
	mov dl, ERROD			# no data is error ERROD
	cmp DWORD PTR [ebx+1], 0	# the link is 4 bytes (IFE XERPAG is no longer needed)
	jnz Q_Q_L084
	jmp ERROR			# if so complain
Q_Q_L084:
	mov dx, WORD PTR [ebx+5]	# get data line #
	mov DATLIN, dx			# save data line number
	add ebx, 6			# now the byte before the first text character
NOWLIN:
	call CHRGTR			# get the statement type
	cmp al, _DATA			# is it "DATA"?
	jnz DATLOP			# not DATA, so look some more
	jmp DATBK			# continue reading

# PEEK and POKE. The 8086 original addressed a 64 KB segment with the segment register; in
# this flat 32-bit port the 16-bit address is zero-extended and used as a flat address. The
# device filters PEKFLT and POKFLT (Stage 4) decide whether the access is performed.
# UNSURE: flat-address model for PEEK/POKE (see NOTES.md).
PEEK:
	call FRQINT			# get an integer in [H,L] (16-bit BX)
	call PRODIR			# don't allow direct if protected file
	call PEKFLT			# PEEK filter
	jnz Q_Q_L085
	jmp PEKVAL			# filter returned the appropriate value
Q_Q_L085:
	movzx ebx, bx			# unsigned 16-bit address
	mov al, BYTE PTR [ebx]		# fetch the PEEK value
PEKVAL:
	jmp SNGFLT			# and float it
POKE:
	call FRMQNT			# read integer formula into [D,E]
	push dx				# save value for later
	call PRODIR			# don't allow direct if protected file
	call SYNCHR
	.byte 44			# check for a comma
	call GETBYT
	pop dx				# get the address back
	call POKFLT			# filter POKEs for "special" addresses
	jnz Q_Q_L086
	jmp POKEX			# do not perform the POKE
Q_Q_L086:
	movzx edi, dx
	mov BYTE PTR [edi], al		# store it away
POKEX:
	ret				# scanned everything

	.globl FRMQNT
FRMQNT:
	call FRMEVL			# read formula
	push ebx			# save text pointer
	call FRQINT			# get value in range -32769..65535
	xchg bx, dx			# value to [D,E]
	pop ebx				# restore text pointer
	ret

# FRQINT: convert the FAC to an integer (value in BX on return). Returns through FRCINT.
	.globl FRQINT
FRQINT:
	mov ecx, offset FRCINT		# return here
	push ecx			# save address
	call GETYPR			# set the cc's on VALTYPE
	jns Q_FRQ_1
	ret				# return if already integer
Q_FRQ_1:
	mov al, BYTE PTR _FAC		# get exponent
	cmp al, 0220			# is magnitude .gt. 32767 (octal 220)
	jz Q_FRQ_2
	ret				# no, force integer
Q_FRQ_2:
	call SIGN			# get sign of number
	# is it negative? only allowable # is -32768
	jns Q_FRQ_3
	ret				# assume that's what it is, else give overflow
Q_FRQ_3:
	call FRCSNG			# make number a single
	# MOVRI 221O,200O,0,0: "LXI B" with 9180h and "LXI D" with 0, i.e. -65536 as single
	mov cx, 0x9180
	mov dx, 0x0000
	jmp FADD			# subtract it, and then force integer

# ---------------------------------------------------------------------------------------
# RENUMBER / RESEQ.
# BLOCKED (see the report, NOTES.md): the original converts each line-number reference to a
# 2-byte pointer in place (3-byte constant). A 4-byte pointer does not fit, and gwmain_b1.S's
# PTRCON is 5 bytes. Until that is decided, RESEQ reports "Illegal function call". The body
# below is the translation of the original logic, kept for the decision.
# ---------------------------------------------------------------------------------------
RESEQ:
	jmp FCERR			# BLOCKED (see above)

RESEQ_BODY:
	mov cx, 10			# assume INC=10
	push cx				# save on stack (value)
	mov dh, ch			# resequence all lines by setting [D,E]=0
	mov dl, ch
	jz RESNN			# if just 'RESEQ' resequence 10 by 10
	cmp al, 44			# comma
	jz EATCOM			# don't use starting # of zero
	push dx				# save [D,E]
	call LINSPC			# get new NN
	mov ch, dh			# get it in [B,C] where it belongs
	mov cl, dl
	pop dx				# get back [D,E]
	jz RESNN			# if EOS, done
EATCOM:
	call SYNCHR
	.byte 44			# expect comma
	call LINSPC			# get new MM
	jz RESNN			# if EOS, done
	pop ax				# get rid of old INC (value)
	call SYNCHR
	.byte 44			# expect comma
	push dx				# save MM (value)
	call LINGET			# get new INC
	jz Q_L087
	jmp SNERR			# should have terminated
Q_L087:
	or dx, dx			# see if INC=0 (illegal)
	jnz Q_L088
	jmp FCERR			# yes, blow him up now
Q_L088:
	xchg ebx, edx			# flip new INC & [H,L] (text pointer moves 32-bit)
	pop esi				# XTHL: MM (value)
	xchg si, bx			# (16-bit value swap)
	push esi				# new INC onto stack
	xchg ebx, edx			# get [H,L] back, original [D,E] back
RESNN:
	push cx				# save NN on stack (value)
	call FNDLIN			# find MM line
	pop dx				# get NN off stack
	push dx				# save NN back
	push ecx			# save pointer to MM line (4 bytes)
	call FNDLIN			# find first line to resequence
	mov ebx, ecx			# get ptr to this line in [H,L]
	pop edx				# get line ptr pointed to by MM
	cmp ebx, edx			# compare to first line resequenced
	xchg ebx, edx			# get ptr to MM line in [H,L]
	jae Q_L089
	jmp FCERR			# can't allow program to be resequenced on top of itself
Q_L089:
	pop dx				# get NN back
	pop cx				# get INC in [B,C]
	pop eax				# get rid of NEWSTT (4-byte return address)
	push ebx			# save ptr to first line to resequence
	push dx				# save NN on stack
	jmp NXTRSL
NXTRSC:
	add bx, cx			# add increment into the accumulator (value)
	jae Q_L090
	jmp FCERR			# uh oh, his inc was too large
Q_L090:
	xchg ebx, edx			# flip link field and accumulator
	push ebx			# save link field (pointer)
	mov bx, 65529			# test for too large line
	cmp bx, dx			# compare to current #
	pop ebx				# restore link field
	jae Q_L091
	jmp FCERR			# uh oh, his inc was too large
Q_L091:
NXTRSL:
	push dx				# save current line accumulator
	mov edx, DWORD PTR [ebx]	# get link field (4 bytes) into edx
	or edx, edx			# set cc's on link field
	xchg ebx, edx			# see if next link zero
	pop dx				# get back accumulator line #
	jz RESSD1			# zero, done
	cmp DWORD PTR [ebx], 0		# next record's link zero? (4-byte test)
	xchg ebx, edx			# back in edx (flags unchanged by xchg)
	jnz NXTRSC			# inc count
RESSD1:
	push cx				# save inc
	call SCCLIN			# scan program converting lines to ptrs
	pop cx				# get back inc
	pop dx				# get nn
	pop ebx				# get ptr to first line to resequence
RESNX1:
	push dx				# save current line
	mov edx, DWORD PTR [ebx]	# prepare for zero link field test
	or edx, edx			# end?
	jz SCCALL			# stop resequencing when see end of program
	xchg ebx, edx			# flip line ptr, link field
	pop esi				# XTHL: current line (value)
	xchg esi, ebx			# (full 32-bit swap; SI low word holds the value)
	push esi			# put link on stack, get new line # off
	xchg ebx, edx			# put new line # in [D,E], this line ptr in [H,L]
	add ebx, 4			# point to line # field (link is 4 bytes)
	mov WORD PTR [ebx], dx		# change to new line #
	xchg ebx, edx			# get this line # in [H,L]
	add bx, cx			# add inc
	xchg ebx, edx			# get new line # back in [D,E]
	pop ebx				# get ptr to next line
	jmp RESNX1			# keep resequencing
SCCALL:
	mov ecx, offset STPRDY		# where to go when done
	push ecx			# save on stack
	jmp SCCPTR			# (the original "DB 74O" skipped into SCCPTR)

# SCCLIN and SCCPTR convert line numbers to pointers and back. Blocked with RESEQ (see above).
# The original "DB 15O" at SCCLIN is an OR AX,imm16 that makes AL nonzero (AL |= 30h).
SCCLIN:
	or al, 0x30
	jmp SCCPTR_STORE
	.globl SCCPTR
SCCPTR:
	xor al, al			# set a=0
SCCPTR_STORE:
	mov BYTE PTR PTRFLG, al		# set to say whether lines or ptrs extant
SCNPGM:
	mov ebx, DWORD PTR TXTTAB	# get ptr to start of program
	dec ebx				# nop next inc
SCNPLN:
	inc ebx				# point to byte after zero at end of line (link field)
	cmp DWORD PTR [ebx], 0		# link field zero: end of program
	jnz Q_SCNPLN_1
	ret				# return if all done
Q_SCNPLN_1:
	add ebx, 4			# point to line # field (link is 4 bytes)
	mov dx, WORD PTR [ebx]		# get line #
	inc ebx				# point to high byte of line #
SCNEXT:
	call CHRGTR			# get next char from line
SCNEX2:
	or al, al			# end of line
	jz SCNPLN			# scan next line
	mov cl, al			# save [A]
	mov al, BYTE PTR PTRFLG		# change line tokens which way?
	or al, al			# set cc's
	mov al, cl			# get back current char
	jz SCNPT2			# changing pointers to #'s
	cmp al, _ERROR			# is it error token?
	jnz NTERRG			# no.
	call CHRGTR			# scan next char
	cmp al, _GOTO			# error goto?
	jnz SCNEX2			# get next one
	call CHRGTR			# get next char
	cmp al, LINCON			# line # constant?
	jnz SCNEX2			# no, ignore
	push dx				# save [D,E]
	call LINGT3			# get it
	or dx, dx			# is it line # zero?
	jnz CHGPTR			# change it to a pointer
	jmp SCNEX3			# yes, don't change it
NTERRG:
	cmp al, LINCON			# line # constant?
	jnz SCNEXT			# not, keep scanning
	push dx				# save current line # for possible error msg
	call LINGT3			# get line # of line constant into [D,E]
CHGPTR:
	push ebx			# save text pointer just at end of LINCON
	call FNDLIN			# try to find line in pgm
	lahf
	dec ecx				# point to zero at end of previous line
	sahf
	mov al, PTRCON			# change line # to ptr
	jb MAKPTR			# if line found change # to ptr
	call CRDONZ			# print CRLF if required
	mov ebx, offset LINM		# print "undefined line" message
	push dx				# save line #
	call STROUT			# print it
	pop ebx				# get line # in [H,L]
	call LINPRT			# print it
	pop ecx				# get text ptr off stack
	pop ebx				# get current line #
	push ebx				# save back
	push ecx			# save back text ptr
	call INPRT			# print it
SCNPOP:
	pop ebx				# pop off current text pointer
SCNEX3:
	pop dx				# get back current line #
	dec ebx				# backup pointer
JSCNXT:
	jmp SCNEXT			# keep scanning
LINM:
	.ascii "Undefined line "
	.byte 0
SCNPT2:
	cmp al, PTRCON			# pointer
	jnz JSCNXT			# no, keep scanning
	push dx				# save current line #
	call LINGT3			# get #
	push ebx			# save text pointer
	xchg ebx, edx			# flip current text ptr & ptr
	add ebx, 5			# point to line # field (pointer+5 in the 4-byte layout, see gwmain_b1.S)
	mov cl, BYTE PTR [ebx]		# pick up line # (low)
	inc ebx				# point to high part
	mov ch, BYTE PTR [ebx]
	mov al, LINCON			# change to line constant
MAKPTR:
	mov ebx, offset SCNPOP		# place to return to after changing constant
	push ebx			# save on stack
CONCHG:
	mov ebx, DWORD PTR CONTXT	# get txt ptr after constant
	.globl CONCH2
CONCH2:
	push ebx			# save ptr to end of constant
	dec ebx
	mov BYTE PTR [ebx], ch
	dec ebx
	mov BYTE PTR [ebx], cl		# change to value in [B,C]
	dec ebx				# point to constant token
	mov BYTE PTR [ebx], al		# change to value in [A]
	pop ebx				# restore pointer to after constant
	ret
# UNSURE/BLOCKED: CONCH2 writes 2 value bytes after the token, which is right for LINCON. For PTRCON the
# 4-byte pointer does not fit (see above). Unreachable while RESEQ is blocked.

	.globl DEPTR
DEPTR:
	mov al, BYTE PTR PTRFLG		# do line pointers exist in pgm?
	or al, al			# set cc's
	jnz Q_DEPTR_1
	ret				# no, just return
Q_DEPTR_1:
	jmp SCCPTR			# convert them to line #'s

# ---------------------------------------------------------------------------------------
# OPTION BASE (ANSI)
# ---------------------------------------------------------------------------------------
OPTION:
	call SYNCHR
	.byte 'B'
	call SYNCHR
	.byte 'A'
	call SYNCHR
	.byte 'S'
	call SYNCHR
	.byte 'E'
	mov al, BYTE PTR [ebx]		# get the base number
	sub al, '0'
	jae Q_OPT_1
	jmp SNERR
Q_OPT_1:
	cmp al, 2			# only 0 and 1 are legal
	jb Q_OPT_2
	jmp SNERR
Q_OPT_2:
	mov ch, al
	mov al, BYTE PTR OPTVAL
	cmp al, ch
	jz OLDOPT			# old option base value
	mov al, BYTE PTR OPTFLG
	or al, al			# have we seen option base before
	jz Q_OPT_3
	jmp DDERR			# if so "double dimension error"
Q_OPT_3:
	push ebx			# save the text pointer
	mov ebx, DWORD PTR ARYTAB	# see if we have any arrays yet
	xchg ebx, edx
	mov ebx, DWORD PTR STREND
	cmp ebx, edx			# if these are equal we have not
	jz Q_OPT_4
	jmp DDERR
Q_OPT_4:
	pop ebx
	mov al, ch
	mov BYTE PTR OPTVAL, al		# save if for DIM and PTRGET
	inc al				# make sure [A] is nonzero
	mov BYTE PTR OPTFLG, al		# flag that we have seen "OPTION BASE"
OLDOPT:
	call CHRGTR			# fetch the terminator
	ret

# Message printing for the math package (PUBLIC $STPRN, and its loop)
	.globl _STPRN
_STPRN:
STRPRN:
	# (the original's INS86 56 fetched from the code segment: dropped in the flat model)
STRPN1:
	mov al, BYTE PTR [ebx]		# get byte from message
	or al, al			# end of message
	jnz Q_STRPN_1
	ret				# yes, done
Q_STRPN_1:
	call CALTTY			# print char
	inc ebx				# increment pointer
	jmp STRPN1			# print next char

# RANDOM
RANDOM:
	jz INPRG			# if no argument ask from terminal
	call FRMEVL			# fetch the formula argument
	push ebx
	call FRCINT			# allow normal integers
	jmp STRNDS			# store the new random seed
INPRG:
	push ebx
INPRAG:
	mov ebx, offset RANMES		# ask for some random input
	call STROUT
	call QINLIN
	pop edx				# get back text pointer
	jae Q_RND_1
	jmp STPEND			# go away if control c
Q_RND_1:
	push edx			# resave text pointer
	inc ebx				# move past BUFMIN to BUF
	mov al, BYTE PTR [ebx]		# get first char of typein (FIN expects it)
	call FIN			# read a number
	mov al, BYTE PTR [ebx]		# get the terminator
	or al, al
	jnz INPRAG			# don't allow bad format
	call FRCINT			# allow normal integers
STRNDS:
	mov WORD PTR RNDX+1, bx
	call RNDMN2
	pop ebx				# get back the text pointer
	ret
RANMES:
	.ascii "Random number seed (-32768 to 32767)"
	.byte 0

# ---------------------------------------------------------------------------------------
# Scanning for the matching WEND/NEXT (WNDSCN, NXTSCN). Works on text, so the text
# pointer is 4 bytes.
# ---------------------------------------------------------------------------------------
	.globl WNDSCN, NXTSCN
WNDSCN:
	mov cl, ERRWH			# scan for matching WEND. this is error if fail
	jmp SCNCNT
NXTSCN:
	mov cl, ERRFN
SCNCNT:
	mov ch, 0			# set up the count of "FOR"s seen
	xchg ebx, edx			# initialize NXTLIN for NEXT on same line
	mov bx, WORD PTR CURLIN
	mov WORD PTR NXTLIN, bx
	xchg ebx, edx			# restore the text pointer to ebx
FORINC:
	inc ch				# increment the count whenever "FOR" is seen
FNLOP:
	dec ebx				# ** FIX HERE FOR 5.03 CAN'T CALL DATA
SCANWF:
	call CHRGTR			# to skip to statement because could
	jz FORTRM			# have statement after "THEN"
	cmp al, 34			# check for quote which may have
	jnz NTQTSC			# eight bit literal data same as $THEN
QTLOPF:
	call CHRGTR			# in it. scan through string.
	or al, al
	jz FORTRM			# terminate only on zero not colon
	cmp al, 34
	jnz QTLOPF
NTQTSC:
	cmp al, _ELSE			# else statement
	jz FNNWST			# then allow NEXT or WEND after it
	cmp al, _THEN			# so scan using CHRGET waiting for end
	jnz SCANWF			# of statement or $THEN
FORTRM:
	or al, al			# see how it ended
	jnz FNNWST			# just new statement -- examine it
	inc ebx				# ebx -> the 4-byte link at the start of the next line
	mov dl, cl			# set up error number (mov keeps the flags)
	cmp DWORD PTR [ebx], 0		# zero link: end of program
	jnz Q_FORTRM_1
	jmp ERROR
Q_FORTRM_1:
	add ebx, 4			# -> the line number
	mov dx, WORD PTR [ebx]		# pick up the new line number
	add ebx, 1			# -> the byte before the text (CHRGTR steps onto the first character)
	mov WORD PTR NXTLIN, dx		# save as "NEXT" line number
FNNWST:
	call CHRGTR			# get the type of the next statement
	cmp al, _REM			# REM statements since they have colons
	jnz NTREMR			# in the middle which may be viewed as
	push cx
	call REM			# statement terminators and the next char
	pop cx
	jmp FORTRM			# might be an eight bit literal that
NTREMR:
	cmp al, _DATA			# accidentally matches the search
	jnz NTDATR			# so we call REM and DATA to do the skipping
	push cx
	call DATA
	pop cx
	jmp FORTRM
NTDATR:
	mov al, cl			# get the error number to see what we are
	cmp al, ERRFN			# scanning for
	mov al, BYTE PTR [ebx]		# get back the character
	jz NXTLOK			# for/next searching
	cmp al, _WHILE			# another WHILE/WEND nest?
	jz FORINC
	cmp al, _WEND
	jnz FNLOP
	dec ch
	jnz FNLOP
	ret
NXTLOK:
	cmp al, _FOR			# another "FOR"?
	jz FORINC			# increment the FOR count
	cmp al, _NEXT			# end with NEXT?
	jnz FNLOP			# skip over this statement
DECNXT:
	dec ch				# decrement the loop count
	jnz Q_DECNXT_1
	ret				# return with [H,L] about to get first character of "NEXT" variable
Q_DECNXT_1:
# Scan the variables listed in a "NEXT" statement.
	call CHRGTR			# see if there is a name
	jz FORTRM			# only one, so scan more statements
	xchg ebx, edx			# save text pointer in [D,E]
	mov bx, WORD PTR CURLIN		# save the current line number
	push ebx				# (value)
	mov bx, WORD PTR NXTLIN		# make error come from "NEXT"
	mov WORD PTR CURLIN, bx
	xchg ebx, edx			# [H,L]= text pointer
	push cx				# save the "FOR" count
	call PTRGET			# skip over the variable name
	pop cx				# get back the "FOR" count
	dec ebx				# check terminator
	call CHRGTR
	mov edx, offset FORTRM		# place to go to
	jz TRMNXT			# end of "NEXT"
	call SYNCHR
	.byte 44			# should have commas in between
	dec ebx				# rescan first character
	mov edx, offset DECNXT		# place to go back to
TRMNXT:
	pop esi				# XTHL: saved CURLIN is a 2-byte value; text pointer goes on the stack (4 bytes)
	xchg esi, ebx
	push esi			# save the text pointer on the stack
	mov WORD PTR CURLIN, bx
	pop ebx
	push edx			# go off to address in edx
	ret

# ---------------------------------------------------------------------------------------
# FINOVC / CLROVC: store the overflow flag, then clear it (the original falls through).
# ---------------------------------------------------------------------------------------
	.globl FINOVC, CLROVC, _FLGOC, _CLROV
	.globl READ, RANDOM, OPTION, SCNEXT, SCCLIN, STRPRN, _STPRN, DATBK
_FLGOC:
FINOVC:
	lahf
	push ax
	mov al, BYTE PTR FLGOVC		# store overflow flag to indicate
	mov BYTE PTR OVCSTR, al		# whether an overflow occurred
	pop ax
	sahf
_CLROV:
CLROVC:
	lahf
	push ax				# save everything
	xor al, al			# normal overflow mode
	mov BYTE PTR FLGOVC, al
	pop ax
	sahf
	ret
