
# ---------------------------------------------------------------------------
# Hand-translated 8086 code from IBMRES.ASM (CRUNCH / LIST / statement and
# function dispatch for the IBM-compatible reserved words).
#
# Notes on the translation:
#  - "INS86 213,362,56,254" is "MOV SI,DX" followed by "CS: LODSB". In the flat model
#    this is "mov esi, edx; lodsb" (no segment prefix needed).
#  - "INS86 56" (CS prefix) before a memory operand is dropped.
#  - Tables of addresses are .long (4 bytes), so the index math scales by 4. Indices
#    are computed in 32-bit registers, so no 8-bit overflow.
#  - Pointer pushes are 4 bytes; value pushes are 2 bytes. Each POP below matches
#    the width of the push that the original pairs it with.
#  - Original labels "??Lnnn" become "Q_Q_Lnnn" (the ? -> Q_ convention).
# ---------------------------------------------------------------------------
	.section .text
	.globl CRUNCX, LISTX, NEWSTX, EVALX

# CRUNCX: search the reserved-word list for the alpha entry of the text at [EBX].
CRUNCX:
	pop ebx			# get back source pointer
	push ebx		# to try again
	dec ebx			# point at character
	call MAKUPL		# convert to upper case
	mov ebx, offset ALPTAX	# pointer to alpha dispatch table
	sub al, 'A'		# subtract alpha offset
	movzx ecx, al
	shl ecx, 2		# index * 4 (4-byte entries)
	add ebx, ecx		# address of entry
	mov edx, [ebx]		# get pointer in EDX
	pop ebx			# get back source pointer
TRYAGA:
	push ebx		# save text pointer to start of search area
LOPPSI:
	call MAKUPL		# translate this char to upper case
	mov cl, al		# save char in CL
	mov esi, edx		# CS: LODSB from reserved list at EDX
	lodsb
	and al, 127		# get rid of high bit
	jnz Q_Q_L000
	jmp NOTRFN		# if zero, end of this char's result
Q_Q_L000:
	inc ebx			# bump source pointer
	cmp al, cl		# compare to char from source line
	jnz LOPSKP		# if no match, search for next reserved word
	mov esi, edx
	lodsb			# fetch from reserved list
	inc edx			# bump reserved-list pointer
	or al, al		# set condition codes
	js Q_Q_L001
	jmp LOPPSI		# see if rest of chars match
Q_Q_L001:
	mov esi, edx
	lodsb			# fetch from reserved list
	call MAKUPL		# get next char in line
	cmp al, '.'		# is it a dot
	jz ISVARS		# yes
	call TSTANM		# is it a letter immediately following reserved word
ISVARS:
	mov al, 0		# set DONUM to -1
	jnae Q_Q_L002
	jmp NOTRFN		# if alpha, cant be reserved word
Q_Q_L002:
	pop eax			# get rid of saved pointer
	mov esi, edx
	lodsb			# fetch from code segment
	or al, al		# set condition codes
	pop cx			# get char count off stack
	pop edx			# get deposit pointer off stack
	jns Q_Q_L003
	jmp NOTFNT		# if minus, wasnt function token
Q_Q_L003:
	or al, 128		# make high order bit one
	stc			# and force leader byte to be 375
NOTFNT:
	push ax			# save fn char
	mov al, 254		# get byte which precedes fns
	sbb al, 0		# make function leader 375
	jmp RENCRN		# reenter crunch with new reserved word
LOPSKP:
	pop ebx			# restore undefined text pointer
LOPSK2:
	mov esi, edx
	lodsb			# fetch from reserved list
	inc edx			# bump reserved-list pointer
	or al, al		# set condition codes
	js Q_Q_L004
	jmp LOPSK2		# not end of reserved word, keep skipping
Q_Q_L004:
	inc edx			# point after token
	jmp TRYAGA		# try another reserved word

# LIST code for extended reserved words
BUFRET:
	ret
NEWFUN:
	mov al, [ebx]		# get function number
	and al, 127		# take off high bit
	jmp BUFCON
LISTX:
	cmp al, 253		# is it a new function
	jz NEWFUN
	cmp al, 254		# is it a new statement
	jnz BUFRET		# no, just continue normal path
	mov al, [ebx]		# get number of statement
BUFCON:
	pop ebx			# get rid of the return address
	mov ebx, offset RESLSX - 1	# get pointer to start of reserved word list
	mov ch, al		# save this char in CH
	mov cl, 'A' - 1		# init leading char value
RESSR3:
	inc cl			# bump leading char value
RESSR1:
	inc ebx			# bump pointer into RESLST
RESSRC:
	mov edx, ebx		# save pointer to start of this reserved word
