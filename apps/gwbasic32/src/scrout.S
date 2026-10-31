# scrout.S: console output path. Translated from GIOSCN.ASM, GIOCON.ASM, GIOTBL.ASM,
# SCNDRV.ASM (whole screen driver, except the INLIN-only read-back is kept too), the output
# part of GIO86.ASM (CRFIN, CRDONZ, CRDO, OUTDO, GIOINI). SCNSEM and OUTCH1 (SCNEDT.ASM)
# are defined in scedit.S, not here, and
# the OEM screen primitives that are NOT in the source tree (SCROUT, SCRINP, SCROLL, CLREOL,
# CLRSCN, SWIDTH, SCRATR, CSRATR, CSRDSP, SETCSR, GWINI, GETHED, PRTMAP, EDTMAP).
#
# Terminal model (see the platform boundary in platform_win32.S):
#  - Every terminal byte goes through PLAT_CHOUT. Sequential output is the only output.
#  - SCROUT stores each character in the shadow screen (SCRSHD) and emits it only when the
#    position is inside the text window (1..WDOBOT). The reserved status line (LINCNT) is
#    shadow-only, so function-key labels never reach the terminal stream.
#  - CR (LCARET) and LF (LFEED) emit CR and LF. A scroll of the window is not emitted: the
#    terminal scrolls by itself when the LF arrives at its bottom row.
#  - Cursor moves that the original does by hardware positioning (LOCATE, CLS, cursor keys,
#    line editing) update the logical state and the shadow screen only. See the report.
#  - The cursor is never drawn. CSRDSP keeps no state; SETCSR keeps CSRFLG.
#  - Widened pointers: TRMCUR holds a displacement into LINTTB (2 * line), as in the
#    original. Only SCNDRV uses it.

	.intel_syntax noprefix
	.include "oem.inc"
	.include "gio86u.inc"
	.include "msdosu.inc"

# ---------------------------------------------------------------- SCNDRV literals
# Character codes used by the control-character dispatch (SCNDRV.ASM)
	.set CHREDT, 1			# EDiT
	.set CHRBKW, 2			# BacK Word
	.set CHRCAN, 3			# CANcel
	.set CHRHCN, 4			# Hard CaNcel
	.set CHRCLE, 5			# CLear to End of line
	.set CHRFDW, 6			# ForwarD Word
	.set CHRBEL, 7			# BELl
	.set CHRBAK, 8			# BAcKspace
	.set CHRTAB, 9			# TAB
	.set CHRLNF, 10			# LiNeFeed
	.set CHRHOM, 11			# HOMe
	.set CHRERA, 12			# ERAse
	.set CHRRET, 13			# RETurn
	.set CHRAPP, 14			# APPend
	.set CHRINS, 18			# INSert
	.set CHRFKD, 20			# Function Key Display key
	.set CHRLDL, 21			# Line DeLete
	.set CHRADV, 28			# cursor RiGhT
	.set CHRREG, 29			# cursor LeFT
	.set CHRUP, 30			# cursor UP
	.set CHRDWN, 31			# cursor DoWN
	.set CHRDEL, 127		# DELete

# Terminator table literals (SCNDRV.ASM)
	.globl TRMLNF, TRMEOL, TRMWRP, TRMNWP, TRMNUL, SCNSIZ
	.set TRMLNF, 10			# LINEFEED terminator
	.set TRMEOL, 0177		# EOL terminator
	.set TRMWRP, 0			# WRAP terminator
	.set TRMNWP, 1			# NULL WRAP terminator
	.set TRMNUL, 0200		# Sign bit indicates initialize line
	.set SCNSIZ, 20+(40*0400)	# DEFAULT SCREEN SIZE (Height = 20, Width = 40); unused here

# Device ids (GIOTBL.ASM: $_name = NUM, NUM counts down from 0377 octal)
	.globl _KYBD, _SCRN, _CONS, _LPT1, _LPT2, _LPT3, _NDEV
	.set _KYBD, 0377
	.set _SCRN, 0376
	.set _CONS, 0375
	.set _LPT1, 0374
	.set _LPT2, 0373
	.set _LPT3, 0372
	.set _NDEV, 6			# number of non-disk devices (0377 - 0371)

# ---------------------------------------------------------------- shadow screen (state)
# The screen is kept as a shadow in SCRSHD: one 256-byte row per physical line 0..NMLINE.
# Index = row*256 + col. SCRINP and SCROLL read and move it. No terminal read exists.
	.section .bss
	.globl SCRSHD
SCRSHD:	.skip 256*(NMLINE+1)

	.section .text

# ---------------------------------------------------------------- GIOSCN.ASM: SCNDSP
# Screen Dispatch Table (4-byte entries; function offset G_xxx (2 per entry) * 2 = byte offset)
	.section .rodata
	.globl SCNDSP
SCNDSP:
	.long DERBFM		# test EOF for file opened to this device
	.long DERBFM		# LOC
	.long DERBFM		# LOF
	.long SCNCLS		# perform special CLOSE functions for this device
	.long SCNSWD		# set device width
	.long DERBFM		# GET/PUT random record from/to this device
	.long SCNOPN		# perform special OPEN functions for this device
	.long DERBFM		# input 1 byte from file opened on this device
	.long SCNSOT		# output 1 byte to file opened on this device
	.long SCNGPS		# POS
	.long SCNGWD		# get device width
	.long SCNSCW		# set device comma width
	.long SCNGCW		# get device comma width
	.long DEVBIN		# block input from file opened on this device
	.long DEVBOT		# block output to file opened on this device

# ---------------------------------------------------------------- GIOSCN.ASM
	.section .text
	.globl SCNINI, SCNTRM, SCNSWD, SCNOPN, SCNGPS, SCNGWD, SCNSCW, SCNGCW
	.globl SCNSOT, CALTTY, _CATTY, POS

# SCNINI is called to initialize Screen when BASIC comes up
SCNINI:
	mov al, BYTE PTR LINLEN		# Get CRT logical line length
	mov BYTE PTR WDTFLG, al		# Default width of device SCRN:
	ret

# SCNTRM is called to Clean-up Screen when BASIC terminates
SCNTRM:
	call TKEYOF			# Turn off KEY display
	call SCNPOS			# [DX]=current cursor location
	ret

# SCNCLS - perform any device dependent close functions.
SCNCLS:
	ret

# SCNSWD - set device width. Entry - [DX] = new device width
SCNSWD:
	mov BYTE PTR WDTFLG, dl		# Set/Reset infinite length flag
	cmp dl, 255
	jnz SCNWD1			# BRIF not infinite length
	ret
SCNWD1:	push cx
	push ax
	mov al, dl			# pass Width in AL
	mov cl, BYTE PTR LINCNT		# pass Height in CL
	call SWIDTH			# Let screen editor set width
	pop ax
	pop cx
	ret

# SCNOPN - perform any device dependent open functions.
# Exit - [ESI] points to new FDB
SCNOPN:
	call SCNGPS			# [AH]=current column position
	mov dh, ah			# [DH]=current column position
	mov ah, MD_SQO			# allow open for output only
	mov dl, BYTE PTR WDTFLG		# initial file logical width
	jmp INIFDB

# CALTTY IS A SPECIAL ROUTINE TO OUTPUT ERROR MESSAGE TO TTY, REGARDLESS OF CURRENT FILE I/O.
# Entry - [AL] = byte to be output. Exit - All registers preserved
_CATTY:
CALTTY:	push DWORD PTR PTRFIL
	mov DWORD PTR PTRFIL, 0		# Make sure we go to the "TTY"
	call OUTDO
	pop DWORD PTR PTRFIL
	ret

# SCNSOT - Sequential Output. Entry - [ESI] points to FDB, [AL] = byte to be output.
# If the last char was the 1st byte of a 2-byte char, TWOBYT holds it (DBLCHR=0: unused).
SCNSOT:
	pushfd
	push ax

	# If last char SCNSOT was called with was the 1st byte of a 2-byte char, SCNSOT saved it
	# in TWOBYT so both are output as one 16-bit character.
	mov ah, BYTE PTR TWOBYT		# If two byte, put first in [AH]
	or ah, ah
	jz SCNSO1			# BRIF not second of two bytes
	mov BYTE PTR TWOBYT, 0		# Clear TWOBYT flag
	jmp SCNSO3			# Output both bytes at once
SCNSO1:
	test BYTE PTR F_EDIT, 0377
	jz SCNS1A			# BRIF not in editor, don't collect FF codes
	cmp al, 255
	jz SCNS1B			# BRIF is first of two bytes
SCNS1A:
	jmp SCNSO2			# branch if not 1st of 2-bytes
SCNS1B:
	mov BYTE PTR TWOBYT, al		# save char for next time
	jmp SCNSOX			# Set two byte flag and return
SCNSO2:
	xor ah, ah			# clear high-byte (not 2-byte char)
SCNSO3:	call SCNOL1
SCNSOX:	pop ax
	popfd
	ret

# SCNSOT level 1. Outputs AX, destroys AX
SCNOL1:
	# For IBM Compatibility, the filter performs the following translations
	#    x x x CR x x x    === x x x CR LF x x x
	#    x x x CR LF x x x === x x x CR LF x x x
	cmp BYTE PTR LSTCHR, ASCCR	# (LOW OFFSET ASCCR)
	mov BYTE PTR LSTCHR, al		# save this char for comparison with next
	jne NTCRLF			# branch if not LF after CR
	cmp al, ASCLF
	je RET22			# eat LF if it follows CR
NTCRLF:
	push ax
	call SCNOL2			# output this char
	pop ax
	cmp al, ASCCR
	jne RET22			# if not CR, just output char
	mov ax, ASCLF			# else map CR to CR LF
	call SCNOL2
RET22:	ret

# SCNSOT level 2. Output [AX], destroys AX
SCNOL2:
	or esi, esi
	jz SCNSO8			# BRIF is not file I/O
	cmp al, ASCCR
	jz SCNSO8			# BRIF CR, don't do wrap
	push ax				# save char to be output
	mov al, BYTE PTR CSRX
	or ah, ah			# see if 2-byte char
	jz SCNSO4			# BRIF not DBLCHR
	inc al				# Need two char posns for DBLCHR
SCNSO4:
	cmp al, BYTE PTR [esi+F_WID]	# Compare posn with file width
	ja SCNSCR			# Beyond max, force CR
	cmp al, BYTE PTR LINLEN
	jbe SCNSO7			# Within line, go ahead and output
	dec al
	mov BYTE PTR CSRX, al		# Make sure there's room before end of line
SCNSO7:	pop ax				# restore char to be output
SCNSO8:
	jmp SCNOUT			# Output the char in [AX] and return

SCNSCR:	cmp al, BYTE PTR LINLEN
	jz SCNSO7			# BRIF file width .EQ. device width, use wrap code
	mov ax, ASCCR
	call SCNOUT			# Force new line
	mov ax, ASCLF
	call SCNOUT
	jmp SCNSO7			# Output the character

# POS(X) function. Exit: [AL] = column (1-relative)
POS:	mov al, BYTE PTR CSRX		# [AL]=current 1 relative position
	cmp al, BYTE PTR LINLEN
	jbe POS0			# BRIF not beyond end of line
	mov al, 1			# Else next char will go in first column
POS0:	jmp SNGFLT			# return result to user

# SCNGPS - return current file position. Entry - [ESI] points to FDB. Exit - [AH] = column (0-rel)
SCNGPS:	mov ah, BYTE PTR CSRX
	pushfd
	cmp ah, BYTE PTR LINLEN
	jbe SCNGP1			# BRIF not beyond edge of screen
	mov ah, BYTE PTR LINLEN		# Force posn within screen
SCNGP1:	popfd
	dec ah				# Make it 0 relative
	ret

# SCNGWD - get device width. Exit - [AH] = device width
SCNGWD:
	mov ah, BYTE PTR LINLEN
	or esi, esi
	jz SCNGWX			# BRIF not file I/O, use device width
	mov ah, BYTE PTR [esi+F_WID]	# Is file I/O, use FDB width
SCNGWX:	ret

# SCNSCW - set device comma width. SCNGCW - get device comma width. (no-ops)
SCNSCW:	ret
SCNGCW:	ret

# ---------------------------------------------------------------- GIOCON.ASM (CONS device)
	.globl CONDSP, _RET, CONSOT, CONGPS, CONGWD, CONOPN

	.section .rodata
# Console Dispatch Table (4-byte entries)
CONDSP:
	.long DERBFM		# test EOF for file opened to this device
	.long DERBFM		# LOC
	.long DERBFM		# LOF
	.long _RET		# perform special CLOSE functions for this device
	.long DERBFM		# set device width
	.long DERBFM		# GET/PUT random record from/to this device
	.long CONOPN		# perform special OPEN functions for this device
	.long DERBFM		# input 1 byte from file opened on this device
	.long CONSOT		# output 1 byte to file opened on this device
	.long CONGPS		# POS
	.long CONGWD		# get device width
	.long _RET		# set device comma width
	.long _RET		# get device comma width
	.long DEVBIN		# block input from file opened on this device
	.long DEVBOT		# block output to file opened on this device

	.section .text
# CONOPN - perform any device dependent open functions. Exit - [ESI] points to new FDB
CONOPN:
	mov ah, MD_SQO			# allow open for output only
	mov dx, 255			# initial file column position=0, initial file width=255
	jmp INIFDB

# CONGPS - return current file position. Exit - [AH] = current file column (0-relative)
CONGPS:	mov ah, 0
_RET:	ret

# CONGWD - get device width. Exit - [AH] = 255 (infinite width)
CONGWD:
	mov ah, 255
	ret

# CONSOT - Write one byte to the console. ENTRY - AL = Character to output.
# Output of FFH is not allowed (the original skips it).
CONSOT:	push ax
	push ebx
	push cx
	push dx
	cmp al, 255
	jz CONSOX			# Don't allow outputing FFH
	call PLAT_CHOUT			# Direct Console I/O (C_DCIO) through the platform boundary (AL)
