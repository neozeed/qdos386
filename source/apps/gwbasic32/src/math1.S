# MATH1.ASM -> 32-bit GAS. Part 1: integer-to-ASCII conversion, PRINT USING fix-up.
# Radix note: the original code in MATH1 is octal (.RADIX 8) except lines 527-570,
# so bare numbers such as 400 and 40 in the source are octal here and are written as 0400 / 040.
	.intel_syntax noprefix
# Cross-module exports (labels referenced from the other math file or the interpreter).
	.globl	MAKINT, MDPTEN, _AEXPS, _CHRGT, _COMPM, _DADDH, _DCMPM, _DIV10, _DP06
	.globl	_DP09, _DP16, _FADDD, _FADDH, _FCI4, _FINE, _FODTB, _FOSTB
	.globl	_FOTB, _LOGQ, _MOVAC, _MOVAF, _MOVBF, _MOVBS, _MUL10, _OVFLS
	.globl	_PUSHF, _RNDA, _RNDC, _RSUBM, _SETDB, _SHDR, _VADDH, _VCMPM
	.globl	_VCOMP, _VMOVM, _VMVFM, _VMVMF, _VNEG, _VPOPF, _ZERO, DDIVFA
	.globl	_VPSHF

	.section .text

# $FOTCI: convert the integer in FACLO (two bytes) to ASCII digits.
# Enter with BX = output pointer, CX = decimal point and comma counts.
	.globl _FOTCI
_FOTCI:
	mov	ah, 5				# MAX DIGITS TO CONVERT
	mov	ebp, offset _FOITB
_FCI4:						# entry for FOTCV: only convert 4 digits
FCI10:	call	_FOTED				# check for needed D.P. or ","
	mov	dx, WORD PTR [ebp]		# fetch power of ten to DX
	inc	ebp				# point (EBP) to next pwr 10
	inc	ebp
	mov	si, WORD PTR _FACLO		# fetch integer to be converted
	mov	al, '0'-1			# will build digit in (AL)
FCI20:	inc	al
	sub	si, dx				# subtract out power of ten
	jnb	FCI20				# continue until digit formed
	add	si, dx				# subtracted out one too many
	mov	BYTE PTR [ebx], al		# move digit to buffer
	inc	ebx				# point (EBX) to next buffer position
	mov	WORD PTR _FACLO, si		# save updated integer
	dec	ah				# loop till digits formed
	jnz	FCI10
	call	_FOTED				# see if decimal point needed
	mov	BYTE PTR [ebx], 0		# and put binary zero after
	ret

# $FOUTO / $FOUTH: octal and hexadecimal output of the integer in FACLO:FACLO+1.
# Return with EBX pointing to _FBUFF.
	.globl _FOUTO, _FOUTH
_FOUTO:
	mov	cx, 0x301			# 3*0400+1: 3 bits at a time, first shift 1
	mov	si, 6				# octal digits
	jmp	FTH10
_FOUTH:
	mov	cx, 0x404			# 4*0400+4: 4 bits at a time
	mov	si, 4				# hex digits
FTH10:	mov	edi, offset _FBUFF		# point EDI to output location
	cld					# so subsequent STOSB increments
	mov	ebx, offset _NUMB		# XLAT table (digit characters)
	mov	dx, WORD PTR _FACLO		# fetch integer
	push	si				# save for zero suppress code (16-bit)
FTH20:	mov	al, dh				# get integer
	xor	ah, ah				# clear upper AX
	shl	ax, cl				# shift high order bits into AH
	xchg	ah, al				# now in AL
	xlat					# look up ASCII (table addressed by EBX)
	stosb
	shl	dx, cl				# shift number
	mov	cl, ch				# get no. bits to shift
	dec	si				# decrement no. of digits
	jnz	FTH20
	mov	BYTE PTR [edi], 0		# denote end of no.
	mov	ebx, offset _FBUFF
	pop	cx				# recall for zero suppress (16-bit)
	dec	cl				# can't delete all the zeros
	movzx	ecx, cx				# LOOP below uses ECX: clear the high half
FTH40:	cmp	BYTE PTR [ebx], '0'		# do zero suppress
	jnz	FTH50				# no more suppress
	inc	ebx
	loop	FTH40
FTH50:	ret

# $PUFXE: PRINT USING fix-up. Called once the number is built in _FBUFF.
	.globl _PUFXE
_PUFXE:
	push	ebx				# save end of buffer pointer
	call	_FOTZS				# do zero suppression (leaves ZF=0 if trailing sign in CH)
	pop	ebx				# recall end-of-buffer pointer
	jz	PFX10				# if no trailing sign proceed
	mov	BYTE PTR [ebx], ch		# put in trailing sign
	inc	ebx				# must now put in binary zero
PFX10:	mov	BYTE PTR [ebx], 0		# to signify end of print
	mov	ebx, offset _FBUFF		# fetch start of buffer address
PFX20:	inc	ebx				# increment (EBX) to next buffer position
PFX30:	mov	edi, DWORD PTR _DPADR		# get decimal point address (pointer, 32-bit)
	mov	dx, WORD PTR _FMTCX		# fetch decimal point info
	mov	al, BYTE PTR _FMTCX+1		# fetch no. digits to left of D.P.
	movzx	eax, al				# (AX) = digits left of decimal point
	sub	edi, ebx			# D.P. address - buffer ptr
	sub	edi, eax			# - digits left of D.P.
	jz	PFX90				# return if all is OK
# FIELD IS TOO LARGE: shorten it by eliminating leading " ", "*", and a leading "0"
# when the character after the decimal point is a digit (see original comment block).
	mov	al, BYTE PTR [ebx]		# fetch next character in buffer
	cmp	al, ' '				# space?
	jz	PFX20				# if so eliminate and see if now OK
	cmp	al, '*'				# asterisk fill?
	jz	PFX20
	mov	ah, 1				# signal end of characters in following code
	dec	ebx				# because CHRGT will do INC first
	push	ebx				# save this location for possible "%"
PFX40:	push	ax				# save character/marker (16-bit)
	call	_CHRGT				# get next character (re-fetches failing char first time)
	xor	ah, ah
	cmp	al, '-'				# if equal need to save this guy
	jz	PFX40
	cmp	al, '+'
	jz	PFX40
	cmp	al, '$'				# CURNCY (default floating currency is dollars)
	jz	PFX40
	cmp	al, '0'				# if equal possible to eliminate
	jnz	PFX70				# must give up and put leading "%"
# Leading zero: can be eliminated only if there are digits beyond the decimal point.
	inc	ebx				# this causes elimination of "0"
	call	_CHRGT				# get character after decimal point
	jnb	PFX70				# no hope: give up and put in "%"
	dec	ebx				# (EBX) now points to decimal point
	jmp	PFX60
PFX50:	dec	ebx				# next character position to move into
	mov	BYTE PTR [ebx], al		# move it in
PFX60:	pop	ax				# fetch character off the stack
	or	ah, ah				# once AH=1 reached, chars have been re-instated
	jz	PFX50				# keep going if AH=0
	add	esp, 4				# discard saved buffer start pointer (4-byte push above)
	jmp	PFX30				# see if we now fit the required space
PFX70:
# Could not fit the number: pop all characters, pop the buffer start, insert "%".
	pop	ax				# get character off the stack
	or	ah, ah				# if ZF=1 after this, more chars
	jz	PFX70
	pop	ebx				# get beginning buffer pointer (4-byte)
	mov	BYTE PTR [ebx], '%'		# insert overflow flag
PFX90:	ret

# $FOTZS: zero suppression on the output in _FBUFF. Leaves ZF=0 if a trailing sign
# is to be written (sign left in CH). Enter with EBX/AL per the original.
	.globl _FOTZS
_FOTZS:
	mov	ebx, offset _FBUFF+1		# address of sign (if leading)
	mov	ch, BYTE PTR [ebx]		# fetch leading character
	mov	cl, ' '				# space to CL
	mov	ah, BYTE PTR _FMTAL		# fetch format specifications
					# bits 76543210 of AH: A free format, B group in 3s with commas,
					# C fill leading spaces with "*", D floating "$", E "+" sign,
					# F trailing sign, G unused, H scientific notation
	test	ah, 0x20			# (040 octal) test for leading "*" first
	jz	FZ10				# leading asterisks not desired
	cmp	ch, cl				# space?
	mov	cl, '*'				# since it's asterisk fill desired
	jnz	FZ10				# jump if not space
	test	ah, 0x04			# check for trailing sign
	jnz	FZ10
	mov	ch, cl
FZ10:	mov	BYTE PTR [ebx], cl		# replace sign as necessary
	call	_CHRGT				# get next character and set condition codes
	jz	FZ50				# jump if end of number
	mov	ebp, offset _FINCH+11
FZ20:	cmp	al, BYTE PTR [ebp]		# see if special character
	jz	FZ30
	cmp	ebp, offset _FINCH		# searched all the table?
	jz	FZ60				# if so check for floating "$"
	dec	ebp				# point to next special character
	jmp	FZ20				# and continue search
FZ30:	sub	ebp, offset _FINCH		# calculate table offset
	shl	ebp, 2				# 4-byte address entries (was *2 for 16-bit)
					# EBP now indexes FZ45
FZ40:
	jmp	DWORD PTR FZ45[ebp]		# dispatch through address table
FZ45:
	.long	FZ50				# "."
	.long	FZ50				# "E"
	.long	FZ60				# "E"+40
	.long	FZ60				# "%"
	.long	FZ60				# "#"
	.long	FZ60				# "!"
	.long	FZ50				# "D"
	.long	FZ60				# "D"+40
	.long	FZ10				# ","
	.long	FZ10				# "0"
FZ50:	dec	ebx				# need to put in leading "0"
	mov	BYTE PTR [ebx], '0'
FZ60:	mov	ah, BYTE PTR _FMTAL		# get format specs again
	test	ah, 0x10			# (020 octal) test for floating "$"
	jz	FZ70				# if not proceed
	dec	ebx
	mov	BYTE PTR [ebx], '$'		# floating "$"
FZ70:	test	ah, 0x04			# recall format specs: trailing sign?
	jnz	FZ90				# if so return
	dec	ebx
	mov	BYTE PTR [ebx], ch		# put in sign
	xor	ch, ch				# must return ZF=1 if not trailing sign
FZ90:	ret

# $FOTZ: put AL leading zeros in the buffer at EBX.
	.globl _FOTZ, _FOTZC
_FOTZ:
	or	al, al				# see if further work to do
	jmp	FTZ15
FTZ10:	mov	BYTE PTR [ebx], '0'		# put in leading zero
	inc	ebx				# next buffer position
	dec	al
FTZ15:	jnz	FTZ10
	ret

FTZC10:	call	_FOTED				# put in comma and decimal point as necessary

# $FOTZC: put AL leading zeros (paying attention to D.P. and comma counts).
_FOTZC:
	mov	BYTE PTR [ebx], '0'		# put in leading zero
	inc	ebx				# next buffer position
	dec	al
	jnz	FTZC10
	ret

# $PUFE: format a single or double precision number in "E" format.
# Enter with EBX = current output pointer, ZF=1 if single precision, ZF=0 if double.
# STACK WIDTHS (must match exactly): pointers are 4 bytes (push ebx), values 2 bytes (push ax),
# PUSHF/POPF are 4 bytes (pushfd/popfd). The caller's pointer push (before the call) is also 4 bytes.
	.globl _PUFE
_PUFE:
	call	_GETYP				# set condition codes according to type
	mov	ah, 7				# 7 print positions if single precision
	jb	PFE10				# jump if single precision
	mov	ah, 16				# double (20 octal)
PFE10:	call	_SIGNS				# set condition codes according to number
	pop	ebx				# get buffer ptr in case number is zero (caller's pointer)
	stc					# CF will be our flag to remember
	jz	PFE20				# if the number was zero, return with CF=0 since $FOTNV
	push	ebx				# save buffer ptr
	push	ax				# save number of digits in number
	call	_FOTNV				# bracket number so digits to print are in the integer part
	pop	dx				# $FOTNV also returns exponent in AL
	pop	ebx				# (CF=0)
	mov	ah, dh				# (AH) = no digits returned from $FOTNV
PFE20:	pushfd					# save CF in case number is zero
	push	ax				# save exponent and no. significant digits
	mov	dx, WORD PTR _FMTCX		# digits left/right of D.P.
	or	dh, dh				# will need to know if sign desired
	pushfd
	or	dl, dl				# see if decimal point desired
	jz	PFE30				# if not proceed
	dec	dl
PFE30:	add	dh, dl				# no. digits desired
	popfd					# see if sign desired
	jz	PFE40				# jump if not desired
	test	BYTE PTR _FMTAX, 4		# see if trailing sign desired
	jnz	PFE40				# if so proceed
	dec	dh				# must use one print position for sign
PFE40:	sub	dh, ah				# must see if have excess digits available
	mov	ah, dh				# if too many we must divide no by 10 until correct
	push	ax				# save comparison of desired-available digits
	js	PFE45
	jmp	PFE65				# if no internal rounding proceed
# (see original comments) Excess digits are divided out so rounding happens at the
# correct print position; the D.P. position is then recomputed by adding the shifted count.
PFE45:	push	ebx				# save buffer ptr
	push	ax				# save no times to divide
PFE50:	push	ax				# save divide count
	call	_DIV10				# shift no to get correct rounding
	pop	ax				# recall divide count
	inc	ah				# increment to reflect divide
	jnz	PFE50				# continue until get correct no.
	call	_VADDH				# round up at the correct position
	call	_VINT				# and make integer
	pop	ax				# recall no. divides (we may need it)
	push	ax				# save # divides
	mov	cx, 3				# will shift AH 3 bits left
	shl	ah, cl				# because DP table is 8 bytes/entry
	call	_GETYP				# determine type
	jb	PFE55				# jump if single precision
	mov	al, ah				# will convert AH to word in AX
	cbw
	mov	ebx, offset _DP16		# address of 10^16
	cwde					# (EAX) = sign-extended AX
	add	ebx, eax			# FAC should be less than no. pointed to
	call	_MOVBF				# move code seg no. to DBUFF
	call	_DCMPM				# to by BX
	jmp	PFE57
PFE55:
	mov	ebx, offset _DP07+4		# address of 10^7
	mov	al, ah				# will convert AH to word in AX
	cbw
	cwde
	add	ebx, eax			# FAC should be less than this no.
	call	_MOVBS				# move code seg no. to DBUFF
	call	_COMPM				# unless round up has occurred
PFE57:	pop	ax				# recall no. divides
	pop	ebx				# get buffer ptr back (4-byte)
	js	PFE66				# round-up did not occur
	pop	ax				# must get to original copy
	pop	cx				# exponent now in CL
	inc	cl				# must increment exponent
	push	cx				# save exp on stack
	push	ax				# save for possible trailing zeros
	push	ebx				# save buffer ptr
	push	ax				# save divided out digits & exponent
	call	_DIV10				# so correct digits to right of decimal point are printed
	pop	ax
	pop	ebx
	jmp	PFE66
PFE65:	xor	ah, ah				# no divides
PFE66:	neg	ah				# if previously negative make positive
	mov	al, BYTE PTR _FMTCX+1		# get desired places to left of dec. pt.
	add	ah, al				# new total
	inc	ah				# because FOTED decrements at first
	or	al, al				# see if places left desired
	jz	PFE70
	test	BYTE PTR _FMTAX, 4		# see if trailing sign
	jnz	PFE70				# if trailing sign proceed
	dec	ah				# allow print position for sign
PFE70:	mov	ch, ah				# set decimal point position
	xor	cl, cl				# no commas for exponential printout
	pop	ax				# recall sig. digit comparison
	push	WORD PTR _FMTCX			# save format specs
	push	ax				# save sig. digit comparison
	mov	BYTE PTR _FMTCX+1, ch		# update digits to left
	call	_FOTCV				# convert to ASCII digits
	pop	ax				# recall sig. digit comparison
	or	ah, ah				# will set SF=0 if trailing zeros
	jle	PFE80				# if trailing zeros not required jump
	mov	al, ah				# no. trailing zeros to AL
	call	_FOTZC				# put in trailing zeros
	call	_FOTED				# put in decimal point (if necessary)
PFE80:	pop	ax				# fetch digits to left/rt. of D.P.
	mov	WORD PTR _FMTCX, ax		# keep copy format specs
	or	al, al				# see if decimal point desired
	jnz	PFE100				# if so proceed
	dec	ebx				# not desired: see if can eliminate
	mov	al, BYTE PTR [ebx]		# fetch last print position
	cmp	al, '.'				# decimal point?
	jz	PFE90				# if so this jump will eliminate
	inc	ebx				# must retain as is
