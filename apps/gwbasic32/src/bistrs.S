# BISTRS.ASM -> bistrs.S
# BASIC string routines: comparison, string functions (STR$, STRING$, SPACE$, CHR$, LEFT$, RIGHT$,
# MID$, LEN, ASC, VAL, INSTR, FRE), concatenation, temporaries and garbage collection.
#
# Widths: a string descriptor is STRSIZ = 5 bytes (length byte, then the 4-byte data address at +1).
# Pointers are 4 bytes on the stack and in registers; the counts and characters stay 8/16-bit.
# DSCTMP is the answer descriptor; DSCPTR is its address field (DSCTMP+1, 4 bytes).
# TEMPST holds NUMTMP temporaries of STRSIZ bytes; the "free temp" limit is DSCTMP+STRSIZ.
#
# Translation notes:
#  - STRSIZ is defined here as 5. oem.inc still says 3 (stale); this file does not include oem.inc.
#  - "DB 260O" (MVI AL, skipping a PUSH) and "DB 272O" (LXI D, skipping a MOV) are written as
#    explicit jumps. In the LEFT$ fall-through the skipped "MOV CL,0" is not executed (CL is kept).
#  - "JNZ SHORT $+3 / RET" and similar pairs are written as conditional jumps around the RET.
#  - "??Lnnn" labels are written as Q_Q_Lnnn.
#  - LAHF/SAHF pairs around INC BX are replaced by LEA, which does not change flags.
#  - PUSHF/POPF are pushfd/popfd. DF is assumed clear (no STOSB/MOVSB here; loops use MOV).

	.intel_syntax noprefix

	.set STRSIZ, 5			# string descriptor: length(1) + address(4)
	.set ERRSO, 14			# out of string space (GWDATA.ASM ERRSO)
	.set ERRLS, 15			# string too long (GWDATA.ASM ERRLS)
	.set ERRST, 16			# string temporary (GWDATA.ASM ERRST)

	.section .text
	.globl STRCMP, STRO_, STRH_, STR_, STRCPY, STRIN1, STRINI, STRAD2, STRAD1, PUTDEI
	.globl STRLIT, STRLTI, STRLT3, STRLT2, PUTNEW, PUTTMP, STROUI, DSOUT, STRPRT
	.globl GETSPA, PPSWRT, GARBA2, CAT, FREFAC, FRESTR, FRETM2, FRETMP, FRETMS
	.globl LEN, LEN1, ASC, ASC2, CHR_, SETSTR, FINBCK, STRNG_, SPACE_, SPACE2
	.globl LEFT_, LEFTUS, RIGHT_, MID_, VAL, PREAM, PREAM2, INSTR, LHSMID, FRE

# -------------------------------------------------------------------------------------
# STRCMP: compare the string at [DE] with the FAC string (FACLO).
# A=0 if equal, A=255 if the stack string is greater, A=1 if less.
# -------------------------------------------------------------------------------------
STRCMP:
	call FRESTR			# free the FAC string; BX = FAC descriptor
	mov al, BYTE PTR [ebx]		# length of the FAC string
	mov ecx, DWORD PTR [ebx+1]	# pointer at the FAC string data
	pop edx				# the stack string descriptor pointer
	push ecx			# save the pointer at the FAC string data
	push ax				# save the FAC string length
	call FRETMP			# free the stack string; BX = its descriptor
	pop ax				# length of the FAC string
	mov dh, al			# [D] = length of FAC string
	mov dl, BYTE PTR [ebx]		# [E] = length of stack string
	mov ecx, DWORD PTR [ebx+1]	# [C] = pointer at stack string data
	pop ebx				# 2nd character pointer (FAC string data)
CSLOOP:
	mov al, dl
	or al, dh			# both strings ended?
	jnz Q_CS1
	ret				# yes: return with A=0
Q_CS1:
	mov al, dh
	sub al, 1			# carry set, A=255 if [D]=0
	jnb Q_CS2
	ret				# FAC string ended: return
Q_CS2:
	xor al, al
	cmp al, dl			# stack string ended first?
	inc al				# return with A=1
	jnae Q_CS3
	ret
Q_CS3:
	dec dh				# decrement both character counts
	dec dl
	mov esi, ecx
	mov al, BYTE PTR [esi]		# character from the stack string
	inc ecx
	cmp al, BYTE PTR [ebx]		# compare with the FAC string
	lahf
	inc ebx				# bump the FAC string pointer (INX does not touch flags)
	sahf
	jz CSLOOP			# same: more to compare
	cmc				# differ: set A from the carry
	jmp SIGNS

# -------------------------------------------------------------------------------------
# String functions STRO$, STRH$, STR$: number to string.
# -------------------------------------------------------------------------------------
	.globl FOUTO, FOUTH, FOUT
STRO_:
	call FOUTO			# octal number into FBUFFR
	jmp STR1
STRH_:
	call FOUTH			# hex number into FBUFFR
	jmp STR1
STR_:
	call FOUT			# do the number's output
STR1:
	call STRLIT			# scan it and turn it into a string
	call FREFAC			# free up the temp
	mov ecx, offset FINBCK
	push ecx			# set up the answer in a new temp

