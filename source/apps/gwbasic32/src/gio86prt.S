# GIO86.ASM PRINT section -> gio86prt.S
# PRINT statement family: PRINT (dispatch of the PRINT list), the ";" and "," separators,
# the comma-zone logic (MODCOM, MODCM1, INCLEN, LINCHK, LINCH2, COMPRT, CHKCOM, MORCOM),
# TAB() and SPC() (TABER and the DOSIZT / ASPAC blank loop), and the PRINT exit path (PRINTX, PRNTX1).
# PRINT USING is in biprtu.S (PRINUS); JPRINU jumps there.
# LPRINT: printer output is deferred. The entry is a stub here; the original translation is kept
# in src/deferred/lprint.S (not built). Enabling LPRINT means replacing the stub with that file.
#
# Stack widths: text pointers are 4 bytes (push ebx / pop ebx), the width/comma word CX is 2 bytes,
# AX pushes are 2 bytes, PUSHF/POPF are pushfd/popfd (4 bytes), return addresses are 4 bytes.
# Every POP that discarded a pointer in the original (POP DX, POP BX) is a 4-byte pop here.
#
# Numbers: GIO86.ASM is .RADIX 8, so bare numbers there are octal. Decimal values written
# with D (44D, 59D, 255D) are decimal; "54O" is octal 44, the comma character.
#
# External symbols (defined in other modules): FILGET, PTRWDC, PTRGPS, CRDO, OUTDO, FINPRT,
# FRMEVL, GETYPR, FOUT, STRLIT, STRPRT, CHRGTR, SYNCHR, IMOD, ADRGET, PRINUS, ADVERR, PTRFIL,
# FACLO. OUTDO preserves all registers and flags; PTRGPS preserves all but AL/AH.

	.intel_syntax noprefix
	.include "msdosu.inc"		# MD_SQO
	.include "tokens.inc"		# TABTK, SPCTK, USINTK

	.set CLMWID, 14			# comma field width (OEM.H CLMWID=14D; gwdata.S CLMWID_)

	.section .text
	.globl PRINT, PRINT1, LPRINT

# PRINT USING: the handler is PRINUS in biprtu.S
JPRINU:	jmp PRINUS			# Print Using

# output terminating CR
PRINTX:	call CRDO			# output terminating CR
PRNTX1:	pop cx				# discard line width, last comma column
	jmp FINPRT			# close file#0, reset PTRFIL to keyboard

# UNSUPPORTED: printer output deferred (see src/deferred/lprint.S)
LPRINT:	jmp ADVERR

PRINT:
	mov cl, MD_SQO			# setup output file
	call FILGET
PRINT1:	call PTRWDC			# [CH]=width, [CL]=last comma column
	push cx				# save on stack
NEWCHR:	dec ebx
	call CHRGTR			# get another character
	jz PRINTX			# print CR if end without punctuation
PRINTC:	jz PRNTX1			# branch if end of statement
	pop cx				# refresh [CH]=width, [CL]=last comma column
	cmp al, USINTK			# is it "print using" ?
	jz JPRINU			# IF SO, USE A SPECIAL HANDLER
	push cx				# save [CH]=width, [CL]=last comma column
	cmp al, TABTK
	jz TABERJ			# the TAB function?
	cmp al, SPCTK
	jz TABERJ			# the SPC function?
	push ebx			# save the text pointer
	cmp al, 44			# 44D = ","
	jz COMPRT			# Print Comma
	cmp al, 59			# is it a ";" (59D)
	jnz PRTS
	jmp NOTABR
TABERJ:	jmp TABER
PRTS:	pop edx				# get rid of old text pointer (4 bytes; was POP DX)
	call FRMEVL			# evaluate the formula
	push ebx			# save text pointer
	call GETYPR			# see if we have a string
	jz STRDON			# if so, print special
	dec ebx
	call CHRGTR			# [BX]=addr of number terminator(non-blank)
	cmp BYTE PTR [ebx], ','		# 54O (44): comma terminated?
	pushfd				# remember if value was comma terminated
	call FOUT			# make a number into a string
	call STRLIT			# make it a string
	mov BYTE PTR [ebx], ' '		# put a space at the end
	popfd
	pop ebx
	pop cx				# restore [CH]=width, [CL]=last comma column
	push cx
	push ebx
	mov ebx, DWORD PTR FACLO	# [BX]=address of string descriptor
	jnz INCLEN			# BRIF not comma terminated(print the space too)
	call PTRGPS			# [AL]=file's current position
	add al, BYTE PTR [ebx]		# add length of string we will output
