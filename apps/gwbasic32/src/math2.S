# MATH2.ASM -> 32-bit GAS. Source radix is octal throughout (.RADIX 8); bare numbers are octal
# here and written as hex or decimal. Explicit D suffixes are decimal.
	.intel_syntax noprefix
# Cross-module exports (labels referenced from the other math file or the interpreter).
	.globl	NGBLDX, _FOUT2

	.section .text

# $FIDIG: add a text digit (AL, already converted) to the accumulated number.
# Entry: (BX) = text pointer, (DI) = digits after D.P., (CX) = D.P. flag.
	.globl _FIDIG
_FIDIG:	adc	di, cx				# (DI) incremented only if D.P. seen
	push	ebx				# save all necessary regs (pointer: 4 bytes)
	push	di
	push	cx
	sub	al, '0'				# subtract out ASCII bias
	push	ax				# save on stack
	call	_GETYP				# set condition codes
	pop	ax				# recall digit
	cbw					# zero AH
	jns	FI05				# must be S.P. or D.P. already
	mov	bx, WORD PTR _FACLO		# fetch the integer already accumulated
	cmp	bx, 3277			# 3277 decimal: already too big to add?
	jnb	FFI10				# if so go make S.P. first
	mov	cx, bx				# save original (BX)
	shl	bx, 1				# (BX)=(BX)*2
	shl	bx, 1				# (BX)=(BX)*4
	add	bx, cx				# (BX)=(BX)*5
	shl	bx, 1				# (BX)=(BX)*10
	add	bx, ax				# add in the digit
	js	FFI10				# if SF=1 we have 32768 or 32769
	mov	WORD PTR _FACLO, bx		# store in $FAC
	jmp	FI50
FI05:						# number was already S.P. or D.P.
	push	ax				# save the number
	jb	FFFI20				# it's currently single precision
	jmp	FI40				# double precision
FFI10:						# previously an integer, now S.P.
	push	ax				# save the number
	call	_CSI				# convert integer to S.P.
	jmp	FI30				# mul by 10 and add in digit
FFFI20:						# already single precision
	mov	WORD PTR _DBUFF+4, 0x2400	# 22000 octal
	mov	WORD PTR _DBUFF+6, 0x9474	# 112164 octal
	mov	ebx, offset _DBUFF+6
	call	_COMPM				# compare to $FAC
	jns	FI35				# go do D.P. if too large for S.P.
FI30:	call	_MUL10				# multiply $FAC by 10
	pop	dx				# recall digit
	push	WORD PTR _FACLO
	push	WORD PTR _FAC-1			# FAC pushed on stack
	call	_FLT				# convert integer to S.P.
	pop	bx				# recall FAC
	pop	dx
	call	_FADDS				# add in the new digit
	jmp	FI50				# get stack right and return
FI35:						# already 7 digits and would overflow S.P.
	call	_CDS				# convert the single to D.P.
FI40:	call	_MUL10				# multiply by 10
	call	_MOVAF				# move $FAC to $ARG
	pop	dx				# recall digit
	call	_FLT				# convert to S.P.
	call	_CDS				# convert to D.P.
	call	_FADDD				# add in the old accumulated value
FI50:	pop	cx				# get decimal point flag back
	pop	di				# get no. digits to right of decimal pt.
	pop	ebx				# get text pointer back
	ret

# $FINEX: exponent input routine.
	.globl _FINEX
_FINEX:	lahf					# save status
	cmp	BYTE PTR _VALTP, 8		# LOW 10 octal: already D.P.?
	jnz	EXA
	sahf					# get stack right
	jmp	EXB
EXA:	sahf					# restore codes
	push	ebx				# save important registers
	push	di
	call	_FINFC				# ZF=1 S.P.; ZF=0 D.P.
	pop	di				# recall digits to right of D.P.
	pop	ebx				# recall text pointer
EXB:	xor	si, si				# in case exponent is positive
	mov	dx, si				# will build exponent in DX
	call	_CHRGT				# get first character of exponent
	jb	FX20				# no sign: default positive
	cmp	al, '-'				# negative exponent
	jnz	FX00				# if not, must be positive
	not	si				# negative exponent
	jmp	FX10
FX00:	cmp	al, '+'
	jz	FX10
	ret					# (BX) pointing here
FX10:	call	_CHRGT				# get next character
	jb	FX20				# if digit, process as exponent
	ret					# otherwise return
FX20:	cmp	dx, 3276			# 3276 decimal: overflow if this doesn't get CF=1
	jb	FX30				# no: use this digit
	mov	dx, 32767			# to assure overflow
	jmp	FX10
FX30:	push	ax				# save new digit
	mov	ax, 10				# must multiply DX by 10
	mul	dx				# answer now in AX
	pop	dx				# recall digit to DX
	sub	dl, 0x30			# subtract out ASCII bias (LOW 60 octal)
	xor	dh, dh				# to be sure AX has correct no.
	add	dx, ax				# add to DX
	jmp	FX10

# $FIND / $FINS: force input to double / single precision.
	.globl _FIND, _FINS
_FIND:	or	al, 1				# signal double precision
_FINS:						# force single (caller has set Z flag)
	push	ebx				# save text pointer
	push	di				# save no digits after decimal point
	jnz	FC10				# force to double for $FIND callers
	call	_FS				# force to single for $FINS callers
	jmp	FC20				# skip over $FD call
FC10:	call	_FD				# force FAC to double prec.
FC20:	pop	di				# recall no digits to right of dec pt
	pop	ebx				# recall text pointer
	xor	si, si				# since this is a forced
	mov	dx, si				# no. exponent is zero
	call	_FINE				# do implied exponent fix-up
FC30:	inc	ebx				# point past the force character
	ret
_FINI:	call	_GETYP				# set cond codes according to type
	js	FC30
	jmp	_SNERR				# can't make integer if not already
_FINFC:	jz	_FD				# if ZF=1 then double precision
FRCSNG:
	.globl _FS, FRCSNG
_FS:						# force single precision
	call	_GETYP				# set cond codes acc. to type
	jpo	FC200				# if already S.P. return
	jnz	FS10
	jmp	_TMERR				# can't force a string
FS10:	jns	_CSD				# if not integer force double to S.P.
	call	_CSI				# force integer to single
	jmp	FC200
	.globl _CSD
_CSD:						# convert double to single precision
	mov	al, 4				# single prec designation
	mov	BYTE PTR _VALTP, al
	mov	bl, BYTE PTR _FAC-1		# fetch high mantissa bits
	mov	BYTE PTR _FAC+1, bl		# move sign to $FAC+1
	mov	dx, WORD PTR _FAC-3		# fetch rest of mantissa
	mov	ah, BYTE PTR _FAC-4		# fetch overflow bits
	or	ah, 0x40			# want round-up if high bit set (LOW 100 octal)
	or	bl, 0x80			# put in understood 1 (LOW 200 octal)
	jmp	_ROUNM				# go round the number
	.globl FRCDBL, _FD
FRCDBL:
_FD:						# force to double precision
	call	_GETYP				# determine current type
	jnb	FC200				# if already double exit
	jnz	FD10
	jmp	_TMERR
FD10:	jns	_CDS				# if not integer proceed
	call	_CSI				# convert integer to single prec.
	.globl _CDS
_CDS:	mov	al, 8				# double prec. indicator (LOW 10 octal)
	mov	BYTE PTR _VALTP, al		# set type to D.P.
	xor	ax, ax				# must zero overflow bytes
	mov	WORD PTR _DFACL, ax
	mov	WORD PTR _DFACL+2, ax
	ret
	.globl _CSI
_CSI:	push	dx				# save (DX)
	push	esi				# save ESI (pointer width)
	mov	dx, WORD PTR _FACLO		# fetch the integer
	call	_FLT				# float the integer and store in FAC
	pop	esi				# get registers right
	pop	dx