# -------------------------------------------------------------------------------------
# STRCPY: copy the string whose descriptor is at [BX] into new string space.
# On return [D,E] points at DSCTMP, which holds the length and the new data address.
# -------------------------------------------------------------------------------------
STRCPY:
	mov al, BYTE PTR [ebx]		# length
	inc ebx				# move up to the pointer
	push ebx			# pointer to the pointer of the argument
	call GETSPA			# get the space
	pop ebx				# find out where the string to copy is
	mov ecx, DWORD PTR [ebx]	# its data pointer (4 bytes)
	call STRAD2			# set up DSCTMP
	push ebx			# save pointer to DSCTMP
	mov bl, al			# character count into [L]
	call MOVSTR			# move the characters in
	pop edx				# restore pointer to DSCTMP
	ret

STRIN1:
	mov al, 1			# one character string (CHR$, INKEY$)
STRINI:
	call GETSPA			# get string space ([A] characters)
STRAD2:
	mov ebx, offset DSCTMP		# the descriptor temp
STRAD1:
	push ebx			# save descriptor pointer
	mov BYTE PTR [ebx], al		# save the character count
PUTDEI:
	inc ebx				# store [D,E] = pointer to the free space (DSCPTR)
	mov DWORD PTR [ebx], edx
	pop ebx				# restore [H,L] as the descriptor pointer
	ret

# -------------------------------------------------------------------------------------
# STRLIT: build a descriptor for the string literal starting at [BX]+1. The result is in
# DSCTMP and PUTNEW moves it into a temporary. A closing quote is skipped.
# -------------------------------------------------------------------------------------
STRLIT:
	dec ebx
STRLTI:
	mov ch, 34			# assume the string ends on a quote
STRLT3:
	mov dh, ch
STRLT2:
	push ebx			# save pointer to the start of the literal
	mov cl, 255			# initialize the character count
STRGET:
	inc ebx
	mov al, BYTE PTR [ebx]		# get char
	inc cl				# bump the character count
	or al, al			# if 0 (end of line), done
	jz STRFIN
	cmp al, dh
	jz STRFIN
	cmp al, ch			# closing quote
	jnz STRGET			# no: go back for more
STRFIN:
	cmp al, 34			# if a quote terminates the string
	jnz Q_Q_L000
	call CHRGTR			# skip over the quote
Q_Q_L000:
	push ebx			# save pointer at end of string
	mov al, ch			# were we scanning an unquoted string?
	cmp al, 44
	jnz NTTRLS			# if not, don't suppress trailing spaces
	inc cl				# fix [C], the character count
LPTRLS:
	dec cl				# decrement until a non-space character
	jz NTTRLS			# don't go past the start (all spaces)
	dec ebx				# look at the previous character
	mov al, BYTE PTR [ebx]
	cmp al, ' '
	jz LPTRLS			# if so, continue looking
NTTRLS:
	pop ebx
	pop esi				# XTHL
	xchg esi, ebx
	push esi
	inc ebx
	xchg ebx, edx			# get pointer to temp
	mov al, cl			# character count in A
	call STRAD2			# save string info

# -------------------------------------------------------------------------------------
# PUTNEW / PUTTMP: make the result in DSCTMP into a temporary and point FACLO at it.
# PUTNEW starts from DSCTMP; PUTTMP starts from the descriptor pointer in [D,E].
# -------------------------------------------------------------------------------------
PUTNEW:
	mov edx, offset DSCTMP		# [D,E] point at the result descriptor
	jmp PUTTMP_NP			# (original: DB 260O skipped the PUSH DX below)
PUTTMP:
	push edx			# save a pointer to the start of the string
PUTTMP_NP:
	mov ebx, DWORD PTR TEMPPT	# [H,L] = pointer to first free temp
	mov DWORD PTR FACLO, ebx	# pointer at where the result descriptor will be
	mov al, 3
	mov BYTE PTR VALTYP, al		# flag this as a string
	call VMOVE			# move the value into a temporary
	mov edx, offset DSCTMP+STRSIZ	# end of the temp area (DSCTMP+3 in the 3-byte original)
	cmp ebx, edx			# if TEMPPT points there, there are no free temps
	mov DWORD PTR TEMPPT, ebx	# save new temporary pointer
	pop ebx				# get the text pointer
	mov al, BYTE PTR [ebx]		# current character into [A]
	jz Q_PT1
	ret
Q_PT1:
	mov dx, ERRST			# "STRING TEMPORARY" error
	jmp ERROR

# -------------------------------------------------------------------------------------
# STROUI / DSOUT / STRPRT: print a string. The string at [BX] (with a zero end) is
# printed from the data segment (DSOUT); a descriptor in FACLO is printed by STRPRT.
# -------------------------------------------------------------------------------------
STROUI:
	inc ebx				# point at next character
DSOUT:
	call STRLIT			# get a string literal
STRPRT:
	call FREFAC			# return the temp pointer in FACLO
	call GETBCD			# [D] = length, [B,C] = pointer at data
	inc dh				# increment and decrement early to test for null
STRPR2:
	dec dh				# decrement the length
	jnz Q_PR1
	ret				# all done
Q_PR1:
	mov esi, ecx
	mov al, BYTE PTR [esi]		# character to print
	call OUTDO
	cmp al, 13
	jnz Q_Q_L001
	call CRFIN
