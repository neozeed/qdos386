# GWMAIN.ASM part A -> gwmain_a.S
# Error handling (READYR, ERROR, the DERMAK error entries), STPRDY/READY/MAIN/EDENT/FINI/LINKER/CHEAD,
# SCNLIN/FNDLIN, the crunch routines (PRE FAST CRUNCH, FAST CRUNCH) and the LIST section, up to line 1422.
# Part B (NEWSTT onward) is in gwmain_b1.S / gwmain_b2.S. FNDFOR, LOOPER, POPGOF and the FOR statement are in gwmain.S.
#
# Width rule for this file (see NOTES.md): pointers (text pointers, SAVTXT, OLDTXT, ERRTXT, ONELIN, TXTTAB,
# VARTAB, FRETOP, the link-field pointers in BX/DX/CX-as-pointer) are 4 bytes in registers, in memory and on the stack.
# Line numbers (CURLIN, ERRLIN, DOT, AUTLIN, AUTINC, OLDLIN, NXTLIN, DATLIN) and counts (CX as CRUNCH count) stay 16-bit.
# Flags pushes are PUSHF/POPF -> pushfd/popfd (4 bytes). Return addresses are 4 bytes.
# The DB 271O ("LXI B" over the next instruction) and DB 260O ("MVI A" over one byte) idioms are written as
# explicit jumps. The fall-through paths are unchanged (each entry sets DL or AL and reaches the same label).
#
# Constants from gwdata_errors.inc (only the .set lines, the table data stays in gwdata.S) are below.

	.intel_syntax noprefix
	.include "gwconst.inc"			# shared constants (tokens, error numbers)
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"

	.set LSTERR, 75
	.set DSKERR, 49
	.set NONDSK, 30
	.set DSKER1, DSKERR-NONDSK	# DSKERR-NONDSK (OFFSET DSKER1 in the original)
	.set ERRUE1, 40
	.set ERRDV0, 11
	.set ERRNF, 1
	.set ERRUF, 18
	.set ERRRE, 20
	.set ERROV, 6
	.set ERRMO, 22
	.set ERRTM, 13
	.set ERRNR, 19
	.set ERRFN, 26
	.set ERRWH, 29
	.set ERRIFN, 64
	.set ERRFNO, 54
	.set ERRDNA, 68
	.set ERRFDR, 66
	.set ERRRAD, 74
	.set ERRDFL, 61
	.set ERRIOE, 57
	.set ERRBFM, 54
	.set ERRBFN, 52
	.set ERRIER, 51
	.set ERRRPE, 62
	.set ERRFAO, 55
	.set ERRNMF, 64
	.set ERRBRN, 63
	.set ERRFOV, 50
	.set ERRTMF, 67
	.set ERRFAE, 58
	.set ERRLBO, 23

	.section .text
	.globl READYR, PRGEND, DATSNE, _SNERR, SNERR, DV0ERR, NFERR, DDERR, UFERR, REERR, _OVERR, OVERR, MOERR
	.globl _TMERR, TMERR, ERROR, ERESET, ERRFIN, STPRDY, READY, REPINI, MAIN, EDENT, FINI, EDTXIT, LINKER, CHEAD
	.globl ERRFN1, UPERR, ERRMOR, NTMDCN
	.globl DERIFN, DERFNO, DERDNA, DERFDR, DERRAD, DERDFL, DERIOE, DERBFM, DERFNF, DERBFN, DERIER, DERRPE
	.globl DERFAO, DERNMF, DERBRN, DERFOV, DERTMF, DERFAE

# THIS ROUTINE IS CALLED TO RESET THE STACK IF BASIC IS EXTERNALLY STOPPED AND THEN RESTARTED.
READYR:	mov	ecx, offset STPRDY	# address to go to; the stacked address is popped by STPRDY
	jmp	ERESET			# reset stack, goto READY
PRGEND:
	mov	bx, WORD PTR CURLIN	# get current line #
	mov	al, bh			# see if direct
	and	al, bl			# and together
	inc	al			# set cc's
	jz	ENDCNJ			# if direct done, allow for debugging purposes
	mov	al, BYTE PTR ONEFLG	# see if in on error
	or	al, al
	mov	dl, ERRNR		# "no resume" error
	jnz	ERROR			# yes, forgot resume
ENDCNJ:	jmp	ENDCON			# no, let it end

# Error entries. Each DERxx sets DL and falls into ERROR (the original chains them with a skip byte;
# this is the same flow). ERRxx codes are the gwdata_errors.inc values.
DERIFN:	mov	dl, ERRIFN
	jmp	ERROR
DERFNO:	mov	dl, ERRFNO
	jmp	ERROR
DERDNA:	mov	dl, ERRDNA
	jmp	ERROR
DERFDR:	mov	dl, ERRFDR
	jmp	ERROR
DERRAD:	mov	dl, ERRRAD
	jmp	ERROR
DERDFL:	mov	dl, ERRDFL
	jmp	ERROR
DERIOE:	mov	dl, ERRIOE
	jmp	ERROR
DERBFM:	mov	dl, ERRBFM
	jmp	ERROR
DERFNF:	mov	dl, ERRFNF
	jmp	ERROR
DERBFN:	mov	dl, ERRBFN
	jmp	ERROR
DERIER:	mov	dl, ERRIER
	jmp	ERROR
DERRPE:	mov	dl, ERRRPE
	jmp	ERROR
DERFAO:	mov	dl, ERRFAO
	jmp	ERROR
DERNMF:	mov	dl, ERRNMF
	jmp	ERROR
DERBRN:	mov	dl, ERRBRN
	jmp	ERROR
DERFOV:	mov	dl, ERRFOV
	jmp	ERROR
DERTMF:	mov	dl, ERRTMF
	jmp	ERROR
DERFAE:	mov	dl, ERRFAE
	jmp	ERROR

DATSNE:	mov	bx, WORD PTR DATLIN	# get data line
	mov	WORD PTR CURLIN, bx	# make it current line
_SNERR:
SNERR:	mov	dl, ERRSN		# "syntax error"
	jmp	ERROR
DV0ERR:	mov	dl, ERRDV0		# division by zero
	jmp	ERROR