PFE90:	mov	DWORD PTR _DPADR, ebx		# save address of decimal point (pointer, 4-byte)
PFE100:	pop	ax				# recall exponent
	popfd					# recall CF (=1 if number is zero)
	jb	PFE110				# jump if number is zero
	add	al, ah				# add exp and no sig digits
	mov	ah, BYTE PTR _FMTCX+1		# subtract out digits to left of D.P.
	sub	al, ah				# subtract digits to left
	or	ah, ah				# if none then no sign
	jz	PFE110
	test	BYTE PTR _FMTAX, 4		# if so will need to increment AL
	jnz	PFE110
	inc	al				# must account for sign position
PFE110:	or	al, al
	call	_PUEXP				# put exponent in buffer
					# $PUEXP leaves the end-of-buffer pointer in ECX and start in EBX
	mov	ebx, ecx			# must go to PUFXE with end of buffer ptr in EBX
	jmp	_PUFXE				# do fix-up

# ---- Constant tables (generated from MATH1.ASM lines 521-930 by tools/conv_data.py) ----
	.section .data
# *****************************************************************
# THIS FILE CONTAINS 8086 CONSTANT TABLES
# *****************************************************************
_FOTB:
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 38
	.byte 37
	.byte 37
	.byte 37
	.byte 36
	.byte 36
	.byte 36
	.byte 35
	.byte 35
	.byte 35
	.byte 34
	.byte 34
	.byte 34
	.byte 34
	.byte 33
	.byte 33
	.byte 33
	.byte 32
	.byte 32
	.byte 32
	.byte 31
	.byte 31
	.byte 31
	.byte 31
	.byte 30
	.byte 30
	.byte 30
	.byte 29
	.byte 29
	.byte 29
	.byte 29
	.byte 28
	.byte 28
	.byte 28
	.byte 27
	.byte 27
	.byte 27
	.byte 26
	.byte 26
	.byte 26
	.byte 25
	.byte 25
	.byte 25
	.byte 25
	.byte 24
	.byte 24
	.byte 24
	.byte 23
	.byte 23
	.byte 23
	.byte 23
	.byte 22
	.byte 22
	.byte 22
	.byte 22
	.byte 21
	.byte 21
	.byte 21
	.byte 20
	.byte 20
	.byte 20
	.byte 19
	.byte 19
	.byte 19
	.byte 19
	.byte 18
	.byte 18
	.byte 18
	.byte 17
	.byte 17
	.byte 17
	.byte 16
	.byte 16
	.byte 16
	.byte 16
	.byte 15
	.byte 15
	.byte 15
	.byte 14
	.byte 14
	.byte 14
	.byte 13
	.byte 13
	.byte 13
	.byte 13
	.byte 12
	.byte 12
	.byte 12
	.byte 11
	.byte 11
	.byte 11
	.byte 10
	.byte 10
	.byte 10
	.byte 10
	.byte 9
	.byte 9
	.byte 9
	.byte 8
	.byte 8
	.byte 8
	.byte 7
	.byte 7
	.byte 7
	.byte 6
	.byte 6
	.byte 6
	.byte 6
	.byte 5
	.byte 5
	.byte 5
	.byte 4
	.byte 4
	.byte 4
	.byte 3
	.byte 3
	.byte 3
	.byte 3
	.byte 2
	.byte 2
	.byte 2
	.byte 1
	.byte 1
	.byte 1
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte -1
	.byte -1
	.byte -1
	.byte -2
	.byte -2
	.byte -2
	.byte -3
	.byte -3
	.byte -3
	.byte -3
	.byte -4
	.byte -4
	.byte -4
	.byte -5
	.byte -5
	.byte -5
	.byte -6
	.byte -6
	.byte -6
	.byte -6
	.byte -7
	.byte -7
	.byte -7
	.byte -8
	.byte -8
	.byte -8
	.byte -9
	.byte -9
	.byte -9
	.byte -9
	.byte -10
	.byte -10
	.byte -10
	.byte -11
	.byte -11
	.byte -11
	.byte -12
	.byte -12
	.byte -12
	.byte -12
	.byte -13
	.byte -13
	.byte -13
	.byte -14
	.byte -14
	.byte -14
	.byte -15
	.byte -15
	.byte -15
	.byte -15
	.byte -16
	.byte -16
	.byte -16
	.byte -17
	.byte -17
	.byte -17
	.byte -18
	.byte -18
	.byte -18
	.byte -18
	.byte -19
	.byte -19
	.byte -19
	.byte -20
	.byte -20
	.byte -20
	.byte -21
	.byte -21
	.byte -21
	.byte -21
	.byte -22
	.byte -22
	.byte -22
	.byte -23
	.byte -23
	.byte -23
	.byte -24
	.byte -24
	.byte -24
	.byte -25
	.byte -25
	.byte -25
	.byte -25
	.byte -26
	.byte -26
	.byte -26
	.byte -27
	.byte -27
	.byte -27
	.byte -28
	.byte -28
	.byte -28
	.byte -28
	.byte -29
	.byte -29
	.byte -29
	.byte -30
	.byte -30
	.byte -30
	.byte -31
	.byte -31
	.byte -31
	.byte -31
	.byte -32
# ****************************************************************
# TABLE DXX CONTAINS DOUBLE PRECISION POWERS OF TEN
# FROM -38 TO +38
# 20-May-82 / MLC - Most of the negative powers of ten have been
# removed.  Routines which used to multiply by these negative powers of
# ten have been changed to divide by the corresponding positive power
# of ten.  ($FINE and $FOTNV)
# *************************************************************
_DPM01:
	.byte 205
	.byte 204
	.byte 204
	.byte 204
	.byte 204
	.byte 204
	.byte 76
	.byte 125  # 10^-01
_DP00:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 129  # 10^00
_DP01:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 32
	.byte 132  # 10^01
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 72
	.byte 135  # 10^02
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 122
	.byte 138  # 10^03
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 64
	.byte 28
	.byte 142  # 10^04
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 80
	.byte 67
	.byte 145  # 10^05
_DP06:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 36
	.byte 116
	.byte 148  # 10^06
_DP07:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 150
	.byte 24
	.byte 152  # 10^07
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 32
	.byte 188
	.byte 62
	.byte 155  # 10^08
_DP09:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 40
	.byte 107
	.byte 110
	.byte 158  # 10^09
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 249
	.byte 2
	.byte 21
	.byte 162  # 10^10
	.byte 0
	.byte 0
	.byte 0
	.byte 64
	.byte 183
	.byte 67
	.byte 58
	.byte 165  # 10^11
	.byte 0
	.byte 0
	.byte 0
	.byte 16
	.byte 165
	.byte 212
	.byte 104
	.byte 168  # 10^12
	.byte 0
	.byte 0
	.byte 0
	.byte 42
	.byte 231
	.byte 132
	.byte 17
	.byte 172  # 10^13
	.byte 0
	.byte 0
	.byte 128
	.byte 244
	.byte 32
	.byte 230
	.byte 53
	.byte 175  # 10^14
	.byte 0
	.byte 0
	.byte 160
	.byte 49
	.byte 169
	.byte 95
	.byte 99
	.byte 178  # 10^15
_DP16:
	.byte 0
	.byte 0
	.byte 4
	.byte 191
	.byte 201
	.byte 27
	.byte 14
	.byte 182  # 10^16
	.byte 0
	.byte 0
	.byte 197
	.byte 46
	.byte 188
	.byte 162
	.byte 49
	.byte 185  # 10^17
	.byte 0
	.byte 64
	.byte 118
	.byte 58
	.byte 107
	.byte 11
	.byte 94
	.byte 188  # 10^18
	.byte 0
	.byte 232
	.byte 137
	.byte 4
	.byte 35
	.byte 199
	.byte 10
	.byte 192  # 10^19
	.byte 0
	.byte 98
	.byte 172
	.byte 197
	.byte 235
	.byte 120
	.byte 45
	.byte 195  # 10^20
	.byte 128
	.byte 122
	.byte 23
	.byte 183
	.byte 38
	.byte 215
	.byte 88
	.byte 198  # 10^21
	.byte 144
	.byte 172
	.byte 110
	.byte 50
	.byte 120
	.byte 134
	.byte 7
	.byte 202  # 10^22
	.byte 180
	.byte 87
	.byte 10
	.byte 63
	.byte 22
	.byte 104
	.byte 41
	.byte 205  # 10^23
	.byte 161
	.byte 237
	.byte 204
	.byte 206
	.byte 27
	.byte 194
	.byte 83
	.byte 208  # 10^24
	.byte 133
	.byte 20
	.byte 64
	.byte 97
	.byte 81
	.byte 89
	.byte 4
	.byte 212  # 10^25
	.byte 166
	.byte 25
	.byte 144
	.byte 185
	.byte 165
	.byte 111
	.byte 37
	.byte 215  # 10^26
	.byte 15
	.byte 32
	.byte 244
	.byte 39
	.byte 143
	.byte 203
	.byte 78
	.byte 218  # 10^27
	.byte 10
	.byte 148
	.byte 248
	.byte 120
	.byte 57
	.byte 63
	.byte 1
	.byte 222  # 10^28
	.byte 12
	.byte 185
	.byte 54
	.byte 215
	.byte 7
	.byte 143
	.byte 33
	.byte 225  # 10^29
	.byte 79
	.byte 103
	.byte 4
	.byte 205
	.byte 201
	.byte 242
	.byte 73
	.byte 228  # 10^30
	.byte 35
	.byte 129
	.byte 69
	.byte 64
	.byte 124
	.byte 111
	.byte 124
	.byte 231  # 10^31
	.byte 182
	.byte 112
	.byte 43
	.byte 168
	.byte 173
	.byte 197
	.byte 29
	.byte 235  # 10^32
	.byte 227
	.byte 76
	.byte 54
	.byte 18
	.byte 25
	.byte 55
	.byte 69
	.byte 238  # 10^33
	.byte 28
	.byte 224
	.byte 195
	.byte 86
	.byte 223
	.byte 132
	.byte 118
	.byte 241  # 10^34
	.byte 17
	.byte 108
	.byte 58
	.byte 150
	.byte 11
	.byte 19
	.byte 26
	.byte 245  # 10^35
	.byte 22
	.byte 7
	.byte 201
	.byte 123
	.byte 206
	.byte 151
	.byte 64
	.byte 248  # 10^36
	.byte 219
	.byte 72
	.byte 187
	.byte 26
	.byte 194
	.byte 189
	.byte 112
	.byte 251  # 10^37
	.byte 137
	.byte 13
	.byte 181
	.byte 80
	.byte 153
	.byte 118
	.byte 22
	.byte 255  # 10^38
_DHALF:  # DOUBLE PRECISION .5D00
	.byte 0  # .5D00
	.byte 0
	.byte 0
	.byte 0
_SHALF:  # SINGLE PRECISION .5E00
	.byte 0  # .5E00
	.byte 0
	.byte 0
	.byte 128
_SQRH:
	.byte 241  # SQR(.5)
	.byte 4
	.byte 53
	.byte 128
# **********************************************************
# FOR LOG CALCULATIONS HART ALGORITHM 2524 WILL BE USED
# IN THIS ALGORITHM WE WILL CALCULATE BASE 2 LOG AS FOLLOWS
# LOG(X)=P(X)/Q(X)
# ***************************************************************
	.globl _LOGP
_LOGP:
	.byte 4
	.byte 154  # 4.8114746
	.byte 247
	.byte 25
	.byte 131
	.byte 36  # 6.105852
	.byte 99
	.byte 67
	.byte 131
	.byte 117  # -8.86266
	.byte 205
	.byte 141
	.byte 132
	.byte 169  # -2.054667
	.byte 127
	.byte 131
	.byte 130
_LOGQ:
	.byte 4
	.byte 0  # 1.
	.byte 0
	.byte 0
	.byte 129
	.byte 226  # 6.427842
	.byte 176
	.byte 77
	.byte 131
	.byte 10  # 4.545171
	.byte 114
	.byte 17
	.byte 131
	.byte 244  # .3535534
	.byte 4
	.byte 53
	.byte 127
# LOG BASE E OF 2.0
_LN2:
	.byte 24
	.byte 114
	.byte 49
	.byte 128
_FINCH:  # SPECIAL CHARACTERS FOR INPUT/OUTPUT
	.byte 46
	.byte 101  # LOWER CASE "E"
	.byte 69
	.byte 37
	.byte 35
	.byte 33
	.byte 68
	.byte 100  # LOWER CASE "D"
	.byte 44  # COMMA
	.byte 48
_FODTB:
	.byte 0  # 1D15
	.byte 128
	.byte 198
	.byte 164
	.byte 126
	.byte 141
	.byte 3
	.byte 0  # 1D14
	.byte 64
	.byte 122
	.byte 16
	.byte 243
	.byte 90
	.byte 0
	.byte 0  # 1D13
	.byte 160
	.byte 114
	.byte 78
	.byte 24
	.byte 9
	.byte 0
	.byte 0  # 1D12
	.byte 16
	.byte 165
	.byte 212
	.byte 232
	.byte 0
	.byte 0
	.byte 0  # 1D11
	.byte 232
	.byte 118
	.byte 72
	.byte 23
	.byte 0
	.byte 0
	.byte 0  # 1D10
	.byte 228
	.byte 11
	.byte 84
	.byte 2
	.byte 0
	.byte 0
	.byte 0  # 1D9
	.byte 202
	.byte 154
	.byte 59
	.byte 0
	.byte 0
	.byte 0
	.byte 0  # 1D8
	.byte 225
	.byte 245
	.byte 5
	.byte 0
	.byte 0
	.byte 0
	.byte 128  # 1D7
	.byte 150
	.byte 152
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 64  # 1D6
	.byte 66
	.byte 15
	.byte 0
	.byte 0
	.byte 0
	.byte 0
# SINGLE PRECISION POWER OF TEN TABLE
_FOSTB:
	.byte 64  # 1,000,000
	.byte 66
	.byte 15
	.byte 160  # 1E5
	.byte 134
	.byte 1
	.byte 16  # 1E4
	.byte 39
	.byte 0
# INTEGER POWER OF TEN TABLE
_FOITB:
	.byte 16  # 10000
	.byte 39
	.byte 232  # 1000
	.byte 3
	.byte 100  # 100
	.byte 0
	.byte 10  # 10
	.byte 0
	.byte 1  # 1
	.byte 0
_S32KM:
	.byte 0  # SINGLE PRECISION -32768
	.byte 0
	.byte 128
	.byte 144
_INFPD:
	.byte 255  # DOUBLE PRECISION +INFINITY
	.byte 255
	.byte 255
	.byte 255
	.byte 255  # SINGLE PREC. +INFINITY
	.byte 255
	.byte 127
	.byte 255
_INFMD:
	.byte 255  # DOUBLE PRECISION -INFINITY
	.byte 255
	.byte 255
	.byte 255
	.byte 255  # SINGLE PREC. -INFINITY
	.byte 255
	.byte 255
	.byte 255
_LG2E:
	.byte 59  # LOG BASE 2 OF E
	.byte 170
	.byte 56
	.byte 129
# *********************************************************
# $EXPCN CONTAINS THE COEFFICIENTS FOR POLYNOMIAL EVALUATION
# OF LOG BASE 2 OF X WHERE .5.LE.X.LE.1
# THE COEFFICIENTS ARE FROM HART #1302
# ***********************************************************
	.globl _EXPCN
_EXPCN:
	.byte 7  # DEGREE + 1
	.byte 124  # .00020745577403-
	.byte 136
	.byte 89
	.byte 116
	.byte 224  # .00127100574569-
	.byte 151
	.byte 38
	.byte 119
	.byte 196  # .00965065093202+
	.byte 29
	.byte 30
	.byte 122
	.byte 94  # .05549656508324+
	.byte 80
	.byte 99
	.byte 124
	.byte 26  # .24022713817633-
	.byte 254
	.byte 117
	.byte 126
	.byte 24  # .69314717213716+
	.byte 114
	.byte 49
	.byte 128
	.byte 0  # 1.0
	.byte 0
	.byte 0
	.byte 129
_SINCN:
# COEFFICIENTS FOR SINE FUNCTION
# SEE HART'S #3341
# RELATIVE ERROR 8.27
	.byte 5  # NUMBER OF CONSTANTS
	.byte 251  # 39.71091766+
	.byte 215
	.byte 30
	.byte 134
	.byte 101  # -76.57498378-
	.byte 38
	.byte 153
	.byte 135
	.byte 88  # 81.60223119+
	.byte 52
	.byte 35
	.byte 135
	.byte 225  # -41.34167747+
	.byte 93
	.byte 165
	.byte 134
	.byte 219  # 6.283185272+
	.byte 15
	.byte 73
	.byte 131
_ATNC1:  # CONSTANTS FOR ARCTANGENT RANGE REDUCTION
	.byte 2
	.byte 215  # SQR(3)
	.byte 179
	.byte 93
	.byte 129
	.byte 0  # -1.0
	.byte 0
	.byte 128
	.byte 129