CONSOX:	pop dx
	pop cx
	pop ebx
	pop ax
	ret

# ---------------------------------------------------------------- GIOTBL.ASM
# Device name table: text followed by device number. 0 ends the table.
	.globl _DVTBL, _DVPTR, _DVINI, _DVTRM
	.section .rodata
_DVTBL:
	.ascii "KYBD"
	.byte _KYBD
	.ascii "SCRN"
	.byte _SCRN
	.ascii "CONS"
	.byte _CONS
	.ascii "LPT1"
	.byte _LPT1
	.ascii "LPT2"
	.byte _LPT2
	.ascii "LPT3"
	.byte _LPT3
	.byte 0

# Dispatch table addresses: entry 0 = disk, then one entry per non-disk device (4 bytes each)
	.section .data
_DVPTR:
	.long DSKDSP			# 1st entry in dispatch table is Disk Dispatch table
	.long KYBDSP
	.long SCNDSP
	.long CONDSP
	.long LPTDSP
	.long LPTDSP
	.long LPTDSP

# Initialization dispatch table (one entry per non-disk device)
_DVINI:
	.long KYBINI
	.long SCNINI
	.long _RET
	.long LPTINI
	.long LPTINI
	.long LPTINI

# Termination dispatch table (one entry per non-disk device)
_DVTRM:
	.long KYBTRM
	.long SCNTRM
	.long _RET
	.long LPTTRM
	.long LPTTRM
	.long LPTTRM

# ---------------------------------------------------------------- GIO86.ASM: output
	.globl CRFIN, CRDONZ, CRDO, OUTDO, FILOU3, GIOINI
	.section .text

# CRFIN: all references should be eliminated
CRFIN:	xor al, al
	ret

# CRDONZ - output carriage return if file PTRFIL is not at left margin
CRDONZ:	call PTRGPS			# [AH]=0-relative column for PTRFIL
	or ah, ah
	jnz CRDO			# if not at column 0, output CRLF
	ret

# CRDO - output ascii carriage return to current file, then line feed.
# Exit: [AL]=0 and FLAGS.Z true
CRDO:	mov al, ASCCR			# output Carriage Return
	call OUTDO
	mov al, ASCLF			# output Line Feed
	call OUTDO
	xor al, al			# must return with [AL]=0 and FLAGS.Z true
	ret

# OUTDO - output [AL] to current file (force CR if end-of-line). All registers preserved.
# Dispatches function G.SOT to the device of PTRFIL.
FILOU3:
OUTDO:
	pushfd
	push ax
	mov ah, G_SOT
	call PTRDSP			# dispatch to function [AH] for PTRFIL
	pop ax
	popfd
	ret

# GIOINI - I/O initialization called by INIT. The DOS interrupt vectors (MSISET) are not
# used on Win32 (see platform_win32.S), so that step is omitted.
GIOINI:
	xor eax, eax
	mov DWORD PTR STKLOW, eax	# so FDBPTR will work during initialization
	mov DWORD PTR FILTAB, eax	# so FDBPTR will work during initialization
	mov BYTE PTR NLONLY, al
	mov ebx, DWORD PTR DEVINI	# DI points to array of init routines
	call DOALDV
	jmp FINPRT			# reset PTRFIL to Keyboard

# DOALDV - call every non-disk device routine in the table at [EBX]. The table is walked from
# the last entry (device number -_NDEV) back to the first. Entry size is 4 bytes, so the
# byte offset is 4 * count (the original's 2 * count).
DOALDV:
	mov eax, _NDEV			# [CX]=number of devices in system
	shl eax, 2			# [DI]=4*number of non-disk devices
	jz DOALLX			# branch if no non-disk devices
	mov edi, eax
DOALLP:	push ebx
	push edi
	call DWORD PTR [ebx+edi-4]	# Call initialization/termination routine
	pop edi
	pop ebx
	sub edi, 4
	jnz DOALLP			# Loop until Device ID = 0 (disk)
DOALLX:	ret

# ---------------------------------------------------------------- SCNDRV.ASM: state and cursor
	.globl SCNIPL, SCNBRK, SCNSWI, SCNCLR, SCNLOC, SCNPOS, SCNGWI, SCNRDT, SCNOUT
	.globl SCNRDL, SCNMRK, SCNWTT, TTBINI

# SCNIPL(): Initialize - called during IPL
SCNIPL:	mov BYTE PTR ESCFLG, 0
	ret

# SCNBRK: reset flags etc. when ^C typed
SCNBRK:	mov BYTE PTR FSTLIN, 0
	mov BYTE PTR LSTLIN, 0377	# Set FSTPOS, LSTPOS to impossible values
	mov BYTE PTR F_CRET, 0377	# Clear "last char was CR" flag
	ret

# SCNSWI(WIDTH,HEIGHT): Set screen width(logical/physical) and height. AL=width, CL=height
SCNSWI:	mov BYTE PTR CRTWID, al		# Save physical width
	mov BYTE PTR LINLEN, al		# Set logical line length
	mov BYTE PTR LINCNT, cl		# Save physical height
	mov BYTE PTR WDOTOP, 1		# Init window top
	push cx
	dec cl				# Reserve status line
	mov BYTE PTR WDOBOT, cl		# Set window bottom
	pop cx
	mov BYTE PTR WDOLFT, 1		# Set window left
	mov BYTE PTR WDORGT, al		# Set window right
	ret

# SCNLOC(X,Y): Locate cursor on physical screen (1,1 = HOME). AH=column, AL=line.
# State only: the terminal cursor is not moved (see the header).
SCNLOC:	mov BYTE PTR CSRX, ah
	mov BYTE PTR CSRY, al
	ret

# SCNCLR(X,Y): Clear screen, home text and graphics cursor, refresh function key display
SCNCLR:
	call TTBINI			# Clear the terminator table
	call GRPINI			# Home the graphics cursor
	call WHOME			# DX=cursor home
	mov ax, dx
	call SCNLOC			# set cursor position
	jmp KEYDSP			# Conditionally display softkeys and return

# SCNPOS: Read current cursor location. Exit: DL=cursor line, DH=cursor column
SCNPOS:	mov dh, BYTE PTR CSRX
	mov dl, BYTE PTR CSRY
	pushfd
	cmp dh, BYTE PTR LINLEN
	jbe SCNPS1			# BRIF not beyond edge of screen
	mov dh, BYTE PTR LINLEN		# Force posn within screen
SCNPS1:	popfd
	ret

# SCNGWI: Read logical width of lines. Exit: AH=width
SCNGWI:	mov ah, BYTE PTR LINLEN
	ret

# SCNRDT(Y): Read terminator value and column. Entry: DL=line. Exit: AH=column, AL=terminator.
# CF=1: EOL. CF=0 and ZF=1: LF terminated.
# TRMCUR receives the displacement of the entry (2 * line) into LINTTB.
SCNRDT:	push ebx
	movzx ebx, dl			# BX = Displacement into table
	add ebx, ebx
	mov DWORD PTR TRMCUR, ebx	# Save address of terminator reading(for set term)
	mov ah, BYTE PTR [ebx+LINTTB-2]	# Get terminator posn
	mov al, BYTE PTR [ebx+LINTTB-1]	# Get terminator value
	and al, 0177			# Get rid of initialize line status bit (377O-TRMNUL)
	cmp al, TRMEOL
	stc
	jz SCNRTX			# BRIF EOL terminator(CF=1)
	cmp al, TRMLNF			# CF=0, set ZF if linefeed terminated
	jnb SCNRTX			# BRIF carry not set
	cmc				# clear carry
SCNRTX:	pop ebx
	ret

# SCNWTT(TERM): Set new value for terminator last read. AH=column, AL=terminator type.
SCNWTT:	push ebx
	mov ebx, DWORD PTR TRMCUR	# Get address of terminator last read
	mov BYTE PTR [ebx+LINTTB-2], ah
	and BYTE PTR [ebx+LINTTB-1], TRMNUL
	or BYTE PTR [ebx+LINTTB-1], al	# Write new terminator, leaving NULL bit
	pop ebx
	ret

# TTBINI: terminator table init (every line: column 0, EOL terminator)
TTBINI:	mov ecx, NMLINE+1		# CX=count of line term table entries
	mov ebx, offset LINTTB		# BX=addr of line term table
TTBIN0:	mov BYTE PTR [ebx+0], 0		# Clear column number
	mov BYTE PTR [ebx+1], TRMEOL	# Set terminator to <CR>
	inc ebx
	inc ebx
	loop TTBIN0			# Init entire line terminator table
	ret

# SCNOUT(CHAR): Output character. Entry: AX = character.
SCNOUT:	push dx
	push cx
	push ebx
	call CTLDSP			# If control char or ESC sequence, do operation
	jb SCNOTX			# No char to output
	call CHWRAP			# Do wrap for char in AX, return DX=posn for output
	mov BYTE PTR CSRTYP, 0		# Set next cursor type to off
	call SETCSR			# Insure that the cursor is off
	call SCROUT			# Send char in AX to the screen at DX posn
	mov BYTE PTR F_CRET, 0377	# Clear last char was Carriage return flag
SCNOTX:	pop ebx				# pushed as 4 bytes above (was 'pop bx': 2-byte width mismatch)
	pop cx
	pop dx
	ret

# MKRMCI: open up space for char if insert mode
MKRMCI:	pushfd
	test BYTE PTR F_INST, 0377
	jz MKRCIX			# BRIF not insert mode(do nothing)
	popfd
	mov ch, al
	inc ch				# CH=space needed
	call MKRMCH			# Open up space at (DH,DL) for count of CH
	call SCNRDX			# Put terminator table values in CX
	ret
MKRCIX:	popfd
	ret

SCNRDX:	xchg ax, cx
	call SCNRDT
	xchg ax, cx
	ret

# ---------------------------------------------------------------- line wrap
# Do single character wrap. Entry: AX=character, DH=CSRX, DL=CSRY.
# Exit: CF=1 if it cannot wrap; DX=posn where character can be output.
CHWRAP:	push ax
	xor ch, ch
	or ah, ah
	jz CHWRP0			# BRIF Single byte character
	mov ch, 1			# It's two byte character
CHWRP0:	mov cl, ch
	add ch, dh			# CH=column after char is output
	call SCNRDT			# Read terminator
	xchg cx, ax
	# IF POSN .LT. EOL: AH=POSN(WHERE GOING TO), DX=CURRENT POSN, AL=LENGTH, CX=TERMINATORS
	pushfd
	cmp ah, BYTE PTR LINLEN
	ja CHWRP1			# BRIF not space before end of physical line
	popfd
	call MKRMCI			# If insert mode, open up space for character
	# IF NOT(LINEFEED .EQ. TERMINATOR(CURRENT_LINE) AND POSN .EQ. LINLEN) RETURN
	jb CHWPXI			# EOL terminated, output at current posn
	jz CHWPLF			# BRIF LF terminated
	cmp cl, TRMWRP
	jz CHWPXI			# BRIF WRAP(always room)
	cmp ah, BYTE PTR LINLEN
	jb CHWPXI			# BRIF NUL_WRAP and not overwriting NULL
	mov cl, TRMWRP
CHWPXI:	jmp CHWRPX			# Overwriting NULL_WRAP, set to WRAP
CHWPLF:	cmp ah, BYTE PTR LINLEN
	jae CHWPL1			# LF terminated and no room for LF
	inc ah				# Need to terminate at one past posn printing at
	xchg ax, cx
	cmp ah, ch
	mov ah, ch
	ja CHWPL0			# BRIF current posn below terminator posn
	call SCNWTT			# Terminate line at one more than print posn
CHWPL0:	clc
	xchg ax, cx
	pushfd
	dec ah				# Restore column actually printing at
	jmp CHWRP9			# Update LSTPOS, FSTPOS, cursor posn, exit
# ELSE MOVE LINEFEED TO NEXT LINE
CHWPL1:	call WRAPLF			# Move linefeed to next line
	jb CHWRNI			# Error, output at posn-(char length)
	clc
	pushfd
	mov ah, ch
	jmp CHWRP9			# Exit without updating terminator
CHWRNI:	jmp CHWRNO

# DO CASE TERMINATOR(CURRENT_LINE): not enough room on current line for character
CHWRP1:	popfd
	jb CHWREL			# BRIF EOL terminated
	jz CHWRLF			# BRIF LF terminated
# CASE: WRAP; NULL_WRAP: set WRAP terminator
CHWRWP:	mov cl, TRMWRP
	mov ch, BYTE PTR LINLEN
	xchg ax, cx
	call SCNWTT			# Set wrap terminator at last column
	cmp dl, BYTE PTR LSTLIN
	jnz CHWRN0			# BRIF not on last line
	inc BYTE PTR LSTLIN		# Wrap LSTLIN
	mov BYTE PTR LSTCOL, 0		# Force update of LSTCOL at CHWRPZ
CHWRN0:	inc dl				# Next line
	call SCNRDT			# Set up for write at new line
	mov dh, BYTE PTR WDOLFT		# First column is output posn
	mov ch, dh
	add ch, cl			# CH=POSN going to
	xchg ax, cx
	call MKRMCI			# If insert mode, open up space for character
	cmp cl, TRMLNF
	jnz CHWRPX			# BRIF not linefeed terminated
	xchg ax, cx			# CH=POSN, AX=current terminators
	inc ch				# Need terminator one beyond actual output
	cmp ah, ch
	mov ah, ch
	ja CHWRN2			# BRIF Current terminator beyond posn for output
	call SCNWTT			# Set current terminator to new max posn
CHWRN2:	dec ch				# Restore POSN going to
	xchg ax, cx
CHWRPX:	clc
	jmp CHWRPZ			# All done