NFERR:	mov	dl, ERRNF		# "next without for" error
	jmp	ERROR
DDERR:	mov	dl, ERRDD		# redimensioned variable
	jmp	ERROR
UFERR:	mov	dl, ERRUF		# undefined function error
	jmp	ERROR
REERR:	mov	dl, ERRRE		# resume without error
	jmp	ERROR
_OVERR:
OVERR:	mov	dl, ERROV		# set overflow error code
	jmp	ERROR
MOERR:	mov	dl, ERRMO		# missing operand error
	jmp	ERROR
_TMERR:
TMERR:	mov	dl, ERRTM		# type mismatch error
ERROR:
	xor	al, al
	mov	BYTE PTR NLONLY, al	# clear don't-close-files flag
	mov	al, BYTE PTR INITFG
	or	al, al			# 0 implies initialization is incomplete
	jnz	INITOK			# initialization has completed
	jmp	CMDERR			# execute command line error routine, which returns to the OS
INITOK:
	mov	bx, WORD PTR CURLIN	# get current line number
	mov	WORD PTR ERRLIN, bx	# save it for ERL variable
	xor	al, al			# clear chain flag in case of error
	mov	BYTE PTR MRGFLG, al	# also merge flag
	mov	BYTE PTR CHNFLG, al	# so it doesn't try to chain
	mov	al, bh			# only set up dot if it isn't direct
	and	al, bl
	inc	al
	jz	ERRESM
	mov	WORD PTR DOT, bx
	.globl ERRESM
ERRESM:	mov	ecx, offset ERRMOR	# get return address in [b,c]
ERESET:	mov	ebx, DWORD PTR SAVSTK	# get a good stack back
	jmp	STKERR
# UNSURE: ERRMOR pops the stopper as a 4-byte address; confirm STKERR pushes a 4-byte return address.
ERRMOR:	pop	ecx			# pop off the FNDFOR stopper (4-byte address)
	mov	al, dl			# [a]=error number
	mov	cl, dl			# also save it for later restore
	mov	BYTE PTR ERRFLG, al	# save it so we know whether to call "EDIT"
	mov	ebx, DWORD PTR SAVTXT	# get saved text pointer
	mov	DWORD PTR ERRTXT, ebx	# save for RESUME
	xchg	ebx, edx		# save SAVTXT ptr
	mov	bx, WORD PTR ERRLIN	# get error line #
	mov	al, bh			# test if direct line
	and	al, bl			# set cc's
	inc	al			# sets zero if direct line (65535)
	jz	NTMDCN			# if direct, don't modify OLDTXT & OLDLIN
	mov	WORD PTR OLDLIN, bx	# set OLDLIN=ERRLIN
	xchg	ebx, edx		# get back SAVTXT
	mov	DWORD PTR OLDTXT, ebx	# save in OLDTXT
NTMDCN:	mov	ebx, DWORD PTR ONELIN	# see if we are trapping errors (ONELIN is a text pointer, 0 = none)
	or	ebx, ebx
	xchg	ebx, edx		# put line to go to in [d,e]
	mov	ebx, offset ONEFLG	# point to error flag
	jz	NOTRAP			# sorry, no trapping
	and	al, BYTE PTR [ebx]	# a is non-zero, setzero if ONEFLG zero
	jnz	NOTRAP			# if flag already set, force error
	dec	BYTE PTR [ebx]		# if already in error routine, force error
	xchg	ebx, edx		# get line pointer in [h,l]
	jmp	GONE4			# go directly to NEWSTT code
NOTRAP:	xor	al, al			# a must be zero for control
	mov	BYTE PTR [ebx], al	# reset ONEFLG
	mov	dl, cl			# get back error code
	mov	BYTE PTR CNTOFL, al	# force output
	call	CRDONZ			# crlf
	mov	ebx, offset ERRTAB	# get start of error table
	mov	al, dl			# get error code
	cmp	al, LSTERR		# is it past last error?
	jae	UPERR			# yes, too big to print
	cmp	al, DSKERR+1		# disk error?
	jae	NTDER2			# yes
	cmp	al, NONDSK+1		# is it between last normal & first disk?
	jb	NTDERR			# yes, ok to print it
UPERR:	mov	al, ERRUE1		# print "unprintable error"
NTDER2:	sub	al, DSKER1		# fix offset into table of messages
	mov	dl, al			# save back error code
NTDERR:
	# on "syntax error"s
LEPSKP:	mov	al, BYTE PTR [ebx]	# code segment fetch (override dropped: flat)
	inc	ebx			# point to next character
	or	al, al
	jz	Q_Q_L001
	jmp	LEPSKP
Q_Q_L001:
	dec	ebx
	inc	ebx			# skip over this error message
	dec	dl			# decrement error count
	jnz	LEPSKP			# skip some more
	push	ebx			# save text pointer
	mov	bx, WORD PTR ERRLIN	# get error line number
	pop	esi			# xthl
	xchg	esi, ebx
	push esi			# save line number (value, 2 bytes); text pointer is in EBX
ERRFIN:
	mov	al, BYTE PTR [ebx]	# get 1st char of error
	cmp	al, '?'			# padded error?
	jnz	ERRFN1			# no, print
	pop ebx			# get line # off stack
	mov	ebx, offset ERRTAB
	jmp	UPERR			# make unprintable error
ERRFN1:
	call	STROUT			# print message
	pop ebx			# restore line number
	mov	dx, 65534		# is init executing?
	cmp	bx, dx
	jnz	Q_Q_L002
	call	CRDO			# do crlf
Q_Q_L002:
	jnz	Q_Q_L003
	jmp	SYSTME			# system error exit
Q_Q_L003:
	mov	al, bh			# see if in direct mode
	and	al, bl
	inc	al			# zero says direct mode
	jz	Q_Q_L004
	call	INPRT			# print line number in [h,l]
Q_Q_L004:				# the original flags 255 here (not user input) for its screen editor; that flag
				# is not displayed, so nothing is output
	mov	al, 0			# the original "MVI A,0" skips the POP CX at STPRDY on this fall-through
	jmp	READY
