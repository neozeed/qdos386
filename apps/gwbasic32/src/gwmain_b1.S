# GWMAIN.ASM lines 1623-2635 -> gwmain_b1.S (part B1 of GWMAIN).
# NEWSTT (statement fetcher), CHRGET/CHRCON (character scanner and inline constants),
# DEFSTR/DEFINT/DEFSNG/DEFDBL, INTIDX, FCERR, LINSPC/LINGET, RUN/GOTO/GOSUB/RETURN/DATA/REM,
# LET, ON..GOTO and ON ERROR, RESUME, ERROR, AUTO, IF..THEN.
#
# Widths: text pointers (BX, CONTXT, SAVTXT, DATPTR...) are 4 bytes. Line numbers are 2 bytes.
# Line records are [link 4][line 2][text ... 0]. A PTRCON constant holds a 4-byte pointer to the
# byte before a line's link, so its line number is at pointer+5 (the original was pointer+3).
# Return addresses and dispatch addresses pushed for "RET" are 4 bytes.
#
# Dropped: segment overrides (CS:), the "DB 271O" LXI skip trick (replaced by explicit jumps),
# and the 8086 MVI/LXI opcode tricks in the DEFxxx entries.

	.intel_syntax noprefix
	.include "gwconst.inc"			# shared constants (tokens, error numbers)
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"
	.include "forent.inc"

	.section .text
	.globl NEWSTT, GONE, CHRGTR, CHRGT2, CHRCON, CONFAC, CONFC1, _FCERR, FCERR
	.globl INTIDX, INTID2, LINSPC, LINGET, DEFSTR, DEFINT, DEFREA, DEFDBL, DEFCON

# Constants that are used as immediates here. The embedded-constant token values from GWMAIN
# 1728-1746 are in gwtok.inc (OCTCON, HEXCON, DBLCON, DBLCN1, ONECON). The rest are local.
	.set CONCN2, 16			# token returned, second type constant is scanned
	.set SNGCON, 29			# single precision (4 byte) constant
	.set CONCON, 30			# token returned by CHRGET after a constant was scanned

# ---------------------------------------------------------------------------------------------
# NEW STATEMENT FETCHER. Back here for a new statement: the character at [BX] is ":" or the
# end of the line. The address of this location is left on the stack when a statement runs,
# so a statement can RET when it is done.
# ---------------------------------------------------------------------------------------------
	jmp NEWSTT			# (original: JMP SHORT NEWSTT)
GOPRGD:	jmp PRGEND			# fix syntax error in unended error routine
GOSNER:	jmp SNERR			# give syntax error
GORET:	ret				# so people can branch here
GONCHR:	call CHRCON			# rest of CHRGET
	jmp GONE0
GISMD_:
	call NEWSTX			# call extended statement dispatcher
	jmp ISMID_			# try for LHS MID$ statement
GONLET:	jmp LET				# goto LET code
GONZER:	or al, al
	jz GONZ1			# a zero ends the line
	cmp al, 13			# RUN starts at TXTTAB-1, which holds CR (the same end-of-line test)
	jnz GOSNER
GONZ1:	inc ebx
	.globl GONE4
GONE4:	mov eax, DWORD PTR [ebx]	# check the link to see if it is zero (end of program)
	test eax, eax
	jz GOPRGD			# fix syntax error in unended error routine
	add ebx, 4			# point at the line number
	mov dx, WORD PTR [ebx]
	add ebx, 1			# point at the byte before the text (GONE steps onto the first character)
	mov WORD PTR CURLIN, dx		# set up CURLIN with the current line number
	test BYTE PTR TRCFLG, 255	# TESTBI TRCFLG,0FFH: tracing?
	jz GONE				# skip this printing
	push ebx			# save the text pointer
	mov al, '['			# format the line number
	call OUTDO			# output it
	xchg bx, dx			# put the line number to print in BX
	call LINPRT			# print the line number in BX
	mov al, ']'			# some more formatting
	call OUTDO
	pop ebx				# restore the text pointer
	jmp GONE			# continue execution

	.globl NEWSTT
NEWSTT:
	mov al, BYTE PTR ONGSBF
	or al, al
	jz 1f
	call GOTRP			# possible ON GOSUB
1:
NWSTRT:					# entry for "device ON" statement
NEWST1:
	mov DWORD PTR SAVSTK, esp	# MOV SAVSTK,SP (widened to 4 bytes)
	mov DWORD PTR SAVTXT, ebx	# save the text pointer
	call CHKINT			# get key from keyboard and check for CTL-C
GONGET:	mov al, BYTE PTR [ebx]		# get the current character which ended the last statement
	cmp al, ':'			# is it a colon?
	jnz GONZER			# should be zero at EOL
	.globl GONE
GONE:	inc ebx				# do part of CHRGET's work
	mov al, BYTE PTR [ebx]		# get byte
	cmp al, ':'			# colon?
	jb GONCHR			# do rest of CHRGET
GONE0:	mov edx, OFFSET NEWSTT		# push on a return address of NEWSTT statement
	push edx