FC200:	ret
	.globl _FI, FRCINT
FRCINT:
_FI:						# force integer
	call	_GETYP				# see what we're in
	jns	FI10				# if not integer already: jump
	mov	bx, WORD PTR _FACLO
	ret
FI10:	jnz	FFI20
	jmp	_TMERR				# if string: error
FFI20:
	.globl _CINC, _CIND
_CINC:						# single precision, operand in FAC
_CIND:						# double precision uses same routine
	push	ax
	push	cx
	mov	ax, WORD PTR _FACM1		# get exponent
	mov	cx, WORD PTR _FACLO		# get mantissa
	.globl CINT, VALINT
CINT:
	xor	bx, bx				# set up zero result
	sub	ah, 0x80			# take bias out of exponent (LOW 200 octal)
	jb	CXRET				# return zero if no integer part
	mov	bh, al				# highest byte of mantissa
	mov	bl, ch
	xchg	ax, cx
	mov	cl, 16				# LOW 16D
	sub	cl, ch				# number of bits to shift mantissa right
	mov	ah, bh				# save sign
	jb	OVERFLOW			# if negative shift, it won't fit in 16 bits
	jz	OVCHK				# only -32768 has 16 bits: go check for it
	or	bh, 0x80			# set implied bit (LOW 200 octal)
	shr	bx, cl				# position the integer
	adc	bx, 0				# perform rounding
	jo	POSBOVER
	or	ah, ah				# check sign now
	jns	CXRET
	neg	bx
CXRET:	pop	cx
	pop	ax
	mov	WORD PTR _FACLO, bx		# result in both FAC and BX
VALINT:
_VALNT:	mov	BYTE PTR _VALTP, 2
	ret
POSBOVER:					# here for either -32768 or overflow
	or	ah, ah				# if signed then -32768
	js	CXRET
	jmp	OVERFLOW
OVCHK:
	cmp	bx, 0x8000			# 100000 octal: the 1 is sign bit, not implied bit
	jnz	OVERFLOW
	test	al, 0x80			# should we be rounding up? (LOW 200 octal)
	jz	CXRET				# if so, that causes overflow
OVERFLOW:
	jmp	_OVERR

# $FLT: convert integer in DX to single precision and store in FAC.
	.globl _FLT
_FLT:	xor	bx, bx				# clear high mantissa byte (BL)
	xor	ah, ah				# clear overflow byte
	mov	esi, offset _FAC+1		# fetch $FAC address
	mov	BYTE PTR [esi-1], 0x90		# set exponent to 16 (LOW 220 octal)
	mov	BYTE PTR [esi], 0		# set sign positive
	or	dx, dx				# sets SF=1 if negative
	jns	FLT10
	neg	dx				# need positive magnitude
	mov	BYTE PTR [esi], 0x80		# set sign to negative (LOW 200 octal)
FLT10:	mov	bl, dh				# will move (DX) to (BLDH)
	mov	dh, dl
	mov	dl, bh
	mov	BYTE PTR _VALTP, 4		# set type to S.P.
	jmp	_NORMS				# go normalize

# $FMULD: double precision multiply, (FAC) := (FAC)*(ARG).
	.globl _DMULT, _FMULD, DMULT
DMULT:
_DMULT:
_FMULD:
	mov	al, BYTE PTR _FAC		# first see if FAC is zero
	or	al, al				# if so just return
	jz	FMD10
	mov	al, BYTE PTR _ARG		# now see if ARG is zero
	or	al, al				# if so set FAC to zero and return
	jnz	FMD20				# if not zero proceed to multiply
	jmp	_DZERO				# zero the FAC
FMD10:	ret
FMD20:	mov	bx, WORD PTR _ARG-1		# fetch sign and exp. to BX
	call	_AEXPS				# add the exponents
	push	WORD PTR _FAC			# exponent, sign
	mov	WORD PTR _ARG-1, bx		# replace unpacked mantissa
	call	_SETDB				# move the FAC to $DBUFF so product
	movzx	esi, ax				# J (offset, value)
	mov	WORD PTR _FAC, ax
	mov	ebx, offset _DBUFF
	mov	WORD PTR _ARG, ax
	mov	ebp, offset _ARGLO		# point to multiplicand base
M1:	mov	ax, WORD PTR [ebx+esi]		# fetch multiplier V(J)
	or	ax, ax				# see if zero
	jz	M4D				# if zero W(J)=0
	xor	edi, edi			# I
	xor	ecx, ecx			# K
M4:	mov	ax, WORD PTR [ebx+esi]		# fetch multiplier V(J)
	mul	WORD PTR [ebp+edi]		# form product V(J)*U(J) in (DXAX)
	push	ebx				# save ptr. to multiplier base
	mov	ebx, esi
	add	ebx, edi			# I+J
	add	ebx, offset _DFACL-8		# W(I+J) address in BX
	add	ax, WORD PTR [ebx]		# (DXAX)=U(I)*V(J)+W(I+J)
	jnb	M4A
	inc	dx
M4A:	add	ax, cx				# T=U(I)*V(J)+W(I+J)+K
	jnb	M4B
	inc	dx
M4B:	mov	WORD PTR [ebx], ax		# W(I+J)= T MOD 2^16
	mov	cx, dx				# K=INT(T/2^16)
	pop	ebx				# recall ptr to multiplier base
	cmp	edi, 6				# finished inner loop?
	jz	M4C				# if so jump and set W(J)
	inc	edi
	inc	edi
	jmp	M4
M4C:	mov	ax, cx				# (AX)=K
M4D:	push	ebx				# save ptr to multiplier base
	mov	ebx, offset _DFACL
	mov	WORD PTR [ebx+esi], ax		# W(J)=K or 0 (0 if V(J) were 0)
	pop	ebx				# recall ptr to multiplier base
	cmp	esi, 6				# finished outer loop?
	jz	M5
	inc	esi
	inc	esi
	jmp	M1
M5:						# multiplication complete and in FAC
	mov	esi, offset _DFACL-2		# will now set ST
	std					# want non-zero byte ASAP so prob. is small
	mov	ecx, 7				# 7-byte check
M5AA:	lodsb					# fetch next byte
	or	al, al
	loopz	M5AA
	jz	M5AB				# don't need to set ST
	or	BYTE PTR _DFACL-1, 0x20		# "OR" in ST bit (LOW 40 octal)
M5AB:	mov	al, BYTE PTR _FAC-1		# see if we need to inc exponent
	or	al, al
	pop	WORD PTR _FAC			# restore exponent, sign
	js	M6
	mov	ebx, offset _DFACL-1		# must shift 1 bit left
	mov	ecx, 4
M5A:	rcl	WORD PTR [ebx], 1
	inc	ebx
	inc	ebx
	loop	M5A
M5B:	jmp	_ROUND				# now round
M6:	inc	BYTE PTR _FAC			# increment exponent
	jnz	M5B
	jmp	_OVFLS				# overflow!

# $FMULS: single precision multiply, (FAC) := (BXDX)*(FAC). Mantissa pieces are 16-bit values.
	.globl _FMULS
_FMULS:	call	_SIGNS				# ZF=1 will be set if (FAC)=0
	jz	FMS00				# just return if (FAC)=0
	or	bh, bh				# if exponent of (BXDX) is zero
	jnz	FMS05				# proceed if non-zero
FMS00:	jmp	_ZERO				# the number is zero
FMS05:	call	_AEXPS				# add the S.P. exponents
	mov	cx, WORD PTR _FAC-1		# (CH)=($FAC):(CL)=($FAC-1)
	xor	ch, ch				# (CX) contains high mantissa bits
	mov	ax, WORD PTR _FAC-3		# (AX) contains low mantissa bits of FAC
	mov	bh, ch				# set (BH)=0 as well
	mov	si, bx				# value pieces: 16-bit forms only
	mov	di, cx
	mov	bp, dx
	push	cx				# high FAC mantissa bits
	push	ax				# low FAC mantissa bits
	mul	dx				# 32 bit product formed (only need most 16 significant bits)
	mov	cx, dx
	pop	ax				# low FAC mantissa bits
	mul	bx				# times high mantissa bits of (BLDX)
	add	cx, ax				# add to previous calculation
	jnb	FMS10				# if carry not produced proceed
	inc	dx