# CASE: EOL
CHWREL:	call MKRMNL			# Make room on next line(make it EOL term'd)
	jb CHWRP3			# If error, make room for char on this line
	call SCNRDX			# Reread terminator for this line
	jmp CHWRWP			# Wrap to next line
# CASE: LF
CHWRLF:	xchg ax, cx
	call WRAPLF			# First wrap LF to next line
	mov al, TRMNWP			# Set to temporary NULL_WRAP at last column
	mov ah, BYTE PTR LINLEN
	call SCNWTT
	xchg ax, cx
	jnb CHWRWP			# Go do wrap for char
# No room, abort
CHWRP3:	mov ah, BYTE PTR LINLEN		# AH=POSN going to
CHWRNO:	mov dh, ah
	sub dh, al			# DH=POSN-(char len)
	stc				# Error return

# EXIT
CHWRPZ:	pushfd
	xchg ax, cx			# CH=POSN, AX=current terminators
	cmp ah, ch
	mov ah, ch
	ja CHWRP9			# BRIF Current terminator beyond posn for output
	push ax
	call SCNRDT
	pop ax
	call SCNWTT			# Set current terminator to new max posn
CHWRP9:	mov BYTE PTR CSRY, dl
	cmp dl, BYTE PTR LSTLIN
	jnz CHWNTL			# BRIF not on last logical line
	cmp ah, BYTE PTR LSTCOL
	jb CHWNTL			# BRIF not new last position
	mov BYTE PTR LSTCOL, ah		# Set new last posn
CHWNTL:	cmp dl, BYTE PTR FSTLIN
	jnz CHWNTF			# BRIF not on first logical line
	cmp dh, BYTE PTR FSTCOL
	jae CHWNTF			# BRIF not new first position
	mov BYTE PTR FSTCOL, dh
CHWNTF:	inc ah
	mov BYTE PTR CSRX, ah		# Set new posn(one past last posn printed at)
	cmp BYTE PTR LINLEN, ah
	jae CHWNTG			# BRIF does not go beyond end of physical line
	call SCNRDT
	jb CHWNTG			# BRIF line does not continue
	inc BYTE PTR CSRY		# Logical line continues, put cursor at start of next physical
	mov ah, BYTE PTR WDOLFT
	mov BYTE PTR CSRX, ah
CHWNTG:	popfd
	pop ax
	ret

# Wrap linefeed on end of line to next line
WRAPLF:	push DWORD PTR TRMCUR
	push cx
	push dx
	mov dh, BYTE PTR LINLEN
	mov ch, 1
	call MKRMCH			# Insert one space before LF
	pop cx
	mov cl, dl
	pop dx
	xchg cx, dx
	pop DWORD PTR TRMCUR
	ret

# ---------------------------------------------------------------- open next line
# MKRMNL(line): make the next line a blank, empty line. Entry DL=current line.
# Exit DL=current line (may change). CF=1 if no room available.
MKRMNL:	push ax
	push ebx
	push cx
	push dx
MKRMN0:	call SCNRDT
	jb MKRMN5
	cmp dl, BYTE PTR WDOBOT
	jz MKRMN6
	jb MKRMN4
	mov dl, BYTE PTR WDOBOT
	jmp MKRMN0
MKRMN4:	inc dl
	jmp MKRMN0

# IF CURRENT_PHYSICAL .LT. WINDOW_BOTTOM
MKRMN5:	cmp dl, BYTE PTR WDOBOT
MKRMN6:	pop dx
	push dx				# Get back posn to scroll from
	jz MKRMN7			# BRIF at bottom of window(scroll up)
	cmc
	jb MKRMNZ			# BRIF outside of window
	cmp dl, BYTE PTR WDOTOP
	jb MKRMNZ			# BRIF outside of window
	test BYTE PTR F_EDIT, 0377
	jz MKRMNZ			# BRIF not editing, no need to scroll
	inc dl				# Scroll down starting at next line
# SCROLL DOWN
	call SCRLD0			# Scroll down to end of window
MKRMNY:	clc				# Indicate successful
MKRMNZ:	pop dx				# Restore current line number
	jmp MKRMNX
# ELSE IF (FIRST_PHYSICAL(LOGICAL) .NE. WINDOW_TOP) OR NOT(INPUT_EDIT)
MKRMN7:	test BYTE PTR F_EDIT, 0377
	jz MKRMN9			# BRIF not input edit, always allow scroll
MKRMN8:	call CSRUP
	jb MKRMNZ			# BRIF logical line starts at top of window(abort)
	push ax
	call SCNRDT
	pop ax
	jnb MKRMN8			# BRIF previous physical part of this logical
# SCROLL UP
MKRMN9:	pop dx
	push dx
	cmp dl, BYTE PTR WDOBOT		# Full screen scroll?
	jae MKRMNF			# BRIF scroll up entire screen
	call SCRLUD			# Scroll up from DL to WDOTOP
	jmp MKRMNU
MKRMNF:	call SCRLUP			# Scroll up entire window
MKRMNU:	pop dx
	call CSRUP			# Return line number of original line
	clc
MKRMNX:	pop cx
	pop ebx
	pop ax
	ret

# ---------------------------------------------------------------- scroll up and down
# Scroll up from DL to WDOTOP
SCRLUD:	push cx
	mov cl, dl
	mov dl, BYTE PTR WDOTOP
	jmp SCRLU1
# SCROLL UP
SCRLUP:	mov dl, BYTE PTR WDOTOP		# Top line number
# Scroll up from DL to bottom
SCRLU0:	push cx
	mov cl, BYTE PTR WDOBOT
SCRLU1:	push ax
	push ebx
	push dx
	mov al, dl
	mov bl, dl			# From = To = Top line number
	inc al				# From = To + 1
	mov bh, BYTE PTR WDOLFT
	mov ah, bh			# From column = To column = left margin
	sub cl, bl			# CL = line count
	ja SCRUL1			# BRIF two or more line scroll
	jb SCRUL4			# BRIF TOP > BOTTOM(should never happen)
	push ax
	mov dl, bl
	jmp SCRUL3			# TOP=BOTTOM, just init line
SCRUL1:	push ax
	mov ch, BYTE PTR CRTWID		# Scroll entire lines
	call SCROLL			# Scroll screen
# If CSRY, FSTLIN, LSTLIN are within scroll, decrement their values
	add al, cl			# AL=Last line scrolled + 1
	dec al				# AL=Bottom line scrolled, BL=Top line scrolled
	mov ah, 255			# decrement line# variables in scroll window
	call TSTSCR
	mov dl, al			# DL=bottom line of scroll (initialize it)
# Clear the last line of the scroll
SCRUL3:	mov dh, BYTE PTR WDOLFT
	pop ax
	or cl, cl
	je SCRUL4			# No scroll(one line init)
	call SCRUTT			# Scroll up terminator table
SCRUL4:	call PLINIT			# Init physical line from (DL,DH) to (DL,WDORGT)
	pop dx
	pop ebx
	pop ax
	pop cx
	ret

# TSTSCR: called after scrolling up or down to update line-number variables in the window.
# Entry: BL=top line of scroll window, AL=bottom line, AH=1 increment, -1 decrement.
TSTSCR:	push esi
	push ax
	mov esi, offset CSRY
	call TSTWDO			# Adjust CSRY if its within scroll window
	mov ah, BYTE PTR WDOTOP
	cmp BYTE PTR [esi], ah
	jb TCSRY1			# BRIF CSRY above top of window
	mov ah, BYTE PTR WDOBOT
	cmp BYTE PTR [esi], ah
	jna CSRYOK			# BRIF CSRY didn't increment beyond bottom
TCSRY1:	mov BYTE PTR [esi], ah		# Bring it back to within scroll window
CSRYOK:	mov esi, offset FSTLIN
	pop ax
	call TSTWDO			# Adjust FSTLIN if its within scroll window
	mov esi, offset LSTLIN
	call TSTWDO			# Adjust LSTLIN if its within scroll window
	pop esi
	ret

# If BL .LEQ. [SI] .LEQ. AL then [SI]=[SI]+AH
TSTWDO:	cmp al, BYTE PTR [esi]
	jb NINWDO			# BRIF [SI] not within scroll
	cmp bl, BYTE PTR [esi]
	ja NINWDO			# BRIF [SI] not within scroll
	add BYTE PTR [esi], ah		# Adjust [SI] for scroll
NINWDO:	ret

# SCROLL DOWN
SCRLDN:	mov dl, BYTE PTR WDOTOP
# Scroll down from DL to WDOBOT
SCRLD0:	push ax
	push ebx
	push cx
	mov al, dl
	mov bl, dl			# From = To = top of scroll
	inc bl				# To = From + 1
	mov bh, BYTE PTR WDOLFT		# Column = left margin
	mov ah, bh
	mov cl, BYTE PTR WDOBOT
	sub cl, al			# Count of lines =(window bottom)-top of scroll
	push ax
	jbe SCRDL3			# BRIF null scroll, just clear line
	mov ch, BYTE PTR WDORGT		# Scroll lines, not columns
SCRLD1:	call SCROLL			# Scroll screen
# If CSRY, FSTLIN, LSTLIN are within scroll, increment their values
	add al, cl			# AL=Last line scrolled
	dec al
	mov ah, 1			# increment line# variables in scroll window
	call TSTSCR
# Clear the last line of the scroll (in DL)
SCRDL3:	mov dh, BYTE PTR WDOLFT
	pop ax
	or cl, cl
	js SCRDL4			# No scroll(out of window)
	je SCRDL4			# No scroll(one line init)
	call SCRDTT			# Scroll down term table(mark last line init'd)
SCRDL4:	call PLINIT			# Init physical line from (DL,DH) to (DL,WDORGT)
	pop cx
	pop ebx
	pop ax
	ret

# SCROLL DOWN current line to next line
SCRDLN:	push ax
	push ebx
	push cx
	mov ax, dx			# Start of scroll is current line
	mov ah, BYTE PTR WDOLFT
	mov bx, ax
	inc bl				# TO start is next line
	mov ch, BYTE PTR WDORGT
	mov cl, 1			# Scroll one entire line
	push ax
	jmp SCRLD1			# Do scroll and update LSTPOS, FSTPOS, CSRY

# ---------------------------------------------------------------- terminator table scroll
# Scroll up terminator table (same parameters as SCROLL). Modifies AX, BX, CX.
SCRUTT:	push dx
	mov dl, bl
	call SCNRDT			# TRMCUR=address of top of scroll
	mov ebx, DWORD PTR TRMCUR
	movzx ecx, cl			# loop count (CH=0)
SCRUTL:	mov ax, WORD PTR [ebx+LINTTB]	# Get a terminator
	mov WORD PTR [ebx+LINTTB-2], ax	# Save a terminator
	inc ebx
	inc ebx
	loop SCRUTL			# Continue till done
	mov BYTE PTR [ebx+LINTTB-2], 0	# Terminate end line of scroll
	mov BYTE PTR [ebx+LINTTB-1], TRMEOL
	pop dx
	ret

# Scroll down terminator table (same parameters as SCROLL). Modifies AX, BX, CX.
SCRDTT:	push dx
	mov dl, al
	add dl, cl
	call SCNRDT			# TRMCUR=displacement of bottom of scroll + 2
	mov ebx, DWORD PTR TRMCUR
	movzx ecx, cl			# loop count (CH=0)
SCRDTL:	sub ebx, 2			# Next posn
	mov ax, WORD PTR [ebx+LINTTB-2]	# Get a terminator
	mov WORD PTR [ebx+LINTTB], ax	# Save a terminator
	loop SCRDTL			# Continue till done
	mov BYTE PTR [ebx+LINTTB-2], 0	# Terminate end line of scroll
	mov BYTE PTR [ebx+LINTTB-1], TRMEOL
	mov dl, BYTE PTR WDOBOT
	call SCNRTL
	jb SCRDT1			# EOL term'd, all done
	jnz SCRDT1
	dec ah
SCRDT1:	mov al, TRMEOL
	call SCNWTT			# Make sure last line in window is EOL term'd
	pop dx
	ret

# Physical line initialization. Entry: (DH,DL) = start of initialization.
PLINIT:	push cx
	push ax
	call SCNRDT
	mov ah, dh
	dec ah
	push ebx
	mov ebx, DWORD PTR TRMCUR	# Terminate line to left of DH
	mov BYTE PTR [ebx+LINTTB-1], TRMEOL
	mov BYTE PTR [ebx+LINTTB-2], ah
	pop ebx
	pop ax
PLINI2:	call CLREOL			# Clear to end of line from (DH,DL)
	pop cx
	ret

# ---------------------------------------------------------------- label keys
# LABELK: if function key display is off then turn on and exit. Else advance display line.
# Entry DX = cursor position (unmodified).
LABELK:	push ax
	push ebx
	push cx
	push dx
	push esi
	push edi
	cmp BYTE PTR KEYSW, 0		# Test for key on
	call FKYADV			# If KEYSW was 0 (ZF=0) then init PF start key, else advance
	jz LABOFF			# Process key off request
	mov BYTE PTR KEYSW, 255		# Turn on key display flag
LABDSP:	call KEYDSP			# Display function keys
	pop edi
	pop esi
	pop dx
	pop cx
	pop ebx
	pop ax
	ret
LABOFF:	mov BYTE PTR KEYSW, 0		# Turn the key flag off
	jmp LABDSP			# Go clear the function key display line

# ---------------------------------------------------------------- editor interface
# SCNMRK: mark current position as first posn of logical line.
# Entry: ZF=1 indicates INPUT statement; AL=1 indicates EDIT statement.
SCNMRK:	pushfd
	mov dl, BYTE PTR CSRY
	mov dh, BYTE PTR CSRX
	dec al
	jnz SCNMK0			# Not begin of EDIT statement
	call LSTART			# Set (DH,DL)=start of current line
	push ebx
	jmp SCNMK2