# FOR "LIST" COMMAND STOPPING AND FOR RETURNING FROM A FAILED "CVER";
# AND TO CORRECT A DIRECT GOSUB WHICH DOES INPUT.
STPRDY:	pop	ecx			# address (4 bytes)
READY:
	call	MRG_END			# a MERGE file is closed at every end of the merge (or its error)
	call	FINLPT			# print any left overs
	xor	al, al
	mov	BYTE PTR CNTOFL, al	# force output
	call	PRGFIN			# finish output of a file
	call	CRDONZ			# if not already at left, send crlf
	mov	ebx, offset REDDY	# "OK" crlf
REPINI:
	call	STROUT
	mov	al, BYTE PTR ERRFLG	# see if it was a "syntax error"
	sub	al, ERRSN
	jnz	Q_Q_L005
	call	ERREDT			# "edit" the bad line
Q_Q_L005:
MAIN:
	mov	bx, 65535
	mov	WORD PTR CURLIN, bx	# setup CURLIN for direct mode
	mov	ebx, offset ENDPRG
	mov	DWORD PTR SAVTXT, ebx	# SAVTXT points at fake end of program
	mov	al, BYTE PTR AUTFLG	# in an AUTO command?
	or	al, al
	jz	NTAUTO			# no, regular mode
	mov	bx, WORD PTR AUTLIN	# get current AUTO line
	push ebx			# save away for later use
	call	LINPRT			# print the line #
	pop	dx			# get it back
	push	dx			# save back again
	call	FNDLIN			# see if it exists
	mov	al, '*'			# char to print if line already exists
	jb	AUTELN			# doesn't exist
	mov	al, ' '			# print space
AUTELN:	call	OUTDO			# print char
	mov	BYTE PTR AUTFLG, al	# save whether have asterisk or not
NTAUTO:
	call	PINLIN			# get program line input
	jae	NTSTOP
	xor	al, al
	mov	BYTE PTR AUTFLG, al	# in case of AUTO, clear it
	jmp	MAIN
NTSTOP:
	call	CHRGTR			# get the first
	inc	al			# see if 0, saving the carry flag
	dec	al
	jz	MAIN			# if so, a blank line was input
	pushfd				# save status indicator for 1st character
	call	LINGET			# read in a line #
	jae	MAINBX			# another digit after line#?
	call	ISFLIO			# and coming from terminal?
	jnz	Q_Q_L006
	jmp	SNERR			# if so, bad input
Q_Q_L006:
MAINBX:
	call	BAKSP			# back up the pointer
	mov	al, BYTE PTR AUTFLG
	or	al, al
	jz	NTASTC			# not auto, don't check for asterisk
	cmp	al, '*'
	jnz	NTASTC			# auto did not append asterisk
	cmp	al, BYTE PTR [ebx]	# do we have asterisk?
	jnz	Q_Q_L007
	call	INXHRT			# yes, skip it
Q_Q_L007:
NTASTC:
	or	dx, dx
	jnz	Q_Q_L008
	jmp	EDENT			# brif no line number, don't skip space
Q_Q_L008:
	mov	al, BYTE PTR [ebx]	# get the char
	cmp	al, ' '			# character a space?
	jnz	Q_Q_L009
	call	INXHRT			# then eat past it
Q_Q_L009:
	# one space always printed after line #
EDENT:
	push	dx			# save line #
	call	CRUNCH			# crunch the line down
	pop	dx			# restore line #
	popfd				# was there a line #?
	mov	DWORD PTR SAVTXT, ebx	# for resuming a direct stmt
	jnae	Q_Q_L010
	jmp	DIRDO			# make sure we're not reading a file
Q_Q_L010:
	push	dx			# line # (value)
	push	cx			# count (value)
	call	PROCHK			# dont allow any funny business with existing pgm
	call	CHRGTR			# remember if this line is
	or	al, al			# set the zero flag on zero lines (lines that start with ":" should not be ignored)
	pushfd				# blank so we don't insert it
	mov	WORD PTR DOT, dx	# save this line # in DOT
	mov	bx, WORD PTR AUTINC	# get increment
	add	bx, dx			# add increment to this line
	jb	AUTRES			# check for pathetic case
	push	dx			# save line number #
	mov	dx, 65530		# check for line # too big
	cmp	bx, dx
	pop	dx			# get back line #
	jae	AUTRES			# if too big, quit
	mov	WORD PTR AUTLIN, bx	# save in next line
	jmp	AUTGOD
AUTRES:	xor	al, al
	mov	BYTE PTR AUTFLG, al	# line number too big, quit
AUTGOD:
	call	FNDLIN			# get a pointer to the line (CX = pointer)
	jb	LEXIST			# line exists, delete it
	popfd				# get flag says whether line blank
	pushfd				# save back
	jz	Q_Q_L011
	jmp	LXISTS			# line exists - OK
Q_Q_L011:
	mov	al, BYTE PTR AUTFLG
	or	al, al			# if not AUTO mode then error
	jnz	Q_Q_L012
	jmp	USERR			# trying to delete non-existant line, error
Q_Q_L012:
	push	ecx			# prepare to leave (CX = pointer)
	jmp	FINI
LXISTS:
	or	al, al			# clear flag that says line exists
	jmp	INONLY
LEXIST:				# save the pointer
	popfd				# don't erase line on null input when in AUTO mode
	pushfd
	jz	Q_Q_L013
	jmp	ERAINS			# line not null - go erase/insert
Q_Q_L013:
	mov	al, BYTE PTR AUTFLG
	or	al, al			# AUTO mode?
	jnz	Q_Q_L014
	jmp	ERAINS			# no - go erase/insert
Q_Q_L014:
	push	ecx			# prepare to exit
	jmp	FINI
ERAINS:	stc
# UNSURE: CX is a 4-byte pointer here (FNDLIN result on the LXISTS/LEXIST paths); widths follow that.
INONLY:
	push	ecx			# save registers (CX = pointer)
	pushfd
	push	ebx			# save [h,l]
	call	DEPTR			# get rid of ptrs in pgm
	pop	ebx			# get back pointer to next line
	popfd			# get back psw
	pop	ecx			# restore pointer to this line
	push	ecx			# save back again
	jnb	Q_Q_L015
	call	DEL