Q_Q_L001:
	inc ecx				# point to the next character
	jmp STRPR2

# -------------------------------------------------------------------------------------
# GETSPA: get space for a string of [A] characters (may force garbage collection).
# Input [A] = count. Output [D,E] = pointer to the space. Out of space: "String space" error.
# -------------------------------------------------------------------------------------
GETSPA:
	or al, al			# must be non zero; signal no garbage collection yet
	jmp TRYGI3
TRYGI2:
	pop ax
	sahf				# in case collected: what was the length?
TRYGI3:
	lahf
	push ax				# save it back
	mov ebx, DWORD PTR STREND
	xchg ebx, edx			# in [D,E]
	mov ebx, DWORD PTR FRETOP	# top of free space in [H,L]
	not al				# -# of chars
	mov cl, al			# in [B,C]
	mov ch, 255
	movsx ecx, cx			# -(count+1) as a signed 32-bit offset
	add ebx, ecx			# subtract from the top of free
	inc ebx
	cmp ebx, edx			# compare the two
	jb GARBAG			# not enough room: out of string space, go collect
	mov DWORD PTR FRETOP, ebx	# save new bottom of memory
	inc ebx				# move back to point to string
	xchg ebx, edx			# return with pointer in [D,E]
	pop ax
	sahf
	ret

PPSWRT:
	pop ax				# pop PSW
	xchg al, ah
	sahf				# get character count
	ret				# return from GETSPA

GARBAG:
	pop ax
	sahf				# have we collected before?
	mov dx, ERRSO			# out of string space error
	jnz Q_Q_L002
	jmp ERROR			# tell the user he lost
Q_Q_L002:
	cmp al, al			# set zero flag to say we've garbaged
	lahf
	push ax				# save flag back on the stack
	mov ecx, offset TRYGI2		# place for garbage collection to return to
	push ecx

# -------------------------------------------------------------------------------------
# GARBA2: garbage collection. Walks the temporaries, simple variables, arrays and parameter
# blocks, moving live strings to the top of string space.
# Stack during the walk: [MAX seen (4)] [variable pointer (4)] [return to TRYGI2 (4)] [flags (2)].
# -------------------------------------------------------------------------------------
GARBA2:
	mov ebx, DWORD PTR MEMSIZ	# start from the top down
FNDVAR:
	mov DWORD PTR FRETOP, ebx	# like so
	xor ebx, ebx			# double zero pointer
	push ebx			# say we didn't see vars this pass (variable pointer)
	mov ebx, DWORD PTR STREND	# force the variables to ignore strings in the program text
	push ebx			# force find high address (MAX seen)
	mov ebx, offset TEMPST		# start of string temps
TVAR:
	mov edx, DWORD PTR TEMPPT	# see if done
	cmp ebx, edx			# test
	mov ecx, offset TVAR		# force a jump to TVAR
	jz Q_Q_L003
	jmp DVAR2			# do a temp variable's garbage collect
Q_Q_L003:
	mov ebx, offset PRMPRV		# setup iteration for parameter blocks
	mov DWORD PTR TEMP9, ebx
	mov ebx, DWORD PTR ARYTAB	# stopping point in [H,L]
	mov DWORD PTR ARYTA2, ebx	# store in stop location
	mov ebx, DWORD PTR VARTAB	# starting point in [H,L]
SVAR:
	mov edx, DWORD PTR ARYTA2	# stopping location
	cmp ebx, edx			# see if at the end of the simple variables
	jz ARYVAR
	mov al, BYTE PTR [ebx]		# VALTYP
	add ebx, 3			# point at the value (type byte + 2 name bytes)
	push ax				# save VALTYP
	call IADAHL			# skip extra characters and count
	pop ax
	cmp al, 3			# is it a string?
	jnz SKPVAR			# if not, just skip it
	call DVARS			# collect it
	xor al, al			# and don't skip anything more
SKPVAR:
	mov dl, al
	movzx edx, dl			# [D,E] = amount to skip
	add ebx, edx
	jmp SVAR			# get the next one

# Parameter blocks: link (4 bytes) at the block start, then a 2-byte length, then data.
ARYVAR:
	mov ebx, DWORD PTR TEMP9	# get link in the parameter block chain
	mov edx, DWORD PTR [ebx]	# go back one level
	or edx, edx			# was that the end?
	mov ebx, DWORD PTR ARYTAB	# setup to start arrays
	jz ARYVA4			# otherwise garbage collect arrays
	xchg ebx, edx			# BX = the block; DX = ARYTAB (not needed)
	mov DWORD PTR TEMP9, ebx	# setup the next link in the chain for iteration
	add ebx, 4			# skip the chain pointer (UNSURE: block header widened to 4 bytes)
	movzx edx, WORD PTR [ebx]	# pick up the length
	add ebx, 2
	mov ecx, ebx
	add ecx, edx			# [C] = actual end address
	mov DWORD PTR ARYTA2, ecx	# set up stop location
	jmp SVAR

ARYVA2:
	pop ecx				# get rid of stack garbage (dims pointer)
ARYVA4:
	mov edx, DWORD PTR STREND	# end of arrays
	cmp ebx, edx			# done with arrays?
	jnz Q_Q_L004
	jmp GRBPAS			# yes: see if done collecting