# IF NOT TOP OF WINDOW
SCNMK0:	call CSRUP			# Move to previous line
	jb SCNMK1			# BRIF at top of window
# TERMINATE PREVIOUS PHYSICAL LINE
	call SCNRTL
	mov al, TRMEOL			# Make sure current line is start of logical line
	call SCNWTT
	inc dl				# Restore current line number
# IF LINE IS MARKED WITH NULL
SCNMK1:	call SCNRDT
	push ebx
	mov ebx, DWORD PTR TRMCUR
	test BYTE PTR [ebx+LINTTB-1], TRMNUL
	jz SCNMK2			# BRIF not specially marked line
	push dx
	push cx
# SCAN LINE FOR NULL CHAR(255 DECIMAL)
	mov dh, BYTE PTR WDOLFT
	mov cl, BYTE PTR LINLEN
SCNMKL:	call SCRINP
	cmp al, 255
	jz SCNML3			# BRIF is specially marked line
SCNML2:	inc dh
	sub cl, 1
	ja SCNMKL			# BRIF more chars to check
	or esp, esp
SCNML3:	pop cx
	pop dx
	jnz SCNMK2			# BRIF char was not on line, don't clear line
# DELETE IT
	call LDELET			# Delete this line
# IF NOT DIRECT, SET LSTPOS, FSTPOS = CURRENT_POSN
SCNMK2:	mov BYTE PTR CSRY, dl
	mov BYTE PTR CSRX, dh		# Update CURS_POSN in case of change
	pop ebx
	popfd
	jz SCNMK3			# INPUT statement, use current posn
	mov BYTE PTR LSTLIN, 0377	# Direct, set FSTPOS, LSTPOS to min and max
	mov BYTE PTR FSTLIN, 0
	jmp SCNMKX
SCNMK3:	mov BYTE PTR LSTLIN, dl
	mov BYTE PTR FSTLIN, dl
	dec dh
	mov BYTE PTR LSTCOL, dh		# Mark LSTCOL one to left of current posn
	inc dh
	mov BYTE PTR FSTCOL, dh
SCNMKX:	ret

# SCNRDL: read a logical line. Entry: ZF=1 ignore FSTPOS, LSTPOS (statement line input).
# DX=current posn, EBX=buffer address, CX=max count. Exit: EBX=last char address plus one,
# CX=CX - (count of chars moved), DX destroyed.
SCNRDL:	pushfd				# Save statement input flag
# Set DX=start of move. WHILE ((DL .NE FSTLIN) AND DL IN_SAME_LOGICAL_LINE)
SCRD00:	cmp dl, BYTE PTR FSTLIN
	jz SCRD01			# Found first line of logical
# DL = PREVIOUS LINE
	call CSRUP
	jb SCRD01			# At top of window, stop here
	call SCNRDT
	jnb SCRD00			# Still same logical, continue
	inc dl				# Set DL = first physical of logical
# IF (STATEMENT_INPUT OR (NOT ON FIRST_LINE))
SCRD01:	popfd				# We have start line, now get start column
	pushfd
	mov dh, BYTE PTR WDOLFT		# Assume start at left margin
	jnz SCRD02			# BRIF statement input
	cmp dl, BYTE PTR FSTLIN
	jnz SCRD02			# BRIF not on FIRST_LINE
	mov dh, BYTE PTR FSTCOL		# ELSE START_COLUMN=FIRST_COLUMN
# WHILE (POSN IS ON SAME LOGICAL) AND ((POSN .LTE. LSTPOS) OR STATEMENT_INPUT)
SCRD02:	popfd
	pushfd
	jnz SCRD03			# BRIF statement input, ignore LSTPOS
	cmp dl, BYTE PTR LSTLIN
	ja SCRDXZ			# BRIF passed LSTPOS (was at prev. line term'r)
	jnz SCRD03			# BRIF not at LSTPOS
	cmp dh, BYTE PTR LSTCOL
	ja SCRDXZ			# BRIF beyond LSTPOS, all done
# READ A CHARACTER INTO THE BUFFER
SCRD03:	call SCNRDT
	push ax
	jb SCRD04			# BRIF not reading a linefeed
	jnz SCRD04			# BRIF not reading a linefeed
	cmp ah, dh
	jnz SCRD04			# BRIF not reading a linefeed
# CASE: LINEFEED
SCRDLF:	mov ax, CHRLNF
	jmp SCRD08			# At linefeed terminator, pass linefeed
SCRD04:	cmp ah, dh
	ja SCRD07			# BRIF within data on screen
	jb SCRD06			# BRIF beyond terminator
# CASE: NULL_WRAP
	cmp al, TRMNWP
	jnz SCRD07			# BRIF not at NULL_WRAP terminator
SCRD06:	cmp al, TRMEOL
	pop ax
	jz SCRDXZ			# BRIF beyond EOL
# CASE: WRAP
	inc dl				# Wrap to next line
	mov dh, BYTE PTR WDOLFT
	jmp SCRD02
# CASE: BEFORE EOL
SCRD07:	clc				# Indicate call is from Screen Editor
	call SCRINP			# AX=Character at (DH,DL)
SCRD08:	mov BYTE PTR [ebx], al
	dec cx
	pop ax
	jz SCRDEX
	cmp al, TRMEOL
	jnz SCRD09			# BRIF not at EOL
	cmp ah, dh
	jbe SCRDEX			# BRIF at EOL (or beyond), all done
SCRD09:	inc ebx
	call CSRADV
	jb SCRDXZ			# If end of window, all done
	jmp SCRD02			# Pass next character
# CASE: AT (OR BEYOND) EOL
SCRDEX:	inc ebx				# Set BX= last posn written plus one
SCRDXZ:	popfd
	call SCNBRK			# Clear any flags associated with INPUT
	ret

# ---------------------------------------------------------------- control characters
# CTLDSP(CONTROL_CHAR): do control char logic. Entry AX=character.
# Exit: CF=0 then AX=character to output; CF=1 then AX undefined.
.set CTLCNT, 13				# count of CTLITB entries (see the table)
CTLDSP:	call SCNPOS			# DH=column, DL=row
	mov BYTE PTR CSRTYP, 0		# Set for off cursor
	call SETCSR			# Ensure that the cursor is off
	mov dh, BYTE PTR CSRX		# Set DH=true posn (not width truncated)
	test BYTE PTR ESCFLG, 0377
	jnz DOESC			# BRIF in middle of escape sequence
	cmp al, 33
	jz DOESC
CTLDS0:
CTLDP0:
CTLDP1:
	test BYTE PTR F_EDIT, 0377	# Test for Screen Edit mode
	jz CTLPRT			# BRIF not edit mode
	cmp ah, 1			# Set PSW.C if two byte character
	cmc
	call EDTMAP			# Map edit function/output character code
	jmp CTLEDT

CTLPRT:	cmp ah, 1			# Set PSW.C if two byte character
	cmc
	call PRTMAP			# Map print function/output character codes
CTLEDT:	jz CTLDPY			# Ignore this character
					# Print or perform the editor function
	cmp ah, 255			# Is it an editor function (&HFF)?
	jnz CTLDSX			# No - print the character
	xor ah, ah			# Clear (no longer needed) editor function flag
	cmp al, 127			# Delete function?
	jnz CTLNDL			# BRIF not DEL
	mov al, 32			# DEL code
CTLNDL:	cmp al, 255
	jnz CTLNMK			# BRIF not "mark line for deletion"
	push ax
	push ebx
	call SCNRDT
	mov ebx, DWORD PTR TRMCUR
	or BYTE PTR [ebx+LINTTB-1], TRMNUL	# Set "not for input" flag on this line
	pop ebx
	pop ax
	ret
CTLNMK:	cmp al, 33			# Test for legal function code
	jnb CTLDPY			# Ignore this code
	push ax				# saved for CTLDPX
	call CTLIRS			# Possibly reset insert flag
	add ax, ax
	movzx ebx, al			# index into FUNTAB (4-byte entries)
	shl ebx, 1
	mov eax, offset CTLDPX
	push eax			# Put return on stack
	mov eax, DWORD PTR [ebx+FUNTAB]
	push eax			# Put routine address on stack
CTLDSX:	clc
	ret				# Go do control routine
# UPDATE CURRENT_POSN, RETURN
CTLDPX:	mov BYTE PTR CSRY, dl
	mov BYTE PTR CSRX, dh		# Update cursor posn
	pop ax
	sub al, CHRRET
	mov BYTE PTR F_CRET, al		# F.CRET = 0 if character was Carriage return
CTLDPY:	stc
CTLIGN:
CTLDPZ:	ret
# ESCAPE SEQUENCE PROCESSING
DOESC:	call SCROUT			# Returns carry set if continuation of escape sequence
	sbb al, al			# If ESC sequence, AL=FF else AL=0
	mov BYTE PTR ESCFLG, al		# Set/Reset flag
	jmp CTLDPY

# THE CONTROL CHARACTERS WHICH RESET INSERT FLAG (CTLITB)
CTLITB:
	.byte CHRLDL			# LINE_DELETE
	.byte CHRERA			# ERASE_SCREEN
	.byte CHRFDW			# FORWARD_WORD
	.byte CHRBKW			# BACKWARD_WORD
	.byte CHRAPP			# APPEND_TO_LINE
	.byte CHREDT			# EDIT_PREVIOUS_LINE
	.byte CHRCLE			# CLEAR_TO_END
	.byte CHRBEL			# BELL
	.byte CHRHOM			# HOME
	.byte CHRADV			# CURSOR_ADV
	.byte CHRREG			# CURSOR_REG
	.byte CHRUP			# CURSOR_UP
	.byte CHRDWN			# CURSOR_DOWN

# CTLIRS: reset insert flag for specified characters. Entry AL=character. DI garbaged.
CTLIRS:	push ax
	push ebx
	push ecx
	push edi
	# IF (SPECIAL_ACTION(CTLKEY))
	mov edi, offset CTLITB		# DI= [control char table]
	mov ecx, CTLCNT			# CX= count of entries in table
	mov ebx, ecx
	cld				# Scan forward
	repne scasb			# Scan for character in table
	jne INCNOT			# BRIF did not find the control character
	# F.INSRT=FALSE
	mov BYTE PTR F_INST, 0		# Reset insert flag
INCNOT:	pop edi
	pop ecx
	pop ebx
	pop ax
	ret

# CONTROL CHARACTER DISPATCH TABLE (33 entries: ^@ .. ^_ then DEL). Entries are 4 bytes.
FUNTAB:	.long CTLIGN			# ^@  -  Ignore
	.long CTLIGN			# ^A  -  Ignore
	.long BCKWRD			# ^B  -  Back one word
	.long CTLIGN			# ^C  -  Ignore
	.long CTLIGN			# ^D  -  Ignore
	.long LTRUNC			# ^E  -  Truncate logical line
	.long FWDWRD			# ^F  -  Forward one word
	.long CBEEP			# ^G  -  Beep
	.long BAKSPC			# ^H  -  Destructive backspace
	.long LTAB			# ^I  -  Destructive tab
	.long LFEED			# ^J  -  Linefeed
	.long WHOME			# ^K  -  Home within window
	.long CCLRSN			# ^L  -  Clear window, home cursor
	.long LCARET			# ^M  -  Carriage return
	.long LAPPND			# ^N  -  Append to end of line
	.long CTLIGN			# ^O  -  Ignore
	.long CTLIGN			# ^P  -  Ignore
	.long CTLIGN			# ^Q  -  Ignore
	.long LINSRT			# ^R  -  Insert a blank
	.long CTLIGN			# ^S  -  Ignore
	.long LABELK			# ^T  -  LABEL Key
	.long LDELET			# ^U  -  Delete a line
	.long CTLIGN			# ^V  -  Ignore
	.long WDELET			# ^W  -  Delete a word
	.long CTLIGN			# ^X  -  Ignore
	.long CTLIGN			# ^Y  -  Ignore
	.long WERASE			# ^Z  -  Erase to end of window
	.long CTLIGN			# ^[ or ESC - Ignore
	.long WCSADV			# ^\  -  Cursor advance within window
	.long WCSREG			# ^]  -  Cursor regress within window
	.long WCSUP			# ^^  -  Cursor up within window
	.long WCSDWN			# ^_  -  Cursor down within window
	.long CDELET			# DEL -  Delete a character

# ---------------------------------------------------------------- cursor movement
# Cursor advance (word/character). Routines return CF=1 when they do not change the posn.
WCSADV:	call CSRADV			# Advance cursor
	jb WCSABT			# BRIF can't do, quit
# UPDATE LSTPOS / FSTPOS for the cursor's current posn (falls through from WCSADV)
UPDPOS:	push dx
	dec dh				# LSTCOL is to left of cursor
	cmp dl, BYTE PTR LSTLIN
	jb UPDPS3			# BRIF not new LSTPOS
	jz UPDPS2			# BRIF on same line, possibly update LSTPOS
	push dx
	call LSTART			# Get start line of current logical
	cmp dl, BYTE PTR LSTLIN
	pop dx
	ja UPDPS3			# BRIF LSTPOS not on current logical line
	# LSTPOS on current logical, previous physical
	mov BYTE PTR LSTLIN, dl		# Set LSTPOS to current
	mov BYTE PTR LSTCOL, dh
UPDPS2:	cmp dh, BYTE PTR LSTCOL
	jbe UPDPS3			# BRIF not new last posn
	mov BYTE PTR LSTCOL, dh		# Set new last posn
# If cursor before previous FSTPOS and on same logical, update FSTPOS
UPDPS3:	inc dh				# Restore column
	cmp dl, BYTE PTR FSTLIN
	jnz UPDPSX			# BRIF not on same line as first position
	cmp dh, BYTE PTR FSTCOL
	jae UPDPSX			# BRIF not new first position
	mov BYTE PTR FSTCOL, dh		# Set new first position to current position