GONE3:	jz GORET			# if a terminator, try again ("IF" comes here)
GONE2:
	sub al, _END			# "ON ... GOTO" and "ON ... GOSUB" come here
	jb GONLET			# must be a LET
	cmp al, NUMCMD
	jae GISMD_			# see if LHS MID$ case
	movzx esi, al			# index the statement table (4-byte entries)
	push DWORD PTR STMDSP[esi*4]	# push the statement's address
# NEWSTT falls into CHRGET. This fetches the first char after the statement token; the
# CHRGET "RET" dispatches to the statement.

	.globl CHRGTR, CHRGT2, CHRCON
CHRGTR:	inc ebx				# duplication of CHRGET RST for speed
CHRGT2:	mov al, BYTE PTR [ebx]		# see CHRGET RST for explanation
	cmp al, ':'
	jb CHRCON
	ret
# CHRCON is the continuation of the CHRGET RST. In EXTENDED, check for an inline constant and
# if there is one, move it into the FAC and set VALTYP appropriately.
CHRCON:	cmp al, ' '			# must skip spaces
	jz CHRGTR			# get another character
	jb LOWCHR			# some funny thing
	cmp al, '0'			# all characters greater than "9" have returned, so see if numeric
	cmc				# make numerics have carry on
	inc al				# set zero if [A]=0
	dec al
	ret
LOWCHR:	or al, al			# null at EOL?
	jnz 1f
	ret				# yes, all done
1:	cmp al, OCTCON			# is it an inline constant?
	jb NOTCON			# no, should be TAB or LF
	cmp al, CONCON			# are we trying to re-scan a constant?
	jnz NTRSCC			# no
	mov al, BYTE PTR CONSAV		# get the saved constant token
	or al, al			# set non-zero, non carry CCs
	ret				# all done

NTRSCC:	cmp al, CONCN2			# going to scan past an embedded constant?
	jz CONSCN			# yes, scan and go on
NTRSC2:	push ax				# save token to return
	inc ebx				# point to number
	mov BYTE PTR CONSAV, al		# save current token
	sub al, INTCON			# is it less than integer constant?
	jae MAKTKN			# no, not line number constant
	sub al, 0365			# <ONECON-INTCON>&^O377: less than embedded 1 byter
	jae ONEI			# was one byter
	cmp al, IN2CON-ONECON		# is it two byter?
	jnz FRCINC			# nope, normal int
	mov al, BYTE PTR [ebx]		# get embedded int
	inc ebx				# point after constant
ONEI:	mov DWORD PTR CONTXT, ebx	# save text pointer
	mov bh, 0			# get upper byte of zero
ONEI2:	mov bl, al			# get value
	mov WORD PTR CONLO, bx		# save constant value
	mov al, 2			# get VALTYPE
	mov BYTE PTR CONTYP, al		# set it up in save place
	mov ebx, OFFSET NUMCON		# point to number re-scanner
	pop ax				# get back token
	or al, al			# make sure number flag re-set
	ret				# return to caller

FRCINC:	mov al, BYTE PTR [ebx]		# get low byte of constant
	inc ebx				# point past it
	inc ebx				# to next thing
	mov DWORD PTR CONTXT, ebx	# save pointer past
	dec ebx				# back to high byte
	mov bh, BYTE PTR [ebx]		# get high byte
	jmp ONEI2			# finish scanning

	.globl CONFAC
CONFAC:	call CONFC1			# scan floating constant
CONSCN:	mov ebx, DWORD PTR CONTXT	# get saved text pointer
	jmp CHRGT2			# and scan thing after constant

MAKTKN:	inc al				# calculate VALTYPE
	rol al, 1			# *2 to get VALTYPE 0=2, 1=4, 3=8
	mov BYTE PTR CONTYP, al		# CONTYPE now set up
	push edx			# save some regs (4 bytes: may hold pointers)
	push ecx
	mov edx, OFFSET CONLO		# place to store saved constant
	xchg ebx, edx			# get text pointer in EDX
	mov ch, al			# set up counter in CH
	call MOVE1			# move data in
	xchg ebx, edx			# get text pointer back
	pop ecx				# restore ECX
	pop edx
FININ1:	mov DWORD PTR CONTXT, ebx	# save the good text pointer
	pop ax				# restore token
	mov ebx, OFFSET NUMCON		# get pointer to fake text
	or al, al			# clear carry so others don't think it's a number, and set
	ret				# non-zero so it is not a terminator

NOTCON:	cmp al, 9			# line feed or tab?
	jnae NOTLFT
	jmp CHRGTR			# yes, eat
NOTLFT:	cmp al, '0'			# all characters greater than "9" have returned, so see if numeric
	cmc				# make numerics have carry on
	inc al				# set zero if [A]=0
	dec al
	ret

# CONFC1 moves the saved constant into the FAC.
	.globl CONFC1
CONFC1:	mov al, BYTE PTR CONSAV		# get constant token
	cmp al, LINCON+1		# line # constant? (ERL=#)
	jae NTLINE			# no
	cmp al, PTRCON			# line pointer constant?
	jb NTLINE			# no
	jz 1f				# PTRCON: pointer path
	mov bx, WORD PTR CONLO		# LINCON: the value is the line number
	jmp INEG2			# float it