Q_Q_L004:
	mov al, BYTE PTR [ebx]		# value type into [A]
	inc ebx
	push ax				# save the VALTYP
	add ebx, 2			# skip the name characters
	call IADAHL			# skip the extra characters
	movzx ecx, WORD PTR [ebx]	# pick up the length
	add ebx, 2
	pop ax				# restore the VALTYP
	push ebx			# save pointer to dims
	add ebx, ecx			# add to current pointer position
	cmp al, 3			# is it a string?
	jnz ARYVA2			# if not, just skip it
	mov DWORD PTR TEMP8, ebx	# save end of array
	pop ebx				# back to the current position (dims)
	movzx ecx, BYTE PTR [ebx]	# number of dims
	add ebx, ecx			# past the dims
	add ebx, ecx			# twice #dims (2-byte entries)
	inc ebx				# one more to account for #dims
ARYSTR:
	xchg ebx, edx			# save current position in [D,E]
	mov ebx, DWORD PTR TEMP8	# end of array
	xchg ebx, edx			# fix [H,L] back to current
	cmp ebx, edx			# at the end of the array?
	jz ARYVA4			# yes: try the next array
	mov ecx, offset ARYSTR		# where to return to
DVAR2:
	push ecx			# goes on the stack
DVAR:
DVARS:
	xor al, al
	or al, BYTE PTR [ebx]		# is it the null string?
	lea ebx, [ebx+1]		# INC BX (LEA keeps the flags)
	mov edx, DWORD PTR [ebx]	# [D,E] = pointer at the value (4 bytes)
	lea ebx, [ebx+4]		# past the descriptor (1 + 4 bytes)
	jnz Q_DV1
	ret				# null string: return
Q_DV1:
	mov ecx, ebx			# move [H,L] to [B,C]
	mov ebx, DWORD PTR FRETOP	# top of string free space
	cmp ebx, edx			# is this string's pointer below FRETOP?
	mov ebx, ecx			# move [B,C] back to [H,L]
	jnb Q_DV2
	ret				# not in string space: no need to go further
Q_DV2:
	pop ebx				# get the return address off the stack
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# get the max seen so far and save the return address
	cmp ebx, edx			# see
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save max seen; get return address off the stack
	push ebx			# save return address back
	mov ebx, ecx			# move [B,C] back to [H,L]
	jnae Q_DV3
	ret				# not the max: look at the next variable
Q_DV3:
	pop ecx				# get return address off the stack
	pop eax				# pop off max seen
	pop eax				# and variable pointer
	push ebx			# save new variable pointer
	push edx			# and new max pointer
	push ecx			# save return address back
	ret

# -------------------------------------------------------------------------------------
# GRBPAS: one complete pass through the string variables. Moves the string with the highest
# address, then tries again (FNDVAR). Stack: [max (4)] [variable pointer (4)].
# -------------------------------------------------------------------------------------
GRBPAS:
	pop edx				# pop off max pointer
	pop ebx				# and get the variable pointer
	or ebx, ebx			# zero pointer?
	jnz Q_GB1
	ret				# end of collection: return to GETSPA
Q_GB1:
	sub ebx, 4			# BX = address field of the descriptor (STRSIZ-1 back)
	mov ecx, DWORD PTR [ebx]	# [C] = pointer at string data
	push ebx			# save this location so the pointer can be updated after the move
	dec ebx
	movzx ebx, BYTE PTR [ebx]	# [BX] = string length
	add ebx, ecx			# [H,L] = pointer beyond string
	mov edx, ecx			# [D,E] = original pointer
	dec ebx				# don't move one beyond the string
	mov ecx, ebx			# top of string in [B,C]
	mov ebx, DWORD PTR FRETOP	# top of free space
	call BLTUC			# move the string
	pop ebx				# back to the descriptor
	mov DWORD PTR [ebx], ecx	# save fixed address
	mov ebx, ecx			# [H,L] = new pointer
	dec ebx				# fix up FRETOP
	jmp FNDVAR			# and try to find the high address again

# -------------------------------------------------------------------------------------
# CAT: concatenate. FACLO holds the first string; [H,L] points past the + sign.
# -------------------------------------------------------------------------------------
CAT:
	push cx				# put the old precedence back on
	push ebx			# save the text pointer
	mov ebx, DWORD PTR FACLO	# pointer to the string descriptor
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save on stack, get the text pointer back
	call EVAL			# evaluate the rest of the formula
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save text pointer, get back the descriptor
	call CHKSTR
	mov al, BYTE PTR [ebx]
	push ebx			# save the descriptor pointer
	mov ebx, DWORD PTR FACLO	# pointer to the 2nd descriptor
	push ebx			# save it
	add al, BYTE PTR [ebx]		# add the two lengths together
	mov dx, ERRLS			# see if result is under 256
	jae Q_Q_L005
	jmp ERROR			# error "long string"