UPDPSX:	pop dx
WCSABT:	ret

# Cursor regress
WCSREG:	call CSRREG			# Move cursor
	jb WCSABT			# Can't move, quit
WCSRET:	call SCNRDT
	cmp ah, dh
	jnb UPDPOS			# Within logical, update FSTPOS, LSTPOS, return
	# Cursor to right of line terminator, don't update LSTCOL
	jmp WCSABT

# Cursor up
WCSUP:	call CSRUP			# Move cursor
	jnb WCSRET			# BRIF successful, possibly update FSTPOS, LSTPOS
	ret				# Unsuccessful, return (NOP)

# Cursor down
WCSDWN:	call CSRDWN			# Move cursor
	jnb WCSRET			# BRIF successful, possibly update FSTPOS, LSTPOS
	ret				# Unsuccessful, return (NOP)

# TAB: move cursor to next TAB stop. Forward moves over blank cells are sent to the terminal
# as spaces, so the terminal cursor keeps pace with the logical cursor.
LTAB:	test BYTE PTR F_INST, 0377
	jz LTABL			# BRIF not insert TAB
# Insert spaces until CSRX is at TAB stop or LINLEN
LTABIL:	mov ax, ' '
	call SCNOUT			# Insert a blank
	mov dl, BYTE PTR CSRY
	mov dh, BYTE PTR CSRX
	cmp dh, BYTE PTR LINLEN
	ja LTABZ			# All done, at end of line
	dec dh
	test dh, 7
	jnz LTABIL			# Keep inserting
	inc dh
	jmp LTABZ			# All done, at TAB stop
# Non-insert TAB, just pass over characters
LTABL:	cmp dh, BYTE PTR LINLEN
	jae LTABX0			# BRIF need to wrap
	inc dh
	mov al, 32
	call PLAT_CHOUT			# terminal cursor moves over one blank cell
	test dh, 7
	jnz LTABL
	inc dh
	mov al, 32
	call PLAT_CHOUT
	jmp LTABX			# BRIF reached good posn
LTABX0:	cmp dl, BYTE PTR WDOBOT
	jae LTABZ			# BRIF no room to end of page
	inc dl
	mov dh, BYTE PTR WDOLFT		# Else move to start of next line
LTABX:	call UPDPOS			# Update LSTPOS
LTABZ:	ret

# Advance cursor to beginning of next word within screen
FWDWRD:	push dx
	push WORD PTR LSTLIN
	cmp dl, BYTE PTR LSTLIN
	jnz FWDWR0			# BRIF LSTPOS not on this line
	cmp dh, BYTE PTR LSTCOL
	jb FWDWR0			# BRIF within logical line
	mov BYTE PTR LSTLIN, 0377	# Ignore LSTPOS in scan
FWDWR0:	clc
FWDWR1:	call FWDWDL			# Try within logical
	jb FWDWR2			# BRIF unsuccessful
	pop WORD PTR LSTLIN
	call UPDPOS
	pop ax				# Remove old posn from stack
	ret
# Reached end of logical line, try next logical
FWDWR2:	call CSRDWN
	jb FWDWRZ			# BRIF end of screen
	mov dh, BYTE PTR WDOLFT		# Start scan again at next line
	stc
	jmp FWDWR1			# Continue scan
# Unsuccessful, return cursor to original posn
FWDWRZ:	pop WORD PTR LSTLIN
	pop dx
	ret

FWDWD0:	call LCSADV
	jb FWDWD3			# At end of logical line
# Advance cursor to beginning of next word within logical line
FWDWDL:	jb FWDWD2			# Start as though between words
	call ANCHK			# Check if current char is within word
	jnb FWDWD0			# BRIF still within previous word
# At space between words
FWDWD1:	call LCSADV
	jb FWDWD3			# At end of logical line, check next logical
FWDWD2:	call ANCHK
	jb FWDWD1
FWDWDX:	ret				# Return, DX=posn of new word or end of logical line
FWDWD3:	jnz FWDWDX			# BRIF not stop at LSTPOS
	# Stopped at LSTPOS, continue scan to end of physical line
	cmp ah, dh
	ja FWDWD4			# BRIF more this line
	stc
	ret
FWDWD4:	mov BYTE PTR LSTLIN, 0377	# Ignore LSTPOS from here on
	jmp FWDWD1			# Since crossed LSTPOS, passed end of current word

# Regress cursor to start of previous word
BCKWRD:	push WORD PTR FSTCOL
	push dx
	cmp dh, BYTE PTR FSTLIN
	jnz BCKWD2
	cmp dh, BYTE PTR FSTCOL
	ja BCKWD2
	mov BYTE PTR FSTLIN, 0
# Assume passed beginning of current word, continue to start of next
BCKWD2:	call LCSREG
	jb BCKWD7			# BRIF At start of line
BCKWD4:	call ANCHK
	jb BCKWD2			# BRIF still between words
# Scanning previous word, any termination is good
BCKWD5:	call LCSREG
	jb BCKWD6			# BRIF at beginning of line, found previous word
	call ANCHK
	jnb BCKWD5			# BRIF not at beginning of word
	call CSRADV			# Advance back to start of word
BCKWD6:	pop ax
	push dx				# Push new posn on stack
BCKWDX:	pop dx
	pop WORD PTR FSTCOL
	call UPDPOS
	ret				# Return, DL=posn of previous word
BCKWD7:	jnz BCKWD8			# BRIF not stop at FSTPOS
	mov BYTE PTR FSTLIN, 0		# Ignore FSTPOS from here on
	jmp BCKWD2			# Since passed beginning of logical, between words
BCKWD8:	call CSRUP			# Try previous logical line
	jb BCKWDX			# At beginning of screen
	call SCNRDT			# Read previous line terminator
	or ah, ah
	jz BCKWD8			# Previous line is null, try next previous
	mov dh, ah
	jmp BCKWD4			# Continue scan

# ---------------------------------------------------------------- bell, clear, home, erase
CBEEP:	call BEEP			# Sound Bell
	jmp SCNPOS			# return current cursor location

# Clear the screen (CLRSCN is the OEM clear), then home the cursor
CCLRSN:	clc				# no-carry = internal parameter
	mov al, 2			# specify text only
	call CLRSCN			# OEM supplied Clear-Screen Routine
WHOME:	mov dl, BYTE PTR WDOTOP		# Home the cursor
	mov dh, BYTE PTR WDOLFT
	ret

# Erase to the end of the screen
WERASE:	call PLINIT			# Clear current physical to end of line
	push dx
	mov dh, BYTE PTR WDOLFT		# Clear rest of lines from left margin
# WHILE POSN .LTE. WDOBOT: IF DATA_EXISTS(POSN) THEN INITIALIZE LINE
WERAS0:	inc dl
	cmp dl, BYTE PTR WDOBOT
	ja WERASX			# Beyond window, exit
	call PLINIT			# Clear this line
	jmp WERAS0			# Init next line
WERASX:	pop dx				# Return DX=cursor posn
	ret

# ---------------------------------------------------------------- line truncate and delete
# Delete entire logical line
LDELET:	call LSTART			# Get start of logical line in DX

# Truncate line from posn in DX
LTRUNC:	push dx
	call SCNRTL			# AH=last valid data on physical line
	pushfd
	stc				# Indicate don't clear line by scrolling
	call LCLEAR			# Clear this physical line
	jb LTRUN9			# Reached LSTPOS, all done
	inc dl
# "Suck up" remaining physical lines attached to current logical
LTRUN1:	popfd
	jb LTRUNX			# Reached end of logical line
	call SCNRTL
	pushfd
	mov dh, BYTE PTR WDOLFT
	clc
	call LCLEAR			# Clear this line from start of line
	jb LTRUN9			# Reached LSTPOS, all done
	jmp LTRUN1
LTRUN9:	popfd
LTRUNX:	pop dx
	ret

# Clear current physical to end of physical or LSTPOS, whichever is smallest.
# (DH,DL)=start posn, AH=TERM_POSN, if CF=1 don't scroll
LCLEAR:	pushfd
	cmp dl, BYTE PTR LSTLIN
	jz LCLER2			# BRIF clear up to LSTPOS
LCLER0:	cmp dh, BYTE PTR WDORGT
	ja LCLER5			# BRIF beyond end of physical (just exit)
	popfd
	jnb LCLER1			# BRIF clear line by scroll
	call PLINIT			# Clear this physical line
	clc
	ret
LCLER1:	call SCRLU0			# Scroll up from bottom to current line
	clc
	ret
LCLER2:	popfd
	call PLINIT
	stc
	ret
LCLER5:	popfd
	clc
	ret

# ---------------------------------------------------------------- character and word delete
# Word delete
WDELET:
	cmp dh, BYTE PTR LINLEN
	jbe WDELT0			# BRIF on physical line
	call CSRADV			# Put cursor on next line
	jb WDELTZ			# BRIF at end of screen
WDELT0:
	cmp dl, BYTE PTR LSTLIN		# **
	jnz WDELT2			# **BRIF not on LSTPOS line
	cmp dh, BYTE PTR LSTCOL		# **
	jae WDELTZ			# **BRIF at or beyond LSTPOS (don't do word delete)
WDELT2:					# **
	push dx
	cmp dh, BYTE PTR LINLEN
	jbe WDELT4			# Good delete posn (within physical line)
	call LCSADV			# Advance cursor to next physical of logical
WDELT4:	push dx
	clc
	push WORD PTR LSTLIN
	call FWDWDL			# Set (DH,DL)= start of next word or end of line
	pop WORD PTR LSTLIN
	jnb WDELT6			# BRIF found another word following
	# No word following, just delete to end of line
	inc dh				# Include terminator in delete
WDELT6:	mov cx, dx
	pop dx
	cmp cl, BYTE PTR LSTLIN		# **
	jb WDELT8			# **BRIF not beyond LSTPOS line
	ja WDELT7			# **BRIF beyond LSTPOS line, use LSTPOS
	cmp ch, BYTE PTR LSTCOL		# **At LSTPOS line, check if column is OK
	jb WDELT8			# **BRIF before LSTPOS, column is OK
WDELT7:	mov cx, WORD PTR LSTLIN		# **Use LSTPOS as end of delete
	inc ch				# **INC because delete is up to char before this posn
WDELT8:					# **
	cmp dl, cl
	jnz WDELTC			# BRIF multiple line delete
WDELT9:	sub ch, dh			# CH=chars to next word minus one
	jbe WDELTX			# BRIF no chars to delete
	call CDELLG			# Delete CH chars in this line
WDELTX:	pop dx
WDELTZ:	ret
WDELTC:	inc dl
WDELTD:	push dx
	push cx
	# Delete across physical lines, delete to end of this line
	cmp dl, cl
	mov dh, BYTE PTR WDOLFT
	jz WDELTE
	call SCNRTL
	mov ch, ah
	call CDELPH			# Erase intervening line
	pop cx
	pop dx
	dec cl				# Each erase of intervening line brings CL closer
	jmp WDELTD
WDELTE:	cmp ch, BYTE PTR WDOLFT
	jbe WDELTF			# BRIF at begin of line
	dec ch
	call CDELLG			# Delete up to the next word on this line
WDELTF:	pop cx
	pop dx
	dec dl				# Go back to original line
	mov ch, BYTE PTR LINLEN
	inc ch
	jmp WDELT9			# Delete to end of line

# Destructive back space
BAKSPC:
	call LCSREG			# Back space within logical line
	call SCNRTL
	cmp dh, ah
	jbe CDELET			# BRIF current posn less than TERM_POSN
	cmp ah, BYTE PTR WDOLFT
	jae BAKSP1			# BRIF not on null line
	mov ah, BYTE PTR WDOLFT
BAKSP1:	mov dh, ah			# If beyond terminator, set posn to terminator

# Delete character
CDELET:	mov ch, 1			# Assume one byte char delete
	cmp dh, BYTE PTR LINLEN
	jbe CDELT1			# BRIF within physical line
	ret
CDELT1:

# CDELLG: delete CH characters @(DH,DL) within logical line.
# On entry: CH=count, CL=flag, (DH,DL)=posn. If CL is non-zero, remove null line from screen.
CDELLG:	push dx
	call SCNRTL			# Get last valid char posn in AH
	pushfd
	call CDELPH			# Delete characters from current physical, returns term posn in AH
	xor cl, cl			# CL is iteration counter
	popfd
	mov al, TRMEOL
	jb CDLGEX			# BRIF was EOL term'd, update terminator, exit
	jnz CDLGWP			# BRIF line was WRAP or NULL_WRAP term'd, get chars from next line
	# Line was LF terminated, check for LF delete
	cmp ah, dh
	mov al, TRMLNF
	ja CDLGEX			# BRIF didn't delete LF, update terminator, exit
	or ah, ah
	jz CDLGRT			# **BRIF entire line deleted and removed
	mov ah, dh
	jmp CDLWP1
# Need to get chars from next physical line at (AH,DL) posn to end of physical
CDLGWP:	or ah, ah
	jz CDLGRT			# BRIF entire line deleted and removed
	inc ah
CDLWP1:	cmp dl, BYTE PTR LSTLIN		# **
	jz CDLGRT			# **LSTPOS on this line, all done
	push WORD PTR LSTLIN		# **Save current LSTPOS
	call CDUNWP			# Unwrap the characters desired, returns terminator in (AH,AL), count in CH
	call SCNWTT			# Update terminator
	or ch, ch
	jnz CDLWP2			# BRIF we have characters to unwrap
	pop ax				# Restore the stack
	jmp CDLGRT			# Return, all done