# At this point we have [AL]=posn after number is printed(without space),
#   [CH]=device width, [CL]=last comma posn, [BX]=addr of string descriptor.
# IF number output stops at comma column minus one
# THEN don't append blank(This insures that another number will appear at the
# next comma column, instead of the second column posn after this string)
	cmp ch, 255			# infinite width?
	jz MODCOM			# do modulus to determine next comma column
	cmp al, cl			# compare with last comma column
	jnb INCLEN			# Will do CR after do output,
					#  make string include blank
# Determine if posn in [AL] is one less than a comma column
MODCOM:	inc al
MODCM1:	sub al, CLMWID			# [AL]=modulus CLMWID
	ja MODCM1
	jz STRDON			# BRIF at comma column, exclude trailing space
INCLEN:	inc WORD PTR [ebx]		# increment the length to include the space
					# NOTE:number is less than 255(can do INC)
STRDON:	pop ebx
	pop cx
	push cx				# refresh [CH]=width, [CL]=last comma column
	push ebx
	mov ebx, DWORD PTR FACLO	# BX points to string descriptor
	inc ch
	jz LINCH2			# branch if infinite (255) line width
	dec ch				# restore [CH]=device width
	call PTRGPS			# [AL]=file's current column pos
	or al, al			# don't CR if string longer than line
	jz LINCH2			#  length if position is 0
	add al, BYTE PTR [ebx]		# [AL]=column + string size
	cmc				# set nc if overflow on check
	jae LINCHK			# start on a new line if overflow
	dec al
	cmp al, ch			# check for overlap
LINCHK:	jb LINCH2			# branch if still on current line
	call CRDO			# else output CR
LINCH2:	call STRPRT			# PRINT THE string/number
	pop ebx				# restore text pointer
	jmp NEWCHR			# print some more

# PRINT comma (text pointer stacked)
COMPRT:	call PTRGPS			# [AL]=file's current column position
	cmp ch, 255			# infinite width?
	jz MORCOM			# do modulus
	cmp al, cl			# compare current with last comma column
CHKCOM:	jb MORCOM			# branch if not beyond last comma col
	call CRDO			# start new line
	jmp NOTABR			# AND QUIT IF BEYOND LAST COMMA FIELD
MORCOM:	sub al, CLMWID			# [AL]=MODULUS CLMWID
	jae MORCOM
	neg al
	dec al				# fill the print position out
					# to an even CLMWID, so
					# we print CLMWID-[AL] MOD CLMWID spaces
	jmp ASPA2			# go print [AL]+1 spaces

# PRINT TAB(N) & SPC(N)
TABER:	push ax				# remember IF [A]=SPCTK or TABTK
	call CHRGTR
	call ADRGET			# [DX]=parameter (0..65535)
	pop ax				# see if its SPC or TAB
	push ax
	or dx, dx
	jg TBNONG			# branch if greater than 0
	mov dx, 0			# map negative parms to 0
	jmp SPCNDC
TBNONG:	cmp al, SPCTK			# if space leave alone
	jz SPCNDC
	dec dx				# offset TAB by 1
SPCNDC:	push ebx			# save the text pointer
	mov bl, ch			# [BL]=file width
	mov al, ch			# [AL]=file width
	inc al				# test for width of 255 (no folding)
	jz LNOMOD			# if so, don't mod
	mov bh, 0			# MOD out by line length
	call IMOD			# [BX]=[DX] MOD filewidth
	xchg dx, bx			# set [DL] = position to go to
LNOMOD:	pop ebx				# get back the text pointer
	call SYNCHR
	.byte ')'
	dec ebx
	pop ax				# get back SPCTK or TABTK
	sub al, SPCTK			# was it SPCTK?
	push ebx			# save the text pointer
	jz DOSIZT			# value in [AL]
	call PTRGPS			# [AL]=file position
DOSIZT:	neg al				# print [E]-[A] spaces
	dec al
	add al, dl
	jb ASPA2			# print if past current
	inc al
	jz NOTABR			# do nothing if at current
	call CRDO			# go to a new line
	mov al, dl			# get the position to go to
	dec al
	js NOTABR
ASPA2:	inc al
ASPAC:	mov dl, al			# [B]=number of spaces to print
	mov al, ' '			# [A]=space
REPOUT:	call OUTDO			# PRINT [AL]
	dec dl				# decrement the count
	jnz REPOUT
NOTABR:	pop ebx				# pick up text pointer
	call CHRGTR			# and the next character
	jmp PRINTC			# and since we just printed
					# spaces, don't call crdo
					# if it's the end of the line