Q_Q_L005:
	call STRINI			# get the initial string
	pop edx				# get the 2nd descriptor
	call FRETMP
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save pointer to it
	call FRETM2			# free up the 1st temp
	push ebx			# save descriptor pointer (first)
	mov ebx, DWORD PTR DSCPTR	# pointer to the first string's data (DSCTMP+1)
	xchg ebx, edx			# in [D,E]
	call MOVINS			# move in the first string
	call MOVINS			# and the second
	mov ebx, offset TSTOP		# CAT reenters formula evaluation at TSTOP
	pop esi				# XTHL
	xchg esi, ebx
	push esi
	push ebx			# text pointer off first
	jmp PUTNEW			# then return address of TSTOP

MOVINS:
	pop ebx				# get return address
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# put back, but get the descriptor
	mov al, BYTE PTR [ebx]		# [A] = string length
	mov ecx, DWORD PTR [ebx+1]	# [C] = pointer at string data
	mov bl, al			# [L] = string length
MOVSTR:
	inc bl
MOVLP:
	dec bl				# set condition codes
	jnz Q_MS1
	ret				# 0: no byte to move
Q_MS1:
	mov esi, ecx
	mov al, BYTE PTR [esi]		# get char
	mov BYTE PTR [edx], al		# save it
	inc ecx				# move pointers
	inc edx
	jmp MOVLP			# keep doing it

# -------------------------------------------------------------------------------------
# FREFAC / FRETM2 / FRETMP / FRETMS: release a string temporary.
# A temp is freed when it is the last one allocated by PUTNEW (TEMPPT moves back).
# If its data is the lowest string space in use, FRETOP moves up over it.
# -------------------------------------------------------------------------------------
FRESTR:
	call CHKSTR			# make sure it's a string
FREFAC:
	mov ebx, DWORD PTR FACLO
FRETM2:
	xchg ebx, edx			# free up the temp in the FACLO
FRETMP:
	call FRETMS			# free up the temporary
	xchg ebx, edx			# put the string pointer into [H,L]
	jz Q_FT1			# freed: check the string space
	ret
Q_FT1:
	push edx			# save [D,E] to return in [H,L]
	mov edx, ecx			# [D,E] = pointer at string
	dec edx				# subtract one
	movzx ecx, BYTE PTR [ebx]	# [C] = length of the string freed up
	mov ebx, DWORD PTR FRETOP	# see if it's the first one in string space
	cmp ebx, edx
	jnz NOTLST			# no, so don't add
	add ebx, ecx			# add the length (C is zero-extended)
	mov DWORD PTR FRETOP, ebx	# and update FRETOP
NOTLST:
	pop ebx				# get pointer at current descriptor
	ret
FRETMS:
	mov ebx, DWORD PTR TEMPPT	# get temp pointer
	sub ebx, 4			# address field of the last temp
	mov ecx, DWORD PTR [ebx]	# [B,C] = pointer at string
	dec ebx				# the descriptor (length byte) of the last temp
	cmp ebx, edx			# see if [D,E] point at the last temp
	jz Q_FM1
	ret				# return now if not freeing
Q_FM1:
	mov DWORD PTR TEMPPT, ebx	# update the temp pointer (decremented by STRSIZ)
	ret

# -------------------------------------------------------------------------------------
# LEN, ASC, CHR$ and related numeric string functions.
# -------------------------------------------------------------------------------------
LEN:
	mov ecx, offset SNGFLT		# call SNGFLT when done
	push ecx
LEN1:
	call FRESTR			# free up temp pointed to by FACLO
	xor al, al			# force numeric flag
	mov dh, al			# set high of [D,E] to zero for VAL
	mov al, BYTE PTR [ebx]
	or al, al			# set condition codes on length
	ret

ASC:
	mov ecx, offset SNGFLT		# where to go when done
	push ecx			# save return address on the stack
ASC2:
	call LEN1			# set up the original string
	jnz Q_Q_L006
	jmp FCERR			# null string: bad argument
Q_Q_L006:
	inc ebx				# bump pointer
	mov edx, DWORD PTR [ebx]	# [D,E] = pointer at string data
	mov esi, edx
	mov al, BYTE PTR [esi]		# [A] = first character
	ret

CHR_:
	call STRIN1			# get string in DSCTMP
	call CONINT			# get integer in range
	# DSCTMP+1 is the data address (INCSTR=1 in the original)
SETSTR:
	mov ebx, DWORD PTR DSCPTR	# get address of the string
	mov BYTE PTR [ebx], dl		# save the ASCII byte
FINBCK:
	pop ecx				# return to the higher level and skip the CHKNUM call
	jmp PUTNEW			# go call PUTNEW

STRNG_:
	call CHRGTR			# get next char following STRING$
	call SYNCHR
	.byte 40			# make sure left paren
	call GETBYT			# evaluate first arg (length)
	push dx				# save it
	call SYNCHR
	.byte 44			# comma
	call FRMEVL			# get formula arg 2
	call SYNCHR
	.byte 41			# expect right paren
	pop cx				# the saved count (2 bytes)
	push ebx			# text pointer on the stack
	push cx				# rep factor back on the stack
	call GETYPR			# get type of arg
	jz STRSTR			# was a string
	call CONINT			# get ASCII value of char
	jmp CALSPA			# now call space code
STRSTR:
	call ASC2			# get value of char in [A]
CALSPA:
	pop dx				# get rep factor in [E]
	call SPACE2			# into space code; dummy entry on stack popped by FINBCK