# Need to delete CH characters unwrapped from next line
CDLWP2:	mov dh, BYTE PTR WDOLFT		# Unwrapped from start of line
	inc dl				# Next line
	dec cl				# Update iteration counter
	call SCNRTL			# Read next lines terminator
	# Need to restore LSTPOS to old value for character delete
	mov di, WORD PTR LSTLIN		# **Save current value of LSTPOS in DI
	pop WORD PTR LSTLIN		# **Restore value before CDUNWP so CDELPH works properly
	pushfd
	push edi				# **Save current "true" value of LSTPOS
	xchg di, ax			# **
	cmp al, BYTE PTR LSTLIN		# **
	pushfd				# **
	xchg di, ax			# **
	call CDELPH			# Delete chars to end of line or LSTPOS
	popfd				# **
	pop edi				# **
	jz CDLGNC			# **BRIF CDUNWP did not change LSTPOS
	mov WORD PTR LSTLIN, di		# **Restore CDUNWP value of LSTPOS
CDLGNC:	or ah, ah
	jnz CDLGW1			# BRIF still chars on this line
	popfd
	jnb CDLGRT			# BRIF LF or wrap term'd, already removed
CDLGNL:	dec dl
	inc cl
	call SCNRTL
	jb CDLGN0			# BRIF previous line EOL term'd, remove null line
	jz CDLGRT			# BRIF previous line LF term'd, leave null line
	mov al, TRMEOL
	call SCNWTT			# Previous line was wrapped, terminate it and remove null line
CDLGN0:	inc dl
	dec cl
	call SCRLU0			# Remove null line by scrolling
	jmp CDLGRT			# All done
CDLGW1:	popfd
	jb CDLGEX			# BRIF at EOL, all done
	jnz CDLGWP			# BRIF wrapped, unwrap chars from next line
# We have terminator in (AH,AL), posn in (DH,DL)
CDLGEX:	call SCNWTT			# Update terminator
CDLGRT:	add cl, dl
	pop dx
	mov dl, cl
	ret

CDUWXI:	mov ch, 0			# Indicate no characters unwrapped
	jmp CDUWPX			# Indirect jump

# CDUNWP: "unwrap" chars from next line. On entry: (AH,DL)=destination.
# On exit: (AH,AL)=new line terminator, CH=number of bytes unwrapped.
CDUNWP:	push ebx			# 4 bytes: popped by 'pop ebx' at CDUWPX
	push cx
	push dx
	push DWORD PTR TRMCUR
	mov al, TRMWRP
	mov ch, BYTE PTR LINLEN
	sub ch, ah
	jb CDUWXI			# BRIF no space to unwrap characters to, just exit
	cmp dl, BYTE PTR LSTLIN		# **
	jnz CDUWP0			# **BRIF not unwrapping to LSTPOS
	test BYTE PTR LSTCOL, 0377	# **
	jnz CDUWP0			# **BRIF have some characters to unwrap
	mov ah, BYTE PTR LINLEN		# **
	jmp CDUWXI			# **If no characters to unwrap, just exit
CDUWP0:	inc ch
	mov bx, ax			# Put DESTINATION in BX
	mov bl, dl
	inc dl
	call SCNRTL			# Read terminator for next line
	push ax
	pushfd
	cmp ch, ah
	jbe CDUWP1			# BRIF enough characters to satisfy demand
	mov ch, ah			# Else get all characters in next line
CDUWP1:	mov cl, 1
	mov dh, BYTE PTR WDOLFT
	mov ax, dx			# Put SOURCE in AX
	or ch, ch
	jnz CDUWPN			# BRIF not a null line following
	popfd
	pushfd
	jz CDUWP4			# BRIF LF term'd, don't remove null line
	push DWORD PTR TRMCUR
	call SCRLU0			# Get rid of null line following
	pop DWORD PTR TRMCUR
	jmp CDUWP4			# Set up terminator for return
CDUWPN:	call SCROLL			# Scroll chars from old line to new
	cmp dl, BYTE PTR LSTLIN
	jnz CDUWP4			# BRIF not on LSTPOS line
	# If LSTPOS is between WDOLFT and SOURCE+COUNT, unwrap LSTPOS
	push ax
	add ah, ch
	sub ah, BYTE PTR LSTCOL
	pop ax
	jbe CDUWP3			# BRIF LSTPOS .GT. SOURCE+COUNT
	sub BYTE PTR LSTCOL, ah		# LSTCOL=DESTINATION+(LSTPOS-SOURCE)
	jnb CDUWPL
	mov BYTE PTR LSTCOL, 0		# LSTCOL was zero, don't let it go negative
CDUWPL:	add BYTE PTR LSTCOL, bh
	dec BYTE PTR LSTLIN		# LSTLIN is line unwrapped to
CDUWP3:
# Return terminator for DL-1
CDUWP4:	add bh, ch			# Get new terminator posn into BH
	dec bh
	popfd
	pop ax				# Restore original terminators for this line
	mov bl, TRMEOL			# Assume line unwrapped is at EOL
	jb CDUWPE			# BRIF at EOL
	mov bl, TRMLNF			# Assume LF was unwrapped on unwrapped line
	jz CDUWP6			# BRIF at LF
# WRAP or NULL_WRAP, if BH .NE. LINLEN, then it must be LINLEN-1 and NULL_WRAP
CDUWP5:	mov bl, TRMWRP			# Assume line is WRAP
	cmp bh, BYTE PTR LINLEN
	jz CDUWP7			# Previous line is full, must be WRAP
	mov bl, TRMNWP
	mov bh, BYTE PTR LINLEN
	jmp CDUWP7			# Previous line not full even though we had enough chars to fill it
# EOL terminated
CDUWPE:	or ah, ah
	jz CDUWP7			# If unwrapped null line, return EOL
# Was EOL or LF terminated, see if entire line was unwrapped
CDUWP6:	cmp ah, ch
	jnz CDUWP5			# BRIF wasn't unwrapped, previous line is WRAP
# terminator=(BH,BL)
CDUWP7:	push bx
CDUWP8:	pop ax
CDUWPX:	pop DWORD PTR TRMCUR
	pop dx
	mov bh, ch
	pop cx
	mov ch, bh
	pop ebx
	ret

# CDELPH: delete chars on physical line.
# On entry: (DH,DL)=posn to start delete from, (AH,AL)=line terminator for line DL, CH=count.
# On exit: AH=term posn, (DH,DL)=posn.
CDELPH:
	xor esi, esi			# Set SI to zero (flag no char restore necessary)
	push ebx
	push cx
	push dx
	push ax
	cmp dh, ah
	ja CDEL9I			# BRIF delete beyond or at physical end
	mov bx, dx			# (BH,BL)=DESTINATION
	mov ax, dx
	add ah, ch			# (AH,AL)=SOURCE
	pop cx				# CH=end of physical line
	push cx
	cmp ah, ch
	jbe CDELP0			# BRIF not deleting more than we have
	mov ah, ch
	inc ah				# Delete entire line
CDELP0:	cmp dl, BYTE PTR LSTLIN
	jnz CDELP1			# BRIF not on line with LSTPOS
	cmp ch, BYTE PTR LSTCOL
	jbe CDELP1			# BRIF physical end less than LSTPOS
	mov ch, BYTE PTR LSTCOL		# Use LSTPOS as end of this line
	cmp dh, ch
	ja CDEL9I			# BRIF delete beyond terminator
CDELP1:
	push cx				# Save last posn of SOURCE
	sub ch, ah
	pushfd
	mov dh, bh
	add dh, ch			# (DH,DL)=end of DESTINATION
	popfd
	jb CDELP4			# BRIF SOURCE greater than logical end of physical
	mov cl, 1
	inc ch
	call SCROLL			# Move characters to left
# Blank from end of SOURCE to end of DESTINATION+1
CDELP4:	pop cx				# CH=end column of SOURCE
	inc dh				# DH=end column of DESTINATION plus one
	push dx
	mov ax, ' '
CDELP5:	cmp dh, ch
	ja CDELP6
	call SCROUT			# Blank this posn
	inc dh
	jmp CDELP5
CDEL9I:	jmp CDELP9			# Indirect to CDELP9
CDELP6:	pop dx				# Restore end column of DESTINATION plus one
	pop ax				# Restore terminator passed to routine
# CH=last valid column, DH=first blanking column, AH=term column, AL=term type
	cmp dl, BYTE PTR LSTLIN
	jnz CDELP7			# BRIF was not delete before LSTPOS
	cmp ch, BYTE PTR LSTCOL
	jb CDELP7			# BRIF delete up to terminator
	# Delete included LSTPOS, subtract total deleted from LSTCOL
	pop dx
	pop cx
	sub BYTE PTR LSTCOL, ch
	jae CDEL8A			# BRIF LSTCOL valid
	mov BYTE PTR LSTCOL, 0		# Set LSTCOL to 0
	jmp CDEL8A			# Delete up to LSTPOS, adjust LSTPOS
# Deleted all data up to terminator, adjust terminator
CDELP7:	mov ah, dh
	dec ah				# Set terminator to last column of DESTINATION
CDELP8:	pop dx				# (DH,DL)=original posn (start of DESTINATION)
	pop cx				# CL=flag
CDEL8A:	cmp dh, ah
	jbe CDELPX			# BRIF still data remaining to right
	cmp dh, 1
	jnz CDELPX			# BRIF still data remaining to left
	cmp al, TRMEOL
	jz CDELPX			# BRIF don't remove null line
	# We have null line, remove it from the screen
	push DWORD PTR TRMCUR
	cmp dl, BYTE PTR LSTLIN		# **If LSTPOS is wiped out, put it at end of previous line
	jnz CDEL8B			# **BRIF not wiped out
	dec BYTE PTR LSTLIN
	mov bh, BYTE PTR WDOLFT
	mov BYTE PTR LSTCOL, bh
CDEL8B:
	call SCRLU0			# Scroll up from bottom of screen to DL
	pop DWORD PTR TRMCUR
	jmp CDELPX			# All done
# We are deleting to right of line terminator
CDELP9:	pop ax				# Restore terminator passed
	cmp al, TRMLNF
	mov dh, ah			# DH=end of valid data plus one
	jz CDELP7			# BRIF LF term'd, delete it
	pop dx				# Delete beyond valid data, ignore it
	pop cx
CDELPX:	pop bx
	cmp dl, BYTE PTR LSTLIN
	jz CDELPF			# BRIF not on LSTPOS line, no update
	cmp ah, BYTE PTR LSTCOL
	jae CDELPF			# BRIF LSTPOS is within line, no update
CDELPF:	ret

# ---------------------------------------------------------------- append, carriage return, linefeed
# Append to end of logical line
LAPPND:
LAPPN0:	call SCNRDT
	jb LAPPN1			# BRIF we are at last line in logical
	inc dl
	jmp LAPPN0			# See if next line is last line in logical
LAPPN1:	mov dh, ah			# Set current posn to EOL
	cmp ah, BYTE PTR LINLEN
	jnb LAPPN2			# If end of window, advance cursor posn here
	jmp WCSADV			# Use logical cursor advance (advances LSTPOS)
# This will set the cursor posn one beyond end (wrap next char typed)
LAPPN2:	inc dh
	ret

# Carriage return. The terminal receives CR (the logical cursor moves to the left margin).
LCARET:	mov al, ASCCR
	call PLAT_CHOUT
	call SCNRDT
	jb LCRET1			# BRIF this line already EOL terminated
	jz LCRET0			# BRIF LF terminated
	cmp al, TRMNWP
	jnz LCRET1			# BRIF not NULL_WRAP terminated
LCRET0:	dec ah				# Do not include LF or NULL_WRAP in line
LCRET1:	mov al, TRMEOL
	call SCNWTT			# Make this line EOL terminated
	mov dh, BYTE PTR WDOLFT		# And move posn to left margin
	ret

# Line feed. The terminal receives LF first, which moves it down (and scrolls it at its bottom).
LFEED:	mov al, ASCLF
	call PLAT_CHOUT
	mov esi, 0
	test BYTE PTR F_CRET, 0377
	jz LFD90I			# BRIF LF following CR
	test BYTE PTR F_INST, 0377
	jz LFED30			# BRIF not insert mode
	call SCNRTL
	cmp ah, dh
	jb LFED30			# Inserting beyond end of physical (just append)
	# Insert mode, move characters from cursor to EOL onto the next line
	cmp dl, BYTE PTR LSTLIN
	jz CBEPI2			# BRIF on LSTPOS line, can't do insert LF
	# IMPROVEMENT: Find last physical in this logical and see if it can be moved down one line.
LFED10:
	clc				# Indicate call is from Screen Editor
	call SCRINP
	mov esi, eax			# Save first char in ESI
	mov ax, ' '
	call SCROUT
	push dx
LFED12:	mov ch, 1
	call MKRMCH			# Move everything over one
	inc dh
	cmp dh, BYTE PTR LINLEN
	jbe LFED12			# Keep going until the end of the line
	mov ah, dl
	pop dx				# Restore posn
	mov dl, ah
	call PLINIT			# Terminate line at current posn
	cmp dl, BYTE PTR WDOBOT
	jnz LFED43			# Change terminator to LF and goto next line
	jmp LFEEDX
# No room!!
CBEEPI:	popfd				# Clean stack
CBEPI2:	jmp CBEEP
LFD90I:	jmp LFED90			# Indirect jump to LFED90
# Insert beyond end of physical = append. Append LF to end of physical line.
LFED30:	call SCNRTL
	pushfd
	jb LFED31			# BRIF need one more space to output LF
	jz LFED32			# BRIF already LF terminated, don't need space
LFED31:	inc ah				# Need one more space at terminator posn
LFED32:	cmp ah, BYTE PTR LINLEN
	jbe LFED40			# BRIF have room on this line
	call LFWRAP			# Wrap linefeed to next line
	jb LFED69			# BRIF no room
	# The possibilities at this point are WRAP or EOL terminated
	popfd
	jnb LFED34			# BRIF not EOL term'd (no change to terminator)
	call SCNRDT
	mov ah, BYTE PTR LINLEN		# EOL terminated, change to WRAP at LINLEN
	mov al, TRMWRP
	call SCNWTT