FMS10:	mov	bx, dx				# probably only 8 bits here
	pop	dx				# high FAC mantissa bits
	mov	ax, bp				# low 16 mantissa bits of (BLDX)
	mul	dx
	add	cx, ax				# add in low order bits
	jnb	FMS20				# jump if carry not produced
	inc	dx
FMS20:	add	bx, dx				# can't produce carry here
	mov	dx, di				# high FAC mantissa bits
	mov	ax, si				# high FAC mantissa bits
	mul	dl				# (AX) has entire product
	add	bx, ax				# add it in
	jnb	FMS30				# if no carry proceed
	rcr	bx, 1				# move everything right
	rcr	cx, 1
	inc	BYTE PTR _FAC			# must now check for overflow
	jnz	FMS30				# proceed if non-zero
	jmp	_OVFLS
FMS30:						# product formed: get mantissa in (BLDXAH) for ROUNS
	or	bh, bh				# must be sure product left justified
	jns	FMS35				# in (BXCX)
	inc	BYTE PTR _FAC			# need to increment exp.
	jnz	FMS37				# if not overflow proceed
	jmp	_OVFLS				# overflow jump
FMS35:	rcl	cx, 1
	rcl	bx, 1
FMS37:	mov	dl, ch
	mov	dh, bl
	mov	bl, bh
	mov	ah, cl				# overflow byte
	jmp	_ROUNS				# go round

# $FOTAN / $FOTED: decimal point and leading zero / comma handling for output.
	.globl _FOTAN, _FOTED
_FOTAN:
	dec	ch				# if negative then leading zeros
	jns	FTD05
	mov	DWORD PTR _DPADR, ebx		# save decimal point pointer (4 bytes)
	mov	BYTE PTR [ebx], '.'		# move in decimal point
FTN10:	inc	ebx				# point to next output position
	mov	BYTE PTR [ebx], '0'		# put in leading zero
	inc	ch				# will increment CH until zero
	jnz	FTN10				# put in leading zeros until CH zero
	inc	ebx				# point to next buffer position
	xor	cx, cx				# zero out decimal point and comma cts.
	jmp	FTD20				# get stack right and return
_FOTED:
	dec	ch				# see if time for D.P.
FTD05:	jnz	FTD10				# if not D.P. time see if comma time
	mov	BYTE PTR [ebx], '.'		# put in D.P.
	mov	DWORD PTR _DPADR, ebx		# save addr of decimal point
	inc	ebx				# increment past D.P.
	xor	cx, cx				# zero counts & set ZF=1
	jmp	FTD20				# get stack right and return
FTD10:	dec	cl				# if zero time for comma
	jnz	FTD20
	mov	BYTE PTR [ebx], ','		# comma (LOW 54 octal)
	inc	ebx				# point to next buffer position
	mov	cl, 3
FTD20:	mov	WORD PTR _FMTCX, cx		# update D.P. & comma counts
	ret

# $FOTCV: convert FAC to ASCII digits (uses the power-of-ten tables; see FOTCI in math1).
	.globl _FOTCV
_FOTCV:	call	_GETYP				# set condition codes for VALTYP
	jpo	FCV40				# if single precision go process
	push	cx				# save digit and comma count
	push	ebx				# save buffer ptr.
	mov	esi, offset _DFACL		# will move FAC to ARG
	mov	edi, offset _ARGLO
	mov	ecx, 4
	cld
	rep	movsw
	call	_DINT				# will see if FAC already integer
	push	ebx				# need to call VCOMP with BX=ARG-1
	mov	ebx, offset _ARG-1
	call	_VCOMP				# do compare
	pop	ebx				# get buffer pointer back
	mov	esi, offset _ARGLO		# will move ARG to FAC
	mov	edi, offset _DFACL
	mov	ecx, 4
	cld
	rep	movsw
	jz	FCV05				# don't add .5 if no diff.
	call	_DADDH				# add .5 to number
FCV05:	mov	cl, BYTE PTR _FAC		# shift count in (CL)
	sub	cl, 0xB8			# LOW 270 octal
	neg	cl				# make shift count positive
	clc					# tell DINT not to normalize
	call	_FTDNT				# make sure it's still integer
	pop	ebx				# recall buffer ptr.
	pop	cx				# recall digit and comma count
	mov	esi, offset _FODTB		# double precision output integers
	mov	al, 9				# 9 digits to be converted with D.P. (LOW 9D)
FCV10:	call	_FOTED				# see if decimal point needed
	push	ax				# save digit count
	mov	al, '0'-1			# will form digit in (AL)
	push	ax
FCV20:	pop	ax				# recall digit
	inc	al				# increment to next digit
	push	ax				# save digit
	call	DSUBI				# subtract power of ten
	jnb	FCV20
	call	DADDI				# add power 10 back in
	pop	ax				# get digit back
	mov	BYTE PTR [ebx], al		# put in ASCII digit
	inc	ebx				# point to next buffer position
	pop	ax				# get digit count
	dec	al				# see if 10 digits formed
	jnz	FCV10				# if not continue the algorithm
	push	cx				# save decimal point count
	mov	esi, offset _DFACL		# will move into FAC
	mov	edi, offset _FACLO
	mov	ecx, 2
	cld
	rep	movsw
	pop	cx				# recall decimal point count
	jmp	FCV50				# go do the rest of the digits
FCV40:
	push	ebx				# save buffer ptr
	push	cx				# save digit and comma counts
	call	_PUSHF				# save $FAC on stack
	call	_INT				# will see if integer currently
	pop	dx				# recall FAC
	pop	bx
	call	_FCOMP				# compare: if equal don't add .5
	jz	FCV45
	mov	WORD PTR _FAC-1, bx		# move back to FAC
	mov	WORD PTR _FACLO, dx
	call	_FADDH				# add .5 to number
FCV45:	mov	al, 1				# flag to QINT we have a positive no.
	call	_QINT				# get integer into (BLDX)
	mov	WORD PTR _FAC-1, bx
	mov	WORD PTR _FACLO, dx		# move to FAC
	pop	cx				# recall digit and comma counts
	pop	ebx				# recall buffer ptr
FCV50:
	mov	al, 3				# will convert 3 digits in this code
	mov	edx, offset _FOSTB		# print S.P. numbers with 7 digits
FCV60:	call	_FOTED				# see if need a decimal point or comma
	push	ax				# save digit count
	push	ebx				# save buffer pointer
	push	edx				# save power of ten pointer (4 bytes)
	call	_MOVRF				# fetch integer
	pop	ebp				# fetch power ten pointer
	mov	al, '0'-1			# will build digit in (AL)
	push	ax				# save digit
FCV70:	pop	ax				# recall digit
	inc	al				# go to next digit
	push	ax
	call	_RSUBM				# subtract no. pointed to by (BP)
	jnb	FCV70				# continue until CF=1
	add	dx, WORD PTR [ebp]		# add word portion
	adc	bl, BYTE PTR [ebp+2]
	inc	ebp				# increment to next power of ten (3-byte entries)
	inc	ebp
	inc	ebp
	call	_MOVFR				# save (BLDX) in FAC
	pop	ax				# recall digit
	xchg	edx, ebp			# save power ten ptr. in DX
	pop	ebx				# recall buffer pointer
	mov	BYTE PTR [ebx], al		# send out digit
	inc	ebx				# increment to next buffer position
	pop	ax				# recall digit count
	dec	al				# see if finished
	jnz	FCV60				# if not continue
	inc	edx				# need to increment past 1st
	inc	edx				# integer so that FOTCI will
	mov	ebp, edx			# FOTCI is expecting pointer in BP
	mov	ah, 4				# convert only 4 digits
	jmp	_FCI4