_ATNC2:  # HART ALGORITHM 4940 COEFFICIENTS
	.byte 4
	.byte 98  # -.1281333
	.byte 53
	.byte 131
	.byte 126
	.byte 80  # .1993573
	.byte 36
	.byte 76
	.byte 126
	.byte 121  # -.3333324
	.byte 169
	.byte 170
	.byte 127
	.byte 0  # 1.0
	.byte 0
	.byte 0
	.byte 129
# ************************************************************
# THE FOLLOWING CONSTANT LABELED $IN2PI IS USED FOR RANGE
# REDUCTION IN SIN,COS, & TAN. THE LOW ORDER BYTES ARE PURPOSELY
# ZERO TO PROVIDE A MORE ACCURATE REDUCTION OF SINGLE PRECISION
# NUMBERS. THIS CONSTANT SHOULD NOT BE USED WITH ORDINARY DOUBLE
# PRECISION NUMBERS.
# ************************************************************
_IN2PI:
	.byte 11  # 1/(2*PI)
	.byte 68
	.byte 78
	.byte 110
	.byte 131
	.byte 249
	.byte 34
	.byte 126
# ***********************************************************
# CONSTANTS FOR THE RANDOM NUMBER GENERATOR FOLLOW
# DO NOT CHANGE THESE WITHOUT CONSULTING KNUTH VOL 2
# CHAPTER 3 FIRST
# ************************************************************
_RNDA:
	.byte 253  # 214013
	.byte 67
	.byte 3
_RNDC:
	.byte 195  # 2531011
	.byte 158
	.byte 38
_RND0:
	.byte 1
	.byte 0  # 1
	.byte 0
_NUMB:
	.byte 48  # TABLE FOR OCTAL/HEX OUTPUT
	.byte 49
	.byte 50
	.byte 51
	.byte 52
	.byte 53
	.byte 54
	.byte 55
	.byte 56
	.byte 57
	.byte 65
	.byte 66
	.byte 67
	.byte 68
	.byte 69
	.byte 70

	.section .text

# ---------------------------------------------------------------
# $SIN, $COS, $TAN, $ATAN  (Hart polynomial evaluation)
# ---------------------------------------------------------------
_COS:
COS:
	and	BYTE PTR _FACM1, 0x7F		# clear sign bit (cos(-x) = cos(x))
	call	RR				# range reduction, FAC = REM(FAC/2pi)
	call	ONEARG				# zero the accumulator
	mov	BYTE PTR _ARG, 0x7F		# ARG = DP 1/4
	call	_FADDD				# FAC = reduced angle + PI/2
	call	RR1				# make sure we are still in 0 - 2*PI
	jmp	SIN30
_SIN:
SIN:						# uses SIN(-X) = -SIN(X)
	mov	ax, WORD PTR _FAC-1		# fetch sign byte
	cmp	ah, 0167			# IF EXP SMALLER THEN RETURN X (octal 167)
	jnb	SIN10
	ret
SIN10:
	or	al, al
	jns	SIN20
# (FAC) is negative: make positive and push NEG on the stack
	and	al, 0x7F			# clear sign byte
	mov	BYTE PTR _FAC-1, al
	mov	eax, offset _NEG		# address of NEG (4-byte return address)
	push	eax
SIN20:
	call	RR				# DP range reduction
# At this point FAC has the remainder of the angle / 2*PI
SIN30:
	mov	al, BYTE PTR _FAC		# UNSURE: original reads "LOW OFFSET $FAC" (an assembler
	or	al, al				# address constant). Intent is the exponent test on FAC,
	jz	SIN31				# so the exponent byte is used here. See report.
	add	BYTE PTR _FAC, 2		# FAC = FAC*4 = reduced angle / (PI/2)
SIN31:
	call	RR3				# ARG = INT(FAC)
# Quadrant analysis (algorithm comment kept from original):
#   FAC = (angle/(2*PI))*4 ; ARG = INT(FAC) = quadrant 0-3 ; FAC = (FAC-ARG)/4
#   quadrant 1 or 3: FAC = 1 - FAC ; polynomial ; quadrant 2 or 3: negate.
	mov	ax, WORD PTR _ARG-1
	cmp	ah, 0202			# ARG now has quadrant information (octal 202)
	pushfd					# Z set if we need to negate when done
	test	ah, 1				# now set NZ if in quadrant 1
	jnz	SIN40
	test	al, 0100			# set NZ if in quadrant 3 (octal 100)
SIN40:
	pushfd					# NZ if we must subtract from 1
	call	RR2				# FAC = X-INT(X) = angle MOD PI/2
	popfd
	jz	SIN50				# nothing more to do, calculate the SIN
	mov	ebx, offset _DP00		# point to a D.P. 1 (10^0)
	call	_MOVAC				# ARG = 1
	call	_FSUBD				# FAC = 1 - FAC = PI/2 - angle MOD PI/2
SIN50:
	sub	BYTE PTR _FAC, 2		# FAC = FAC/4 = (angle / 2*PI)
	jnb	SIN51
	call	_DZERO				# return 0 if underflow
SIN51:
	call	_CSD				# convert DP reduced angle back to SP
	mov	al, BYTE PTR _FAC		# will see if small enough that X = SIN(X)
	cmp	al, 0164			# if FAC smaller than this just (octal 164)
	jnb	SIN60
	mov	dx, 07733			# 2*PI (octal 7733 = 0x0FDB)
	mov	bx, 0101511			# (octal 101511 = 0x8349) 16-bit value, not an address
	call	_FMULS
	jmp	SIN65				# go set sign properly
SIN60:
	mov	ebx, offset _SINCN		# point to HART 3341 coefficients
	call	_POLYX				# evaluate the polynomial
SIN65:
	popfd					# recall sign
	jnz	SIN70
	xor	BYTE PTR _FAC-1, 0x80		# flip sign
SIN70:
	ret

# Converts FAC to double precision and reduces the result to 0 - 2*PI.
RR:
	mov	ebx, offset _IN2PI		# point to 1/(2*PI)
	call	_MOVAC				# move to ARG
	call	_CDS				# convert to double
	call	_FMULD				# X
RR1:
	call	RR3
RR2:
	call	_FSUBD				# INT(X)-X
	jmp	_NEG				# X-INT(X)
RR3:
	call	_VPSHF				# save X
	call	_DINT				# (FAC) = INT(X)
	call	_MOVAF				# save in ARG
	call	_VPOPF				# fetch X
	ret
ONEARG:
	mov	ebx, offset _DP00
	jmp	_MOVAC

# Tangent
_TAN:
TAN:
	push	WORD PTR _FACM1			# save arg (16-bit values)
	push	WORD PTR _FACLO
	call	SIN				# calculate sine
	pop	dx				# recall argument
	pop	bx
_TAN_END:
# Cotangent-style tangent: TAN = SIN / COS (value pushes are 2 bytes each)
	push	WORD PTR _FACLO			# save SIN(X)
	push	WORD PTR _FACM1
	call	_MOVFR				# move original arg to FAC
	call	COS				# calculate cosine
	pop	bx				# recall SIN(X) (16-bit values)
	pop	dx
	jmp	_FDIVS				# go divide to effect tangent

# ---------------------------------------------------------------
# $ATAN: arctangent of FAC (Hart 4940)
# ---------------------------------------------------------------
_ATN:
ATN:
_ATAN:
	mov	ax, WORD PTR _FAC-1		# want only positive X
	or	al, al
	jns	ATN10
	mov	edi, offset _NEG		# identity: ATAN(X) = -ATAN(-X)
	push	edi				# 4-byte return address
	and	al, 0x7F			# clear sign
	mov	BYTE PTR _FAC-1, al
ATN10:	cmp	ah, 0x81			# see if X > TAN(PI/4) (octal 201)
	jb	ATN20				# if not proceed
	mov	edi, offset ATN100		# will want to form PI/2-(FAC) at end
	push	edi
	xor	dx, dx				# will set (BXDX)=1.0
	mov	bx, 0x8100			# (octal 100400)
	call	_FDIVS				# this puts 1/X in FAC
ATN20:	mov	dx, 0x30A2			# fetch TAN(PI/12) (octal 30242)
	mov	bx, 0x7F09			# (octal 77411)
	call	_FCOMP				# see if larger
	js	ATN30				# if not proceed
	mov	edi, offset ATN200		# will want (FAC)=(FAC)+PI/6 at end
	push	edi
	push	WORD PTR _FACLO		# save X
	push	WORD PTR _FACM1
	mov	dx, 0xB3D7			# fetch SQR(3) (octal 131727)
	mov	bx, 0x815D			# (octal 100535)
	call	_FADDS				# (FAC)=X+SQR(3)
	pop	bx
	pop	dx				# fetch X back
	push	WORD PTR _FACLO		# save X+SQR(3)
	push	WORD PTR _FACM1
	call	_MOVFR				# X to FAC
	mov	ebx, offset _ATNC1
	call	_POLY				# form X*SQR(3)-1
	pop	bx
	pop	dx				# recall X+SQR(3)
	push	WORD PTR _FACLO		# push X*SQR(3)-1 on stack
	push	WORD PTR _FACM1
	call	_MOVFR				# X+SQR(3) to FAC
	pop	bx
	pop	dx				# recall X*SQR(3)-1
	call	_FDIVS				# form (X+SQR(3)-1)/(X+SQR(3))
ATN30:	mov	ebx, offset _ATNC2		# prepare for HART 4940
	jmp	_POLYX
ATN100:	mov	dx, 0x0FDB			# fetch PI/2 (octal 7733)
	mov	bx, 0x8149			# (octal 100511)
	jmp	_FSUBS				# (FAC)=PI/2-(FAC)
ATN200:	mov	dx, 0x0A92			# fetch PI/6 (octal 5222)
	mov	bx, 0x8006			# (octal 100006)
	jmp	_FADDS				# add PI/6 to FAC

# ---------------------------------------------------------------
# $FINE: adjust input number for exponent (multiply/divide by 10^n in D.P.)
# Enter: SI = 0 if positive exponent, all bits set if negative.
#        DX = input exponent; DI = digits to right of decimal point. (16-bit values)
# ---------------------------------------------------------------
_FINE:
	or	si, si				# see if DX should be negated
	jns	FIN05				# if not proceed
	neg	dx				# negate
FIN05:	sub	dx, di				# subtract out digits to right of D.P.
	jo	FIN80				# underflow if overflow flag set
	jz	FIN55				# number complete as is
MDPTEN:
	push	ebx				# preserve text pointer (4-byte)
	call	_GETYP				# get the current type,
	pushfd					# and save, so that we can convert back
	jnb	FIN20				# if already D.P., no conversion necessary
	call	FRCDBL				# force result to be double precision
FIN20:	or	dx, dx				# test sign of exponent
	js	FIN30				# if negative exponent jump
# POSITIVE EXPONENT. If greater than D^38 we have overflow, unless the number is zero.
	test	BYTE PTR _FAC, 0xFF		# is the number zero?
	jz	FIN80				# yes, just return zero then
	cmp	dx, 39
	jb	FIN40				# OK proceed
	popfd					# get back type flags
	jnb	FIN25				# already D.P., nothing to convert
	call	_CSD				# convert double to single
FIN25:	pop	ebx				# restore text pointer
	jmp	_OVFLS				# overflow
# Negative exponent. Might require two divisions since highest table entry is 10^38.
FIN30:	cmp	dx, -38				# will one pass be enough?
	jge	FIN40				# yes
	add	dx, 38				# no, will two divisions get it?
	cmp	dx, -38
	jl	FIN80				# no, underflow: return zero
	call	MDP10				# yes, do the first one
	mov	dx, -38				# then divide by 10^38
FIN40:	call	MDP10				# do the division
FIN45:	popfd					# get back type flags
	jnb	FIN50				# already D.P., nothing to convert
	call	_CSD				# convert double to single
# Number restored to original type (integers converted to single precision).
FIN50:	pop	ebx				# restore text pointer
FIN55:	ret
FIN80:	call	_ZERO				# UNDERFLOW
	jmp	FIN45				# restore proper type

# MDP10: multiply (positive exponent) or divide (negative exponent) by 10^|DX| (D.P. table).
MDP10:
	or	dx, dx				# is the exponent negative?
	pushfd					# remember whether to multiply or divide
	jns	POSEXP				# positive exponent
	neg	dx				# negative exponent: make it positive
POSEXP:	mov	cx, 3				# DX := DX*8 to get offset into power of ten table
	shl	dx, cl
	movzx	edx, dx				# offset is non-negative and < 0x100
	add	edx, offset _DP00		# DX := pointer to power of ten (32-bit)
	xchg	ebx, edx			# move it to EBX
	call	_MOVAC				# move D.P. no. to ARG
	popfd					# divide?
	js	DBLDIV				# yes
	jmp	_FMULD				# no, multiply and return to caller
DBLDIV:	jmp	DDIVFA				# double precision divide: FAC=FAC/ARG

# ---------------------------------------------------------------
# $OVFLS / $DIV0S: overflow and divide-by-zero
# ---------------------------------------------------------------
_OVFLS:
	push	ebx				# (4-byte pointer)
	call	OVFLS				# do the overflow code
	pop	ebx				# restore text pointer
	ret
OVFLS:	call	DOINF				# put in appropriate infinity
	mov	eax, offset _OVERR		# error trapping routine address (4 bytes)
	mov	ebx, offset _OVMSG		# address appropriate error message
	jmp	DV010				# go print message
_DIV0S:
	push	ebx				# save text pointer
	call	DIV0S				# do the divide overflow code
	pop	ebx
	ret					# return with text pointer saved
DIV0S:	call	DOINF				# put in appropriate infinity
	mov	eax, offset DV0ERR		# error trapping routine address
	mov	ebx, offset _DIV0M		# and error message
DV010:	call	TRAPER				# trap error if error trapping enabled
	cmp	BYTE PTR _FLGOV, 1		# if 0,1 print; if 1 make 2
	js	DV015				# print if 0
	jnz	DV020				# don't print if 2
	mov	BYTE PTR _FLGOV, 2		# make 2 if 1
DV015:	call	_STPRN
	mov	al, 13				# carriage return (original LOW 15 is octal = 13)
	call	_CATTY				# send it
	mov	al, 10				# line feed (original LOW 12 is octal = 10)
	call	_CATTY				# send it
DV020:	ret

# DOINF: put the correct infinity in the FAC.
DOINF:
	cld					# clear the direction flag
	or	bh, bh				# see if zero to a neg power (literal original test of BH)
	mov	esi, offset _INFPD		# assume positive
	jz	INF10				# positive infinity for zero^-X
	test	BYTE PTR _FAC+1, 0x80		# negative?
	jns	INF10				# if not proceed
	mov	esi, offset _INFMD		# negative infinity
INF10:	call	_GETYP				# set codes according to type
	jb	INF20
# double precision
	mov	edi, offset _DFACL		# receipient
	mov	ecx, 4				# 4 words for double precision
	jmp	INF30
INF20:					# single precision
	add	esi, 4				# so we point to single precision no.
	mov	edi, offset _FACLO
	mov	ecx, 2				# 2 words for single
INF30:					# must fetch from code segment (flat here)
	movsw					# to FAC
	loop	INF30
	ret

# TRAPER: if ON ERROR trapping is enabled (ONELIN != 0) return into the trap routine
# whose address is in EAX (4-byte return address pushed here).
TRAPER:	push	eax				# save error trapping routine address
	mov	eax, DWORD PTR ONELIN		# (ONELIN is a 4-byte line pointer; see NOTES)
	or	eax, eax			# see if we are trapping errors
	jnz	TRAPIT				# branch if errors are being trapped
	pop	eax				# discard error trapping routine address
TRAPIT:	ret					# return to caller or error trap routine

# $FOFMT: format the number in the FAC (fixed or scientific). Enter with BX = output pointer,
# condition codes as if $GETYP was just called. Stack: BX ptr (4), CX (2), PUSHF (4), AX (2).
	.globl _FOFMT
_FOFMT:
	push	ebx				# save buffer pointer
	mov	al, 8				# assume single precision: print 7 digits (LOW 8D)
	jb	FFM10				# jump if single precision
	mov	al, 17				# double precision possible digits (LOW 17D)
FFM10:	mov	ch, al				# will save a second copy for later
	mov	cl, al
	push	cx				# save for later
	pushfd					# save type
	call	_FOTNV				# bracket the number so significant digits are in the
					# integer part and the necessary exponent is in AL
	or	al, al				# if positive don't call SIGD
	jz	FFM11
	jns	FFM15
FFM11:	popfd					# recall type for test to see if SIGD called.
	pop	cx				# recall possible significant digits
	push	ax				# save exp.
	jpo	FFM16				# single precision
	add	al, 16				# this must make positive to not call
	pop	ax
	jns	FFM18				# pop AX and do fixed output
	jmp	FFM17				# must call SIGD
FFM15:	popfd					# recall type flag
	pop	cx				# recall CX
	jmp	FFM20