Q_Q_L015:
NODEL:	pop	edx			# pop pointer at place to insert
	popfd				# see if this line had anything on it
	push	edx			# save place to start fixing links
	jz	FINI			# if not, don't insert
	pop	edx			# get rid of start of link fix
	mov	al, BYTE PTR CHNFLG	# only change FRETOP if not chaining
	or	al, al
	jnz	LEVFRE			# leave FRETOP alone
	mov	ebx, DWORD PTR MEMSIZ	# delete all strings
	mov	DWORD PTR FRETOP, ebx	# so REASON doesn't use them
# UNSURE: count (16-bit) and line (16-bit) pops match the EDENT pushes; VARTAB arithmetic widened to 32 bits.
LEVFRE:
	mov	ebx, 0			# clear error trap before inserting
	mov	DWORD PTR ONELIN, ebx	# to prevent OM error from being trapped
	mov	ebx, DWORD PTR VARTAB	# current end
	pop esi			# count of chars (low word); the 4-byte link adds 2 bytes to the slot
	add	esi, 2
	mov	ecx, ebx		# [B,C]=old VARTAB (pointer)
	push esi			# save count of chars to move
	movzx	ebx, si			# BX = count
	add	ebx, ecx		# new VARTAB = count + old VARTAB
	push	ebx			# save new VARTAB
	call	BLTU
	pop	ebx			# pop off VARTAB
	mov	DWORD PTR VARTAB, ebx	# update VARTAB
	xchg	ebx, edx
	mov	DWORD PTR [ebx], -1	# fool CHEAD with a non-zero (4-byte) link
	pop	cx			# restore count of chars to move
	pop	dx			# get line # off stack
	push	ebx			# save start of place to fix links
	add	ebx, 4			# skip the 4-byte link (CHEAD fixes it later)
	mov	WORD PTR [ebx], dx	# put down line #
	add	ebx, 2
	mov	edx, offset KBUF	# move line from KBUF to program area
	sub	cx, 6			# text bytes: less the 4-byte link and 2-byte line #
	movzx	ecx, cx
MLOOPR:	mov	esi, edx
	mov	al, BYTE PTR [esi]	# now transfering line in from buf
	mov	BYTE PTR [ebx], al
	inc	ebx
	inc	edx
	dec	cx			# decrement char count by 1
	mov	al, cl			# test for count exhausted
	or	al, ch			# by seeing if [b,c]=0
	jnz	MLOOPR
FINI:
EDTXIT:	pop	edx			# pop the link fixing area (pointer): balances the stack
	mov	edx, DWORD PTR TXTTAB	# relink from the start of the program, so the predecessor
					# of an inserted or deleted line gets its link fixed too
	call	CHEAD			# fix links
	call	RUNC			# do clear & set up stack
	jmp	MAIN			# go to main code
LINKER:
	mov	ebx, DWORD PTR TXTTAB
	xchg	ebx, edx
# CHEAD goes through program storage and fixes up all the links. The end of each line is found
# by searching for the zero at the end. The double zero link is used to detect the end of the program.
CHEAD:	mov	ebx, edx		# [h,l]=[d,e]
	cmp	DWORD PTR [ebx], 0	# end of chain: the link is a 4-byte pointer, so test 4 bytes
	jnz	Q_Q_CH1
	ret
Q_Q_CH1:
	add	ebx, 5			# skip the 4-byte link and 2-byte line number, less 1 (CZLOOP bumps)
CZLOOP:	inc	ebx			# bump pointer
	mov	al, BYTE PTR [ebx]	# get byte
CZLOO2:	or	al, al			# set cc's
	jz	CZLIN			# end of line, done.
	cmp	al, DBLCON+1		# embedded constant?
	jae	CZLOOP			# no, get next
	cmp	al, 11			# is it linefeed or below?
	jb	CZLOOP			# then skip past
	# Embedded constant (11..31): skip its data bytes by size. Data may be zero, so the
	# expression scanner must not be used here.
	sub	al, 11
	movzx	ecx, al
	movzx	ecx, BYTE PTR CZCSIZ[ecx]
	add	ebx, ecx		# now on the last data byte; CZLOOP bumps past it
	jmp	CZLOOP
CZLIN:	inc	ebx			# make [h,l] point after text
	xchg	ebx, edx		# switch temp
	mov	DWORD PTR [ebx], edx	# store fixup (link field is a 4-byte pointer)
	jmp	CHEAD			# keep chaining til done
# Data bytes after each embedded-constant token 11..31 (token numbers in gwtok.inc, gwmain_b1.S).
	.globl CZCSIZ
CZCSIZ:	.byte 2, 2, 4, 2, 1		# 11 OCTCON, 12 HEXCON, 13 PTRCON, 14 LINCON, 15 IN2CON
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	# 16..27: no data (ONECON 17..26 are one-byte tokens)
	.byte 2, 4, 0, 8		# 28 INTCON, 29 SNGCON, 30 unused, 31 DBLCON

# SCNLIN scans a line range of the form #-#, #, #-, -# or blank, and then finds the first line in the range.
	.globl SCNLIN
SCNLIN:	mov	dx, 0			# assume start list at zero
	push	dx			# save initial assumption
	jz	ALLLST			# if finished, list it all
	cmp	al, 44			# comma is ok
	jz	ALLLST			# maybe list all to gen device
	pop	dx			# we are going to grab a #
	call	LINSPC			# get a line #. if none, returns zero
	push	dx			# save first
	jz	SNGLIN			# if only # then done.
	cmp	al, 44			# comma is ok
	jz	SNGLIN			# maybe only one to gen device
	call	SYNCHR
	.byte	MINUTK			# must be a dash
	jz	ALLLST			# brif EOS
	cmp	al, 44			# else must be comma or number
ALLLST:	mov	dx, 65530		# assume max end of range
	jz	Q_Q_L016
	call	LINSPC			# get the end of range
Q_Q_L016:
	jz	SNGLIN			# brif EOS
	cmp	al, 44
	jz	Q_Q_L017
	jmp	SNERR			# must be terminator