# DSUBI / DADDI: subtract / add the 7-byte integer at (ESI) from / to $DFACL.
DSUBI:	push	cx				# save CX, SI, DI
	push	esi
	mov	ecx, 7				# 7 bytes
	mov	edi, offset _DFACL
	clc
	cld					# so LODS will increment
DSUBI1:	lodsb					# fetch byte from table
	sbb	BYTE PTR [edi], al		# subtract
	inc	edi
	loop	DSUBI1
	pop	esi
	pop	cx
	ret
DADDI:	push	cx				# save CX
	mov	ecx, 7
	mov	edi, offset _DFACL
	clc
	cld					# so LODS will increment SI
DADDI1:	lodsb					# fetch next byte to add
	adc	BYTE PTR [edi], al		# add it in
	inc	edi
	loop	DADDI1
	pop	cx
	ret

# $FOTNV: bracket FAC so the integer part has printable digits.
	.globl _FOTNV
_FOTNV:
	push	ebx				# won't alter (BX) or (CX)
	push	cx
	xor	di, di				# initialize exponent
	push	di				# save exponent
FNV10:	mov	ebx, offset _FOTB		# address of bracket control table
	mov	al, BYTE PTR _FAC		# fetch the exponent
	xlat					# get multiplier
	or	al, al				# if zero: done
	jz	FNV20
	pop	di				# recall exponent
	cbw					# convert AL to word
	sub	di, ax				# get exponent correct
	push	di				# save exponent
	mov	dx, ax				# DX := exponent for MDPTEN
	call	MDPTEN				# multiply or divide by power of ten
	jmp	FNV10				# see if need to do it again
FNV20:	mov	ebx, offset _DP06+4		# lower bound
	call	_MOVBS				# move out to "DS" area
	call	_COMPM				# one more mult. possible
	jnb	FNV30				# jump if not needed
	call	_MUL10				# multiply by ten
	pop	di				# recall exponent
	dec	di				# account for multiply by 10
	push	di
FNV30:	call	_GETYP				# set condition codes for type
	jb	FNV40				# done if single precision
	mov	ebx, offset _DP09		# must multiply by 10^9
	call	_MOVAC				# move 10^9 to $ARG
	call	_FMULD				# perform multiplication
	pop	ax				# recall exponent
	sub	al, 9				# LOW 11 octal: subtract 9
	push	ax				# resave the exponent
	mov	ebx, offset HIDBL		# is the number too big? (will it overflow
	call	_MOVBF				# when $FOTCV adds .5 to it?)
	call	_DCMPM
	jna	FNV40				# no
FNV44:	call	_DIV10				# yes: divide by ten and fix up the
	pop	ax				# restore the exponent
	inc	al				# adjust for $DIV10
	push	ax
FNV40:	pop	ax				# recall exponent
FNV50:	pop	cx				# restore registers
	pop	ebx
	or	al, al				# set condition codes according to exp
	ret

# 9999999999999999. as 8 bytes (octal source values: 375,377,3,277,311,33,16,266)
	.section .data
HIDBL:	.byte 0xFD, 0xFF, 0x03, 0xBF, 0xC9, 0x1B, 0x0E, 0xB6
	.section .text

# $FOUT: control output conversion. S: sign fix-up.
S:	mov	ebx, offset _FBUFF+1		# fetch buffer pointer
	mov	BYTE PTR [ebx], ' '		# move in space for possible sign
	push	ebx				# save buffer pointer
	call	_SIGNS				# determine sign of number
	pop	ebx				# recall buffer pointer
	pushfd					# save flags for later
	jns	FO20				# jump if positive
	mov	BYTE PTR [ebx], '-'		# put in minus sign
	push	ebx				# save text pointer
	call	_VNEG				# negate no. so we work only with positive nos.
	pop	ebx				# recall text pointer
	or	al, 1				# pos. nos. and set ZF=0
FO20:	inc	ebx				# point to next buffer position
	mov	BYTE PTR [ebx], '0'		# put in zero in case no is zero
	popfd					# recall flags
	ret
	.globl _FOUT, FOUT
FOUT:
_FOUT:						# free-format entry point
	call	S				# do sign fix-up
	jnz	_FOUT2				# if non-zero proceed
	inc	ebx				# point to next output buffer pos.
	mov	BYTE PTR [ebx], 0		# indicate end of number
	mov	ebx, offset _FBUFF+1		# point (BX) to start position
	ret
_FOUT2:	call	_GETYP				# get type no.
	jns	FO50				# go format single or double prec.
	mov	cx, 0x0700			# default 7 digits prior to dp (7*400 octal)
	xor	ax, ax				# clear comma count
	mov	WORD PTR _FMTAX, ax
	mov	WORD PTR _FMTCX, cx
	call	_FOTCI				# convert integer to ASCII
	jmp	_FOTZS				# do leading zero suppression
FO50:	jmp	_FOFMT				# single or double prec. output

# $DINT: double precision INT function (truncation).
	.globl _DINT, _FTDNT
_DINT:	mov	cl, BYTE PTR _FAC		# CL := exponent
	sub	cl, 0xB8			# LOW 270 octal: is there a fractional part?
	jnb	DNT20				# return if no fractional bits
	neg	cl				# CL now positive
_FTDNT:	pushfd					# FOUT entry point. this is separate
	test	BYTE PTR _FAC, 0xFF		# is the exponent zero? (LOW 377 octal)
	jne	DINTNZ				# no, proceed
	popfd					# yes: if the exponent is zero the
	ret					# number is zero. don't operate on it
DINTNZ:	mov	ebx, offset _FAC-2
	mov	al, BYTE PTR [ebx+1]		# fetch sign byte
	mov	BYTE PTR [ebx+3], al		# and put in $FAC+1 for $NORMD
	or	al, al				# see if negative
	pushfd
	or	al, 0x80			# restore hidden 1 (LOW 200 octal)
	mov	BYTE PTR [ebx+1], al		# and replace
	mov	BYTE PTR [ebx+2], 0xB8		# set exponent for post shift value (LOW 270 octal)
	popfd					# recall SF
	pushfd
	jns	DNT10				# if positive proceed
	call	DINTA				# subtract 1 from mantissa bits
DNT10:	xor	ch, ch				# (CX)=shift count
	call	_SHRD				# double precision shift right
	popfd					# recall SF
	jns	DNT15				# if positive proceed
	call	ADD1D				# add 1 to mantissa bits
DNT15:	mov	BYTE PTR _DFACL-1, 0		# zero the overflow byte
	popfd					# see if called by FOUT (CF=0)
	jnb	DNT20				# if so just return
	jmp	_NORMD				# otherwise normalize
DNT20:	ret

# $SHRD: shift right double precision (CX = bits to shift).
	.globl _SHRD
_SHRD:	movzx	ecx, cx
SHRD10:	push	ecx				# save outer loop variable (bits to shift)
	push	ebx				# save pointer to high byte to shift
	clc					# CF=0
	call	_SHDR				# shift 1 bit right
	pop	ebx
	pop	ecx				# get outer loop variable
	loop	SHRD10
	ret
DINTA:	push	ebx
	mov	ebx, offset _DFACL		# beginning address for subtract
DINA10:	sub	WORD PTR [ebx], 1		# need CF so can't use DEC
	jnb	DINA20
	inc	ebx
	inc	ebx				# can do word subtracts since high bit
	jmp	DINA10
DINA20:	pop	ebx
	ret
ADD1D:						# add 1 to double precision mantissa bits
	push	ebx
	mov	ebx, offset _DFACL