FFM16:	add	al, 7
	pop	ax
	jns	FFM18
FFM17:	push	ax				# save necessary exponent in AL
	call	_SIGD				# increment CL for each zero digit from the right
	pop	ax				# recall necessary exponent
	mov	ah, al				# will perform first test on AH
	add	ah, cl				# if SF=1 after this use "E" format
	jle	FFM20				# jump if too small
	add	ch, al
	jmp	FFM19
FFM18:	add	al, ch
	inc	ch
	cmp	ch, al
	mov	ch, 3				# to flag "E" format if necessary (octal 3 = 3)
	jb	FFM30
	mov	ch, al
FFM19:	inc	ch				# (CH) = number digits + 2
	mov	al, 2				# this will cause ZF=1 in later calc and signal fixed output
	jmp	FFM30
FFM20:	add	al, ch
	mov	ch, 3				# signals "E" type format
FFM30:	dec	al				# (AL) 2 too large
	dec	al
	pop	ebx				# recall buffer pointer (4-byte)
	push	ax				# save exponent
	pushfd					# save condition codes
	xor	cl, cl				# (CL)=0 (no commas after each 3 digits)
	call	_FOTAN				# put in decimal point and leading zeros as necessary
	mov	BYTE PTR [ebx], '0'		# ZF=1 if this "0" necessary
	jnz	FFM40
	inc	ebx
FFM40:	call	_FOTCV				# convert the FAC to decimal digits and put in D.P. as necessary
FFM50:	dec	ebx				# now see if there are trailing zeros to suppress
	cmp	BYTE PTR [ebx], '0'		# we know zeros found here are trailing because $FOTCV
	jz	FFM50				# puts in the decimal point
	cmp	BYTE PTR [ebx], '.'		# can we delete the decimal point also?
	jz	FFM60
	inc	ebx
FFM60:	popfd					# let's have a look at the possible exponent
	pop	ax
	jz	FFM100				# if no exponent just clean up and return
_PUEXP:
	pushfd					# save SF (if 1 negative exponent)
	push	ax				# save exponent
	call	_GETYP				# set cond. codes for type number
	mov	ah, 'E'				# assume single precision
	jpo	FFM70				# if S.P. proceed
	mov	ah, 'D'				# it's double precision
FFM70:	mov	BYTE PTR [ebx], ah		# put in exponent designator
	inc	ebx				# point to next buffer position
	pop	ax				# recall exponent
	popfd					# now see if positive or negative
	mov	BYTE PTR [ebx], '+'		# assume positive exponent
	jns	FFM80				# jump if so
	mov	BYTE PTR [ebx], '-'
	neg	al				# must make exponent positive
FFM80:	mov	ah, '0'-1			# will build digit in (AL)
FFM90:	inc	ah				# increment to next digit
	sub	al, 10				# (LOW 10D)
	jnb	FFM90
	add	al, '0'+10			# add to determine second digit
	inc	ebx				# point to next buffer position
	xchg	al, ah				# prepare for word store
	mov	WORD PTR [ebx], ax		# store 2 digits of exponent
	inc	ebx				# increment to next buffer position
	inc	ebx
FFM100:	mov	BYTE PTR [ebx], 0		# signal end of buffer data
	xchg	ebx, ecx			# end-of-buffer ptr in ECX
	mov	ebx, offset _FBUFF+1		# start in EBX
	ret

# ---------------------------------------------------------------
# Utilities
# ---------------------------------------------------------------
# STROUT: print the string (NUL-terminated) at EBX. Calls OUTDO for each char.
	.globl _STROUT
_STROUT:
STROUT:						# print the NUL-terminated string at EBX (internal messages)
STOULP:	mov	al, BYTE PTR [ebx]		# get char (loaded BEFORE output: OUTDO sends AL)
	inc	ebx				# point at next one
	or	al, al				# end of string?
	jz	STROUT_END			# yes
	cmp	al, 255				# flag byte (e.g. after "Ok": not user input): not printed
	je	STOULP
	call	OUTDO				# send char (OUTDO preserves AL and EBX)
	cmp	al, 13				# carriage return? (LOW 13D)
	jnz	STOULP				# no
	call	CRFIN				# force carriage back to left
	jmp	STOULP
STROUT_END:
	ret

# $DZERO: zero the double precision FAC
_DZERO:
	mov	edi, offset _DFACL
	mov	ecx, 4
	mov	ax, 0
	cld
	rep	stosw
	ret

# $ZERO: zero the FAC
_ZERO:
ZERO:
_ZERO_:
	mov	ax, 0
	mov	WORD PTR _FACLO, ax		# zero lower 2 bytes
	mov	WORD PTR _FAC-1, ax		# zero upper 2 bytes
	ret

# $SIGNS: sign of FAC. ZF=1 if FAC=0, SF=1 if FAC<0, neither if FAC>0.
SIGN:
_SIGNS:
	call	_GETYP				# if not integer can look at FAC:FAC-1
	jne	SIS01
	jmp	TMERR				# strings illegal
SIS01:	jns	SIS05				# not integer: proceed
	mov	ax, WORD PTR _FACLO		# fetch integer
	or	ax, ax				# determine sign
	jz	SIS10
	mov	al, 1
	jns	SIS10
	neg	al
	ret
SIS05:	mov	al, BYTE PTR _FAC		# first check for zero
	or	al, al
	jz	SIS10				# if zero just return
	mov	al, BYTE PTR _FAC-1		# fetch sign byte
SIGNAL:	or	al, al				# signset now
	jz	SIS07				# must make AL=1 and ZF=0
	mov	al, 1
	jns	SIS10
	neg	al
	ret
SIS07:	or	al, 1				# know positive non-zero
SIS10:	ret

# $GETYP: set condition codes according to the type value (see original comments):
#   S,C = integer; Z,C = string; C = single precision; none = double precision.
#   ZF=1 if string, SF=1 if integer, PO=1 if single.
_GETYP:
GETYP:
	mov	al, BYTE PTR _VALTP		# fetch type variable
	cmp	al, 8				# CF=1 except for double precision (LOW 10 octal = 8)
	dec	al				# will subtract 3 with decrements
	dec	al
	dec	al
	ret

# $VADDH: add .5 according to type
_VADDH:
	call	_GETYP				# set codes according to type
	jb	_FADDH
# $DADDH: double precision add .5
_DADDH:
	push	ebx
	mov	ebx, offset _DHALF
	call	_MOVAC				# move to ARG
	call	_FADDD				# add to FAC
	pop	ebx
	ret
# $FADDH: single precision add .5
_FADDH:
	xor	dx, dx				# will set (BXDX)=.5
	mov	bx, 0x8000			# (octal 100000)
	call	_FADDS				# add .5 to FAC
	ret

# $DIV10: divide FAC by 10. $MUL10: multiply FAC by 10.
_DIV10:
	call	_GETYP
	mov	ebx, offset _DPM01		# address .1
	jb	ML10
	jmp	ML05				# multiply
_MUL10:
	call	_GETYP				# see if single or double precision
	mov	ebx, offset _DP01		# address of double precision 10.
	jb	ML10				# jump if single
ML05:	call	_MOVAC				# move 10. to ARG
	call	_FMULD				# multiply
	ret
# ML10: multiply single precision FAC by number pointed to by EBX
ML10:
_MLSP:
	push	WORD PTR _FACM1			# push FAC on the stack
	push	WORD PTR _FACLO
	mov	BYTE PTR _VALTP, 8		# set up D.P. (LOW 10 octal = 8)
	call	_MOVFC				# move double precision 10 to FAC
	call	_CSD				# convert to single precision
	pop	dx				# get low mantissa bits (16-bit values)
	pop	bx				# exp sign and high mantissa bits
	call	_FMULS				# multiply
	ret

# $RDL: rotate left the double precision FAC by one bit (4 words). EBX = address.
_RDL:
	mov	ecx, 4				# will shift 4 words left
RDL120:	rcl	WORD PTR [ebx], 1		# shift left 1 bit shifting in CF
	inc	ebx
	inc	ebx
	loop	RDL120
	ret

# $SHDR: double precision right shift. $SHRM: shift number at EBX, ECX words, right through CF.
_SHDR:
	mov	ecx, 4				# double precision right shift
_SHRM:
	rcr	WORD PTR [ebx], 1		# shift 1 word right through CF
	dec	ebx
	dec	ebx				# to next word
	loop	_SHRM				# do this ECX times
	ret

# SHST: OR the sticky bit into the FAC's low byte, then continue shifting.
SHST:	or	BYTE PTR [ebx+2], 0x20		# "OR" in ST bit (LOW 40 octal = 0x20)
	loop	_SHRA
	ret

# $SHRA: shift $ARG right by ECX bits (sticky bit kept in the bit above the mantissa).
_SHRA:
	mov	ebx, offset _ARG-2
	cmp	cl, 8				# can I do a byte move? (LOW 10 octal = 8)
	jb	SHRA30				# if not proceed as normal
# For speed, a byte move to the right (7 bytes plus overflow byte).
	push	cx				# save bits to shift (16-bit)
	mov	ecx, 7				# 7 byte move
	mov	ebx, offset _ARGLO-1
	mov	ah, BYTE PTR [ebx]		# fetch overflow byte
SHRA11:	mov	al, BYTE PTR [ebx+1]
	mov	BYTE PTR [ebx], al
	inc	ebx
	loop	SHRA11
	xor	al, al
	mov	BYTE PTR [ebx], al
	pop	cx				# recall bit count (16-bit)
	movzx	ecx, cx
	sub	cl, 8				# (LOW 10 octal = 8)
	and	ah, 0x20			# will need to re-establish ST (LOW 40 octal)
	jz	_SHRA				# no-ST just proceed
	or	BYTE PTR _ARGLO-1, ah
	jmp	_SHRA
SHRA30:
	or	cl, cl
	jz	SHRA40				# jump if done
	push	cx				# save no. bits to shift (16-bit)
	clc					# don't want the carry shifted in
	call	_SHDR
	pop	cx
	movzx	ecx, cx
	test	BYTE PTR [ebx+2], 0x10		# see if shifted through "ST" (LOW 20 octal)
	jnz	SHST				# must "OR" ST bit in if non-zero
	loop	_SHRA
SHRA40:	ret

# $XCGAF: exchange FAC and ARG (double precision)
_XCGAF:
	mov	esi, offset _DFACL		# will exchange FAC and ARG (D.P.)
	mov	edi, offset _ARGLO
	cld					# so MOVSW will increment indices
	mov	ecx, 4				# will move 4 words (8 bytes)
XCG10:	mov	ax, WORD PTR [edi]		# fetch destination word
	movsw					# move FAC to ARG & increment indices
	mov	WORD PTR [esi-2], ax		# ARG to FAC
	loop	XCG10				# continue
	ret

# ---------------------------------------------------------------
# Special move routines for constants from the code segment (flat here).
# ---------------------------------------------------------------
# $MOVBS: single precision constant at EBX -> $DBUFF
_MOVBS:
	mov	edi, offset _DBUFF+4
	mov	ecx, 2
	jmp	MBF10
# $MOVBF: double precision constant at EBX -> $DBUFF
_MOVBF:
	mov	edi, offset _DBUFF
MBF05:	mov	ecx, 4
MBF10:	cld
	mov	esi, ebx			# so we can use LODSW
MBF20:	lodsw					# fetch constant word
	stosw					# store in DBUFF
	loop	MBF20				# continue with move
	mov	ebx, edi			# update EBX
	sub	ebx, 2				# get pointer correct
	ret
# $MOVAC: double precision constant at EBX -> ARG
_MOVAC:
	mov	edi, offset _ARGLO
	jmp	MBF05
# $MOVFC: double precision constant at EBX -> FAC
_MOVFC:
	mov	edi, offset _DFACL
	jmp	MBF05
# MOVAM: double precision number at EBX -> ARG
MOVAM:
	mov	edi, offset _ARGLO		# low address of ARG
	mov	ecx, 4				# move double prec no. pointed to by EBX to no.
						# pointed to by EDI
# MOVEM: move the number pointed to by EBX to EDI for ECX words
MOVEM:
	xchg	ebx, esi			# so MOVSW can be used
	cld					# so MOVSW will increment
	rep	movsw				# move "from" to "to"
	xchg	ebx, esi			# get registers straight
	ret
# $MOVAF: move FAC to ARG (saves CX (2 bytes), BX and DI (4 bytes each))
_MOVAF:
	push	cx
	push	ebx
	push	edi
	mov	ebx, offset _DFACL		# "from" address
	mov	edi, offset _ARGLO		# "to" address
MAF05:	mov	ecx, 4				# 4 16-bit quantities to move
	call	MOVEM				# do the move
	pop	edi
	pop	ebx
	pop	cx				# registers restored
	movzx	ecx, cx
	ret
# $MOVFA: move ARG to FAC (same save/restore pattern as $MOVAF)
_MOVFA:
	push	cx
	push	ebx
	push	edi
	mov	ebx, offset _ARGLO		# "from" address
	mov	edi, offset _DFACL		# "to" address
	jmp	MAF05

# $MOVFR: move (BXDX) to FAC (16-bit values)
_MOVFR:
	mov	WORD PTR _FACLO, dx
	mov	WORD PTR _FAC-1, bx
	ret
# $MOVRF: move FAC into registers (BXDX)
_MOVRF:
	mov	dx, WORD PTR _FACLO
	mov	bx, WORD PTR _FAC-1
	ret

# ---------------------------------------------------------------
# Comparison routines. BX and DI are saved as 4-byte pointers throughout this block
# (CPM80 pops them as 4 bytes). CX is saved as 2 bytes where it is used for loop control.
# ---------------------------------------------------------------
# ZCMPCK: zero-exponent special cases. Entry: BH = exponent of comperand, BL bit 7 = its sign.
ZCMPCK:
	or	bh, bh				# is the comperand zero?
	jnz	NZCOMP				# no
	call	SIS05				# yes, result is the sign of the FAC
	stc					# set carry to say comparison is done
	ret
NZCOMP:	mov	al, BYTE PTR _FAC		# is the FAC zero?
	or	al, al				# (also clears carry)
	jnz	ZCMPRT				# no: return with carry clear
	mov	al, bl				# yes: opposite of the sign of the comperand
	not	al
	call	SIGNAL
	stc					# comparison finished
ZCMPRT:	ret

# ZCMPBX: zero check with BX pointing to the comperand.
ZCMPBX:
	push	ebx				# preserve the comperand pointer
	mov	bx, WORD PTR [ebx]		# BH = exponent, BL bit 7 = sign
	call	ZCMPCK				# check for the zero cases
	pop	ebx				# retrieve the comperand pointer
	ret

# $VCOMP: compare FAC with (BXDX) type-directed (the caller sets the type)
_VCOMP:
	call	_GETYP				# set codes according to type
	jb	_COMPM				# single precision compare
	jmp	_DCMPM				# double precision compare

# $FCOMP: single precision compare of FAC and (BXDX).
#   AL=1 & ZF=0,SF=0,CF=0 if FAC > (BXDX); AL=0 & ZF=1 if equal; AL=377 & SF=1,CF=1 if less.
_FCOMP:
	call	ZCMPCK				# check for zero exponent cases
	jb	ZEREXP				# all done if carry set
	push	ebx
	push	edi
	mov	al, bl				# sign of (BXDX) to AL
	xor	al, BYTE PTR _FAC-1		# set codes for signs
	js	CPM05
	or	bl, bl
	js	UFC10				# negative nos.
	mov	ax, WORD PTR _FAC-1		# signs same: must compare
	sub	ax, bx				# subtract high word
	jb	CPM10				# FAC smaller
	jnz	CPM06				# FAC larger
	mov	ax, WORD PTR _FACLO		# must compare low word
	sub	ax, dx
	jmp	TSTFLG				# return comparison result based on flags
UFC10:	mov	ax, bx				# since negative, larger magnitude is smaller no.
	sub	ax, WORD PTR _FAC-1
	jb	CPM10
	jnz	CPM06				# (BXDX) smaller
	mov	ax, dx				# must compare low word
	sub	ax, WORD PTR _FACLO
TSTFLG:	jb	CPM10				# FAC smaller than comperand
	jnz	CPM06				# FAC larger than comperand
	xor	al, al				# FAC equal to comperand
	jmp	CPM80
# One of the exponents was zero (the number is zero). Map AL from (377,0,1) to (0,1,2)
ZEREXP:	inc	al
	sub	al, 1				# map it back, setting flags appropriately
	ret

# $COMPM: compare FAC to the number pointed to by EBX (EBX points at the sign byte).
_COMPM:
	call	ZCMPBX				# check for zero exponent cases
	jb	ZEREXP				# all done if carry set
	push	ebx				# save initial address
	push	edi				# save EDI
	mov	edi, offset _FAC-1
	mov	ax, WORD PTR [ebx]		# fetch sign byte into AL
	xor	al, BYTE PTR _FAC-1		# SF=1 if signs different
	jns	CPM20