Q_Q_L017:
SNGLIN:
	mov	DWORD PTR TEMP, ebx	# save text pointer
	xchg	bx, dx			# [H,L] = FINAL (line numbers, 16-bit)
	pop	dx			# get initial in [d,e] (16-bit line number)
# UNSURE: the return address moved by XTHL is 4 bytes; the max line on the stack is 2 bytes.
	.globl FNDLN1
FNDLN1:	pop	esi			# xthl: the return address (4 bytes)
	xchg	esi, ebx		# ebx = return address, esi = FINAL
	push esi			# put max on stack (2-byte line number)
	push	ebx			# save return address back
# FNDLIN searches the program text for the line whose line number is in [D,E] (preserved).
# Returns:
#   1) zero flag set, carry not set: line not found, no line greater than the one sought.
#      [B,C] points to two zero bytes at the end of the program; [H,L]=[B,C].
#   2) zero, carry set: [B,C] points to the link field of the line searched for;
#      [H,L] points to the link field of the next line.
#   3) non-zero, carry not set: line not found; [B,C] points to a line greater than the one searched for;
#      [H,L] points to the link field of the next line.
	.globl FNDLIN
FNDLIN:
	mov	ebx, DWORD PTR TXTTAB	# get pointer to start of text
	.globl LOOP
LOOP:				# search from [EBX] (entry used by RESTRP's LOOP call in gwmain_b1)
FNLOOP:
	mov	ecx, ebx		# if exiting because of end of program, [B,C] points to double zeroes
	cmp	DWORD PTR [ebx], 0	# 4-byte link: zero means the end of the program (sets ZF, clears CF)
	jnz	FNCONT
	ret				# if zero then done
FNCONT:
	add	ebx, 4			# skip the 4-byte link to get the line #
	mov	bx, WORD PTR [ebx]	# into [h,l] for comparison with the line # being searched for (in [d,e])
	cmp	bx, dx			# see if it matches or if we've gone too far
	mov	ebx, ecx		# make [h,l] point to the start of the line beyond this one, by picking
	mov	ebx, DWORD PTR [ebx]	# up the link that [b,c] points at
	cmc				# turn carry on
	jnz	FNEQ
	ret				# equal return
FNEQ:
	cmc				# make carry zero
	jnae	FNGT
	ret				# no match return (greater)
FNGT:
	jmp	FNLOOP			# keep looping

# PRE FAST CRUNCH - COMPACTIFICATION / FAST CRUNCH - COMPACTIFICATION  (lines 926-1422 follow)

# ------------------------------------------------------------------------------------------
# FAST CRUNCH. Reserved words become single (or two-byte, first 0377) tokens with the high bit set.
# BX = source text pointer, DX = deposit pointer (both 32-bit), CX = free count of KBUF (16-bit).
# INS86 213,362,56,254 (the 8086 "MOV SI,DX / CS: LODSB" fetch from the keyword tables) is written
# as "mov esi, edx / mov al, [esi]". The code-segment override and INS86 56 prefixes are dropped.
# ------------------------------------------------------------------------------------------
	.globl CRUNCH, LBOERR, NOTRES, NOTRFN, RENCRN, BAKSP, KRNSAV, KRNSVC
	.set KBFLEN, 318			# KBUF length = BUFLEN + BUFLEN/4 (BUFLEN=255, oem.inc)
CRUNCH:
	xor	al, al			# say expecting floating numbers
	mov	BYTE PTR DONUM, al	# set flag accordingly
	mov	BYTE PTR DORES, al	# allow crunching
	mov	cx, KBFLEN-3		# length of krunch buffer minus three because of zeros at end
	mov	edx, offset KBUF	# setup destination pointer
KLOOP:	mov	al, BYTE PTR [ebx]	# get character from buf
	or	al, al			# end of line?
	jnz	NCRDON			# no, continue
CRDONE:
	mov	bx, KBFLEN+2		# get offset
	mov	al, bl			# get count to subtract from
	sub	al, cl			# subtract
	mov	cl, al
	mov	al, bh
	sbb	al, ch
	mov	ch, al			# bc := # of chars crunched
	mov	ebx, offset KBUF-1	# get pointer to char before KBUF (as "GONE" does a CHRGET)
	xor	al, al			# get a zero
	mov	edi, edx
	mov	BYTE PTR [edi], al	# need three 0's on the end
	inc	edx			# one for end-of-line
	mov	edi, edx
	mov	BYTE PTR [edi], al	# and 2 for a zero link
	inc	edx			# since if this is a direct statement
	mov	edi, edx
	mov	BYTE PTR [edi], al	# its end must look like the end of a program
	inc	edx
	mov	edi, edx
	mov	BYTE PTR [edi], al	# and the 4-byte link (GONE4 reads it): two more zeros
	inc	edx
	mov	edi, edx
	mov	BYTE PTR [edi], al
	ret				# end of crunching

NCRDON:	cmp	al, 34			# quote sign?
	jnz	Q_Q_L018
	jmp	STRNG			# yes, go to special string handling
Q_Q_L018:
	cmp	al, ' '			# space?
	jz	STUFFH			# just stuff away
	mov	al, BYTE PTR DORES	# in data statement and no crunch?
	or	al, al
	mov	al, BYTE PTR [ebx]	# get the character again
	jz	NTDATA			# if no crunching just store the character
STUFFH:	inc	ebx			# entry to bump [h,l]
	push	ax			# save char as KRNSAV clobbers
	call	KRNSAV			# save char in krunch buffer
	pop	ax			# restore char
	sub	al, ':'			# see if it is a colon
	jz	COLIS			# if so allow crunching again
	cmp	al, _DATCO
	jnz	NODATT			# see if it is a DATA token
	mov	al, 1			# set line number allowed flag (kludge: has to be non-zero)
COLIS:	mov	BYTE PTR DORES, al	# setup flag
	mov	BYTE PTR DONUM, al	# set number allowed flag
NODATT:	sub	al, _REMCO
	jnz	KLOOP			# keep looping
	push	ax			# save terminator on stack