RESSR2:
	mov al, [ebx]		# get char from RESLST
	or al, al		# set condition codes
	jz RESSR3		# if end of this chars table, bump C and go back
	lahf
	inc ebx			# bump source pointer
	sahf
	js Q_Q_L005
	jmp RESSR2		# if not end of this reserved word, keep looking
Q_Q_L005:
	mov al, [ebx]		# get pointer to reserved word value
	cmp al, ch		# same as the one we search for?
	jnz RESSR1		# no, keep looking
	xchg ebx, edx		# save found pointer
	mov al, cl		# get leading char
	pop dx			# restore line char count
	pop ecx			# restore deposit pointer
	mov dl, al		# save leading char in E
NTSPCH:
	mov al, BYTE PTR TEMPA	# what did we do last?
	or al, al		# special?
	mov al, 255		# flag in reserved word
	mov BYTE PTR TEMPA, al	# clear flag
MORLNZ:
	jz MORLN0		# get char and proceed
	mov al, ' '		# put space in buffer
	mov edi, ecx
	stosb
	inc ecx
	dec dh			# any space left in buffer
	jnz Q_Q_L006
	jmp PPSWRT		# no, return
Q_Q_L006:
MORLN0:
	mov al, dl
	jmp MORLN1		# continue
MORPUR:
	mov al, [ebx]		# get byte from reserved word
	inc ebx			# bump pointer
MORLNP:
	mov dl, al		# save char
MORLN1:
	and al, 127		# and off high order bit for disk & edit
	mov edi, ecx
	stosb			# store this char
	inc ecx			# bump pointer
	dec dh			# bump down remaining char count
	jnz Q_Q_L007
	jmp PPSWRT		# if end of line, just return
Q_Q_L007:
	or al, dl		# set condition codes
	js Q_Q_L008
	jmp MORPUR		# end of reserved word?
Q_Q_L008:
	pop ebx			# restore source pointer
	inc ebx			# skip over reserved word
	jmp PLOOP2		# get next char from line

# Extended statement dispatching
SNERRS:
	jmp SNERR
NEWSTX:
	cmp al, 254 - 129	# check for new statement prefix (OFFSET 376O-201O)
	jz GONE4		# if so, dispatch
	cmp al, 255 - 129	# function?
	jnz GIIRET
	pop ecx			# put return address into B
	inc ebx
	mov al, [ebx]
	cmp al, _PEN & 255	# PEN as stmt?
	jnz Q_Q_L009
	jmp PENV		# brif so
Q_Q_L009:
	cmp al, _STRIG & 255	# STRIG as stmt?
	jnz Q_Q_L010
	jmp STRIGV		# brif so
Q_Q_L010:
	dec ebx
	mov al, 255 - 129
	push ecx		# put the return address back on
GIIRET:
	ret			# MID$ OR SYNTAX ERROR
PENV:
	call CHRGTR
	jmp PENS
STRIGV:
	call CHRGTR
	jmp STRIGS
GONE4:
	pop ecx			# get rid of the return address
	inc ebx			# look at next char
	mov al, [ebx]		# fetch it
	sub al, 129		# get relative position in STMDSX
	jb SNERRS		# if too small, syntax error
	movzx esi, al		# index into STMDSX (4-byte entries)
	shl esi, 2
	push DWORD PTR STMDSX[esi]	# push STMDSX(SI) address
	jmp CHRGTR		# start statement

# EVAL code for extended functions
EVALX:
	cmp al, 254
	jz EVALNS		# brif possible stmt as function
	cmp al, 253
	jz EVALNF		# new function if 253 in front
	ret
EVALNS:
	inc ebx
	mov al, [ebx]
	cmp al, _DATE_ & 255	# DATE$
	jz DATEV
	cmp al, _TIME_ & 255	# TIME$
	jz TIMEV
	dec ebx
	mov al, [ebx]
	ret
DATEV:
	pop ecx			# get rid of the hook return address
	jmp DATEF		# do DATE function
TIMEV:
	pop ecx			# get rid of the hook return address
	jmp TIMEF		# do TIME$ function
EVALNF:
	pop ecx			# get rid of the return address
	inc ebx
	mov al, [ebx]
	sub al, 129
	mov ch, 0
	rol al, 1		# index * 2 (2-byte value, scaled again below)
	mov cl, al
	push cx
	call CHRGTR
	call PARCHK
	pop si			# XTHL: index (value) back in SI
	xchg esi, ebx		# SI = text pointer, BX = index
	push esi		# save text pointer
	mov edx, offset LABBCK
	push edx
	mov al, 1
	mov BYTE PTR FLGOVC, al
	movzx esi, bx		# index (value, 2 * fn number)
	shl esi, 1		# * 4 for 4-byte FUNDSX entries
	push DWORD PTR FUNDSX[esi]
	ret