# SIGNS ARE DIFFERENT: the one with the positive sign is bigger.
CPM05:	mov	ah, BYTE PTR _FAC-1		# fetch FAC sign
	or	ah, ah
	js	CPM10				# FAC smaller: must set AL=377
CPM06:	mov	al, 1
	or	al, al				# might have got ZF=1 on last "or"
	jmp	CPM80
CPM10:	mov	al, 0xFF			# know SF=1 (LOW 377 octal)
	stc					# want CF=1 if FAC smaller
	jmp	CPM80
CPM20:
# SIGNS ARE THE SAME: must do the actual compare.
	push	cx				# will use CX for loop control (2 bytes)
	mov	ecx, 2				# (2 words)
CPM22:	xchg	ebx, esi			# so CMPSW can be used
	mov	al, BYTE PTR _FAC-1		# if negative no. need to reverse compare
	or	al, al
	jns	CPM25
	xchg	esi, edi			# causes correct compare for negative
CPM25:	std					# so CMPSW will decrement
CPM30:	cmpsw					# form codes for (.SI)-(.DI)
	jnz	CPM50
	loop	CPM30				# continue through values
	mov	al, 0				# ZF=1 already set
	jmp	CPM70
CPM50:	jnb	CPM60				# memory value larger
	mov	al, 1				# FAC larger: SF=0, ZF=0
	or	al, al
	jmp	CPM70
CPM60:					# memory is larger: must set SF=1 and AL=377
	mov	al, 0xFF
	or	al, al
	stc					# want CF=1 if memory value larger
CPM70:	pop	cx				# restore CX register
	movzx	ecx, cx
CPM80:	pop	edi
	pop	ebx
	ret

# $DCMPA: double precision compare of FAC and ARG
_DCMPA:
	mov	ebx, offset _ARG-1
# $DCMPM: double precision compare of FAC and the number at (EBX)
_DCMPM:
	call	ZCMPBX				# check for zero exponent cases
	jb	ZEREXP				# all done if carry set
	push	ebx				# save registers
	push	edi
	mov	edi, offset _FAC-1		# D.P. high FAC word
	mov	al, BYTE PTR [edi]		# will first see if signs different
	xor	al, BYTE PTR [ebx]
	jns	DC10				# proceed if not different
	jmp	CPM05				# signs different: set codes accordingly
DC10:	push	cx				# (2 bytes)
	mov	ecx, 4				# four words to compare
	jmp	CPM22				# go compare

# $CONI2: check FAC for -32768 (single precision) and if equal convert to integer
_CONI2:
	mov	ebx, offset _S32KM		# address of S.P. -32768
	call	_MOVBS				# move it out to DBUFF for comparison
	call	_COMPM				# only interested in ZF
	jnz	CON10				# if not equal just return
	mov	BYTE PTR _VALTP, 2		# to indicate integer
	mov	WORD PTR _FACLO, 0x8000		# -32768 (OFFSET 200*400 octal)
CON10:	ret

# $RSUBM: subtract the number pointed to by EBP (code segment constant) from (BLDX)
_RSUBM:
	sub	dx, WORD PTR [ebp]		# first 16 bits
	sbb	bl, BYTE PTR [ebp+2]
	ret					# cond. flags all set

# ---------------------------------------------------------------
# V utilities: type-directed operations on the FAC / ARG / buffers.
# NOTE: VPSHF/VPOPF move the return address off the stack. Return addresses are
# 4 bytes in 32-bit code, so they are popped/pushed as 4 bytes (EBP/EAX); the saved data
# words stay 2 bytes each.
# ---------------------------------------------------------------
# ABS
_ABSFN:
ABSFN:
_VABS:
	call	_GETYP				# see what type variable
	js	_IABS				# if integer proceed
	mov	al, BYTE PTR _FAC-1
	or	al, al				# see if currently negative
	js	_NEG				# and if so negate
	ret
_IABS:	mov	ax, WORD PTR _FACLO		# fetch integer
	or	ax, ax				# and see if already positive
	js	VN15
	ret					# return if positive
# NEG
_VNEG:
VNEG:
	call	_GETYP				# set cond. codes according to type
	js	_INEG				# integer: must complement
# single or double: flip the sign bit
_NEG:
NEG:
	xor	BYTE PTR _FAC-1, 0x80		# flip sign of FAC (LOW 200 octal)
	ret
_INEG:
INEG:
	mov	ax, WORD PTR _FACLO		# will check for -32768
VN15:	cmp	ax, 0x8000			# see if -32768 (octal 100000)
	jnz	VN20				# if not just negate
	push	ebx				# preserve BX (text pointer)
	call	_CSI				# convert to single precision
	pop	ebx				# recall BX
	jmp	_NEG				# go negate
VN20:	neg	WORD PTR _FACLO			# negate
	ret

# $SETDB: move FAC to DBUFF for multiply and zero the FAC
_SETDB:
	mov	ebx, offset _DBUFF+1
	call	_VMVMF				# move FAC to DBUFF
	mov	edi, offset _DFACL-8		# will now zero 16 bytes of FAC
	mov	ecx, 8 
	mov	ax, 0
	cld
	rep	stosw				# stores AX into the locations
	mov	BYTE PTR _DBUFF, al		# zero overflow byte
	mov	BYTE PTR _ARGLO-1, al		# zero overflow byte of ARG
	ret

# $VMVAM: value at EBX -> (BXDX) if single, -> ARG if double
_VMVAM:
	call	_GETYP				# set codes according to type
	jb	VM10				# single precision
	jmp	MOVAM				# move the value to the ARG
VM10:	mov	dx, WORD PTR [ebx]		# fetch low mantissa bits
	mov	bx, WORD PTR [ebx+2]		# fetch high mantissa and exponent
	ret

# $VMOVM: variable at EBX -> variable at EDI (double 4 words, single 2 words)
_VMOVM:
	mov	ecx, 4				# assume double precision
	call	_GETYP
	jb	VMM10				# single precision
	jmp	MOVEM				# go do move
VMM10:	mov	ecx, 2
	jmp	MOVEM

# $VMVMF: move FAC to the number pointed to by EBX
_VMVMF:
	mov	ecx, 4				# assume double prec.
	xchg	edi, ebx			# EDI = destination, EBX = origin
	mov	ebx, offset _DFACL
	call	_GETYP				# set cond. codes according to VALTP
	jb	VMVM1				# single precision
	jmp	MOVEM				# go do move
VMVM1:	xchg	ebx, edi
_MOVMF:
	mov	ecx, 2
	mov	edi, offset _FACLO
	xchg	edi, ebx
	jmp	MOVEM				# go do move

# $VMVFM: move the number pointed to by EBX to the FAC
_VMVFM:
	mov	ecx, 4				# assume double prec
	mov	edi, offset _DFACL
	call	_GETYP
	jb	_MOVFM
	jmp	MOVEM
_MOVFM:
	mov	ecx, 2
	mov	edi, offset _FACLO
	jmp	MOVEM				# go do move

# $VCMPM: compare FAC to the number pointed to by EBX
_VCMPM:
	call	_GETYP
	jb	VCMP10
	jmp	_DCMPM				# double precision compare
VCMP10:	jmp	_COMPM				# single precision compare

# $VPSHF: push the FAC on the stack according to type (return address kept on top)
_VPSHF:
	call	_GETYP				# set cond codes according to VALTP
	mov	ecx, 4				# for double precision
	jnb	VPS10				# if double precision proceed
_PUSHF:						# push single precision FAC on the stack
	mov	ecx, 2
VPS10:	pop	ebp				# get return address off stack (4 bytes)
	mov	edi, offset _FAC-1
VPS20:	push	WORD PTR [edi]
	dec	edi
	dec	edi
	loop	VPS20
	push	ebp				# put return address back (4 bytes)
	ret					# and return

# $POPA: pop the stack into ARG (4 words)
_POPA:
	mov	edi, offset _ARGLO
	mov	ecx, 4
	jmp	VPO17

# $VPOPF: pop either 4 or 8 bytes off the stack into the FAC
_VPOPF:
	call	_GETYP				# set codes according to VALTP
	mov	edi, offset _DFACL
	mov	ecx, 4
	jnb	VPO17				# jump if double precision
	mov	edi, offset _FACLO
	mov	ecx, 2
VPO17:	pop	eax				# get return address off stack (4 bytes)
VPO20:	pop	WORD PTR [edi]
	inc	edi
	inc	edi
	loop	VPO20
	push	eax				# push return address on stack
	ret

# $VINT: INT according to type
_VINT:
VINT:
	call	_GETYP				# set codes according to type
	jns	VIN05
	ret					# return if integer already
VIN05:	jb	VIN10				# single precision
	jmp	_DINT				# double precision
VIN10:	jmp	_INT

# ---------------------------------------------------------------
# $FIN: numeric input from text at EBX (BX is the BASIC text pointer).
# Stack: return address to $FLGOC (4 bytes), PUSHF (4), CX (2), EBX (4), DI (2).
# ---------------------------------------------------------------
_FIN:
FIN:
	xor	al, al				# let input number determine type
	jmp	FN20
_FINDBL:
FINDBL:
_FINDB:						# force double precision entry point
	mov	al, 1				# flag to indicate D.P.
	mov	BYTE PTR _VALTP, 8		# set to double precision (LOW 10 octal = 8)
FN20:
	mov	BYTE PTR _FLGOV, 1		# set once-only overflow mode
	mov	esi, offset _FLGOC		# address of input overflow code
	push	esi				# push on stack so return is here (4-byte)
	xor	di, di				# digits past decimal point
	mov	cx, di				# decimal point flag
	mov	si, di				# (SI) will flag pos/neg exponent
	not	cx				# set all bits
	push	ax
	call	_ZERO				# (FAC)=0
	pop	ax
	or	al, al				# must see if VALTP should start at integer or not
	jnz	FN30				# (not zero)
	mov	BYTE PTR _VALTP, 2		# set to integer
FN30:	mov	al, BYTE PTR [ebx]		# fetch first character from text
	cmp	al, '&'				# signifies special input number
	jnz	FN40				# if not, proceed
	jmp	_OHCNS				# otherwise go process oct. or hex
FN40:	cmp	al, '-'				# negative number?
	pushfd					# will save ZF for possible negation
	jz	FN50				# once number is formed positive
	cmp	al, '+'				# need to advance text pointer for leading sign
	jz	FN50
	dec	ebx
FN50:	call	_CHRGT				# get next character of number
	jnb	FN60				# if not digit go examine further
FN55:	call	_FIDIG				# multiply FAC by ten and add in the digit
	jmp	FN50
FN60:
	mov	ebp, offset _FINCH+7
	xor	dx, dx				# must set exponent to zero
	mov	si, dx				# for subsequent call to FINE
FN70:
	cmp	al, BYTE PTR [ebp]		# see if special character (table in code segment)
	jz	FN80				# found special char (., D, D+40, !, #, %, E, E+40)
	cmp	ebp, offset _FINCH		# if equal none of the special chars.
	jz	FN96				# go finish up: non-recognised char
	dec	ebp				# proceed to next character
	jmp	FN70				# if not finished look at next char
FN80:	sub	ebp, offset _FINCH
	shl	ebp, 2				# branch table entries are 4 bytes each
FN90:
	jmp	DWORD PTR FN95[ebp]		# dispatch through address table
FN95:
	.long	FN100
	.long	FN200
	.long	FN200
	.long	FN400
	.long	FN500
	.long	FN600
	.long	FN92
	.long	FN92
FN92:						# it was a "D"
	xor	al, al				# set condition codes correctly
FN94:	call	_FINEX				# calculate exponent (DX, SI); BX is on the character after it
	cmp	BYTE PTR [ebx], '#'		# a "#" after an exponent, as in 1E3#, makes the number double
	jnz	FN96				# precision (_FIND's force, done here so the exponent is kept)
	push	ebx
	push	di
	push	dx
	push	si
	call	_FD				# force FAC to double precision (keeps the exponent)
	pop	si
	pop	dx
	pop	di
	pop	ebx
	inc	ebx				# eat the "#"
FN96:	call	_FINE				# modify number to reflect exponent
	jmp	FINF				# clean up, negate as necessary
FN100:						# it was a "."
	inc	cx				# to denote decimal point detected
	jnz	FN96				# go finish up: no. 2ed decimal point seen!
	call	_GETYP				# if sng or double just return
	jns	FN50
	push	cx
	push	ebx
	push	di
	call	_CSI				# convert to single precision
	pop	di
	pop	ebx
	pop	cx
	jmp	FN50				# go process next char
FN200:						# it was an "E" or "E"+40 (lower case E)
	call	_CHKEL				# make sure we don't have an "ELSE"
	jz	FN96				# it was an "ELSE" or "EQV"
	jmp	FN94				# go determine exponent
FN400:						# it was a "%" (integer)
	inc	ebx				# eat the percent sign
	jmp	FN96				# call FINE since could be working on floating point
FN500:						# it was a "#" (double precision)
	call	_FIND
	jmp	FINF				# go finish up
FN600:						# it was a "!" (single precision)
	xor	al, al				# flag as single
	call	_FINS				# force to single precision
FINF:	popfd					# recall sign flag
	jnz	FN990				# return if not negative no.
	call	_VNEG				# negate number
	call	_GETYP				# set condition codes according to type
	jpe	FN990				# if not single precision return
	push	ebx				# save text pointer
	call	_CONI2				# check for -32768 and convert to int.
	pop	ebx				# recall text pointer
FN990:	ret					# (IF FOUND)

# ---------------------------------------------------------------
# $SQR: square root of single precision FAC. All values are 16-bit register arithmetic.
# ---------------------------------------------------------------
_SQR:
SQR:
	mov	dx, WORD PTR _FAC-2
	or	dh, dh				# test sign bit
	js	NEGNUM
	mov	al, BYTE PTR _FAC		# get exponent
	or	al, al
	jz	DONE				# SQR(0)=0
	mov	bh, BYTE PTR _FAC-3
	mov	bl, 0
	or	dh, 128				# set implied bit (LOW 128D)
	shr	al, 1				# divide exponent by two
	jnb	EVENEX
	shr	dx, 1				# half mantissa if exp odd
	rcr	bx, 1
	stc					# restore carry for round up
EVENEX:
	adc	al, 0x40			# add back 1/2 of bias (LOW 100 octal = 0x40)
	mov	BYTE PTR _FAC, al		# save final exponent
	mov	ecx, 25				# 24 bits plus 1 for rounding (25D)
	xor	bp, bp				# clear some registers
	mov	si, bp
	mov	di, bp
	mov	ax, bp
RTLP:
	shl	bx, 1				# 64 bit left shift of input
	rcl	dx, 1
	rcl	di, 1
	rcl	bp, 1
	shl	bx, 1				# shift it twice
	rcl	dx, 1
	rcl	di, 1
	rcl	bp, 1
	stc					# rotate in test bit
	rcl	si, 1				# 32-bit shift of partial result
	rcl	ax, 1
	sub	di, si				# try test bit
	sbb	bp, ax
	jnb	ROOTBT				# jump out if it fit
	add	di, si				# restore if it didn't
	adc	bp, ax
	dec	si
	loop	RTLP
	jmp	FPWRND
ROOTBT:
	inc	si				# here if test bit fit
	loop	RTLP				# set real result bit and continue
FPWRND:
	shr	ax, 1				# half to a 25-bit result
	rcr	si, 1
	shr	ax, 1				# to a 24-bit result
	rcr	si, 1
	adc	si, 0				# round if a 1 shifted out
	adc	ax, 0				# and propagate carry
	mov	WORD PTR _FAC-3, si		# store result
	and	al, 0x7F			# clear sign bit (LOW 177 octal)
	mov	BYTE PTR _FAC-1, al		# and store
DONE:	ret
NEGNUM:	jmp	_FCERR

# ---------------------------------------------------------------
# $FPWR: involution (FAC) := (BXDX)^(FAC)  i.e. X^Y, with X in BXDX and Y in FAC.
# ---------------------------------------------------------------
_FPWR:
	mov	edi, offset _CLROV		# return address (4 bytes)
	push	edi
	mov	edi, offset _FLGOV
	mov	BYTE PTR [edi], 1		# once-only overflow mode
	call	_SIGNS				# see if Y is zero
	jnz	FP10
	jmp	_EXP				# answer is 1 if Y=0
FP10:	jns	FP20				# if negative potential overflow
	or	bh, bh				# if X is zero
	jnz	FP25
	jmp	_DIV0S				# divide-by-zero code
FP20:	or	bh, bh				# see if zero to power
	jnz	FP25
	jmp	_ZERO				# answer is zero