STR1:	mov	al, BYTE PTR [ebx]	# get a char
	or	al, al			# set condition codes
	pop	ax			# get al back without affecting psw
	jz	CRDONE			# if end of line then done
	cmp	al, BYTE PTR [ebx]	# compare char with this terminator
	jz	STUFFH			# if yes, done with string
STRNG:
	push	ax			# save terminator
	mov	al, BYTE PTR [ebx]	# get back line char
STRNG2:	inc	ebx			# increment text pointer
	call	KRNSAV			# save char in krunch buffer
	jmp	STR1			# keep looping

# now check high-bit chars, ? for PRINT
NTDATA:
	inc	ebx			# if so skip it
	or	al, al			# is this a kana character in a bad place?
	jns	Q_Q_L019
	jmp	KLOOP			# move to the next character
Q_Q_L019:
	dec	ebx			# restore the text pointer
	cmp	al, '?'			# a qmark?
	mov	al, _PRINT		# then use a "PRINT" token
	push	edx			# save store pointer
	push	cx			# save char count
	jnz	Q_Q_L020
	jmp	NOTFN2
Q_Q_L020:
	# *** 5.11 don't allow following line # ***
	mov	edx, offset SPCTAB	# assume we'll search special char table
	call	MAKUPL			# translate this char to upper case
	call	ISLET2			# letter?
	jae	Q_Q_L021
	jmp	TSTNUM			# not a letter, test for number
Q_Q_L021:
	push	ebx			# save text pointer
	# ANSI says you can use "GO TO" and "GO SUB" with any number of spaces
	mov	edx, offset GOSTEX	# check for "GO "
	call	CHKRES			# that it?
	jnz	NOTGOS			# nope
	call	CHRGTR			# skip any number of spaces
	mov	edx, offset TOTEX	# is it TO?
	call	CHKRES			# check
	mov	al, _GOTO		# assume so
	jnz	Q_Q_L022
	jmp	GPUTRS			# use it
Q_Q_L022:
	mov	edx, offset SUBTEX	# "GO SUB"
	call	CHKRES
	jnz	NOTGOS			# no
	mov	al, _GOSUB
GPUTRS:	pop	ecx			# pop off the old text pointer (32-bit)
	jmp	NOTFN2			# store the reserved word

# CHECK MEMORY POINTED TO BY TEXT POINTER WITH A SPECIFIED PARTIAL RESRWORD
CHKRES:
	mov	esi, edx		# fetch from code segment (INS86 213,362,56,254)
	mov	al, BYTE PTR [esi]
	or	al, al			# end of matched partial reswords?
	jnz	CHKR1
	ret				# yes, done
CHKR1:	mov	cl, al			# save char
	call	MAKUPL			# get char from text
	cmp	al, cl			# chars the same?
	jz	CHKR2
	ret				# done
CHKR2:	inc	ebx			# bump pointer into text
	inc	edx			# and reword text
	jmp	CHKRES			# loop till done
GOSTEX:	.ascii	"GO "
	.byte	0
TOTEX:	.ascii	"TO"
	.byte	0
SUBTEX:	.ascii	"UB"
	.byte	0
NOTGOS:	pop	ebx
	call	MAKUPL			# get back the character
	push	ebx			# resave the text pointer

# now search alpha character tables: take the first character as an index into the 26 tables
	mov	ebx, offset ALPTAB	# get pointer to alpha dispatch table
	sub	al, 'A'			# subtract alpha offset
	add	al, al			# multiply by two (table entries are 4 bytes: multiply by four)
	add	al, al
	movzx	ecx, al			# offset in [c] for dad
	add	ebx, ecx		# add to table address
	mov	edx, DWORD PTR [ebx]	# get pointer in [d,e]
	pop	ebx			# get back source pointer
	inc	ebx			# point to char after first alpha
TRYAGA:	push	ebx			# save txtptr to start of search area
LOPPSI:
	call	MAKUPL			# translate this char to upper case
	mov	cl, al			# save char in [c]
	mov	esi, edx		# fetch from code segment
	mov	al, BYTE PTR [esi]
	and	al, 127			# get rid of high bit
	jnz	Q_Q_L023
	jmp	NOTRES			# if = 0 then end of this char's result
Q_Q_L023:
	inc	ebx			# bump source pointer
	cmp	al, cl			# compare to char from source line
	jnz	LOPSKP			# if no match, search for next reswrd
	mov	esi, edx
	mov	al, BYTE PTR [esi]
	inc	edx			# bump reslst pointer
	or	al, al
	js	Q_Q_L024
	jmp	LOPPSI			# see if rest of chars match
Q_Q_L024:
	mov	al, cl			# get last char of reswrd
	cmp	al, '('			# if TAB( or SPC(, space need not follow
	jz	ISRESW			# is a reswrd
	mov	esi, edx
	mov	al, BYTE PTR [esi]
	cmp	al, _FN			# function?
	jz	ISRESW			# then no space need afterward
	cmp	al, _USR		# or USR definition?
	jz	ISRESW
	call	MAKUPL			# get next char in line (MC 6/22/80)
	cmp	al, '.'			# is it a dot
	jz	ISVARS			# yes
	call	TSTANM			# is it a letter immediately following reswrd
ISVARS:	mov	al, 0			# set DONUM to -1
	jnae	Q_Q_L025
	jmp	NOTRES			# if alpha, can't be reserved word
Q_Q_L025:
# UNSURE: the saved text pointer popped here is 4 bytes (TRYAGA push); DX pop is 4 bytes, CX pop is 2 bytes.
ISRESW:
	pop	eax			# get rid of saved [h,l]
	mov	esi, edx
	mov	al, BYTE PTR [esi]	# fetch from code segment
	or	al, al			# set cc's
	jns	Q_Q_L026
	jmp	NOTFNT			# if minus, wasn't function token
Q_Q_L026:
	pop	cx			# get char count off stack
	pop	edx			# get deposit pointer off stack
	or	al, 0200		# make high order bit one (0200 octal = 128)
	push	ax			# save fn char
	mov	al, 0377		# get byte which precedes FNs
RENCRN:
	call	KRNSAV			# save in krunch buffer
	xor	al, al			# make a zero
	mov	BYTE PTR DONUM, al	# to reset DONUM (floatings allowed)
	pop	ax			# get function token
	call	KRNSAV			# store it
	jmp	KLOOP			# keep krunching