ADD10:	inc	BYTE PTR [ebx]
	jnz	ADD20
	inc	ebx				# point to next byte: there was a carry
	jmp	ADD10
ADD20:	pop	ebx
	ret

# $INT: single precision INT function (truncation).
	.globl _INT, INT
INT:
_INT:	jmp	SNGINT				# single precision INT: see SNGINT below
QINTX:						# $QINTX entry point (QINT only)
	mov	dx, WORD PTR _FACLO		# fetch low mantissa bits
	mov	bx, WORD PTR _FAC-1		# fetch exp, sign, high mantissa bits
	inc	bh				# is the exponent zero? (test for zero
	dec	bh				# without affecting the carry)
	jne	QINTNZ				# no, proceed
	xor	bl, bl				# yes: put zero into BL,DX for QINT
	xor	dx, dx				# rather than work with garbage mantissa
	ret
QINTNZ:	pushfd					# save carry (clear indicates no. was zero)
	or	bl, bl				# see if negative
	pushfd
	mov	BYTE PTR _FAC+1, bl		# save sign for NORMS
	mov	BYTE PTR _FAC, 0x98		# set exp for post shift (LOW 230 octal)
	or	bl, 0x80			# restore hidden 1 (LOW 200 octal)
	popfd
	pushfd					# save sign
	jns	INT10
	sub	dx, 1				# can't do a 'DEC' because need CF
	sbb	bl, 0				# do appropriate subtract to high byte
INT10:	xor	ch, ch				# CX has loop count
	or	cl, cl				# if shift count zero must jump over
	jz	INT12				# right shifts
INT11:	shr	bl, 1				# shift right into CF
	rcr	dx, 1				# rotate right
	loop	INT11				# will do (CX) right shifts
INT12:	popfd					# recall sign of no.
	lahf					# store flags temporarily
	jns	INT15				# proceed if positive
	inc	dx
	jnz	INT15
	inc	bl
INT15:	popfd					# CF=0 if called by QINT
	jnb	INT20				# just return if QINT call
	xor	ah, ah				# clear overflow byte
	jmp	_NORMS				# normalize and return
INT20:	sahf					# must see if negative
NGBLDX:	jns	INT30				# if not proceed as normal
	not	dx				# complement DX
	not	bl				# and BL
	add	dx, 1				# need CF set if DX overflows
	adc	bl, 0				# 2's complement now formed
INT30:	ret

# SNGINT: single precision INT(FAC) = floor(FAC), result left in FAC.
# Packed single as the $INT and $DINT code reads it: FACLO (2 bytes) low mantissa; FACM1 holds the sign
# in bit 7 and the high mantissa in bits 6..0, with the leading 1 implicit; FAC is the exponent
# (value = mantissa * 2^(exponent-152)). FAC+1 is set to the sign too, for the routines that test it.
# The truncated magnitude n = m >> s (s = 152 - exponent). A discarded fraction makes a negative result
# one larger in magnitude (floor): INT(-1.5) = -2, INT(-0.5) = -1. Registers are preserved.
SNGINT:
	push	eax				# save the registers (GAS 1.38, used by the QDOS build, has no PUSHAD)
	push	ecx
	push	edx
	push	ebx
	push	ebp
	push	esi
	push	edi
	movzx	eax, BYTE PTR _FAC		# exponent
	or	eax, eax
	jz	sni_out				# zero: already an integer
	cmp	eax, 0x98
	jae	sni_out				# magnitude >= 2^24: already an integer
	movzx	ebx, BYTE PTR _FACM1		# sign and high mantissa
	mov	ebp, ebx
	shr	ebp, 7				# EBP = 1 when negative
	and	ebx, 0x7F
	or	ebx, 0x80			# restore the hidden 1
	shl	ebx, 16
	movzx	edx, WORD PTR _FACLO		# low mantissa word
	or	ebx, edx			# EBX = 24-bit mantissa m
	mov	ecx, 0x98
	sub	ecx, eax			# CL = shift count s (1..151)
	xor	edi, edi			# EDI = n (truncated magnitude)
	xor	esi, esi			# ESI = 1 when a fraction was discarded
	cmp	ecx, 24
	jae	sni_frac			# s >= 24: n = 0, and the fraction is nonzero
	mov	edi, ebx
	shr	edi, cl				# n = m >> s
	mov	edx, 1
	shl	edx, cl
	dec	edx				# mask = 2^s - 1
	and	ebx, edx			# remainder: the discarded fraction
	jz	sni_have
	mov	esi, 1
	jmp	sni_have
sni_frac:
	mov	esi, 1
sni_have:
	mov	eax, edi			# positive: result = n
	test	ebp, ebp
	jz	sni_store
	add	eax, esi			# negative: result = -(n + fraction)
	neg	eax
sni_store:
	test	eax, eax
	jz	sni_zero
	mov	ebp, 0				# EBP = sign of the result (0 positive); MOV keeps the flags
	jns	sni_mag
	neg	eax
	mov	ebp, 1
sni_mag:
	# Normalise EAX (1 .. 2^24) so that its leading 1 is at bit 23; EDX = bit index of that 1.
	# (A loop, not BSR: the QDOS translator, tools/intel2att.py, has no BSR.)
	cmp	eax, 0x1000000			# 2^24 can occur (a negative fraction rounded up): index 24,
	jnz	sni_norm0			# with mantissa 1.0, so it needs no shifting
	mov	eax, 0x800000
	mov	edx, 24
	jmp	sni_pack
sni_norm0:
	xor	ecx, ecx			# CL = shift count
sni_norm:
	test	eax, 0x800000			# leading 1 at bit 23 yet?
	jnz	sni_normd
	shl	eax, 1
	inc	ecx
	jmp	sni_norm			# EAX is nonzero and below 2^24, so this stops
sni_normd:
	mov	edx, 23
	sub	edx, ecx			# index = 23 - shift count
sni_pack:
	mov	WORD PTR _FACLO, ax		# low mantissa word
	shr	eax, 16
	and	eax, 0x7F			# leading 1 is implicit
	mov	ecx, ebp
	shl	ecx, 7				# sign in bit 7 of the high mantissa byte
	or	eax, ecx
	mov	BYTE PTR _FACM1, al
	mov	BYTE PTR _FAC+1, cl
	lea	ecx, [edx+129]			# exponent: 129 + index (1.0: index 0, exponent 0x81)
	mov	BYTE PTR _FAC, cl
	jmp	sni_out
sni_zero:
	mov	BYTE PTR _FAC, 0
	mov	BYTE PTR _FAC+1, 0
sni_out:
	pop	edi
	pop	esi
	pop	ebp
	pop	ebx
	pop	edx
	pop	ecx
	pop	eax
	ret

# $QINT: INT(FAC) left in (BLDX).
	.globl _QINT, QINT
QINT:
_QINT:	mov	cl, 0x98			# LOW 230 octal
	sub	cl, BYTE PTR _FAC		# get shift count to CL
	clc					# CF=0
	jmp	QINTX				# leave right adjusted

# $LOG: single precision natural log.
	.globl _LOG, LOG
LOG:
_LOG:	call	_SIGNS				# error if (FAC) .LE. 0
	jle	LG100
	mov	dx, 0				# fetch 1.0 to (BXDX)
	mov	bx, 0x8100			# 100400 octal
	call	_FCOMP
	jnz	LOG10				# proceed if not 1.0
	mov	WORD PTR _FACLO, dx		# will zero $FAC and return
	mov	WORD PTR _FACM1, dx
	ret