FP25:					# X is non-zero. If X is negative then Y must be an
					# integral power, otherwise it is a function call error.
	or	bl, bl				# is X negative?
	jns	FP30				# if not proceed
	cmp	BYTE PTR _FAC, 0231		# see if Y too large to determine sign of result (231 octal)
	jb	FP27				# jump if OK to compute
	jmp	_FCERR
FP27:
	push	dx				# save X on the stack (16-bit values)
	push	bx
	push	WORD PTR _FACLO			# push Y on the stack
	push	WORD PTR _FACM1
	call	_INT				# make Y an integer
	pop	bx				# Y to the registers
	pop	dx
	call	_FCOMP				# see if Y is an integer
	call	_MOVFR				# move original Y back to FAC
	pop	bx				# recall X
	pop	dx
	jz	FP30				# proceed if Y is an integer
	jmp	_FCERR				# otherwise function call error
FP30:	mov	al, BYTE PTR _FAC-1		# need to know if Y is negative
	or	al, al
	jns	FP30A				# Y positive
	and	al, 0x7F			# Y is negative: will invert when finished. Clear sign of Y (LOW 177 octal)
	mov	BYTE PTR _FAC-1, al
	push	bx				# save the sign of X
	push	WORD PTR _FACM1			# save Y
	push	WORD PTR _FACLO
	call	FP30A				# FAC := X^positive Y
	test	BYTE PTR _FAC, 0xFF		# did it underflow? (LOW 377 octal)
	jnz	INVERT				# no, go invert it
	pop	WORD PTR _FACLO			# yes, pop Y into the FAC
	pop	WORD PTR _FACM1
	call	_QINT				# get least significant byte of Y into DL
	pop	bx				# get sign of X into BL bit 7
	ror	dl, 1				# set sign bit of FAC if X negative
	and	bl, dl				# and Y is odd
	mov	BYTE PTR _FAC+1, bl
	mov	bh, 0xFF			# make BH nonzero for DOINF (LOW 377 octal) so it uses
						# FAC+1 to determine the sign of the infinity
	jmp	_OVFLS				# give overflow error and return
# INVERT: discard the saved words (3 x 2 bytes), then BXDX := 1.0 and divide (1/result)
INVERT:	add	esp, 6				# pop off the garbage (three 2-byte words)
	mov	dx, 0				# BXDX := 1.0
	mov	bx, 0x8100			# (octal 100400)
	jmp	_FDIVS				# invert the result and return

# FP30A: FAC := X^Y for positive Y (called by FP30 and recursively for negative Y)
FP30A:
	push	bx				# save X on the stack (2-byte values)
	push	dx
	or	bl, 0x7F			# set codes to reflect signs of X (LOW 177 octal)
	pushfd					# save X pos/neg information (4 bytes)
	push	WORD PTR _FACM1			# push Y on the stack
	push	WORD PTR _FACLO
	call	_INT				# must determine if Y is an integer
	pop	dx				# recall original Y
	pop	bx
	call	_FCOMP				# ZF=1 if Y is an integer
	jnz	FP34				# if not integer proceed as normal
	push	dx				# save Y
	push	bx
	mov	dx, 0				# will make (BXDX) 32768
	mov	bx, 0x9000			# (octal 110000)
	call	_FCOMP				# see if too large
	pop	bx				# recall Y
	pop	dx
	jns	FP34				# and if so compute with LOG & EXP
	popfd					# get X pos/neg info off stack
	pop	dx				# get X to registers
	pop	bx
	jmp	XTON				# and compute X^N
FP34:	popfd					# get X pos/neg information
	jns	FP40				# no problems if X is positive
	push	bx				# save Y on the stack
	push	dx
	call	_QINT				# need even-odd information
	mov	al, dl				# get low byte of integer
	call	_NORMS				# (FAC) = INT(Y)
	pop	dx				# recall Y
	pop	bx
	rcr	al, 1				# CF=1 if odd
FP40:	pop	WORD PTR _FACLO			# get X to FAC
	pop	WORD PTR _FACLO+2
	lahf					# save even/odd info
	and	BYTE PTR _FACM1, 0x7F		# clear any sign bit (LOW 177 octal)
	sahf					# get even/odd info back
	jnb	FP60
	mov	edi, offset _NEG
	push	edi				# will negate at the end if CF=1 (4-byte return address)
FP60:	push	bx				# save Y again
	push	dx
	call	_LOG				# LOG(X)
	pop	dx				# recall Y
	pop	bx
	call	_FMULS				# (FAC) = Y*LOG(X)
	jmp	_EXP				# complete calculation

# XTON: (FAC) := (BXDX)^(FAC), where (BXDX) is an integer 0 < N <= 32767 (Knuth, Algorithm A)
XTON:
	push	bx				# save X on stack
	push	dx
	call	_QINT				# convert to integer
	mov	WORD PTR _ARG, dx		# N to ARG
	mov	WORD PTR _FACLO, 0		# will move 1.0 to FAC
	mov	WORD PTR _FACM1, 0x8100		# (octal 100400)
XTON10:	shr	WORD PTR _ARG, 1		# shift right 1 bit
	jnb	XTON20				# if no multiply then Z=Z**2
	pop	dx				# fetch Z
	pop	bx
	push	bx				# and put it back on stack
	push	dx
	call	_FMULS				# Y = Y*Z
XTON20:	test	WORD PTR _ARG, 0xFFFF		# see if N has gone to zero (LOW 177777 octal)
	jz	XTON30				# get out if finished
	pop	dx				# fetch Z to registers
	pop	bx
	call	_PUSHF				# Y to stack
	call	_MOVFR				# Z to FAC
	call	_FMULS				# Z = Z*Z
	pop	dx				# fetch Y
	pop	bx
	call	_PUSHF				# new Z to stack
	call	_MOVFR				# Y to FAC
	jmp	XTON10				# try again
XTON30:	pop	dx				# get stack clear of Z
	pop	bx
	ret

# ---------------------------------------------------------------
# F4 map between the 8080 converted F3 routines and the 8086 math package
# ---------------------------------------------------------------
_CHRGT:
	jmp	CHRGTR
_DSUB:
DSUB:
	xor	BYTE PTR _FAC-1, 0x80		# flip signs of each argument (LOW 200 octal)
	xor	BYTE PTR _ARG-1, 0x80
	jmp	_FSUBD
_FADD:
FADD:
	xchg	bx, cx
	jmp	_FADDS
_FSUB:
FSUB:
	xchg	bx, cx
	jmp	_FSUBS
_FDIV:
FDIV:
	xchg	bx, cx
	jmp	_FDIVS

# CONSIH: integer in BX -> single precision FAC (convert)
CONSIH:
	mov	WORD PTR _FACLO, bx
	jmp	_CSI

# FLOAT: AL (signed byte) -> single precision FAC via $FLT. Value regs only.
FLOAT:
	push	dx
	cbw					# sign-extend AL into AX
	mov	dx, ax
	call	_FLT
	pop	dx
	ret

# F4-map wrappers (BX/CX register conventions of the older F3 interface)
FMULT:
	xchg	bx, cx
	jmp	_FMULS
FPWR:
	xchg	bx, cx
	jmp	_FPWR

# ---------------------------------------------------------------
# INTEGER ARITHMETIC (16-bit values; IMUL/IDIV use DX:AX as in the original)
# ---------------------------------------------------------------
ISUB:						# (BX) = (DX) - (BX)
	cmp	bx, 0x8000			# check for -32768 (octal 100000)
	jnz	IS10				# proceed if not
	call	_FLT				# float DX
	xor	dx, dx				# will set (BXDX)=-32768.0
	mov	bx, 0x8080			# (octal 110200 = 0x9080 for -32768.0 in single precision)
	call	_FSUBS				# do subtract
	call	_NEG				# negate
	jmp	_FI
IS10:
	neg	bx				# negate and fall into add
IADD:						# (BX) = (DX) + (BX)
	push	bx				# save 1st arg
	add	bx, dx
	jo	IAD10				# if overflow jump
	pop	ax				# clear stack
	jmp	MAKINT
IAD10:
	call	_FLT				# float 2nd arg
	pop	dx				# recall first arg
	push	WORD PTR _FACLO
	push	WORD PTR _FAC-1			# FAC pushed on the stack
	call	_FLT				# float 1st arg
	pop	bx
	pop	dx				# get 2nd arg to (BXDX)
	jmp	_FADDS				# go add floating
IMULT:						# (BX) = (BX)*(DX)
	mov	ax, bx
	push	dx				# save 2nd arg
	imul	dx				# perform multiplication
	pop	dx				# recall 2nd arg
	jb	IM10				# must float args if CF=1
	mov	bx, ax
	jmp	MAKINT
IM10:
	push	bx				# save first arg
	call	_FLT				# float 2nd arg
	pop	dx				# recall 1st arg
	push	WORD PTR _FACLO
	push	WORD PTR _FAC-1
	call	_FLT				# float 1st arg
	pop	bx
	pop	dx				# get 2nd arg to (BXDX)
	jmp	_FMULS				# go multiply floating
IDIV:						# (BX) = (DX)/(BX)
	or	bx, bx				# make sure it's non-zero
	jnz	IDIV10
	mov	BYTE PTR _FAC+1, dh		# for correct sign
	inc	bh				# hack for DOINF to work
	mov	BYTE PTR _VALTP, 4		# (LOW 4)
	jmp	_DIV0S
IDIV10:
	cmp	dx, 0x8000			# check for -32768 (octal 100000)
	jnz	IDIV15				# proceed if not
	cmp	bx, 0xFFFF			# check for special case of -32768\-1 (octal 177777)
	jne	IDIV11
	call	_FLT				# float DX
	call	_NEG
	ret
IDIV11:
	cmp	bx, 1				# other special case
	jne	IDIV15
	mov	bx, dx				# put -32768\1 = -32768 in BX so we can
	jmp	MAKINT				# return it as an integer (IDIV doesn't work)
IDIV15:
	mov	WORD PTR _FACLO, bx		# will perform (DX:AX)/(FAC)
	mov	ax, 0
	mov	WORD PTR _FACM1, ax
	xchg	ax, dx				# get no. rt. adjusted in AX
	or	ax, ax
	jns	IDV20
	mov	dx, 0xFFFF			# sign extend into DX
IDV20:	or	bx, bx				# see if denominator needs sign ext
	jns	IDV30
	mov	WORD PTR _FACM1, 0xFFFF
IDV30:	idiv	WORD PTR _FACLO			# 16-bit divide
	mov	bx, ax
	jmp	MAKINT

# F4-map move routines (BX as address where noted)
MOVFR:	xchg	bx, cx
	call	_MOVFR				# move to FAC
	xchg	bx, cx				# get registers right
	ret
MOVFM:	push	ebx				# BX is an address here (4-byte)
	call	_MOVFM
	pop	ebx
	add	ebx, 4
	ret
MOVRF:	mov	dx, WORD PTR _FACLO
	mov	cx, WORD PTR _FAC-1
	ret
MOVMF:	pushfd
	push	ebx
	call	_MOVMF
	pop	ebx
	add	ebx, 4
	popfd
	ret
FOUTO:	call	FRQINT
	mov	WORD PTR _FACLO, bx
	jmp	_FOUTO
FOUTH:	call	FRQINT
	mov	WORD PTR _FACLO, bx
	jmp	_FOUTH
FADDS:						# add number pointed to by EBX to FAC
	mov	dx, WORD PTR [ebx]
	mov	bx, WORD PTR [ebx+2]
	jmp	_FADDS
# PUSHF (F4 map): the name is PUSHF_ here because "PUSHF" is an x86 mnemonic.
PUSHF_:
	pop	esi				# get our return address off the stack (4 bytes)
	push	WORD PTR _FACLO
	push	WORD PTR _FAC-1
	jmp	esi
FCOMP:
	xchg	bx, cx
	call	_FCOMP
	xchg	bx, cx
	ret

# ---------------------------------------------------------------
# Type checks, integer conversion, and the F4 "V" dispatch helpers.
# ---------------------------------------------------------------
FRCSTR:
CHKSTR:						# want to assure string
	call	_GETYP
	jz	CH10
	jmp	TMERR
CH10:	ret
INEG2:	xchg	bx, dx
	call	VALSNG
	xor	al, al
	.globl	INEGAD
INEGAD:	mov	ch, 0x98			# LOW 230 octal
FLOATR:
	mov	ebx, offset _FAC
	mov	cl, al
	mov	BYTE PTR [ebx], ch
	mov	ch, 0				# zero overflow byte
	inc	ebx
	mov	BYTE PTR [ebx], ch
	rcl	al, 1
FADFLT:
	jnb	FL10
	call	NEGR
FL10:	mov	ah, ch				# overflow byte to AH
	mov	bl, cl				# BL is high mantissa bit
	jmp	_NORMS
RNDMON:	call	_CHRGT
RNDMN2:	push	ebx				# save text pointer (4-byte)
	xor	bx, bx				# will set FAC to 1.0
	mov	WORD PTR _FACLO, bx
	mov	bh, 0x81			# LOW 201 octal
	mov	WORD PTR _FACM1, bx
	mov	BYTE PTR _VALTP, 4		# set correct type
	call	_RND
	pop	ebx
	mov	BYTE PTR _VALTP, 4
	ret
UMULT:	mov	ax, cx				# will form (DX)=(CX)*(DX)
	mul	dx
	xchg	ax, dx
	jnb	UM10
	jmp	BSERR
UM10:	ret

# VMOVFA / VMOVFM / VMOVAF / VMOVMF / VMVVFM / VDFACS: the routine address is pushed into DX
# (4 bytes) and the RET at VD15 jumps to it.
VMOVFA:	mov	ebx, offset _ARGLO
VMOVFM:	mov	edx, offset MOVVFM
	jmp	VMVVFM
VMOVAF:	mov	ebx, offset _ARGLO
VMOVMF:	mov	edx, offset VMOVE
VMVVFM:	push	edx
VDFACS:	mov	edx, offset _FACLO
	call	_GETYP
	jb	VD15
VD10:	mov	edx, offset _DFACL
VD15:	ret
MOVE1:	mov	cl, ch
	xor	ch, ch
	movzx	ecx, cx
	jmp	F4VM10				# move for the BASIC interpreter
MOVVFM:	xchg	ebx, edx
VMOVE:	mov	al, BYTE PTR _VALTP
	movsx	ecx, al				# count = type size in bytes
	cmp	al, 3				# string: the descriptor is 5 bytes (STRSIZ), not the 3-byte original
	jnz	F4VM10
	mov	ecx, 5
F4VM10:	cld					# so we will increment
	mov	esi, edx
	mov	edi, ebx
	rep	movsb
	mov	edx, esi
	mov	ebx, edi
	ret

# BLTU / BLTUC: block transfer, makes space by shoving everything forward.
# BX = destination of high address, DX = low address to transfer, CX = high address.
BLTU:	call	REASON				# check destination so string space won't be overwritten
BLTUC:	mov	esi, ecx			# set up high address
	mov	edi, ebx			# set up destination of high address
	std					# move down
	sub	ecx, edx			# count of bytes to transfer
	inc	ecx				# always move one byte
	rep	movsb				# move bytes
	mov	ebx, edx			# BX = LOW
	mov	ecx, edi			# CX = destination of LOW
	inc	ecx				# correct as decremented one too many
	cld					# direction up
	ret
DCXBRT:	pushfd
	dec	cx
	popfd
	ret

# VSIGN: sign of a value by type.
VSIGN:	call	_GETYP				# determine type
	jnz	VS10				# if not string proceed
	jmp	TMERR
VS10:	js	ISI10
	jmp	_SIGNS				# single or double precision
ISI10:	mov	ax, WORD PTR _FACLO		# get the integer
	or	ax, ax				# set signs correctly
	jz	ISIGN1
ISI15:	mov	al, 1
	jns	ISIGN1
	mov	al, 0xFF			# LOW 377 octal
ISIGN1:	ret
ISIGN:	xor	ax, ax				# in case (BX) is zero
	or	bx, bx				# need sign of (BX)
	jnz	ISI15
	ret
# POPHRT: pop a saved word into BX and return. UNSURE: treated as a 4-byte pointer
# (text pointer) since callers save BX as a pointer; see report.
POPHRT:	pop	ebx
	ret
CONIA:						# convert no. in A to an integer
	cbw
	mov	bx, ax
MAKINT:	mov	BYTE PTR _VALTP, 2		# make integer
	mov	WORD PTR _FACLO, bx		# store integer in FAC
	ret
VALSNG:	mov	BYTE PTR _VALTP, 4
	ret
SGN:	call	VSIGN
	jmp	CONIA
FDIVT:	pop	bx
	pop	dx
	jmp	_FDIVS
FPWRQ:	call	_FS				# force FAC to SP
FPWRT:	pop	bx
	pop	dx
	jmp	_FPWR
MOVE:	mov	ecx, 4
	jmp	F4VM10
MOVRM:	pushfd
	mov	dl, BYTE PTR [ebx]
	inc	ebx
	popfd