SPACE_:
	call CONINT			# get number of chars in [E]
	mov al, 32			# get space char
SPACE2:
	push ax				# save char
	mov al, dl			# get number of chars in [A]
	call STRINI			# get a string that long
	mov ch, al			# count of chars back in [B]
	pop ax				# get back char to put in string
	inc ch				# test for null string
	dec ch
	jz FINBCK			# yes, all done
	mov ebx, DWORD PTR DSCPTR	# get descriptor pointer (data address)
SPLP_:
	mov BYTE PTR [ebx], al		# save char
	inc ebx				# bump ptr
	dec ch				# decrement count
	jnz SPLP_			# keep storing chars
	jmp FINBCK			# put temp desc when done

# -------------------------------------------------------------------------------------
# LEFT$, RIGHT$, MID$. The shared tail (LEFT2) copies the substring into new string space.
# -------------------------------------------------------------------------------------
LEFT_:
	call PREAM			# test the parameters
	xor al, al			# LEFT never changes the string pointer
LEFT3:
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save text pointer
	mov cl, al			# offset now in [C]
	jmp LEFT2			# (original: DB 260O skipped the dummy push below)

# Entry for PRINT USING: the dummy push offsets the extra pop in PUTNEW.
LEFTUS:
	push ebx
LEFT2:
	push ebx			# save descriptor for FRETMP
	mov al, BYTE PTR [ebx]		# string length
	cmp al, ch			# entire string wanted?
	jb ALLSTR			# if the count asked for >= length, yes
	mov al, ch			# truncated length of the string
	jmp PUSHOFF			# (original: DB 272O skipped "MOV CL,LOW 0")
ALLSTR:
	mov cl, 0			# make offset zero
PUSHOFF:
	push cx				# save offset on the stack
	call GETSPA			# get space for the new string
	pop cx				# get back offset
	pop ebx				# get back descriptor pointer
	push ebx			# but keep it on the stack
	mov ebx, DWORD PTR [ebx+1]	# BX = data pointer
	movzx ecx, cl			# offset (CH cleared)
	add ebx, ecx			# add it
	mov ecx, ebx			# offset pointer in [C]
	call STRAD2			# save info in DSCTMP (count in AL, destination in DX)
	mov bl, al			# number of chars to move in [L]
	call MOVSTR			# move them in
	pop edx				# get back descriptor pointer
	call FRETMP			# free it up
	jmp PUTNEW			# put temp in temp list

RIGHT_:
	call PREAM			# check arg
	pop edx				# get descriptor pointer
	push edx			# save back for LEFT
	mov esi, edx
	mov al, BYTE PTR [esi]		# present length of string
	sub al, ch			# subtract 2nd parm
	jmp LEFT3			# continue with the LEFT code

# MID$(s,n[,m]): substring from position n, for m characters.
MID_:
	xchg ebx, edx			# text pointer into [H,L]
	mov al, BYTE PTR [ebx]		# first character
	call PREAM2			# get offset off the stack; length in CH
	inc ch
	dec ch				# equal to zero?
	jnz Q_Q_L007
	jmp FCERR			# must not be 0
Q_Q_L007:
	push cx				# offset on the stack
	call MIDRST			# parse the optional 3rd argument
	pop ax				# (POP PSW)
	xchg al, ah
	sahf				# get the offset back in A
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save text pointer; get the descriptor
	mov ecx, offset LEFT2		# where to return to
	push ecx
	dec al				# subtract one from the offset
	cmp al, BYTE PTR [ebx]		# pointer past the end of the string?
	mov ch, 0			# assume null length string
	jnae Q_MD1
	ret				# yes: use the null string (returns to LEFT2)
Q_MD1:
	mov cl, al			# save offset of character pointer
	mov al, BYTE PTR [ebx]		# present length of string
	sub al, cl			# subtract index (2nd arg)
	cmp al, dl			# truncation?
	mov ch, al			# calculated length in [B]
	jnb Q_MD2
	ret				# not truncated: use partial string
Q_MD2:
	mov ch, dl			# use truncated length
	ret				# return to LEFT2

# -------------------------------------------------------------------------------------
# VAL: the string is terminated in place (the character after it is replaced by 0) and FIN
# is called on it.
# -------------------------------------------------------------------------------------
VAL:
	call LEN1			# set up, result = real
	jnz Q_Q_L008
	jmp SNGFLT
Q_Q_L008:				# make sure the type is set up OK in extended
	movzx edx, al			# length of the string (DH is 0 from LEN1)
	inc ebx				# step to the data pointer field
	mov ebx, DWORD PTR [ebx]	# [BX] = the string's data pointer
	push ebx			# FIN is called pointing at the first character
	add ebx, edx			# the character after the string
	mov ch, BYTE PTR [ebx]		# save it
	mov BYTE PTR [ebx], dh		# store 0 beyond the string
	pop esi				# XTHL
	xchg esi, ebx
	push esi
	push cx				# save the modified character
	dec ebx				# call CHRGTR to make sure VAL(" -3") = -3
	call CHRGTR
	call FINDBL			# in extended, get all the precision we can
	pop cx				# the modified character of the next string into [B]
	pop ebx				# the pointer to the modified character
	mov BYTE PTR [ebx], ch		# restore the character
	ret