LOG10:	mov	al, BYTE PTR _FAC		# fetch exponent
	sub	al, 0x80			# take out bias (LOW 200 octal)
	cbw					# convert byte to word
	push	ax				# and save
	mov	BYTE PTR _FAC, 0x80		# zero the exponent (LOW 200 octal)
	call	_PUSHF				# save arg for Q(X) calculation
	mov	ebx, offset _LOGP		# now use Hart approx for P(X)
	call	_POLY
	pop	dx				# get X off the stack
	pop	bx
	call	_PUSHF				# save P(X)
	call	_MOVFR				# move X to FAC
	mov	ebx, offset _LOGQ
	call	_POLY				# calculate Q(X)
	pop	dx				# recall P(X)
	pop	bx
	call	_FDIVS				# calculate P(X)/Q(X)
	pop	dx				# fetch raw exponent
	call	_PUSHF				# save FAC on the stack
	call	_FLT				# float the value into the $FAC
	pop	dx				# recall LOG2 value
	pop	bx
	call	_FADDS				# add
MLLN2:	mov	bx, 0x8031			# 100061 octal: fetch LN(2) to (BXDX)
	mov	dx, 0x7218			# 71030 octal
	jmp	_FMULS				# multiply to complete
LG100:	jmp	_FCERR

# $NORMD: double precision normalization.
	.globl _NORMD
_NORMD:	mov	dl, 57				# max bits to shift left (LOW 71 octal)
	mov	ebx, offset _DFACL-1
NORD5:	mov	edi, offset _FAC-1
	mov	esi, offset _FAC
	jmp	NORD30
NORD10:	mov	ecx, 4
	clc					# CF=0
NORD20:	rcl	WORD PTR [ebx], 1
	inc	ebx
	inc	ebx				# point to next word
	loop	NORD20
	mov	ebx, offset _DFACL-1		# point back to end of number
NORD25:	dec	BYTE PTR [esi]			# decrement exponent
	jz	NORD40				# do clean-up if underflow
	dec	dl				# see if max bits shifted
	jz	NORD40				# if so terminate shifts
NORD30:	test	BYTE PTR [edi], 0xFF		# SF=1 if now normalized (LOW 377 octal)
	js	NORD40				# normalized
	jnz	NORD10				# must shift bit at a time
	sub	BYTE PTR [esi], 8		# subtract 8 (LOW 10 octal)
	jbe	NORD40				# underflow
	sub	dl, 8				# see if max bits shifted (LOW 10 octal)
	jbe	NORD40				# and if so quit
	mov	esi, offset _FAC-2
	mov	ecx, 7				# 7 bytes to move
	std					# so following MOVSB will decrement
	rep	movsb				# move
	mov	BYTE PTR _DFACL-1, 0		# zero overflow
	jmp	NORD5				# see if more cases
NORD40:	jbe	NORD50				# underflow jump
	jmp	_ROUND
NORD50:	jmp	_DZERO

# $NORMS: single precision normalization.
	.globl _NORMS
_NORMS:	mov	bh, BYTE PTR _FAC		# exponent to BH
	mov	ecx, 4
NOR10:	or	bl, bl				# see if sign bit set
	js	NOR20				# if so normalization complete
	jnz	NOR15				# upper byte non-zero
	sub	bh, 8				# can we subtract 8 w/o underflow? (LOW 10 octal)
	jbe	NOR17
	mov	bl, dh
	mov	dh, dl
	mov	dl, ah
	xor	ah, ah				# clear overflow byte
	loop	NOR10
	jz	NOR17				# underflow!
NOR15:	clc					# clear carry flag
	rcl	ah, 1				# shift overflow byte left
	rcl	dx, 1				# shift lower mantissa word left
	rcl	bl, 1				# shift high mantissa byte left
NOR16:	dec	bh				# decrement exponent
	jnz	NOR10				# continue unless underflow
NOR17:	jmp	_ZERO				# zero the FAC and return
NOR20:	mov	BYTE PTR _FAC, bh		# update exponent
	jmp	_ROUNS

# $POLYX / $POLY: polynomial evaluator. Coefficient table pointer is in BX (32-bit).
	.globl _POLYX, _POLY
_POLYX:	mov	dx, WORD PTR _FACLO		# fetch low bytes
	mov	bp, WORD PTR _FACM1		# and high bytes
	push	dx				# save X on the stack
	push	bp
	push	ebx				# save coefficient pointer
	mov	bx, bp				# (BXDX)=X
	call	_FMULS				# form X**2
	pop	ebx				# recall coefficient pointer
	call	_POLY				# form P(X**2)
	pop	bx				# fetch X to registers
	pop	dx
	jmp	_FMULS
_POLY:	cld					# so LODS and MOVSW increment
	mov	esi, ebx			# so we can use string moves
	lodsb					# fetch number of coefficients
	cbw					# (AH)=0
	push	ax				# push number of elements on stack
	push	WORD PTR _FACM1			# save the FAC on the stack
	push	WORD PTR _FACLO
	mov	edi, offset _FACLO
	movsw
	movsw
POL10:	pop	dx				# fetch low X
	pop	bx				# fetch high X
	pop	ax				# recall coefficient counter
	dec	ax				# decrement
	jz	POL30				# jump when complete
	push	ax				# save counter
	push	bx				# save X on the stack
	push	dx
	push	esi				# save pointer to coefficients
	call	_FMULS				# multiply by X
	pop	esi				# fetch coefficient pointer
	cld
	lodsw					# fetch low word
	xchg	ax, dx
	lodsw					# fetch high word
	xchg	ax, bx
	push	esi				# save coefficient pointer
	call	_FADDS				# add it in
	pop	esi				# get pointer back
	jmp	POL10
POL30:	ret					# clear the stack (values popped above)

# $PUF: print-using for floating point numbers.
	.globl _PUF
_PUF:	push	ebx				# save output buffer pointer
	shr	al, 1				# CF=1 if "E" format desired
	jnb	PUF10				# if fixed proceed
	jmp	_PUFE				# go do it
PUF10:	mov	ebx, offset _DP16		# will first see if no. too large
	call	_MOVBF				# no. moved to DBUFF for comparison
	call	_VCOMP				# (FAC)-10^16 (CF=1 if OK)
	jb	PUF30				# jump if small enough to process
	pop	ebx				# recall print buffer pointer
	call	_FOUT				# print in free-format
	dec	ebx				# point to position 1
	mov	BYTE PTR [ebx], '%'		# put in overflow indicator
	ret					# done
PUF30:	call	_GETYP				# determine type
	mov	ch, 16				# assume double prec. (16 printable pos, LOW 20 octal)
	jnb	PUF40
	mov	ch, 7				# 7 print positions if single precision
PUF40:	call	_SIGNS				# see if no. is zero
	jz	PUF50				# if so don't call $FOTNV
	call	_FOTNV				# bracket no. so all printable digits
PUF50:	pop	ebx				# get output buffer pointer back
	js	PUF80				# go print no. with digits to right
	mov	dl, al				# save this positive exp. for later
	add	al, ch				# this is necessary print positions
	sub	al, BYTE PTR _FMTCX+1		# subtract digits to left
	jns	PUF60				# no leading zeros required
	neg	al				# must make (AL) positive for $FOTZ
	call	_FOTZ				# put in (AL) leading zeros
PUF60:	xor	cl, cl				# zero created zeros
	call	DPCOM				# set-up decimal point and comma ct.
	push	WORD PTR _FMTCX			# save decimal point info.
	push	dx				# save exponent
	call	_FOTCV				# convert the binary no. to ASCII digits
	pop	dx				# recall exponent
	pop	WORD PTR _FMTCX			# recall decimal point info.
	push	WORD PTR _FMTCX			# save desired positions left/rt.
	xor	al, al				# want DL in AL with appropriate signs
	or	al, dl				# DL=exponent
	jz	PUF70				# if no zeros prior to D.P. jump
	call	_FOTZC				# put in zeros and pay attention to
	call	_FOTED				# put in decimal point
PUF70:	pop	WORD PTR _FMTCX			# recall desired positions left/rt.
	push	WORD PTR _FMTCX			# and save again
	mov	al, BYTE PTR _FMTCX		# fetch no. positions to rt. of D.P.
	jmp	_PUIZ				# go put in any required trailing zeros