GETBCD:	pushfd
	mov	dh, BYTE PTR [ebx]		# length
	inc	ebx
	mov	ecx, DWORD PTR [ebx]		# 4-byte address of the string data (5-byte descriptor)
	add	ebx, 3			# to the last byte of the descriptor; INXHRT below steps past it
	popfd
INXHRT:	pushfd
	inc	ebx
	popfd
	ret
INPRT:	push	ebx
	mov	ebx, offset INTXT
	call	STROUT
	pop	ebx
LINPRT:	mov	ecx, offset STROUI
	push	ecx
LINOUT:	call	MAKINT
	xor	al, al
FOUINI:	mov	BYTE PTR TEMP3, al
	mov	ebx, offset _FBUFF+1
	mov	BYTE PTR [ebx], 0x20		# LOW " "
	or	al, BYTE PTR [ebx]
	inc	ebx
	mov	BYTE PTR [ebx], 0x30		# LOW "0"
	jmp	_FOUT2
SIGNC:	mov	al, BYTE PTR _FAC-1
	jmp	ICOMPS
DCOMP:	call	_DCMPA
	jz	SG15
	not	al
ICOMPS:	sal	al, 1
SIGNS:	sbb	al, al
	jz	INRART
SG15:	ret
NEGR:	xor	BYTE PTR _FAC+1, 0x80		# flip sign (LOW 200 octal)
	xor	bx, bx
	neg	ch
	mov	ax, bx
	sbb	ax, dx
	mov	dx, ax
	mov	al, bl
	sbb	al, cl
	mov	cl, al
NG10:	ret
FIXER:	call	_GETYP
	js	NG10
	call	_SIGNS
	js	F4FX20
	jmp	_VINT
F4FX20:	call	_NEG
	call	_VINT
	jmp	_NEG
ICOMP:	mov	ax, bx
ICMPA:	sub	ax, dx
	jz	IC40
	jo	IC20
	js	IC30
IC10:	xor	al, al
INRART:	inc	al
IC15:	ret
IC20:	js	IC10
IC30:	stc
	sbb	al, al
IC40:	ret
IMOD:	cmp	bx, dx
	jnz	IMD10
IMD05:	xor	bx, bx
	jmp	IMD40
IMD10:	cmp	dx, 0x8000			# LOW 100000 octal
	jnz	IMD15
	cmp	bx, 0xFFFF			# 177777 octal
	je	IMD05
	cmp	bx, 1
	je	IMD05
IMD15:	mov	ax, dx
	mov	WORD PTR _FACLO, bx
	or	bx, bx
	jnz	IMD17
	mov	BYTE PTR _FAC+1, dh
	inc	bh
	mov	BYTE PTR _VALTP, 4
	jmp	_DIV0S
IMD17:	mov	WORD PTR _FAC-1, 0
	jns	IMD20
	mov	WORD PTR _FAC-1, 0xFFFF
IMD20:	or	ax, ax
	mov	dx, 0
	jns	IMD30
	mov	dx, 0xFFFF			# MOV DX,177777 (sign-extend the dividend)
IMD30:	idiv	WORD PTR _FACLO
	mov	bx, dx
IMD40:	mov	WORD PTR _FACLO, bx
	ret

# ---------------------------------------------------------------
# Symbol table search, non-subscripted variables. SI/BP/BX are pointers (32-bit).
# Table-relative offsets are 8/16-bit values, extended before the pointer add.
# DX returns a pointer here (FNDITV), so it is EDX.
# ---------------------------------------------------------------
LOPTOP:	lodsw				# VALTYP in AL, 1st char in AH
	cmp	ah, cl
	jz	ISIT
NOTIT1:	inc	esi				# ESI -> the extra-character count byte
	cmp	al, 3				# a string value is a 5-byte descriptor (STRSIZ), not 3
	jnz	1f
	mov	al, 5
1:	add	al, BYTE PTR [esi]
	inc	al
NOTIT0:	movsx	eax, al
	add	esi, eax
NOTIT2:	cmp	esi, ebp
	jnz	LOPTOP
	mov	edx, esi
	jmp	NOTFNS
ISIT:	cmp	al, BYTE PTR VALTYP
	jnz	NOTIT1
	cmp	ch, BYTE PTR [esi]
	jnz	NOTIT1
	inc	esi
	mov	dl, al
	cmp	dl, 3			# size of the value: a string is a 5-byte descriptor
	jnz	2f
	mov	dl, 5
2:	lodsb
	cmp	al, BYTE PTR NAMCNT
	jz	LENMAT
	add	al, dl
	jmp	NOTIT0
LENMAT:	or	al, al
	jz	FNDITV
	movsx	eax, al
	xchg	ax, cx				# save 2 chars of var, length into CX
	movzx	ecx, cx
	mov	edi, offset NAMBUF
	rep	cmpsb
	xchg	ax, cx				# flip back; AX = chars left
	jz	FNDITV
	movzx	eax, ax
	add	esi, eax
	mov	al, dl
	jmp	NOTIT0
FNDITV:	mov	edx, esi
	pop	ebx				# restore text pointer (4 bytes)
	ret
LOPFND:	mov	esi, ebx
	mov	ebp, DWORD PTR ARYTA2		# UNSURE-WIDTH: stored pointer, must be DWORD in GWRAM
	cld
	jmp	NOTIT2

# ---------------------------------------------------------------
# Symbol table search, arrays.
# ---------------------------------------------------------------
LOPFD1:	mov	esi, ebx
	mov	ebp, DWORD PTR STREND		# stored pointer, must be DWORD in GWRAM
	cld
	jmp	LOPFDI
LOPFD0:	lodsw
	cmp	ah, cl
	jz	ISARY
NMARY1:	inc	esi
NMARY2:	lodsb
NMARY3:	movsx	eax, al
	add	esi, eax
NMARY4:	lodsw
	movzx	eax, ax
	add	esi, eax
LOPFDI:	cmp	ebp, esi
	jnz	LOPFD0
	mov	ebx, esi
	jmp	NOTFDD
ISARY:	cmp	al, BYTE PTR VALTYP
	jnz	NMARY1
	cmp	ch, BYTE PTR [esi]
	jnz	NMARY1
	inc	esi
	lodsb
	cmp	al, BYTE PTR NAMCNT
	jnz	NMARY3
	or	al, al
	jz	CNOMAT
	movsx	eax, al
	xchg	ax, cx
	movzx	ecx, cx
	mov	edi, offset NAMBUF
	rep	cmpsb
	xchg	ax, cx
	jz	CNOMAT
	movzx	eax, ax
	add	esi, eax
	jmp	NMARY4
CNOMAT:	lodsw
	mov	dx, ax
	mov	ebx, esi
	jmp	ARYEXT

# ---------------------------------------------------------------
# $AEXPS / $SEXPS: add/subtract exponents for multiply/divide.
# CF on entry selects add (CF=1) or subtract (CF=0).
# ---------------------------------------------------------------
_AEXPS:	stc
	jmp	SES00
_SEXPS:	clc
SES00:	mov	esi, ebx			# WILL NEED FOR LATER
	pushfd					# save multiply/divide flag
	mov	cx, WORD PTR _FAC-1		# (CH)=$FAC : (CL)=$FAC-1
	mov	al, bl
	xor	al, cl
	mov	BYTE PTR _FAC+1, al
	mov	al, bh
	xor	ah, ah
	mov	bl, ch
	xor	bh, bh
	popfd
	jnb	SES05				# jump if subtract
	add	ax, bx				# two biases
	sub	ax, 0x101			# LOW 401 octal: raw sum less 1
	jmp	SES07
SES05:	sub	ax, bx				# biases cancel
SES07:	or	ah, ah
	js	SES10
	cmp	ax, 0x80			# LOW 200 octal; CF=0 if overflow
	jb	SES20
	mov	ebx, esi
	add	esp, 4				# drop the $SEXPS return address (4 bytes)
	jmp	_OVFLS
SES10:	add	ax, 0x80
	jns	SES30
	mov	ebx, esi
	add	esp, 4				# drop return address (4 bytes)
	jmp	_ZERO
SES20:	add	ax, 0x80
SES30:	mov	BYTE PTR _FAC, al
	mov	ebx, offset _FAC-1
	or	BYTE PTR [ebx], 0x80		# hidden "1"
	mov	ebx, esi
	xor	bh, bh
	or	bl, 0x80
	ret

# ---------------------------------------------------------------
# $CHKEL: check input for ELSE / EQV. Text pointer saved as 4 bytes.
# ---------------------------------------------------------------
_CHKEL:	push	ebx
	call	_CHRGT
	cmp	al, 'l'				# LOW OFFSET "L"+40 octal (lower-case L)
	jz	CK10
	cmp	al, 'L'
	jz	CK10
	cmp	al, 'q'				# LOW OFFSET "Q"+40 octal (lower-case Q)
	jz	CK10
	cmp	al, 'Q'
CK10:	pop	ebx
	ret

# ---------------------------------------------------------------
# $EXP (part): y = FAC*LOG2(e), range checks, integer-power split.
# ---------------------------------------------------------------
EXP:
_EXP:	mov	dx, 0xAA3B			# LOG2(e) mantissa (125073 octal)
	mov	bx, 0x8138			# (100470 octal)
	call	_FMULS				# y = FAC*LOG2(e)
	mov	al, BYTE PTR _FAC		# must see if too large
	cmp	al, 0x88			# LOW 210 octal: ABS.GE.128?
	jnb	EXP110
	cmp	al, 0x68			# LOW 150 octal: too small (result 1.)
	jb	EXP200
	push	WORD PTR _FACLO			# save y
	push	WORD PTR _FACLO+2
	call	_QINT				# integer power of 2
	mov	ah, dl
	add	ah, 0x81			# LOW 201 octal (bias + hidden bit position)
	jz	EXP100				# overflow
	push	ax
	test	BYTE PTR _FAC+1, 0x80		# SF = sign saved by $QINT
	call	NGBLDX
	xor	ah, ah				# clear overflow for $NORMS
	call	_NORMS				# FAC = INT(y)
	pop	ax
	pop	bx
	pop	dx				# recall y
	push	ax				# save exponent
	call	_FSUBS				# fractional part for HART #1302
	mov	ebx, offset _EXPCN		# coefficients
	call	_POLY				# do the calculation
	pop	bx				# recall exponent of integer 2^y
	xor	dx, dx
	mov	bl, dl				# 2^Ny in (BXDX); fraction is in FAC
	jmp	_FMULS
EXP100:	add	esp, 4				# overflow with values on the stack
EXP110:	and	BYTE PTR _FAC-1, 0x80		# is it positively too big? (LOW 200 octal)
	jz	EXP115				# yes, overflow
	jmp	_ZERO				# no, underflow for negative
EXP115:	xor	ah, ah				# overflow with stack clear
	mov	BYTE PTR _FAC+1, ah		# positive infinity
	jmp	_OVFLS
EXP200:	mov	edi, offset _FACLO		# result is 1.
	xor	ax, ax				# .0
	cld
	stosw
	mov	WORD PTR [edi], 0x8100		# 100400 octal = 1.0
	ret

# ---------------------------------------------------------------
# $FSUBD / $FADDD: double precision add/subtract. Mantissa bits in BX/CX/AX are values (16-bit);
# pointers are 32-bit.
# ---------------------------------------------------------------
FADDX1:	call	_MOVFA				# move double-precision arg to FAC
FADDX2:	ret
_FSUBD:						# (FAC) := (ARG) - (FAC)
	mov	ax, WORD PTR _FAC-1
	or	ah, ah				# if ZF=1 ARG is the answer
	jz	FADDX1
	xor	BYTE PTR _FAC-1, 0x80		# flip sign of FAC (LOW 200 octal)
DADD:
_FADDD:						# (FAC) := (ARG) + (FAC)
	mov	al, 0				# want to zero overflow bytes
	mov	BYTE PTR _DFACL-1, al
	mov	BYTE PTR _ARGLO-1, al
	mov	al, BYTE PTR _ARG		# if (ARG)=0 then just return
	or	al, al
	jz	FADDX2
	mov	ax, WORD PTR _FAC-1		# if exponent=0, no. is zero
	or	ah, ah
	jz	FADDX1				# arg is the answer
	mov	bx, WORD PTR _ARG-1		# fetch sign and exponent
	or	BYTE PTR _FAC-1, 0x80		# restore hidden 1 (LOW 200 octal)
	or	BYTE PTR _ARG-1, 0x80
	mov	cl, ah				# shift count in CL
	sub	cl, bh
	mov	BYTE PTR _FAC+1, al		# assume sign of FAC
	jz	FDD25				# proceed if exponents equal
	jnb	FDD20				# if FAC larger (or equal) jump
# $ARG has the larger exponent: exchange FAC and ARG and use the sign of ARG.
	xchg	al, bl				# high mantissa byte exchange
	neg	cl				# positive shift count
	mov	BYTE PTR _FAC+1, al		# adapt arg sign
	mov	BYTE PTR _FAC, bh		# adapt arg exponent
	push	ax				# save arg mantissa bits
	push	cx				# will need as loop count
	call	_XCGAF				# exchange arg and FAC
	pop	cx
	pop	ax
FDD20:						# suspected larger number is in FAC; hidden bit restored above.
	cmp	cl, 57				# LOW 57D: must set CF to continue
	jnb	FDD95				# return if CF=0
	push	bx				# save mantissa bits
	clc					# so we don't get CF in there
	call	_SHRA				# shift arg right (CL) bits
	mov	al, BYTE PTR _FAC+1		# recall sign (AL destroyed by $SHRA)
	pop	bx
FDD25:	xor	al, bl				# determine add/sub
	mov	ebx, offset _DFACL-1
	mov	esi, offset _ARGLO-1
	mov	ecx, 4				# 4 sixteen-bit operations
	clc
	cld					# so LODSW will increment (also for FDD50)
	js	FDD50				# if SF=1 go subtract
FDD30:	lodsw
	adc	WORD PTR [ebx], ax		# add it to FAC
	inc	ebx
	inc	ebx
	loop	FDD30
	jnb	FDD40				# go round if CF=0
FDD35:	mov	ebx, offset _FAC		# address of exponent
	inc	BYTE PTR [ebx]			# increment the exponent
	jz	FDD90				# ZF=1 - overflow
	dec	ebx
	dec	ebx				# BX points to $FAC-2
	mov	ecx, 4				# 4 sixteen-bit shifts
FDD37:	rcr	WORD PTR [ebx], 1
	dec	ebx
	dec	ebx
	loop	FDD37
FDD40:	jmp	_ROUND				# go round the result
FDD50:	lodsw					# fetch next word of arg
	sbb	WORD PTR [ebx], ax		# subtract from FAC
	inc	ebx
	inc	ebx
	loop	FDD50
	jnb	FDD80				# go normalize and round
	not	BYTE PTR [ebx+1]		# complement sign
	mov	ecx, 4				# 4 sixteen-bit complements
FDD60:	dec	ebx
	dec	ebx
	not	WORD PTR [ebx]			# complement FAC
	loop	FDD60
	mov	ecx, 4
FDD70:	inc	WORD PTR [ebx]			# if ZF=1 then carry
	jnz	FDD80
	inc	ebx
	inc	ebx
	loop	FDD70
	jz	FDD35				# NOTE: ZF here comes from the last INC EBX, as in the original
FDD80:	jmp	_NORMD				# go normalize
FDD90:	jmp	_OVFLS				# overflow
FDD95:	jmp	_ROUNX				# put in the sign and done

# ---------------------------------------------------------------
# $FSUBS / $FADDS: single precision add/subtract. (BXDX) is the operand, FAC the accumulator.
# ---------------------------------------------------------------
FEXIT1:	mov	WORD PTR _FAC-1, bx		# MOV (BXDX) TO $FAC
	mov	WORD PTR _FACLO, dx
EXIT2:	ret
_FSUBS:	mov	ax, WORD PTR _FAC-1		# fetch FAC
	or	ah, ah				# if ZF=1 (BXDX) is the answer
	jz	FEXIT1
	xor	BYTE PTR _FAC-1, 0x80		# flip sign (LOW 200 octal)
_FADDS:	or	bh, bh				# first check exponent of (BXDX)
	jz	EXIT2				# answer already in $FAC
	mov	ax, WORD PTR _FAC-1		# check $FAC: if zero, answer in (BXDX)
	or	ah, ah
	jz	FEXIT1
	xor	cx, cx				# (CX)=0
	movzx	ecx, cx				# keep upper ECX clear: LOOP uses ECX
	mov	si, WORD PTR _FACLO		# (SI)=($FAC-2,$FACLO)
	mov	BYTE PTR _FAC+1, al		# assume sign of $FAC
	mov	cl, ah				# since assume $FAC larger
	sub	cl, bh				# CL holds shift count
	jnb	FA20				# jump if $FAC exp equal or larger
	neg	cl				# positive shift count
	xchg	bl, bh
	mov	WORD PTR _FAC, bx		# (BXDX) is larger magnitude
	xchg	bl, bh				# get exp/sgn correct again
	xchg	bx, ax				# exchange (BXDX) and (AXSI)
	xchg	dx, si