# USED BY RIGHT$ AND LEFT$ FOR PARAMETER CHECKING AND SETUP
PREAM:
	xchg ebx, edx			# text pointer into [H,L]
	call SYNCHR
	.byte 41			# parameter list should end with )
# USED BY MID$ FOR PARAMETER CHECKING AND SETUP (PREAM falls through here)
PREAM2:
	pop ecx				# get return address off the stack
	pop dx				# get length of arg off the stack
	push ecx			# save return address back on
	mov ch, dl			# save initial length
	ret

# MIDRST: the optional third argument of MID$ (default 255).
MIDRST:
	mov dl, 255			# if two args, truncate
	cmp al, 41			# ")" means use all chars
	jz MID2
	call SYNCHR
	.byte 44			# comma: must delineate the 3rd arg
	call GETBYT			# get the argument in [E]
MID2:
	call SYNCHR
	.byte 41			# must be followed by )
	ret

# -------------------------------------------------------------------------------------
# INSTR(I%,S1$,S2$) or INSTR(S1$,S2$). Returns 0 if S1$ is null or S2$ not found.
# -------------------------------------------------------------------------------------
INSTR:
	call CHRGTR			# eat the first char
	call FRMPRN			# evaluate the first arg
	call GETYPR			# set zero if arg a string
	mov al, 1			# if so, assume search starts at the first char
	push ax				# save offset in case string
	jz WUZSTR			# was a string
	pop ax				# get rid of the saved offset
	call CONINT			# force arg1 (I%) to be integer
	or al, al			# don't allow zero offset
	jnz Q_Q_L009
	jmp FCERR			# kill him
Q_Q_L009:
	push ax				# save for later
	call SYNCHR
	.byte 44			# eat the comma
	call FRMEVL			# eat first string arg
	call CHKSTR			# blow up if not string
WUZSTR:
	call SYNCHR
	.byte 44			# eat comma after arg
	push ebx			# save the text pointer
	mov ebx, DWORD PTR FACLO	# get descriptor pointer
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# put on stack and get back text pointer
	call FRMEVL			# get last arg
	call SYNCHR
	.byte 41			# eat right paren
	push ebx			# save text pointer
	call FRESTR			# free up temp and check string
	xchg ebx, edx			# save 2nd desc pointer in [D,E]
	pop ecx				# text pointer in [B]
	pop ebx				# desc pointer for S1$
	pop ax				# offset
	push ecx			# text pointer on the bottom
	mov ecx, offset POPHRT		# address of POP H, RET
	push ecx
	mov ecx, offset SNGFLT		# address of the [A] returner
	push ecx
	push ax				# save offset back
	push edx			# save desc of S2
	call FRETM2			# free up S1 desc
	pop edx				# restore desc S2
	pop ax				# get back offset
	mov ch, al			# save unmodified offset
	dec al				# make offset OK
	mov cl, al			# save in [C]
	cmp al, BYTE PTR [ebx]		# is it beyond length of S1?
	mov al, 0			# if so, return zero (error)
	jnae Q_IN1
	ret
Q_IN1:
	mov esi, edx
	mov al, BYTE PTR [esi]		# length of S2$
	or al, al			# null?
	mov al, ch			# get offset back
	jnz Q_IN2
	ret				# all if S2 null: return offset
Q_IN2:
	mov al, BYTE PTR [ebx]		# length of S1$
	mov ebx, DWORD PTR [ebx+1]	# BX = S1$ data pointer
	movzx ecx, cl			# index (CH cleared)
	add ebx, ecx			# index into S1$
	sub al, cl			# remaining length of S1$
	mov ch, al			# save it in [B]
	push cx				# counter and offset
	push edx			# S2$ descriptor
	pop esi
	xchg esi, ebx			# BX = S2$ descriptor, SI = S1$ data
	push esi			# S1$ data pointer on the stack
	mov cl, BYTE PTR [ebx]		# length of S2$
	mov edx, DWORD PTR [ebx+1]	# S2$ data pointer
	pop ebx				# S1$ data pointer

CHK1:
	push ebx			# save position in the search string
	push edx			# save start of the substring
	push cx				# save where we started the search
CHK:
	mov esi, edx
	mov al, BYTE PTR [esi]		# char from the substring
	cmp al, BYTE PTR [ebx]		# = char pointed to by [H,L]?
	jnz OHWELL			# no
	inc edx				# bump compare pointer
	dec cl				# end of the search string?
	jz GOTSTR			# found it
	inc ebx				# bump pointer into the string being searched
	dec ch				# decrement remaining length
	jnz CHK				# end of string: you lose
RETZER:
	add esp, 2			# CX item (was POP DX)
	add esp, 4			# DX item (was POP DX)
	add esp, 4			# BX item (was POP CX)
RETZR1:
	pop dx				# the counter saved before the search
	xor al, al
	ret

GOTSTR:
	pop bx				# CX item: BH = remaining count
	pop edx				# DX item
	pop edx				# BX item (discard)
	pop cx				# counter and offset saved before the search
	mov al, ch			# original source counter
	sub al, bh			# subtract final counter
	add al, cl			# add original offset (N1%)
	inc al				# make offset of zero = position 1
	ret