LOPSKP:	pop	ebx			# restore undefiled text pointer
LOPSK2:
	mov	esi, edx
	mov	al, BYTE PTR [esi]	# fetch from code segment
	inc	edx			# bump reslst pointer
	or	al, al			# set cc's
	js	Q_Q_L027
	jmp	LOPSK2			# not end of reswrd, keep skipping
Q_Q_L027:
	inc	edx			# point after token
	jmp	TRYAGA			# try another reswrd

# CHECK TO SEE IF RESERVED WORD MATCHES A LIST OF RESERVED WORDS THAT HAVE LINE NUMBERS
# FOLLOWING THEM INSTEAD OF FLOATING POINT NUMBERS. IF A MATCH IS FOUND, SET DONUM TO INDICATE
# THAT IF A NUMBER OCCURS, IT IS CRUNCHED AS A LINE NUMBER.
NOTFNT:	dec	ebx			# fix text pointer
NOTFN2:	push	ax			# save char to be saved in krunch buffer
	mov	edx, offset LINRES	# point to reserved words that have line args
	mov	cl, al			# save character to match in [c]
NOTFN3:	mov	esi, edx
	mov	al, BYTE PTR [esi]	# fetch from code segment
	or	al, al			# end of list?
	jz	NOTFN4			# yes, done
	inc	edx			# point to next reword
	cmp	al, cl			# same as one we're looking at?
	jnz	NOTFN3			# keep looking
	jmp	NOTRS2			# doesn't have line # arg
LINRES:	.byte	_RESTORE		# RESTORE can have following line number
	.byte	_AUTO			# AUTO command
	.byte	_RENUM			# RENUMBER?
	.byte	_DELETE			# DELETE?
	.byte	_EDIT			# EDIT?
	.byte	_RESUME			# RESUME?
	.byte	_ERL			# error line (so that if "ERL=...THEN" will resequence properly;
					# this can make statements like "PRINT ERL,1E20" do strange things)
	.byte	_ELSE			# if ELSE, crunch following line #
	.byte	_RUN			# RUN?
	.byte	_LIST			# LIST?
	.byte	_LLIST			# LPT list?
	.byte	_GOTO			# if GOTO, crunch line #
	.byte	_RETURN			# non local return allows line # after RETURN
	.byte	_THEN			# crunch line #'s after 'THEN'
	.byte	_GOSUB			# if GOSUB, crunch line #'s
	.byte	0
NOTFN4:	xor	al, al			# get a zero (expect usual numbers)
	jmp	NOTRS6
NOTRS2:	mov	al, 1			# say line #'s allowed
NOTRS6:	mov	BYTE PTR DONUM, al	# save in flag
	pop	ax			# restore character to save in krunch buffer
	pop	cx			# get back the character count
	pop	edx			# get stuff pointer back
	cmp	al, _ELSE		# have to put a hidden colon in front of "ELSE"s
	push	ax			# save current char (ELSE)
	jnz	Q_Q_L028
	call	KRNSVC			# save ":" in crunch buffer
Q_Q_L028:
	pop	ax			# get back token
	cmp	al, _WHILE		# see if WHILE to in a plus to avoid
	jnz	CKSNGQ			# brif not
	call	KRNSAV			# emit WHILE token, then plus (+)
	mov	al, PLUSTK		# plus sign is ok and avoids constant
CKSNGQ:	cmp	al, SNGQTK		# single quotation mark?
	jz	Q_Q_L029
	jmp	NTSNGT
Q_Q_L029:
	push	ax			# save SNGQTK
	call	KRNSVC			# save ":" in crunch buffer
	mov	al, _REM		# store ":REM" in front for execution
	call	KRNSAV			# save it
	pop	ax			# get SNGQTK back
	push	ebx			# save text pointer
	mov	ebx, 0
	pop	esi			# xthl
	xchg	esi, ebx
	push esi			# save terminator (0, 2 bytes), restore text pointer
	jmp	STRNG2			# stuff the rest of the line without crunching

TSTNUM:	mov	al, BYTE PTR [ebx]	# get char
	cmp	al, '.'			# test for start of floating #
	jz	NUMTRY			# try inputting it as constant
	cmp	al, '9'+1		# is it a digit?
	jnae	Q_Q_L030
	jmp	SRCSPC			# no, try other things
Q_Q_L030:
	cmp	al, '0'			# try lower end
	jae	Q_Q_L031
	jmp	SRCSPC			# no, try other possibilities
Q_Q_L031:
NUMTRY:	mov	al, BYTE PTR DONUM	# test for numbers allowed
	or	al, al			# set cc's
	mov	al, BYTE PTR [ebx]	# get char if going to STUFFH
	pop	cx			# restore char count
	pop	edx			# restore deposit pointer
	jns	Q_Q_L032
	jmp	STUFFH			# no, just stuff it (!)
Q_Q_L032:
	jz	FLTGET			# if DONUM=0 then floating #'s allowed
	cmp	al, '.'			# is it dot?
	jnz	Q_Q_L033
	jmp	STUFFH			# yes, stuff it for heavens sake! (EDIT .)
Q_Q_L033:
	mov	al, LINCON		# get line # token
	call	KRNSAV			# save it
	push	edx			# save deposit pointer
	call	LINGET			# get the line #. (returns DX = line number)
	call	BAKSP			# back up pointer to after last digit
# UNSURE: the deposit pointer on the stack is 4 bytes; LINGET returns the line number in DX.
SAVINT:	pop	esi			# xthl: deposit pointer
	xchg	esi, ebx
	push	esi			# exchange current [h,l] with saved [d,e]
	xchg	ebx, edx		# get saved [d,e] in [d,e]; BL/BH = line number
SAVI:	mov	al, bl			# get low byte of value returned by LINGET
	call	KRNSAV			# save the low byte of line #
	mov	al, bh			# get high byte
POPSTF:	pop	ebx			# restore [h,l] (text pointer, 4 bytes)
	call	KRNSAV			# save it too
	jmp	KLOOP			# eat some more