1:	mov ebx, DWORD PTR CONLO	# pointer to the byte before the line's link
	add ebx, 5			# link is 4 bytes (P+1..P+4), line number at P+5
	mov dl, BYTE PTR [ebx]		# get line # in DX
	inc ebx
	mov dh, BYTE PTR [ebx]		# get high part
	xchg bx, dx			# value to BX
	jmp INEG2			# float it
NTLINE:	mov al, BYTE PTR CONTYP		# get saved constant VALTYP
	mov BYTE PTR VALTYP, al		# save in real VALTYP
	cmp al, 8			# double precision
	jz CONFDB			# yes
	mov bx, WORD PTR CONLO		# get low two bytes of FAC
	mov WORD PTR FACLO, bx		# save them
	mov bx, WORD PTR CONLO+2	# get next two bytes
	mov WORD PTR FACLO+2, bx	# save them
	ret

CONFDB:	mov ebx, OFFSET CONLO		# get pointer to saved constant area
	jmp VMOVFM			# move into FAC

	.set ERRUS, 8			# "Unprintable error": undefined line (gwdata_errors.inc)
	.set ERRRG, 3			# "RETURN without GOSUB"
	.set F_TRPO, 1			# trap on (bit in the event flag byte)
	.set F_TRPS, 2			# trap stopped

# ---------------------------------------------------------------------------------------------
# RUN, GOSUB, GOTO
#
# A GOSUB entry on the stack (low address first). B is the pad byte. 16 bytes with the return:
#   [NEWSTT return address 4]  -- pushed last, so it is the top of the stack
#   B+0 pad (1)   B+1 $GOSUB token (1)
#   B+2 event flag pointer (4, zero for a plain GOSUB; points at the trap byte for ON event GOSUB)
#   B+6 line number of the GOSUB (2)
#   B+8 text pointer into the GOSUB statement (4)
# RETURN finds the entry with FNDFOR (which returns B+2) and pops event, line, text.
# ---------------------------------------------------------------------------------------------
	.globl RUN
RUN:	jnz 1f				# (flags from the caller's CHRGTR)
	jmp RUNC			# no line number argument
1:	cmp al, LINCON			# line number constant?
	jz CONRUN			# yes
	cmp al, PTRCON			# line pointer (rather unlikely)
	jz CONRUN
	jmp LRUN
CONRUN:
	# clean up and set [H,L]=[TXTTAB]-1 and return to NEWSTT
	call CLEARC			# clean up: reset the stack, DATPTR, variables
	mov ecx, offset NEWSTT
	jmp RUNC2			# put NEWSTT on and fall into GOTO

	.globl GOSUB, GOSUB2
GOSUB:	mov cl, 8			# room for the 16-byte entry (8 words)
	call GETSTK			# make sure there is room
	call LINGET			# must scan the line number now
	pop ecx				# pop off the return address of NEWSTT
	push ebx			# really push the text pointer
	push ebx			# save the text pointer
	movzx ebx, WORD PTR CURLIN	# get the current line number
	pop esi				# XTHL
	xchg esi, ebx
	push si				# put CURLIN (2 bytes: the entry's line number) on the stack, BX = text pointer
	xor ecx, ecx			# so we know it is not an event flag pointer
	push ecx			# (4-byte event pointer = 0)
	mov ah, _GOSUB			# $GOSUB token (AL is the pad)
	push ax
	mov ecx, offset NEWSTT		# the return address of the GOSUB
	push ecx
	jmp GOTO2			# have now grabbed the line number properly

# Entry point for GOSUB with DE = line pointer (not line number). Used by ON EVENT GOSUB.
# The event flag pointer is passed in ECX; the frame has no NEWSTT return address.
GOSUB2:
	push ebx			# put current text position on the stack
	push ebx
	movzx ebx, WORD PTR CURLIN
	pop esi				# XTHL
	xchg esi, ebx
	push si				# put CURLIN (2 bytes) on the stack
	push ecx			# save the address of the return-modify address (event pointer)
	mov ah, _GOSUB			# put GOSUB on the stack (final GOSUB entry)
	push ax
	xchg ebx, edx			# BX points to the subroutine text
	dec ebx				# NEWSTT does INX H after loading SAVTXT
	mov DWORD PTR SAVTXT, ebx	# save text pointer here, CURLIN later
	inc ebx
	jmp GONE4			# continue with the subroutine

RUNC2:	push ecx			# restore the return address of NEWSTT

# GOTO. In the 8K version we start searching where we are if going to a forward location.
	.globl GOTO
GOTO:	call LINGET			# pick up the line number (DX value, or pointer for PTRCON)
GOTO2:
	mov al, BYTE PTR CONSAV		# get the token for the line number back
	cmp al, PTRCON			# was it a pointer?
	xchg ebx, edx			# assume so (XCHG does not change the flags)
	jnz 1f
	ret				# if it was, go back to NEWSTT with BX as the text pointer