OHWELL:
	pop cx				# CX item
	pop edx				# DX item: start of substring
	pop ebx				# BX item: where we started to compare
	inc ebx				# point to next char
	dec ch				# decrement # chars left in source string
	jnz CHK1			# try searching some more
	jmp RETZR1			# end of string: return 0

# -------------------------------------------------------------------------------------
# LHSMID: MID$(LHS$,n[,m]) = RHS$. Copies RHS$ into the LHS$ string in place.
# -------------------------------------------------------------------------------------
LHSMID:
	call SYNCHR
	.byte 40			# must have (
	call PTRGET			# get a string var
	call CHKSTR			# make sure it was a string
	push ebx			# save text pointer
	push edx			# save desc pointer
	xchg ebx, edx			# put desc pointer in [H,L]
	inc ebx				# move to address field
	mov edx, DWORD PTR [ebx]	# address of LHS in [D,E]
	mov ebx, DWORD PTR STREND	# see if LHS string is in string space
	cmp ebx, edx			# by comparing it with STREND
	jb NCPMID			# already in string space: don't copy
	mov ebx, DWORD PTR TXTTAB	# is this a fielded string?
	cmp ebx, edx
	jae NCPMID			# yes, don't copy
NCP1:
	pop ebx				# get back desc pointer
	push ebx			# save back on stack
	call STRCPY			# copy the string literal into string space
	pop ebx				# get back desc pointer
	push ebx			# back on stack again
	call VMOVE			# move new desc into old slot
NCPMID:
	pop ebx				# get desc pointer
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# get text pointer to [H,L]; desc to stack
	call SYNCHR
	.byte 44			# must have comma
	call GETBYT			# get arg 2 (offset into string)
	or al, al			# make sure not zero
	jnz Q_Q_L010
	jmp FCERR			# blow him up if zero
Q_Q_L010:
	push ax				# save arg 2 on stack
	mov al, BYTE PTR [ebx]		# restore current char
	call MIDRST			# use MID$ code to evaluate possible third arg
	push dx				# save third arg ([E]) on stack
	call FRMEQL			# evaluate RHS of the statement
	push ebx			# save text pointer
	call FRESTR			# free up temp RHS if any
	xchg ebx, edx			# put RHS desc pointer in [D,E]
	pop ebx				# text pointer to [H,L]
	pop cx				# arg 3 to C
	pop ax				# arg 2 to A
	mov ch, al			# and [B]
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# get LHS desc pointer to [H,L]; text pointer to stack
	push ebx			# save text pointer
	mov ebx, offset POPHRT		# address to return to
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save on stack and get back text pointer
	mov al, cl			# arg 3
	or al, al			# set condition codes
	jnz Q_LH1
	ret				# if zero, do nothing
Q_LH1:
	mov al, BYTE PTR [ebx]		# length of LHS
	sub al, ch			# characters in remainder of string
	jae Q_Q_L011
	jmp FCERR			# can't assign past LEN(LHS)
Q_Q_L011:
	inc al				# make proper count
	cmp al, cl			# is the count greater than the third arg?
	jb BIGLEN			# if so, don't truncate
	mov al, cl			# truncate using the 3rd arg
BIGLEN:
	mov cl, ch			# offset of string in [C]
	dec cl				# make proper offset
	movzx ecx, cl			# [B,C] set up for add
	push edx			# save [D,E]
	mov ebx, DWORD PTR [ebx+1]	# LHS data pointer (4 bytes)
	add ebx, ecx			# add offset
	mov ch, al			# count of LHS in [B]
	pop edx				# restore [D,E]
	xchg ebx, edx			# BX = RHS desc, DX = LHS data
	mov cl, BYTE PTR [ebx]		# length of RHS
	inc ebx
	mov ebx, DWORD PTR [ebx]	# RHS data pointer (4 bytes)
	xchg ebx, edx			# [H,L] = LHS pointer, [D,E] = RHS pointer
# now [H,L] = LHS pointer, [D,E] = RHS pointer, C = LEN(RHS), B = LEN(LHS)
	mov al, cl			# is RHS null?
	or al, al
	jnz Q_LH2
	ret				# then all done
Q_LH2:
MID_LP:
	mov esi, edx
	mov al, BYTE PTR [esi]		# byte from RHS
	mov BYTE PTR [ebx], al		# store in LHS
	inc edx				# bump RHS pointer
	inc ebx				# bump LHS pointer
	dec cl				# bump down count of RHS
	jnz Q_LH3
	ret				# if zero, all done; if LHS ended, also done
Q_LH3:
	dec ch
	jnz MID_LP
	ret				# back to NEWSTT

# -------------------------------------------------------------------------------------
# FRE: free string space. A numeric argument returns the free bytes; a string argument frees
# the temporary first. Garbage collection runs before the count is taken.
# -------------------------------------------------------------------------------------
FRE:
	call GETYPR
	jz Q_Q_L012
	jmp CLCDIF
Q_Q_L012:
	call FREFAC			# free up the argument and set up for free string space
	call GARBA2			# do garbage collection
CLCDIF:
	mov edx, DWORD PTR STREND
	mov ebx, DWORD PTR FRETOP	# top of free area
	jmp GIVFRE