LFED34:	mov dh, BYTE PTR WDOLFT
	cmp dl, BYTE PTR LSTLIN
	jnz LFED35			# BRIF not extending LSTPOS
	inc BYTE PTR LSTLIN		# Move LSTPOS with LF
	mov BYTE PTR LSTCOL, dh
LFED35:	inc dl				# Move to new line (created by MKRMNL above)
	mov ah, dh
	jmp LFED43
LFED40:	popfd
	jnb LFED42			# BRIF not EOL, just change terminator to LF
LFED41:	call MKRMNL			# Open up next line
	jb LFEEDX			# No room for LF
# SUCCESS: terminate current line with LF
LFED42:	mov dh, ah
	mov ax, ' '
	call SCROUT			# Put a blank at terminator posn
LFED43:	call SCNRDT			# Reread terminator in case of MKRMNL scroll up
	mov ah, dh
	mov al, TRMLNF
	call SCNWTT			# And terminate line with LF
	cmp dl, BYTE PTR LSTLIN
	jnz LFED95			# BRIF no LSTPOS update
	mov BYTE PTR LSTCOL, 0		# LSTPOS moves to line beyond linefeed
	inc BYTE PTR LSTLIN
	jmp LFED95
LFED90:	push esi
	call MKRMNL			# Open up next line
	pop esi
	jb LFEEDX			# No room
LFED95:	call CSRDWN			# Move to next line
	mov dh, BYTE PTR WDOLFT
	or esi, esi
	jz LFEEDX
	mov eax, esi
	call SCROUT			# Restore first char of insert linefeed
LFEEDX:	cmp dl, BYTE PTR WDOBOT
	jbe LFEEDZ
	mov dl, BYTE PTR WDOBOT
LFEEDZ:	ret
# No room for linefeed, terminate this line and exit
LFED69:	pop eax				# discard saved flags (pushfd is 4 bytes)
	jmp LFEEDX

# Insert toggle (^R)
LINSRT:	xor BYTE PTR F_INST, 0377	# Toggle insert flag
	ret

# ---------------------------------------------------------------- insert and MKRMCH
# MKRMCH: open up space in logical lines. Entry (DH,DL)=posn, CH=number of spaces (1 or 2).
# Exit (DH,DL)=same posn (DL may change).
MKRMCH:	push esi
	push ax
	push cx
	push dx
	mov esi, offset BUF		# SI and DI are overflow character buffers
	mov edi, offset BUF+128
	call SCNRDT
	pushfd
	push ax
	call MVCHLN			# Move characters over, overflow at [DI]
	xor cl, cl			# CL=Iteration depth (and count of physical lines)
MKRMC0:	or ch, ch
	jnz MKRMC1			# BRIF line overflowed
	pop ax
	popfd
MKRMCX:	add cl, dl			# CL=original line number
	pop dx				# DH=orginal column
	mov dl, cl			# DL=original line number
MKRMX2:	pop cx
	pop ax
	pop esi
	ret
# Line overflow: we must wrap char to next line
MKRMC1:	pop ax
	popfd				# Get back old terminator
	jb MKRMC3			# BRIF was EOL term'd
	jnz MKRMC4			# BRIF was WRAP or NULL_WRAP term'd
	# Linefeed, make room for linefeed on next line
	push ax
	call LFWRAP
	pop ax
	jb MKRMCQ			# No room, throw away overflow
	dec ch
	jz MKRMCX			# Only wrapped the linefeed, done
	jmp MKRMC4			# Pass overflow to next line
# EOL terminated
MKRMC3:	push ax
	call MKRMNL			# Open up next line for overflow
	pop ax
	jnb MKRMC4			# BRIF room, wrap overflow
# No room for wrap, terminate this line with EOL and exit
MKRMCQ:	call SCNRDT
	mov al, TRMEOL
	call SCNWTT			# No room for overflow, just terminate line
	jmp MKRMCX
# Insert overflow on next line
MKRMC4:	cmp dl, BYTE PTR LSTLIN
	jnz MKRMC6			# BRIF not on LSTPOS line
	# AH=Old terminator posn, CH=Overflow count
	sub ah, ch
	cmp al, TRMNWP
	jnz MKRMC5			# BRIF wasn't NULL_WRAP, don't adjust posn
	dec ah
# AH=First position wrapped - 1
MKRMC5:	cmp ah, BYTE PTR LSTCOL
	jae MKRMC6			# BRIF did not wrap LSTCOL
	# Wrapped LSTPOS
	mov BYTE PTR LSTCOL, 0		# Set LSTPOS to start of next line
	inc BYTE PTR LSTLIN
MKRMC6:	inc dl				# (DH,DL)=start of next line
	mov dh, BYTE PTR WDOLFT
	dec cl				# Decrement iteration counter
	xchg esi, edi			# Use other overflow buffer for this line
	call SCNRDT
	pushfd
	push ax
	push cx				# Save char count
	call MVCHLN			# Make room on this line for previous overflow
	mov bx, cx			# Save count of overflow
	pop cx				# Get back overflow count in CH
	push esi			# Save pointer to characters for insertion
# Loop to output characters which overflowed onto this line from previous line
MKRMC7:	cld				# Scan forwards
	lodsw				# Get a character
	call SCROUT			# Output it
	inc dh
	sub ch, 1
	ja MKRMC7			# Output the next char
	pop esi
	mov ch, bh			# Restore overflow count for this line
	jmp MKRMC0			# Wrap new overflow

# Wrap a linefeed to next line
LFWRAP:	inc dl
	call MKRMNL
	jb LFWPEX			# room, throw away overflow
	push dx
	call SCRDLN			# Scroll this line down to next
	pop dx
	call LFTERM			# Make this line LF term'd
	clc
LFWPEX:	dec dl				# Restore original line number
	ret

# MVCHLN: open up space in physical line. Entry (DH,DL)=posn to open space at, ESI=buffer for
# overflow chars. Exit (DH,DL)=same posn, CH=No. of bytes overflowed.
# NOTE (report): the overflow-beyond-end path (MVCH30 -> MVCH23) leaves one extra word on the
# stack, as the original source does; it is kept literal.
MVC30I:	jmp MVCH30
MVCHLN:	push edi
	push cx
	cmp dl, BYTE PTR LSTLIN
	mov cl, BYTE PTR LINLEN
	jnz MVCH00			# BRIF within logical EOL (unless beyond TERM_POS)
	mov cl, BYTE PTR LSTCOL
MVCH00:	call SCNRTL			# AX=current physical terminator
	cmp ah, dh
	jb MVC30I			# BRIF inserting beyond end of physical
	cmp dl, BYTE PTR LSTLIN
	jnz MVCH03
	cmp cl, ah
	jnb MVCH03			# BRIF LSTPOS .GTE. TERM_POSN
	mov ah, cl			# Else use LSTPOS as TERM_POSN
MVCH03:	push dx
	push ax
	add ch, ah			# CH=last column for space needed
	sub ch, BYTE PTR LINLEN
	ja MVCH10			# BRIF overflow
	xor bh, bh			# Indicate no overflow
	jmp MVCH20
# We have overflow bytes to save
MVCH10:	mov dh, ah
	sub dh, ch
	inc dh				# DH=column of first overflow to save
	clc				# Indicate call is from Screen Editor
	call SCRINP			# Read an overflow char
	mov bx, cx			# Save overflow count in BH
# Loop to move overflow character to save buffer
MVCH14:	cld
	stosw				# Save overflow char
	dec ch
	jz MVCH20			# All done with overflow save
	inc dh
	clc				# Indicate call is from Screen Editor
	call SCRINP			# Read an overflow char
	jmp MVCH14			# And save it
# Move characters in line over
MVCH20:	mov cx, bx			# Restore # bytes overflowed to CH
	pop ax				# AH=term posn
	sub ah, ch			# Adjust term posn so move doesn't overflow
	pop dx				# DH=start column of insert
	pop bx				# BX=number of bytes to move over (pushed as 2-byte CX at MVCHLN)
	mov cl, bl
	push cx				# Save overflow count and CL for restore on exit
	mov ch, ah			# CH=last column to move
	sub ch, dh
	inc ch				# CH=count of chars from CURPOS to TERMPOS
	pushfd
	mov cl, 1			# Move on one line
	mov ax, dx			# FROM=CURPOS
	mov bl, dl
	add bh, dh			# TO=CURPOS+COUNT
	popfd
	jnz MVCH21			# BRIF count not zero, do move
	mov bh, dh			# All characters to move overflowed
	jmp MVCH22			# Just update terminator to current posn
MVCH21:	call SCROLL			# Move over characters
	add bh, ch			# New TERMPOS=TO+number of characters moved
	dec bh
MVCH22:	pop cx				# CH=-(No. of bytes wrapped)
# CH=overflow count, CL=insert count, BH=terminator posn
MVCH23:	call SCNRDT
	jb MVCH25
	jz MVCH25
# We have wrapped chars, terminate either WRAP or NULL_WRAP
MVCH24:	cmp bh, BYTE PTR LINLEN
	mov al, TRMWRP
	jz MVCH26
	or ch, ch
	jz MVCH26
	mov al, TRMNWP			# Null wrap that does not overflow changes to wrap
	mov bh, BYTE PTR LINLEN
	push ax
	push dx
	mov dh, bh
	mov ax, ' '			# Blank DBLCHR destroyed by scroll
	call SCROUT
	pop dx
	pop ax
	jmp MVCH26
# Was EOL or LF terminated
MVCH25:	or ch, ch
	jnz MVCH24			# Overflow means WRAP or NULL_WRAP
MVCH26:	cmp ah, bh
	ja MVCH27			# Terminator beyond scroll end
	mov ah, bh
# AH=New terminator, CL=insert count
	call SCNWTT			# Update terminator
MVCH27:	cmp dl, BYTE PTR LSTLIN
	jnz MVCHEX			# BRIF move is not on LSTPOS line
	cmp bh, BYTE PTR LSTCOL
	jbe MVCHEX			# BRIF end of move is before LSTPOS
	mov BYTE PTR LSTCOL, bh		# Else add number moved over to LSTPOS
MVCHEX:	pop edi
	ret
# Inserting beyond end of logical
MVCH30:	mov cl, ch			# Save insert count in CL
	add ch, dh			# CH=end of insert
	cmp al, TRMLNF
	jz MVCH31			# BRIF LF term'd cannot push null off end
	dec ch				# Ignore one null push off end of physical
MVCH31:	cmp ch, BYTE PTR LINLEN
	jbe MVCH33			# BRIF have room
	# No room, return blanks overflow, update terminator
	sub ch, BYTE PTR LINLEN		# CH=No. of overflow bytes
	mov ax, ' '
	push cx
MVCH32:	cld				# Store overflow bytes (All blanks)
	stosw
	dec ch
	jnz MVCH32
	pop cx				# Restore CH=number of overflow bytes
	pop ebx
	mov bh, ch
	push ebx				# Put overflow count with original CL on stack
	mov bh, BYTE PTR LINLEN		# BH=term posn, CH=overflow count, CL=insert count
	jmp MVCH23			# Update terminator and LSTPOS, exit
# Have room, just update terminator
MVCH33:	pop bx
	mov bh, 0			# Put overflow count (=0) on stack for return
	xchg bh, ch			# BH=term posn, CH=overflow count, CL=insert count
	jmp MVCH23			# New term posn still .LTE. LINLEN

# ---------------------------------------------------------------- physical terminator, LF wrap, cursor movement
#SUBROUTINE SCNRTL              ; Read logical terminator for physical line
#**                     On entry:   DL=line number
#**                     On exit:    AX=terminators
#**                                 CF set indicates EOL terminator
#**                                 CF clear, ZF set indicates LF terminator
#**                                 If NULL_WRAP terminator AH=TERM_POS-1
SCNRTL:	call SCNRDT			# Get terminators
	jb SCNRT2			# BRIF not NULL_WRAP
	jz SCNRT2			# BRIF not NULL_WRAP
	pushfd
	cmp al, TRMNWP
	jnz SCNRT1			# BRIF not NULL_WRAP
	dec ah				# Is NULL_WRAP, decrement posn
SCNRT1:	popfd
SCNRT2:	ret

# Wrap a linefeed (already a successful MKRMNL)
LFTERM:	push DWORD PTR TRMCUR		# Save current terminator address (4 bytes)
	push ax
	call SCNRDT			# Read next line terminator
	mov al, TRMLNF
	mov ah, BYTE PTR WDOLFT
	call SCNWTT			# Write out LF terminator at left margin plus one
	pop ax
	pop DWORD PTR TRMCUR
	ret

# These routines return CF=1 when they do not change the posn.
#Cursor right within window(or within physical line if outside window)
CSRADV:	inc dh				# Next column
	cmp BYTE PTR LINLEN, dh
	jae CSRRET			# BRIF Good value
	dec dh
	call CSRDWN
	jb CSRRET			# BRIF can't change physical lines
	mov dh, BYTE PTR WDOLFT		# First column
CSRRET:	ret

#Cursor left within window(or within physical line if outside window)
CSRREG:	dec dh				# Previous column
	cmp dh, BYTE PTR WDOLFT
	jnb CSRRET			# BRIF good value
	inc dh				# Restore column
	call CSRUP
	jb CSRRET			# BRIF can't change physical lines
	mov dh, BYTE PTR LINLEN		# Last column
	ret

#Cursor up within window(or NOP if outside window)
CSRUP:	cmp BYTE PTR WDOBOT, dl
	jb CSRRET			# BRIF outside of window
	cmp dl, BYTE PTR WDOTOP
	jb CSRRET
	stc
	jz CSRRET			# BRIF at top or outside of window
	clc
	dec dl
	ret

#Cursor down within window(or NOP if outside window)
CSRDWN:	cmp dl, BYTE PTR WDOTOP
	jb CSRRET			# BRIF outside window
	cmp BYTE PTR WDOBOT, dl
	jb CSRRET			# BRIF at bottom or outside of window
	stc
	jz CSRRET
	clc
	inc dl				# Next line
	ret