PUF80:	mov	dl, al				# save the complementing exponent
	mov	al, BYTE PTR _FMTCX		# fetch digits desired to right
	or	al, al				# if non-zero must subtract 1
	jz	PUF90				# proceed if no dig. to rt. desired
	dec	al				# take out for decimal point count
PUF90:	mov	dh, al				# save desired digits to right count
	add	al, dl				# subtract digits available to rt.
	mov	cl, al				# will need this for later
	js	PUF100				# jump if internal rounding necessary
	xor	al, al				# will need all printable digits
	mov	cl, al				# no digits shifted off
	jmp	PUF110				# do not need to divide if positive
PUF100:	push	ax
	push	cx				# save generated leading zeros
	push	dx
	push	ebx
	call	_DIV10				# need to eliminate excess printable
	pop	ebx
	pop	dx
	pop	cx				# recall generated leading zeros
	pop	ax				# recall excess digits
	inc	al				# because AL was originally negative
	js	PUF100				# continue for internal rounding
	mov	ah, cl				# will need created leading zeros
PUF110:	mov	al, dl				# get negative exponent
	sub	al, cl				# add created zeros
	add	al, ch				# if SF=1 then no digits to left
	jns	PUF120				# jump if digits to left of decimal pt.
	mov	al, BYTE PTR _FMTCX+1		# desired leading digits
	call	_FOTZ				# insert (AL) leading zeros
	mov	BYTE PTR [ebx], '.'		# put in decimal point
	mov	DWORD PTR _DPADR, ebx		# save DP position for $FOTED, etc.
	inc	ebx				# next buffer position
	xor	cl, cl				# comma count to zero
	mov	al, dh				# number of digits after decimal point
	sub	al, ch				# subtract number that will be printed by $FOTCV
	call	_FOTZ				# put in (AL) leading zeros
	xor	cx, cx				# no longer need comma & decimal info
	push	dx				# balance PUSHes and POPs
	push	WORD PTR _FMTCX
	jmp	PUF140				# go put in the digits
PUF120:	mov	al, BYTE PTR _FMTCX+1		# fetch desired digits
	push	dx				# save digits to right
	push	WORD PTR _FMTCX			# save digits to left/right
	sub	al, ch				# subtract printable digits
	sub	al, dl				# add digits to right
	add	al, cl				# subtract created leading zeros
	js	PUF130				# if no leading zeros required jump
	call	_FOTZ				# put in required leading zeros
PUF130:	call	DPCOM				# set up decimal point and comma cts.
PUF140:	push	WORD PTR _FMTCX			# save decimal point info.
	call	_FOTCV				# convert FAC to ASCII digits and
	pop	WORD PTR _FMTCX			# retrieve the right and left counts.
	pop	ax				# recall digits to left
	pop	dx				# recall digits to right
	test	BYTE PTR _FMTCX, 0xFF		# is the right side count zero? (LOW 377 octal)
	jnz	PUF150				# no, retain the decimal point
	mov	ebx, DWORD PTR _DPADR		# yes, get rid of the decimal point
	jmp	_PUFXE				# go do fix-up
PUF150:	add	al, dl				# subtract digits to the right
	dec	al				# because decimal point was included
	js	PUF160				# go do fix-up
	call	_FOTZ				# put in trailing zeros
PUF160:	jmp	_PUFXE				# go do fix-up

# DPCOM: set up decimal point and comma counts.
DPCOM:	mov	al, ch				# fetch printable digits
	add	al, dl				# subtract digits to right
	sub	al, cl				# add created zeros
	inc	al				# for sign
	mov	ch, al
DPC10:	sub	al, 3				# subtract out AL multiples of 3
	jg	DPC10
	add	al, 3
	mov	cl, al				# comma count now set
	mov	al, BYTE PTR _FMTAX		# fetch format flags
	and	al, 0x40			# see if commas desired (LOW 100 octal)
	jnz	DPC20
	mov	cl, al				# zero comma count
DPC20:	ret

# $PUFOT: print-using output routine.
	.globl _PUFOT, PUFOUT
PUFOUT:
_PUFOT:
	mov	ah, al				# fetch format specs
	test	ah, 0x40			# ZF=0 if commas desired (LOW 100 octal)
	mov	ah, 3				# if commas desired
	jnz	PFO05				# jump if commas desired
	xor	ah, ah				# commas not desired
PFO05:	mov	WORD PTR _FMTAX, ax		# save comma count and format specs
	mov	WORD PTR _FMTCX, cx		# save positions to rt./left
	mov	ah, al				# for testing
	mov	ebx, offset _FBUFF+1		# fetch buffer pointer
	mov	BYTE PTR [ebx], ' '		# move in space for possible sign
	test	ah, 8				# see if "+" desired (LOW 10 octal)
	jz	PFO10				# jump if not
	mov	BYTE PTR [ebx], '+'		# move in plus
PFO10:	push	ebx				# save buffer pointer
	call	_SIGNS				# determine sign of number
	pop	ebx				# recall buffer pointer
	jns	PFO20				# jump if positive
	mov	BYTE PTR [ebx], '-'		# put in minus sgn
	push	ebx				# save buffer ptr.
	call	_VNEG				# negate no. so we work only with positive
	pop	ebx				# recall buffer ptr.
PFO20:	inc	ebx				# point to next buffer position
	mov	BYTE PTR [ebx], '0'		# put in zero in case no is zero
	call	_GETYP				# set cond codes according to type
	mov	ax, WORD PTR _FMTAX		# fetch format specs
	mov	cx, WORD PTR _FMTCX
	js	PFO40				# if integer jump
	jmp	_PUF				# do floating point formatting
PFO40:	jmp	_PUI				# process integer

# $PUI: print using for integers.
	.globl _PUI
_PUI:	mov	ax, WORD PTR _FMTAX		# fetch print description data
	mov	cl, ah				# $FOTCI will need comma info
	mov	ch, 6				# at most 6 places prior to decimal pt
	shr	al, 1				# sets CF=1 if "E" type print desired
	mov	dx, WORD PTR _FMTCX		# fetch places to left/right of dec. pt
	jnb	PI10				# if not "E" format jump
	push	ebx				# save buffer pointer
	push	dx				# save places to left/right of dec. pt
	call	_CSI				# convert the integer to S.P.
	xor	al, al				# must set ZF=1 for $PUFE
	pop	dx				# fetch places to left/right of D.P.
	jmp	_PUFE				# go format the single precision no.
PI10:	mov	al, dh				# fetch places to left of D.P.
	sub	al, 5				# let's see if leading zeros needed
	js	PI20				# jump if less than 5
	call	_FOTZ				# zero fill (AL) positions
PI20:	push	dx				# save positions to left/right of D.P.
	call	_FOTCI				# convert the integer to ASCII digits
	pop	ax				# recall no. places to left/right of D.P.
	push	ax				# and save again
_PUIZ:	or	al, al				# if 0 then no decimal point desired
	jnz	PI30				# if desired proceed
	dec	ebx				# this will eliminate decimal point
PI30:	dec	al				# may need to insert zeros to right
	js	PI40				# jump if not desired
	call	_FOTZ				# put (AL) ASCII zeros in buffer
	mov	BYTE PTR [ebx], 0		# re-establish end-of-print
PI40:	pop	WORD PTR _FMTCX			# maintain digits left/rt. of dp.
	jmp	_PUFXE				# be sure format specs met

# $RND: pseudo-random number generator.
	.globl _RND
RND:
_RND:	call	_SIGNS				# find which mode requested
	jz	OLDRN				# return previous no.?
	jns	RND10				# go do new seed
	mov	ax, WORD PTR _FACLO		# fetch number
	mov	WORD PTR _RNDX, ax		# and initialize last random #
	mov	al, BYTE PTR _FAC-1
	mov	BYTE PTR _RNDX+2, al		# so sequence starts again