1:	cmp al, LINCON			# make sure it is a line number constant
	jz 2f
	jmp SNERR			# if not, bad argument present
2:	xchg ebx, edx			# flip back if not
	push ebx			# save the current text pointer on the stack
	mov ebx, DWORD PTR CONTXT	# get the pointer to right after the constant
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# save on the stack, restore the current text pointer
	call REM			# skip to the end of this line
	inc ebx				# point at the link beyond it
	push ebx			# save the pointer
	movzx ebx, WORD PTR CURLIN	# get the current line number
	cmp bx, dx			# [D,E] is where we are going, [H,L] the current line number:
				# comparing tells whether to search from here or from TXTTAB
	pop ebx				# [H,L] = the saved pointer
	jnb 3f
	call LOOP			# search from this point
3:	jnae 4f
	call FNDLIN			# search from the beginning (actually: search again if above failed)
4:	jae USERR			# line not found, death
	dec ecx				# point to zero at end of previous line
	pop ebx				# discard the saved pointer (the constant stays a LINCON)
	# No PTRCON conversion here. The 8086 replaced the line number in the text by a 2-byte pointer.
	# A 4-byte pointer does not fit the 2-byte LINCON slot (CONCH2 wrote only 2 value bytes, so a
	# second GOTO/GOSUB read a truncated pointer and faulted). PTRFLG stays 0, so DEPTR is a no-op.
	mov ebx, ecx			# [H,L] = pointer to the start of the matched line
	ret				# go to NEWSTT

	.globl USERR
USERR:	mov dl, ERRUS
	jmp ERROR

# ---------------------------------------------------------------------------------------------
# RETURN. See GOSUB for the stack format. RETURN restores the line number and text pointer
# after eliminating all the FOR entries in front of the GOSUB entry.
# ---------------------------------------------------------------------------------------------
	.globl RETURN
RETURN:	mov DWORD PTR TEMP, ebx		# save the text pointer to see if there is a line number
	mov edx, 0xFFFFFFFF		# make sure this variable pointer never gets matched
	call FNDFOR			# go past all the FOR entries
	cmp al, _GOSUB			# see if a matching GOSUB was found
	jz 1f				# branch if so
	dec ebx				# else error, reset the stack pointer
	dec ebx
1:	mov esp, ebx			# update the stack
	mov DWORD PTR SAVSTK, ebx	# update the saved stack
	cmp al, _GOSUB
	mov dl, ERRRG			# error ERRRG is "RETURN without GOSUB"
	jz 2f
	jmp ERROR
2:	pop ebx				# get the trap-bits re-enable address if GOSUB (event pointer)
	or ebx, ebx			# see if it is zero
	jz NOTEVT			# branch if not an ON EVENT GOSUB
	mov al, BYTE PTR [ebx]		# was a trap, else just a FAC address
	and al, F_TRPO			# is the trap on?
	jz 3f
	call RSTTRP			# if yes, reset stop
3:
NOTEVT:	pop cx				# get the line number of the GOSUB
	mov ebx, offset NEWSTT
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# put the return address of NEWSTT back on the stack; get the text
				# pointer from the GOSUB
	xchg ebx, edx			# DX = text pointer from GOSUB (BX is the old DX)
	mov ebx, DWORD PTR TEMP		# go back to the start
	dec ebx
	call CHRGTR
	jz 4f
	jmp GOTO			# go process the RETURN [line #]
4:	mov bh, ch			# set CURLIN for GOSUB
	mov bl, cl
	mov WORD PTR CURLIN, bx
	xchg ebx, edx			# BX = GOSUB text pointer again
	# skip over some characters: the line number argument was not read yet when GOSUB
	# put the text pointer on the stack. The original's "MVI A" skip of POP BX is a jump here.
	jmp DATA

	.globl DATAH
	.globl DATAS
DATAH:	pop ebx				# get the text pointer off the stack

# ---------------------------------------------------------------------------------------------
# DATA and REM. DATA terminates on ":" or 0 (the ":" applies only when quotes have matched up).
# REM terminates on 0 only. Inside quotes the only terminator is 0 (for REM) or ":" and 0 (DATA).
# ---------------------------------------------------------------------------------------------
	.globl DATA
DATA:	mov cl, ':'			# "DATA" terminates on ":" and 0 (":" applies only when quotes match)
	jmp REMZER
	# DATAS = DATA (the original's dispatch-table name). Set after DATA: GNU as 1.38 leaves an alias of a later label undefined.
	.set DATAS, DATA

	.globl ELSES
ELSES:				# executed ELSEs are skipped (same as REM)
	.globl REM
REM:	mov cl, 0			# the only terminator is zero
REMZER:	mov ch, 0			# inside quotes the only terminator is zero
EXCHQT:	mov al, cl			# when a quote is seen the second terminator is traded
	mov cl, ch
	mov ch, al			# so colons inside quotations have no effect
REMER:	dec ebx				# NOP the INX H in CHRGET
REMER1:	call CHRGTR			# get a char
	or al, al			# zero is always a terminator
	jnz 1f
	ret