# ---------------------------------------------------------------- cursor movement within logical line
#Cursor advance within logical line
#                   on entry:   DH=current column, DL=current line
#                   on exit:    If CF clear, DH=new column, DL=new line
#                                   Else DH=EOL column, DL=EOL line
#                                   ZF=1 if stop at LSTPOS
#                               AH=terminator column, AL=terminator value
LCSADV:	cmp dl, BYTE PTR LSTLIN
	jnz LCSAD0			# BRIF not at LSTPOS
	cmp dh, BYTE PTR LSTCOL
	jb LCSAD0			# BRIF not at LSTPOS
	call SCNRDT
	cmp ah, ah			# Set ZF=1 if stop at LSTPOS
	stc
LCSADX:	ret
LCSAD0:	call SCNRDT
	jb LCSAD2			# BRIF at last line
LCSAD1:	call CSRADV
	jb LCSADA			# BRIF at end of window
	cmp ah, dh
	jnb LCSADX			# BRIF within logical line
	inc dl
	mov dh, BYTE PTR WDOLFT		# Else wrap to next physical of logical
	clc
	ret
LCSAD2:	cmp ah, dh
	ja LCSAD1			# BRIF not at EOL or beyond
LCSADA:	or dh, dh			# ZF=1(not stop at LSTPOS)
	stc
	ret

#Cursor regress within logical line
#                   on entry:   DH=current column, DL=current line
#                   on exit:    If CF clear, DH=new column, DL=new line
#                                   Else DH=EOL column, DL=EOL line
#                                   If stop at FSTPOS, ZF is set
LCSREG:	cmp dl, BYTE PTR FSTLIN
	jnz LCSRG0			# BRIF not at FSTPOS
	cmp dh, BYTE PTR FSTCOL
	ja LCSRG0			# BRIF not at FSTPOS
	stc
	ret				# ZF=1, CF=1, stop at FSTPOS
LCSRG0:	push bx
	mov bx, dx
	call CSRREG
	jb LCSRGA			# BRIF at top of window
	cmp bl, dl
	jz LCSRGX			# BRIF on same line, must be good posn
	call SCNRDT
	jnb LCSRGX			# BRIF still on same logical line
	mov dl, bl			# Move back to line we were on
	mov dh, BYTE PTR WDOLFT		# And posn at start of that line
LCSRGA:	or dh, dh			# ZF=0(not stop at FSTPOS)
	stc
LCSRGX:	pop bx
	ret				# Return CF set(at EOL)

# LSTART: set (DH,DL)=start of the current logical line
LSTART:
LSTRT0:	cmp dl, BYTE PTR FSTLIN
	jz LSTRT1
	call CSRUP
	jb LSTRT1			# Top of window must be start of line
	call SCNRDT
	jnb LSTRT0			# BRIF this line is part of logical
	inc dl
LSTRT1:	mov dh, BYTE PTR WDOLFT
	jb LSTRTX			# BRIF not using FSTPOS, use left margin
	mov dh, BYTE PTR FSTCOL
LSTRTX:	ret

# ---------------------------------------------------------------- miscellaneous
#CHECK FOR WORD OR NON-WORD CHAR(ALPHA NUMERIC)
ANCHK:	push dx
	clc				# Indicate call is from Screen Editor
	call SCRINP			# Read character at current posn in DX
	jnb ANCH0			# BRIF DH was not decremented
	inc dh
ANCH0:	or ah, ah
	jnz ANCNO			# BRIF is two-byte character - reject
	cmp al, 48			# '0' (60 octal)
	jb ANCNO			# BRIF is not alpha-numeric
	cmp al, 58			# ':' (72 octal)
	jb ANYES			# BRIF is numeric
	cmp al, 65			# 'A' (101 octal)
	jb ANCNO			# BRIF is not alpha-numeric
	cmp al, 91			# '[' (133 octal)
	jb ANYES			# BRIF is alpha
	cmp al, 97			# 'a' (141 octal)
	jb ANCNO			# BRIF is not alpha-numeric
	cmp al, 123			# '{' (173 octal)
	jb ANYES			# BRIF is small alpha
ANCNO:	stc
	pop dx
	ret

ANYES:	clc
	pop dx
	ret

# ---------------------------------------------------------------- OEM screen primitives
# These are NOT in the source tree (the OEM screen code was not supplied). They are rebuilt here
# on the shadow screen SCRSHD (index row*256+column). Output goes to the terminal through
# PLAT_CHOUT only for rows 1..WDOBOT; the function-key line and positioning are shadow-only.

# SCROUT: display AL at (DH=column, DL=row). Preserves all registers. CF=0 (no escape sequences,
# ESCCTL=0 in OEM.H).
SCROUT:	push eax
	push ebx
	push ecx
	push edx
	cmp dl, NMLINE
	ja SCRO20			# BRIF row not in shadow screen
	movzx ebx, dl
	shl ebx, 8			# row*256
	movzx ecx, dh
	add ebx, ecx			# index = row*256+column
	lea ecx, [SCRSHD]
	mov BYTE PTR [ecx+ebx], al	# Store in shadow screen
SCRO20:	cmp dl, 1
	jb SCRO30			# BRIF row 0: shadow only
	cmp dl, BYTE PTR WDOBOT
	ja SCRO30			# BRIF below the window (function-key line): shadow only
	call PLAT_CHOUT			# Send the character to the terminal (preserves AL, flags)
SCRO30:	pop edx
	pop ecx
	pop ebx
	pop eax
	clc
	ret

# SCRINP: read the character at (DH=column, DL=row) from the shadow screen. Returns AX (AH=0)
# and CF=0. A position outside the shadow screen reads as a blank. Preserves all other registers.
SCRINP:	push ebx
	push ecx
	mov eax, ' '
	cmp dl, NMLINE
	ja SCRI20			# BRIF row not in shadow screen
	movzx ebx, dl
	shl ebx, 8
	movzx ecx, dh
	add ebx, ecx			# index = row*256+column
	lea ecx, [SCRSHD]
	movzx eax, BYTE PTR [ecx+ebx]
SCRI20:	pop ecx
	pop ebx
	clc
	ret

# SCROLL: copy a rectangle of the shadow screen. On entry AL=source row, AH=source column,
# BL=destination row, BH=destination column, CL=number of rows, CH=number of columns.
# No terminal output. Preserves all registers. Rows are copied so that a source row is read
# before it is overwritten; each row goes through SCRTMP, so overlapping columns are safe.
SCROLL:	push eax
	push ebx
	push ecx
	push edx
	push esi
	push edi
	mov BYTE PTR SCRPAR+6, 0	# ascending order
	cmp bl, al
	jbe SCRL01			# BRIF destination row not below source row
	mov BYTE PTR SCRPAR+6, 1	# descending order (destination row below source row)
SCRL01:	mov BYTE PTR SCRPAR+0, al	# source row
	mov BYTE PTR SCRPAR+1, ah	# source column
	mov BYTE PTR SCRPAR+2, bl	# destination row
	mov BYTE PTR SCRPAR+3, bh	# destination column
	mov BYTE PTR SCRPAR+4, cl	# number of rows
	mov BYTE PTR SCRPAR+5, ch	# number of columns
	movzx ecx, BYTE PTR SCRPAR+4
	or ecx, ecx
	jz SCRL90			# BRIF no rows
	xor esi, esi			# ESI=k, step through the rows
SCRL10:	mov edx, esi
	cmp BYTE PTR SCRPAR+6, 0
	je SCRL20			# BRIF ascending
	movzx edx, BYTE PTR SCRPAR+4
	dec edx
	sub edx, esi			# EDX=row offset counting down from the last row
SCRL20:	call SCRLROW			# copy one row, row offset in EDX
	inc esi
	cmp esi, ecx
	jb SCRL10
SCRL90:	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# SCRLROW: copy one row of the SCROLL rectangle. EDX=row offset from the first row.
# Preserves all registers.
SCRLROW:
	push eax
	push ebx
	push ecx
	push edx
	push esi
	push edi
	movzx esi, BYTE PTR SCRPAR+0
	add esi, edx			# source row
	cmp esi, NMLINE
	ja SCRLR9			# BRIF source row not in shadow screen
	movzx edi, BYTE PTR SCRPAR+2
	add edi, edx			# destination row
	cmp edi, NMLINE
	ja SCRLR9			# BRIF destination row not in shadow screen
	shl esi, 8
	shl edi, 8
	movzx eax, BYTE PTR SCRPAR+1
	add esi, eax			# source index = source row*256+source column
	movzx eax, BYTE PTR SCRPAR+3
	add edi, eax			# destination index = destination row*256+destination column
	movzx ecx, BYTE PTR SCRPAR+5	# number of columns
	or ecx, ecx
	jz SCRLR9
	lea ebx, [SCRSHD]
SCRLR1:	lea edx, [esi+ecx-1]		# source element ECX-1
	mov al, BYTE PTR [ebx+edx]
	mov BYTE PTR [SCRTMP-1+ecx], al
	dec ecx
	jnz SCRLR1
	movzx ecx, BYTE PTR SCRPAR+5
SCRLR2:	lea edx, [edi+ecx-1]		# destination element ECX-1
	mov al, BYTE PTR [SCRTMP-1+ecx]
	mov BYTE PTR [ebx+edx], al
	dec ecx
	jnz SCRLR2
SCRLR9:	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# CLREOL: clear the shadow screen from (DH=column, DL=row) to the physical right edge.
# Preserves all registers.
CLREOL:	push eax
	push ebx
	push ecx
	push edx
	cmp dl, NMLINE
	ja CLRE9			# BRIF row not in shadow screen
	movzx eax, dh			# first column to clear
	movzx ecx, BYTE PTR CRTWID
	sub ecx, eax			# CRTWID-DH
	jb CLRE9			# BRIF column beyond the physical width
	inc ecx				# count inclusive of CRTWID
	movzx ebx, dl
	shl ebx, 8
	add ebx, eax			# index = row*256+column
	lea edx, [SCRSHD]
CLRE1:	mov BYTE PTR [edx+ebx], ' '
	inc ebx
	dec ecx
	jnz CLRE1
CLRE9:	pop edx
	pop ecx
	pop ebx
	pop eax
	clc
	ret

# CLRSCN: clear the shadow screen rows 1..LINCNT (OEM clear screen). Preserves all registers.
CLRSCN:	push eax
	push ecx
	push edi
	cld
	lea edi, [SCRSHD+256]		# row 1
	movzx ecx, BYTE PTR LINCNT
	shl ecx, 8			# LINCNT rows of 256 bytes
	mov al, ' '
	rep stosb
	pop edi
	pop ecx
	pop eax
	ret

# SWIDTH: set the width (AL) and the height (CL) through SCNSWI. CF=0.
SWIDTH:	call SCNSWI
	clc
	ret

# SCRATR: the terminal has no attribute store. AX=0, CF=0.
SCRATR:	xor eax, eax
	clc
	ret

# CSRATR: no cursor attribute on the terminal. CF=0.
CSRATR:	clc
	ret

# CSRDSP: display the cursor. The terminal cursor follows the output stream, so no action.
CSRDSP:	ret

# SETCSR: keep the cursor display in step with CSRTYP (GIOKYB.ASM SETCSR).
SETCSR:	push eax
	mov al, BYTE PTR CSRTYP		# Get cursor type
	cmp BYTE PTR CSRFLG, al		# Test for cursor change
	mov BYTE PTR CSRFLG, al		# Remember the new cursor type
	jz CSROK			# Cursor already set properly
	call CSRDSP			# Display the cursor
CSROK:	pop eax
	ret

# GWINI: OEM initialization. 80 columns (LINLN) by NMLINE lines; window and terminators are set
# by SCNSWI and TTBINI; shadow screen cleared; cursor at (1,1).
GWINI:	push eax
	push ecx
	mov al, LINLN			# LINLEN=80
	mov cl, NMLINE			# LINCNT=25
	call SCNSWI
	call TTBINI
	call CLRSCN
	mov BYTE PTR CSRX, 1
	mov BYTE PTR CSRY, 1
	pop ecx
	pop eax
	ret

# GETHED: OEM heading. There is none: EBX = pointer to an empty string and ZF=1, so GWINIT
# checks the command line for a program file and prints the standard heading.
GETHED:	lea ebx, OEMHED
	xor eax, eax			# ZF=1
	ret

# PRTMAP and EDTMAP: map an output character for CTLDSP. Editor functions already flagged
# AH=255 are kept. Control codes 0..31 get AH=255 so that CTLDSP runs its function table; any
# other character gets AH=0. Returns CF=0 and ZF=0 (display the character).
# UNSURE: reconstruction. The original PRTMAP and EDTMAP sources are not in the tree.
PRTMAP:
EDTMAP:	cmp ah, 255
	je PRTMA1			# editor function already flagged
	xor ah, ah
	cmp al, 32
	jae PRTMA1
	mov ah, 255			# control code: CTLDSP runs its function table
PRTMA1:	clc
	test esp, esp			# ZF=0 (esp is never zero)
	ret

	.section .rodata
OEMHED:	.byte 0				# empty OEM heading
	.section .text

	.globl SCROUT, SCRINP, SCROLL, CLREOL, CLRSCN, SWIDTH, SCRATR, CSRATR, CSRDSP, SETCSR
	.globl GWINI, GETHED, PRTMAP, EDTMAP

	.section .bss
SCRPAR:	.skip 8				# SCROLL parameters: 0 src row, 1 src col, 2 dst row, 3 dst col, 4 rows, 5 cols, 6 descending
SCRTMP:	.skip 256			# one row segment during SCROLL
	.section .text

	.section .text
	.globl CHWRAP, UPDPOS, SCNOL1, SCNOL2