FA20:	mov	ah, al				# determine add or subtract
	xor	ah, bl
	pushfd					# SF=1 if subtract
	mov	ah, 0x80			# replace understood 1 (LOW 200 octal)
	or	al, ah
	or	bl, ah
	xor	ah, ah				# (AH) overflow byte
	mov	bh, ah				# (AH)=(BH)=0
FA22:	or	cx, cx				# ZF=1 if exponents the same
	jz	FA40
	cmp	cx, 25				# 31 octal: must see if within 24 bits
	jb	FA23				# if so proceed
	popfd					# clear subtract/add flag
	mov	WORD PTR _FACLO, si		# restore lower mantissa bits
	mov	ah, BYTE PTR _FAC+1		# fetch sign
	and	ax, 0x807F			# 100177 octal: clear sign in AH, keep sign in AL
	or	al, ah				# restore sign
	mov	BYTE PTR _FAC-1, al		# $FAC now correctly built
	ret
FA23:	cmp	cl, 8				# LOW 10 octal: need at least 8 bits
	jb	FA27				# if not proceed as normal
	mov	di, ax				# will want to check this for ST
	mov	ah, dl				# shift overflow bits
	test	di, 0xFF00			# 177400 octal: did we shift through ST?
	jz	FA24
	or	ah, 0x20			# put ST back in (LOW 40 octal)
FA24:	mov	dl, dh
	mov	dh, bl
	xor	bl, bl				# clear upper bits
	sub	cl, 8				# LOW 10 octal
	test	ah, 0x1F			# LOW 37 octal: shift thru ST
	jz	FA22				# if not try again
	or	ah, 0x20
	jmp	FA22
FA25:	or	ah, 0x20			# "OR" in ST bit (LOW 40 octal)
	loop	FA30				# continue loop
	jmp	FA40				# if finished jump
FA27:	clc					# make sure carry clear before shift
FA30:	rcr	bl, 1
	rcr	dx, 1
	rcr	ah, 1				# shift (BLDXAH) right one bit
	test	ah, 0x10			# LOW 20 octal: see if ST set
	jnz	FA25
	loop	FA30				# loop until (CX)=0
FA40:	popfd					# if SF=1 we must subtract mantissas
	jns	FA50				# if SF=0 go add mantissas
	sub	cl, ah				# subtract underflow byte
	mov	ah, cl				# must go to NORMS with mantissa in (BLDXCL)
	sbb	si, dx
	mov	dx, si
	sbb	al, bl				# if carry (CF) not set then
	mov	bl, al
	jnb	FA90				# assumption of $FAC larger valid
	not	BYTE PTR _FAC+1			# must use other sign: $FAC wasn't larger
	not	ah
	not	dx
	not	bl
	inc	ah				# increment by one and set carry
	jnz	FA90				# if ZF=0 go normalize
	inc	dx				# increment by one
	jnz	FA90
	inc	bl
	jnz	FA90
	jmp	FA60
FA50:	add	dx, si				# addition of low bits
	adc	bl, al				# addition of high bits
	jnb	FA70
FA60:						# overflowed the high mantissa byte
	inc	BYTE PTR _FAC			# increment the exponent
	jz	FA80				# overflow!
	rcr	bl, 1				# must shift right one bit
	rcr	dx, 1
	rcr	ah, 1
FA70:	jmp	_ROUNS
FA80:	jmp	_OVFLS				# do overflow code
FA90:	jmp	_NORMS				# go normalize

# ---------------------------------------------------------------
# DDIV: double precision divide (FAC) := (ARG)/(FAC). Pointers SI/DI are 32-bit.
# ---------------------------------------------------------------
EXIT1:	jmp	_DZERO				# zero the FAC
DOVCKJ:	jns	EXIT1
	jmp	_OVFLS
DDIV:
DDIVFA:	mov	esi, offset _DFACL
	mov	edi, offset _ARGLO
	jmp	_DDIV
_FDIVD:
	mov	esi, offset _ARGLO
	mov	edi, offset _DFACL
_DDIV:
	mov	ax, WORD PTR [esi+6]		# high half of numerator
	mov	cx, WORD PTR [edi+6]		# high half of denominator
	xor	al, cl				# compute sign
	mov	BYTE PTR _FAC+1, al		# and store in $FAC
	or	ch, ch				# denominator zero?
	jz	DDIV0
	or	ah, ah				# numerator zero?
	jz	EXIT1
	sub	ah, 128				# remove bias from exponents
	sub	ch, 128
	sub	ah, ch				# compute result exponent
	jo	DOVCKJ
	cld					# 9-Aug-82/MLC - good for the LODC
	add	ah, 128				# put bias back
	push	ax				# save sign and exponent
	lodsb					# load up dividend
	mov	ch, al
	xor	cl, cl
	lodsw
	xchg	ax, bx
	lodsw
	xchg	ax, dx
	lodsw
	or	ah, 0x80			# set implied bit (LOW 200 octal)
	xchg	ax, dx				# divisor in DX:AX:BX:CX
	mov	esi, edi			# MOV SI,DI (DI was the divisor pointer)
	add	esi, 5				# point to high end of divisor
	mov	edi, offset _FAC-1
	std					# direction DOWN
	movsw
	movsw
	movsw
	inc	esi
	inc	edi
	movsb
	cld					# DRESTR direction
	mov	BYTE PTR [edi], 0		# extend to 64 bits with a zero
	or	BYTE PTR _FAC, 0x80		# set implied bit
	shr	dx, 1				# make sure dividend is smaller than divisor
	rcr	ax, 1				#   by dividing it by two
	rcr	bx, 1
	rcr	cx, 1
	call	DDIV16				# get a quotient digit
	push	di
	call	DDIV16
	push	di
	call	DDIV16
	push	di
	call	DDIV16
	or	ax, bx				# remainder zero?
	or	ax, cx
	or	ax, dx
	mov	dx, di				# get lowest word in position
	jz	DNSTK1
	or	dl, 1				# set sticky bit if not
DNSTK1:	pop	cx				# recover quotient digits
	pop	bx
	pop	di
	jmp	DNRMCHK
DDIV0:	mov	BYTE PTR _FAC+1, al
	jmp	_DIV0S
DDIV16:	mov	si, WORD PTR _DFACL+6		# high word of divisor (value)
	xor	di, di				# quotient digit starts at zero
	cmp	dx, si				# will we overflow?
	jae	DMXQUO				# if so, handle special
	or	dx, dx				# is dividend small?
	jnz	DODIV
	cmp	si, ax				# will divisor fit at all?
	ja	ZERQUO				# no: quotient is zero
DODIV:	div	si				# AX is our digit "guess"
	push	dx				# save remainder
	xchg	ax, di				# quotient digit in DI
	xor	bp, bp				# initialize quotient * divisor
	mov	si, bp
	mov	ax, WORD PTR _DFACL
	or	ax, ax				# if zero, save multiply time
	jz	REM2
	mul	di				# begin computing quotient * divisor
	mov	si, dx
REM2:	push	ax				# save lowest word of quotient * divisor
	mov	ax, WORD PTR _DFACL+2
	or	ax, ax
	jz	REM3
	mul	di
	add	si, ax
	adc	bp, dx
REM3:	mov	ax, WORD PTR _DFACL+4
	or	ax, ax
	jz	REM4
	mul	di
	add	bp, ax
	adc	dx, 0
	xchg	ax, dx
REM4:					# quotient * divisor in AX:BP:SI:[SP]
	pop	dx				# recover lowest word of quotient * divisor
	neg	dx				# subtract from dividend
	sbb	cx, si
	sbb	bx, bp
	pop	bp				# remainder from DIV
	sbb	bp, ax
	xchg	ax, bp
ZERQUO:					# remainder in AX:BX:CX:DX
	xchg	ax, dx
	xchg	ax, cx
	xchg	ax, bx
	jnb	RETRES				# remainder in DX:AX:BX:CX
DRESTR:	dec	di				# drop quotient since it didn't fit
	add	cx, WORD PTR _DFACL		# add divisor back in until remainder goes +
	adc	bx, WORD PTR _DFACL+2
	adc	ax, WORD PTR _DFACL+4
	adc	dx, WORD PTR _DFACL+6
	jnb	DRESTR
RETRES:	ret
DMXQUO:	dec	di				# DI=FFFF=2**16-1
	sub	cx, WORD PTR _DFACL
	sbb	bx, WORD PTR _DFACL+2
	sbb	ax, WORD PTR _DFACL+4
	add	cx, WORD PTR _DFACL+2
	adc	bx, WORD PTR _DFACL+4
	adc	ax, dx
	mov	dx, WORD PTR _DFACL
	cmc
	jmp	ZERQUO
DNRMCHK:
	pop	ax				# get exp. and sign back
	or	di, di				# see if normalized
	js	DINCEX				# yes: increment exponent
	shl	dx, 1				# normalize
	rcl	cx, 1
	rcl	bx, 1
	rcl	di, 1
	or	ah, ah
	jnz	DDRND
	jmp	_DZERO
DINCEX:	inc	ah
	jz	DDOVFL
_DROUND:
DDRND:	cmp	dl, 0x80			# check extended bits (LOW 200 octal)
	ja	FPRNDUP
	jb	DDSV
	test	dh, 1				# already even?
	jz	DDSV
FPRNDUP:	add	dh, 1
	adc	cx, 0
	adc	bx, 0				# propagate carry
	adc	di, 0
	jnb	DDSV				# overflow?
	inc	ah				# increment exponent
	jnz	DDSV
DDOVFL:	jmp	_OVFLS
DDSV:	and	al, 0x80			# strip to sign bit (LOW 200 octal)
	xchg	bx, di
	and	bh, 0x7F			# mask off implied bit (LOW 177 octal)
	or	al, bh				# combine sign with mantissa
	mov	WORD PTR _DFACL+6, ax
	mov	BYTE PTR _FAC-2, bl
	mov	bx, di
	mov	edi, offset _DFACL
	mov	al, dh
	cld
	stosb
	xchg	ax, cx
	stosw
	xchg	ax, bx
	stosw
	ret

# ---------------------------------------------------------------
# $FDIVS / $SDIV: single precision divide. SI/DI are pointers at entry to $SDIV, then values in DIV16.
# ---------------------------------------------------------------
_FDIVS:	call	_SIGNS				# must be sure (FAC) not zero
	jnz	FDS00
	mov	BYTE PTR _FAC+1, bl		# put in for correct overflow sign
	jmp	_DIV0S				# if so overflow!
FDS00:	or	bh, bh				# if (BXDX)=0 answer is zero
	jnz	FDS10				# proceed if (BXDX) non-zero
FDS05:	jmp	_ZERO
FDS10:	mov	WORD PTR _DFACL, dx		# will put numerator in $DFAC
	mov	WORD PTR _DFACL+2, bx
	mov	esi, offset _DFACL
	mov	edi, offset _FACLO
_SDIV:	mov	cx, WORD PTR [edi+2]		# exponent and sign of denominator
	mov	ax, WORD PTR [esi+2]		# exponent and sign of numerator
	xor	al, cl				# compute sign
	mov	BYTE PTR _FAC+1, al		# save sign in case OVERFLOW
	sub	ah, 128				# remove bias from exponents
	sub	ch, 128
	sub	ah, ch				# compute result exponent
	jo	OVCHKJ
	add	ah, 128				# put bias back
	push	ax				# save sign and exponent
	or	cl, 0x80			# set implied bit (LOW 200 octal)
	mov	dx, WORD PTR [esi+1]		# most significant part of numerator mantissa
	mov	ah, BYTE PTR [esi]		# least significant part
	xor	al, al				# extend with zero
	or	dh, 0x80			# and set implied bit
	mov	bx, WORD PTR [edi+1]		# load denominator into BX:CX
	mov	ch, BYTE PTR [edi]
	xor	cl, cl
	or	bh, 0x80
	shr	dx, 1
	rcr	ax, 1				# ensure DX<BX so divide won't OVERFLOW
	call	DIV16				# get first 16 bits of quotient
	push	si				# save high half of quotient on stack
	call	CDIV16				# get second 16 bits of quotient
	or	ax, dx				# remainder 0?
	mov	dx, si				# want low half of result in DX
	jz	NSTK1
	or	dl, 1				# set sticky bit if remainder not zero
NSTK1:	pop	bx				# recover high half of result
	jmp	NORMCHK
OVCHKJ:	jns	FDS05				# zero the FAC
SDIVOVFL:
	jmp	_OVFLS				# overflow

DIV0:	jmp	_DIV0S
CDIV16:	cmp	dx, bx				# will we overflow on divide?
	jae	MAXQUO				# special code for maximum quotient
# Single-precision 16-bit division step. DX:AX / BX, quotient in SI. Values only (SI/DI are values here).
DIV16:	div	bx				# 32 by 16 bit division
	xchg	ax, si				# put 16-bit quotient in SI
	xor	ax, ax				# DX:AX has remainder
	jcxz	RETJ				# is this complete result?
	mov	di, dx				# save remainder while we multiply
	mov	ax, si
	mul	cx				# low half of divisor times quotient
	neg	ax				# subtract from remainder, whose low half is 0
	sbb	di, dx
	mov	dx, di				# remainder again in DX:AX
	jnb	RETJ				# if not negative, divide was good
RESTORE:
	dec	si				# quotient is too big
	add	ax, cx				# and remainder too small (negative)
	adc	dx, bx				# so add divisor back in till positive
	jnb	RESTORE
RETJ:	ret
NORMCHK:
	pop	ax				# get exp. and sign back
	or	bh, bh				# see if normalized
	js	INCEXP				# yes: increment exponent
	shl	dx, 1				# normalize
	rcl	bx, 1
	or	ah, ah				# test exponent
	jnz	ROUND				# brif exponent not zero
	jmp	_ZERO				# underflow if exponent zero
INCEXP:	inc	ah
	jz	SDIVOVFL
_SRND:
ROUND:	cmp	dl, 0x80			# check extended bits (LOW 200 octal)
	ja	ROUNDUP
	jb	SAVE
	test	dh, 1				# already even?
	jz	SAVE
ROUNDUP:	add	dh, 1
	adc	bx, 0				# propagate carry
	jnb	SAVE				# overflow?
	inc	ah				# increment exponent
	jz	SDIVOVFL
SAVE:	and	al, 0x80			# strip to sign bit (LOW 200 octal)
	and	bh, 0x7F			# mask off implied bit (LOW 177 octal)
	or	al, bh				# combine sign with mantissa
	mov	WORD PTR _FACLO+2, ax
	mov	ah, bl
	mov	al, dh
	mov	WORD PTR _FACLO, ax		# save lower part of mantissa
	ret
MAXQUO:	mov	si, -1				# quotient of (2^16)-1
	sub	ax, cx				# dividend - divisor (always negative)
	add	dx, ax				# add to high half of divisor
	mov	ax, cx
	jnb	RESTORE				# carry means it's not negative any more
	ret

# Global symbols (PUBLIC list from MATH1.ASM)
.globl	_EXP, _FDIVD, ABSFN, ATN, COS, DADD, DDIV, EXP, FINDBL, FIN
.globl MDP10
.globl _MOVFC, _DP00
.globl	INEG, INEG2, LOPFND, NEG, SIGN, SIN, SQR, TAN
.globl	VINT, VNEG, ZERO, _DCMPA, _DIV0S, _DZERO, _EXPCN, _FADDS
.globl	_FCOMP, _FDIVS, _FOUTH, _FOUTO, _FPWR, _FSUBD, _FSUBS, _GETYP
.globl	_LOGP, _MOVFA, _MOVFM, _MOVFR, _MOVMF, _MOVRF, _NEG, _POPA
.globl	_SIGNS, _VINT, _VPSHF, SIGNS, DCXBRT, FADD, VSIGN, MOVFR
.globl	ISIGN, IMOD, GETBCD, ISUB, FPWRQ, INRART, MOVRF, DCOMP
.globl	VDFACS, MOVFM, FDIVT, ICOMP, VMOVAF, UMULT, FOUTH, FSUB
.globl	ICMPA, VMOVFA, IDIV, VMOVFM, IMULT, RNDMN2, BLTU, FMULT
.globl	FRCSTR, POPHRT, MOVE, MOVMF, CONIA, VMOVE, RNDMON, CHKSTR
.globl	LOPFD1, INPRT, MAKINT, FDIV, SGN, SIGNC, FCOMP, INXHRT
.globl	IADD, FOUTO, DSUB, VALSNG, BLTUC, FIXER, MOVRM, FADDS
.globl	LINPRT, MOVE1, CONSIH, PUSHF_, VMOVMF, STROUT, _FOTCI