1:	cmp al, ch			# test for the other terminator
	jnz 2f
	ret
2:	inc ebx
	cmp al, 34			# is it a quote?
	jz EXCHQT			# if so, time to trade
	# When an IF takes a false branch it must find the matching ELSE. DATA counts the IFs it sees
	# so the ELSE code can match ELSEs with IFs; the count is kept in DH.
	inc al				# function token? then ignore the following FN number
	jz REMER1
	sub al, _IF+1			# is it an IF?
	jnz REMER			# if not, continue on
	cmp al, ch			# since REM cannot smash DE, only increment DH when CH is not zero
	adc al, dh			# carry on if CH is not zero (the IF count)
	mov dh, al			# update DH
	jmp REMER

# ---------------------------------------------------------------------------------------------
# DEFSTR, DEFINT, DEFSNG, DEFDBL. The original's "DB 271O" (LXI B, skipping the next MVI) is
# replaced by an explicit jump to DEFCON.
# ---------------------------------------------------------------------------------------------
	.globl DEFSTR, DEFINT, DEFREA, DEFDBL
DEFSTR:	mov dl, 3			# default some letters to string
	jmp DEFCON
DEFINT:	mov dl, 2			# default some letters to integer
	jmp DEFCON
DEFREA:	mov dl, 4			# default some letters to single precision
	jmp DEFCON
DEFDBL:	mov dl, 8			# default some letters to double precision
DEFCON:	call ISLET			# make sure the argument is a letter
	mov ecx, OFFSET SNERR		# prepare the "syntax error" return
	push ecx
	jnb 1f
	ret				# return (to SNERR) if there is no letter
1:	sub al, 'A'			# make an offset into DEFTBL
	mov cl, al			# save the initial offset
	mov ch, al			# assume it will be the final offset
	call CHRGTR			# get the possible dash
	cmp al, MINUTK			# a range argument?
	jnz NOTRNG			# if not, just one letter
	call CHRGTR			# get the final position
	call ISLET			# check for a letter
	jnb 2f
	ret				# give a syntax error if improper
2:	sub al, 'A'			# make it an offset
	mov ch, al			# put the final in CH
	call CHRGTR			# get the terminator
NOTRNG:	mov al, ch			# get the final character
	sub al, cl			# subtract the start
	jnb 3f
	ret				# if it's less, that's nonsense
3:	inc al				# set up the count right
	pop esi				# XTHL: save the text pointer and get rid of the "syntax error" return
	xchg esi, ebx
	push esi
	mov ebx, offset DEFTBL		# point to the table of defaults
	movzx ecx, cl			# two-byte starting offset (CX = CL)
	add ebx, ecx			# make BX point to the first entry to be modified
LPDCHG:	mov BYTE PTR [ebx], dl		# modify the default table
	inc ebx
	dec al				# count the number of changes to make
	jnz LPDCHG
	pop ebx				# get back the text pointer
	mov al, BYTE PTR [ebx]		# get last character
	cmp al, 44			# is it a comma?
	jz 4f
	ret				# if not, the statement should have ended
4:	call CHRGTR			# otherwise set up to scan the new range
	jmp DEFCON

# INTIDX reads a formula and turns it into a positive integer, leaving the result in DX.
# Negative arguments are not allowed. BX points at the terminating character on return.
	.globl INTIDX, INTID2
INTIDX:	call CHRGTR
INTID2:	call GETIN2			# read a formula and get the result as an integer in DX;
					# the condition codes reflect the high-order bit of the result
	js 1f				# do not allow negative numbers
	ret
1:
	.globl _FCERR, FCERR
_FCERR:
FCERR:	mov dl, ERRFC			# too big: function call error
	jmp ERROR

# ---------------------------------------------------------------------------------------------
# LINSPC, LINGET. LINGET reads a line number from the current text position. The answer is in
# DX: a 16-bit line number for an embedded LINCON, or the full 4-byte pointer for a PTRCON
# (the original's 16-bit pointer, widened). [BX] points at the terminating character on return.
# Line numbers range from 0 to 65529.
# ---------------------------------------------------------------------------------------------
	.globl LINSPC, LINGET
LINSPC:	mov al, BYTE PTR [ebx]		# get char from memory
	cmp al, '.'			# is it the current line specifier?
	mov dx, WORD PTR DOT		# get the current line number (MOV does not change flags)
	jnz LINGET
	jmp CHRGTR			# all done

LINGET:	dec ebx				# backspace the pointer
LINGT2:	call CHRGTR			# fetch char (gobble line constants)
	cmp al, LINCON			# embedded line constant?
	jz LINGT4			# yes, return the double byte value
	cmp al, PTRCON			# also check for a pointer
	jnz LINGT5			# neither: scan the digits
	mov edx, DWORD PTR CONLO	# UNSURE: PTRCON: DX = the 4-byte pointer (original returned a 16-bit pointer here; widened)
	jmp CHRGTR			# eat the following char