RND10:	mov	ax, WORD PTR _RNDX		# fetch low 16 bits of RNDX
	mul	WORD PTR _RNDA			# multiply by low bits of A
	mov	di, ax				# save low 16 bits
	mov	cl, dl				# save upper 8 bits
	mov	al, BYTE PTR _RNDA+2		# fetch upper 8 bits of A
	xor	ah, ah				# clear upper AX
	mul	WORD PTR _RNDX			# multiply by low RNDX
	add	cl, al				# add to upper 8 bits
	xor	ah, ah				# clear upper AX
	mov	al, BYTE PTR _RNDX+2		# fetch high 8 bits of RNDX
	mul	WORD PTR _RNDA			# multiply by low 16 of A
	add	cl, al				# add in to upper 8
	xor	ah, ah				# clear overflow byte for norm
	mov	dx, WORD PTR _RNDC		# fetch low 16 of C
	add	dx, di				# add in low of RND(N)*A
	mov	bl, BYTE PTR _RNDC+2		# fetch high of C
	adc	bl, cl				# add with carry high RND(N)*A
	mov	BYTE PTR _FAC+1, ah		# sign is positive
	mov	al, 0x80			# so NORM will produce no. (LOW 200 octal)
	mov	BYTE PTR _FAC, al
	mov	WORD PTR _RNDX, dx		# save new RND(N+1)
	mov	BYTE PTR _RNDX+2, bl
	mov	al, 4				# must set VALTP to single prec.
	mov	BYTE PTR _VALTP, al
	jmp	_NORMS
NEWSD:						# determine new random no. seed
	mov	ebx, offset _FBUFF		# will sum the first 32 words of RAM
	mov	ecx, 32				# LOW 40 octal
NEW10:	add	ax, WORD PTR [ebx]
	inc	ebx
	inc	ebx
	loop	NEW10
	and	al, 0xFE			# be sure bit 0=0 so as to not (LOW 376 octal)
	mov	WORD PTR _RNDX, ax
	jmp	RND10				# go proceed as usual
OLDRN:						# return the previous random no.
	mov	dx, WORD PTR _RNDX		# fetch low 16 bits
	mov	bl, BYTE PTR _RNDX+2		# fetch high 8 bits
OLDRN2:
	xor	ax, ax				# clear overflow byte (AH)
	mov	al, 0x80			# exponent of 0 (LOW 200 octal)
	mov	BYTE PTR _FAC, al
	mov	BYTE PTR _FAC+1, ah		# positive sign
	jmp	_NORMS				# go normalize

# $ROUND: double precision round and pack.
	.globl _ROUND, _ROUNX
_ROUND:						# double precision round and pack
	mov	ebx, offset _DFACL-1
	add	WORD PTR [ebx], 0x80		# add to high bit of overflow byte (LOW 200 octal)
	mov	ecx, 3				# 3 more bytes to look at potentially
	jnb	RDD20				# if CF=0 we are done
RDD10:	inc	ebx
	inc	ebx
	inc	WORD PTR [ebx]			# if this gets ZF=1 then carry
	jnz	RDD20				# finished when ZF=0
	loop	RDD10
	inc	BYTE PTR _FAC			# must increment exponent
	rcr	WORD PTR [ebx], 1		# set high byte to 200
RDD20:	jz	RDD30				# overflow hook
	test	BYTE PTR _DFACL-1, 0xFF		# see if overflow byte zero (LOW 377 octal)
	jnz	_ROUNX
	and	BYTE PTR _DFACL, 0xFE		# make answer even (LOW 376 octal)
_ROUNX:
	and	BYTE PTR _FAC-1, 0x7F		# clear sign bit (LOW 177 octal)
	mov	al, BYTE PTR _FAC+1		# fetch sign byte
	and	al, 0x80			# clear all but sign (LOW 200 octal)
	or	BYTE PTR _FAC-1, al		# and set sign appropriately
	ret
RDD30:	jmp	_OVFLS

# $ROUNS / $ROUNM: single precision rounding. Entry: AH=overflow byte, DX/BL mantissa.
	.globl _ROUNS, _ROUNM
_ROUNS:	and	ah, 0xE0			# clear superfluous bits (LOW 340 octal)
_ROUNM:	add	ah, 0x80			# add to most significant bit of AH (LOW 200 octal)
	jnb	PAKSP				# if no carry return
	pushfd					# if ZF=1 want to round to even
	inc	dx				# if ZF=1 must increment BL
	jnz	TSTEVN
	popfd					# know result will be even
	inc	bl				# if ZF=1 must increment exponent
	jnz	PAKSP
	stc					# CF=1
	rcr	bl, 1				# this will set high bit of BL
	inc	BYTE PTR _FAC			# if this causes ($FAC)=0 we have
	jnz	PAKSP
	jmp	_OVFLS
TSTEVN:	popfd					# if ZF=1 must clear low bit of DL
	jnz	PAKSP				# go pack the FAC
	and	dl, 0xFE			# clear low bit (LOW 376 octal)
PAKSP:						# pack single precision FAC. exponent in FAC, sign in FAC+1
	mov	esi, offset _FAC-3		# load address of $FAC in SI
	mov	WORD PTR [esi], dx		# move lower mantissa word in
	inc	esi				# increment to high mantissa byte
	inc	esi
	mov	bh, BYTE PTR _FAC+1		# fetch sign
	and	bx, 0x807F			# clear all but sign in BH; sign in BL (100177 octal)
	or	bl, bh				# (BL) now in correct format
	mov	BYTE PTR [esi], bl		# put into FAC-1
	ret

# $SIGD: count significant digits for $FOUT.
	.globl _SIGD
_SIGD:						# (CL)=(CL)+1 for each zero digit starting from
	mov	si, cx				# save CX
	call	_VPSHF				# push either 4 or 8 bytes of $FAC
	mov	cx, si				# must maintain (CL)
	push	cx
	call	_GETYP
	jb	SI31				# jump if single prec
	cmp	BYTE PTR _FAC, 0xB8		# LOW 270 octal: if equal or SF=0 then integer already
	jns	SI20				# don't add .5
	jmp	SI32				# process FAC normally
SI31:	cmp	BYTE PTR _FAC, 0x98		# LOW 230 octal: if SF=0 then integer already
	jns	SI20
SI32:	call	_VADDH				# add .5 to FAC
	call	_VINT				# make sure we still have an integer
SI20:	mov	ebx, offset _ZLO		# fetch address of Z into (BX)
	call	_VMVMF				# move FAC to Z, Z=INT(FAC+.5)
	pop	cx				# get (CL) back
SI30:	push	cx				# save CX for later
	mov	edi, offset _Z1LO		# get address of Z1
	mov	ebx, offset _ZLO		# get address of Z
	call	_VMOVM				# Z1=Z
	mov	ebx, offset _ZLO		# fetch address of Z
	call	_VMVFM				# move Z to FAC
	call	_DIV10				# FAC=Z/10
	call	_VINT				# FAC=INT(Z/10)
	mov	ebx, offset _ZLO
	call	_VMVMF				# Z=INT(Z/10)
	call	_MUL10				# FAC=INT(Z/10)*10
	mov	ebx, offset _Z1-1		# address of Z1-1
	call	_GETYP				# need to decrement BX if S.P.
	jnb	SI35
	sub	ebx, 4				# now (BX) points to S.P.
SI35:	call	_VCMPM				# compare to FAC
	pop	cx				# get significant digit count back
	jnz	SI40				# through when non-zero
	inc	cl				# (CL)=(CL)+1
	jmp	SI30				# let's go see if we can find another
SI40:	movzx	ebp, cx				# save CX
	call	_VPOPF				# restore original FAC
	mov	cx, bp				# restore CX
	ret

# Exports for names declared PUBLIC in MATH1.ASM but defined here.
	.globl	RND