FLTGET:	push	edx			# save deposit pointer
	push	cx			# save char count
	mov	al, BYTE PTR [ebx]	# FIN assumes char in [a]
	call	FIN			# read the #
	call	BAKSP			# back up pointer to after last digit
	pop	cx			# restore char count
	pop	edx			# restore deposit pointer
	push	ebx			# save text pointer
	mov	al, BYTE PTR VALTYP	# get value type
	cmp	al, 2			# integer?
	jnz	NTINTG			# no
	mov	bx, WORD PTR _FACLO	# get it
	mov	al, bh			# get high part
	or	al, al			# is it zero?
	mov	al, 2			# restore int VALTYP
	jnz	NTINTG			# then isn't single byte int
	mov	al, bl			# get low byte
	mov	bh, bl			# get low byte in high byte to store
	mov	bl, IN2CON		# get constant for 1 byte ints
	cmp	al, 10			# is it too big for a single byte constant?
	jae	SAVI			# too big, use single byte int
	add	al, ONECON		# make single byte constant
	jmp	POPSTF			# pop [h,l] & stuff away char
NTINTG:	push	ax			# save for later
	ror	al, 1			# divide by two
	add	al, INTCON-1		# add offset to get token
	call	KRNSAV			# save the token
	mov	ebx, offset _FACLO	# get start pointer
	call	GETYPR			# set cc's on VALTYPE
	jb	NTDBL			# if not double, start moving at FACLO
	mov	ebx, offset DFACLO	# double, start moving at DFACLO
NTDBL:	pop	ax
MOVCON:	push	ax			# save byte move count
	mov	al, BYTE PTR [ebx]	# get a byte
	call	KRNSAV			# save it in krunch buffer
	pop	ax			# get back count
	inc	ebx			# bump pointer into FAC
	dec	al			# move it down
	jnz	MOVCON			# keep moving it
	pop	ebx			# get back saved text pointer
	jmp	KLOOP			# keep looping

SRCSPC:	mov	edx, offset SPCTAB-1	# get pointer to special character table
SRCSP2:	inc	edx			# move pointer ahead
	mov	esi, edx
	mov	al, BYTE PTR [esi]	# fetch from code segment
	and	al, 0177		# mask off high bit
	jnz	Q_Q_L034
	jmp	NOTRS5			# if end of table, stuff away, don't change DONUM
Q_Q_L034:
	inc	edx			# bump pointer
	cmp	al, BYTE PTR [ebx]	# is this special char same as current text char?
	mov	esi, edx
	mov	al, BYTE PTR [esi]	# fetch from code segment (MOV does not change flags)
	jnz	SRCSP2			# if no match, keep looking
	jmp	NOTRS1			# found, save away and set DONUM=1

NTSNGT:	cmp	al, '&'			# octal constant?
	jz	Q_Q_L035
	jmp	STUFFH			# just stuff it away
Q_Q_L035:
	push	ebx			# save text pointer
	call	CHRGTR			# get next char
	pop	ebx			# restore text pointer
	call	MAKUPS			# make char upper case
	cmp	al, 'H'			# hex constant?
	mov	al, OCTCON		# assume octal constant
	jnz	WUZOCT			# yes, it was
	mov	al, HEXCON		# no, was hex
WUZOCT:	call	KRNSAV			# save it
	push	edx			# save current deposit pointer
	push	cx			# save count
	call	OCTCNS			# get the value
	pop	cx			# restore [b,c]
	jmp	SAVINT			# save the integer in the krunch buffer

KRNSVC:	mov	al, ':'			# get colon
KRNSAV:
	mov	edi, edx
	mov	BYTE PTR [edi], al	# save byte in krunch buffer
	inc	edx			# bump pointer
	dec	cx			# decrement count of bytes left in buffer
	mov	al, cl			# test if it went to zero
	or	al, ch			# by seeing if double byte zero.
	jz	LBOERR			# buffer full
	ret				# all done if still space left
LBOERR:	mov	dl, ERRLBO		# get error code
	jmp	ERROR			# jump to error routine
NOTRES:
	jmp	CRUNCX			# go to extended CRUNCH code.
NOTRFN:
	pop	ebx			# get back pointer to original char
	dec	ebx			# now point to first alpha char
	dec	al			# set a to minus one
	mov	BYTE PTR DONUM, al	# flag we're in variable name
	pop	cx			# get back char count
	pop	edx			# get back deposit pointer
	call	MAKUPL			# get char from line, make upper case
KRNVAR:	call	KRNSAV			# save char
	inc	ebx			# increment source pointer
	call	MAKUPL			# make upper case (?)
	call	ISLET2			# is it a letter?
	jae	KRNVAR			# yes, eat
	cmp	al, '9'+1		# digit?
	jae	JKLOOP			# no, too large
	cmp	al, '0'
	jae	KRNVAR			# yes, eat
	cmp	al, '.'			# is it dot
	jz	KRNVAR			# yes, dots ok in var names
JKLOOP:	jmp	KLOOP			# done looking at variable name
NOTRS5:	mov	al, BYTE PTR [ebx]	# get char from line
	cmp	al, 32			# space or higher?
	jae	NOTRS1			# yes = save it
	cmp	al, 9			# tab?
	jz	NOTRS1			# yes = that's ok
	cmp	al, 10			# also allow...
	jz	NOTRS1			# ...line feeds
	mov	al, 32			# force rest to spaces
NOTRS1:	push	ax			# save this char
	mov	al, BYTE PTR DONUM	# get number ok flag
	inc	al			# see if in a variable name.
	jz	JNTRS6			# if so & special char seen, reset DONUM
	dec	al			# otherwise leave DONUM unchanged.
JNTRS6:	jmp	NOTRS6

# BAKSP backs up the pointer after a # is eaten.
BAKSP:	dec	ebx			# point to previous char
	mov	al, BYTE PTR [ebx]	# get the char
	cmp	al, ' '			# a space?
	jz	BAKSP			# yes, keep backing up
	cmp	al, 9			# tab?
	jz	BAKSP			# yes, back up
	cmp	al, 10			# LF?
	jz	BAKSP
	inc	ebx			# point to char after last non-space
	ret				# all done.