LINGT4:	movzx edx, WORD PTR CONLO	# LINCON: the embedded line number
	jmp CHRGTR			# eat the following char
LINGT5:	xor al, al			# set flag that no constant was seen, so GOTO2 does not change LINCON
	mov BYTE PTR CONSAV, al
	xor edx, edx			# zero the accumulated line number
	dec ebx				# back up the pointer
MORLIN:	call CHRGTR
	jnae 1f
	ret
1:	push ebx
	lahf
	push ax
	mov bx, 6552			# see if the line number is too big
	cmp bx, dx
	jb POPHSR			# yes, do not scan any more digits if so: force the caller to see
					# the digit and give a syntax error (cannot just SNERR: non-fast RENUM)
	mov bh, dh			# save DX
	mov bl, dl
	add bx, dx
	add bx, bx
	add bx, dx
	add bx, bx			# putting DX*10 into BX
	pop ax
	sahf
	sub al, '0'
	mov dl, al
	mov dh, 0
	add bx, dx			# add the new digit
	xchg bx, dx
	pop ebx				# get back the text pointer
	jmp MORLIN
POPHSR:	pop ax
	sahf				# get off the terminating digit
	pop ebx				# get back the old text pointer
	ret

# ---------------------------------------------------------------------------------------------
# LET. LETCON is the LET entry point with VALTYPE-3 in AL (GETYPR has been called).
# ---------------------------------------------------------------------------------------------
	.globl LETCON, LET, LETCN4
LETCON:	pop ax				# get VALTYPE off the stack
	add al, 3			# make VALTYPE correct
	jmp LETCN2			# continue

LET:	call PTRGET			# get the pointer to the variable named in the text, into DX
	call SYNCHR
	.byte EQULTK			# check for "="
	mov DWORD PTR TEMP, edx		# set up TEMP for "FOR" here, so user functions work
REDINP:	push edx			# save the variable pointer
	mov al, BYTE PTR VALTYP
	push ax				# save VALTYP
	call FRMEVL			# get the value of the formula
	pop ax				# get the VALTYP of the variable into AL
	.globl LETCN2
LETCN2:	pop esi				# XTHL: BX = pointer to variable, text pointer to top of stack
	xchg esi, ebx
	push esi
	.globl INPCOM
INPCOM:	mov ch, al			# save VALTYP
	mov al, BYTE PTR VALTYP		# get present VALTYPE
	cmp al, ch			# compare the two
	mov al, ch			# get back current
	jz LETCN5			# VALTYPE already set up, go
	call DOCNVF			# force VALTYPEs to be AL's
LETCN4:	mov al, BYTE PTR VALTYP		# get VALTYPE
LETCN5:	mov edx, offset FACLO		# assume this is where to start moving
	cmp al, 5			# is it?
	jb LETCN6			# yes
	mov edx, offset DFACLO		# no, use double precision FAC
LETCN6:	push ebx			# save the pointer at the value position
	cmp al, 3			# string?
	jnz COPNUM			# numeric, so force it and copy
# The following code makes a new copy of the string if:
#  - the string data resides in keyboard or file buffers (temporary data), or
#  - the string data resides in string space (each string variable needs its own copy), or
#  - the source string descriptor is not a temporary descriptor.
	mov ebx, DWORD PTR FACLO	# get the pointer to the descriptor of the result
	push ebx			# save the pointer at the descriptor
	inc ebx
	mov edx, DWORD PTR [ebx]	# the address of the string data
	mov ebx, DWORD PTR TXTTAB	# if the data is in BUF or on disk
	cmp ebx, edx			# since BUF changes all the time
	jae INBUFC			# go copy, if the data really is in BUF
	mov ebx, DWORD PTR STREND	# see if it points into string space
	cmp ebx, edx			# if not, don't copy
	pop edx				# get back the pointer at the descriptor
	jae DNTCPY			# don't copy literals
	mov ebx, offset DSCTMP-1
	cmp ebx, edx			# is the descriptor a temp?
	jb INBFC0			# no, must point to a variable (copy it)
	mov ebx, offset TEMPST-1
	cmp ebx, edx
	jb DNTCPY			# yes, don't copy
INBFC0:	jmp INBFC1			# copy it (the original's "MVI A," skipped the POP DX here)
INBUFC:	pop edx				# get the pointer to the descriptor in EDX
INBFC1:	call FRETMS			# free up a temporary pointing into BUF
	xchg ebx, edx			# STRCPY copies from BX
	call STRCPY			# copy variables in string space or strings with data in BUF
DNTCPY:	call FRETMS			# free up the temporary without freeing any string space
	pop esi				# XTHL: [BX] = place to store the descriptor
	xchg esi, ebx
	push esi			# leave a nonsense entry on the stack; the POP D never matters here
COPNUM:	call VMOVE			# copy a descriptor or a value
	pop edx				# for "FOR", pop off a pointer at the loop variable into EDX
	pop ebx				# get the text pointer back
	ret

# ---------------------------------------------------------------------------------------------
# ON ... GOTO, ON ... GOSUB, ON ERROR GOTO, ON event GOSUB. The "ON event GOSUB" trap entries
# are set through SETGSB (gwsts.S), which takes the line pointer in EDX and the event in AL.
# ---------------------------------------------------------------------------------------------
	.globl ONGOTO
ONGOTO:	cmp al, _ERROR			# "ON ... ERROR"?
	jnz NTOERR			# no
	call CHRGTR			# get next thing
	call SYNCHR
	.byte _GOTO			# must have ... GOTO
	call LINGET			# get following line number
	or edx, edx			# is the line number zero?
	jz RESTRP			# if ON ERROR GOTO 0, reset the trap
	call FNDLN1			# see if the line exists (saves BX on the stack)
	mov edx, ecx			# get the pointer to the line in EDX (link field of the line)
	pop ebx				# restore the text pointer
	jnae 1f
	jmp USERR			# error if line not found
1:
RESTRP:	mov DWORD PTR ONELIN, edx	# UNSURE: gwdata.S has ONELIN as 2 bytes; it must be 4 (report)
	jnb 2f
	ret				# (cannot happen: OR above clears CF)
2:	mov al, BYTE PTR ONEFLG		# are we in an "ON...ERROR" routine?
	or al, al			# set condition codes
	mov al, dl			# (the original wanted an even stack pointer here)
	jnz 3f
	ret				# if not, trapping is already disabled
3:	mov al, BYTE PTR ERRFLG		# get error code
	mov dl, al			# into DL
	jmp ERRESM			# force the error to happen

NTOERR:	call ONGOTP			# check for "ON event GOSUB ..." (CX = first trap, count)
	jb NTONGS			# not an event trap
	push cx				# save start trap number (in CH) and number of events (in CL)
	call CHRGTR			# skip over the event token
	call SYNCHR
	.byte _GOSUB			# "ON event GOSUB nn,nn, ..."
	xor al, al			# AL = line number count
ONGSB0:	pop cx
	push cx
	cmp al, cl			# within range of allowable traps?
	jnae 4f
	jmp SNERR
4:	lahf				# save the flags
	xchg al, ah
	push ax
	xchg al, ah
	call LINGET
	mov al, dh
	or al, dl			# line 0?
	jz RSTRAP			# yes, just reset the port trap
	call FNDLN1			# line number exists? (saves BX on the stack)
	mov edx, ecx			# DX = pointer to line
	pop ebx				# restore the text pointer
	jnae 5f
	jmp USERR			# error if line not found
5:
RSTRAP:	pop ax				# UNSURE: ON event stack balance (RSTRAP) checked by hand, not run
	# the saved flags
	xchg al, ah
	sahf
	pop cx				# AL = port number (from the count)
	lahf
	xchg al, ah
	push ax
	xchg al, ah
	add al, ch			# trap number for this event
	push cx
	call SETGSB			# set the GOSUB address for this event in AL
	lahf
	dec ebx
	sahf
	call CHRGTR
	pop cx
	pop dx				# (the flags word, 2 bytes)
	jnz 6f
	ret
6:	push cx
	push dx
	call SYNCHR
	.byte 054			# comma (054 octal = 44)
	pop ax				# the flags word
	xchg al, ah
	sahf
	inc al				# next area
	jmp ONGSB0			# do next GOSUB address

NTONGS:	call GETBYT			# get value into DL
	.globl NTONG1
NTONG1:				# jumped to by MENU statement
	mov al, BYTE PTR [ebx]		# get the terminator back
	mov ch, al			# save this character for later
	cmp al, _GOSUB			# an "ON ... GOSUB" perhaps?
	jz ISGOSU			# yes, some feature use
	call SYNCHR
	.byte _GOTO			# otherwise must be "GOTO"
	dec ebx				# back up the character pointer
ISGOSU:	mov cl, dl			# get count into CL
LOOPON:	dec cl				# see if enough skips
	mov al, ch			# put the dispatch character in place
	jnz 1f
	jmp GONE2			# if done, go off
1:	call LINGT2			# skip over a line number
	cmp al, 44			# a comma
	jz 2f
	ret				# if a comma doesn't delimit the end of the line number, we are at the
					# end of the line
2:	jmp LOOPON			# continue gobbling line numbers

# ---------------------------------------------------------------------------------------------
# RESUME and ERROR statements
# ---------------------------------------------------------------------------------------------
	.globl RESUME
RESUME:	mov al, BYTE PTR ONEFLG		# get flag
	or al, al			# were we called by an ON ERROR?
	jnz 1f
	jmp REERR			# not in an ON ERROR routine
1:	xor al, al
	mov BYTE PTR ERRFLG, al		# clear error flag so ^C does not give an error
	mov al, BYTE PTR [ebx]		# get the current character back
	cmp al, _NEXT			# RESUME NEXT?
	jz RESNXT			# yup
	call LINGET
	jz 2f
	ret				# should terminate
2:	or edx, edx			# is the line number zero?
	jz RES0				# yes, go set non-zero CCs
	call GOTO2			# go find the line
	xor al, al			# now zero flag (otherwise a RESUME to a nonexistent line would be
	mov BYTE PTR ONEFLG, al		# trapped by ON ERROR and loop forever)
	ret

RESNXT:	call CHRGTR			# must terminate
	jz 3f
	ret				# blow him up
3:	jmp RESTXT

RES0:	xor al, al
	mov BYTE PTR ONEFLG, al		# clear RESUME-trap flag
	inc al				# set non-zero CCs
RESTXT:	mov ebx, DWORD PTR ERRTXT	# get pointer into line
	xchg ebx, edx			# save ERRTXT in EDX
	movzx ebx, WORD PTR ERRLIN	# get line number
	mov WORD PTR CURLIN, bx		# save in current line number
	xchg ebx, edx
	jz 4f
	ret				# go to NEWSTT if just "RESUME"
4:	mov al, BYTE PTR [ebx]		# get ":" or the line header
	or al, al			# set CC
	jnz NOTBGL			# #0 means must be ":"
	add ebx, 6			# skip the header: link (4) and line number (2)
NOTBGL:	inc ebx				# point to start of this statement
	xor al, al
	mov BYTE PTR ONEFLG, al
	jmp DATA			# get next statement

# "ERROR <code>" forces an error of type <code>, which must be 0..255.
	.globl ERRORS
ERRORS:	call GETBYT			# get the parameter
	jz 1f
	ret				# should have terminated
1:	or al, al			# error code 0?
	jnz GOERR
	jmp FCERR			# yes, error in itself
GOERR:	jmp ERROR			# force an error

# ---------------------------------------------------------------------------------------------
# AUTO [beginning line[,[increment]]]. Generates line numbers for insertion. The beginning
# line defaults to 10, the increment to 10 (or to the previous increment if only a comma).
# ---------------------------------------------------------------------------------------------
	.globl AUTO
AUTO:	mov dx, 10			# assume initial line number of 10
	push dx				# save it (2 bytes: a line number)
	jz SNGAUT			# if end of command, use 10,10
	call LINSPC			# get line number, allowing "." for the current line
	xchg ebx, edx			# get text pointer in EDX
	pop esi				# XTHL
	xchg si, bx
	push esi				# put initial line on the stack; get 10 in BX
	jz SNGAU1			# if terminator, use increment of 10
	xchg ebx, edx			# get text pointer back in BX
	call SYNCHR
	.byte 054			# comma must follow (054 octal = 44)
	mov dx, WORD PTR AUTINC		# get the previous increment
	jz SNGAUT			# use previous increment if terminator
	call LINGET			# get increment
	jz 1f
	jmp SNERR			# should have finished
1:
SNGAUT:	xchg ebx, edx			# get increment in BX
SNGAU1:	mov al, bh			# see if zero
	or al, bl
	jnz 2f
	jmp FCERR			# zero increment gives FCERR
2:	mov WORD PTR AUTINC, bx		# save increment
	mov BYTE PTR AUTFLG, al		# set flag to use AUTO in main code
	pop ebx				# get initial line number
	mov WORD PTR AUTLIN, bx		# save in initial line
	pop ecx				# get rid of the NEWSTT address
	jmp MAIN			# jump into main code (rest of AUTO is after MAIN:)

# ---------------------------------------------------------------------------------------------
# IF ... THEN ... ELSE
# ---------------------------------------------------------------------------------------------
	.globl IFS
IFS:	call FRMEVL			# evaluate a formula
	mov al, BYTE PTR [ebx]		# get terminating character of formula
	cmp al, 44
	jnz 1f
	call CHRGTR			# if so skip it
1:	cmp al, _GOTO			# allow "GOTO" as well
	jz OKGOTO
	call SYNCHR
	.byte THENTK			# must have a THEN
	dec ebx
OKGOTO:	push ebx			# save the text pointer
	call VSIGN
	pop ebx				# get back the text pointer
	jz FALSIF			# handle possible "ELSE"
DOCOND:	call CHRGTR			# pick up the first line-number character
	jnz 2f
	ret				# return for "THEN :" or "ELSE :"
2:	cmp al, LINCON			# line number constant?
	jnz 3f
	jmp GOTO			# do a "GOTO"
3:	cmp al, PTRCON			# pointer constant
	jz 4f
	jmp GONE2			# execute statement, not GOTO
4:	mov ebx, DWORD PTR CONLO	# get the text pointer (4 bytes)
	ret				# fetch new statement

# "ELSE" handler: here on a false IF condition.
FALSIF:	mov dh, 1			# number of ELSEs that must be seen; DATA increments this for each
					# IF it sees
SKPMRF:	call DATA			# skip a statement. ":" is put in front of ELSEs so that DATA stops
					# before ELSE clauses
	or al, al			# end of line?
	jnz 1f
	ret				# if so, no ELSE clause
1:	call CHRGTR			# see if we hit an ELSE
	cmp al, _ELSE
	jnz SKPMRF			# no, still in the THEN clause
	dec dh				# decrement the number of ELSEs that must be seen
	jnz SKPMRF			# skip more if have not seen enough
	jmp DOCOND			# found the right ELSE: go execute
