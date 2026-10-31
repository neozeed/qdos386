# GWRAM.ASM -> gwram.S
# OEM-independent RAM declarations (the DS: data block).
#
# The original is a DSEG preloaded by the linker (its comment: "the linker can
# preload segments with data"), so there is no copy routine. This file lays the
# whole block out in one .data section in the original order, with the original
# initial values. Uninitialized "R" variables are zero-filled.
#
# Widened stored pointers (16-bit in the original, 4-byte here). Each is an address
# in the flat address space:
#   MCLPTR   (2 -> 4)  MAC LANG pointer (DRAW)
#   MCLTAB   (2 -> 4)  pointer to MAC LANG command table (DRAW)
#   SNDBAS   (2 -> 4)  base address of sound queue cells
#   SNDOUT   (2 -> 4)  base address of sound queue buffer
#   TRMCUR   (2 -> 4)  address of current terminator entry (SCNDRV)
#   MAXUPD   (2 -> 4)  graphics: address of major-axis move update (ADVGRP, dropped)
#   MINUPD   (2 -> 4)  graphics: address of minor-axis move update (ADVGRP, dropped)
# Not widened: CMDOFF (index), LINTTB (device table of 2-byte entries), counts and
# lengths, and TRPTBL (3-byte event flags, no addresses).
#
# The original's R1 macro (MAXUPD, MINUPD) overlapped the previous byte with ORG $-1.
# That overlap is dropped here because it only mattered to graphics code.
#
# Symbol names keep the original spelling; "$" would become "_" (none in this file
# except _DATE, handled below).

	.section .rodata
	.globl HEDING, _DATE, CERMSG, LINCNT, LINLEN, CRTWID, WDOTOP, WDOBOT, WDOLFT, WDORGT, WDTFLG
	.globl LINTTB, TRMCUR, FSTLIN, FSTCOL, LSTLIN, LSTCOL
# Sign-on message (CSEG constants)
HEDING:
	.byte 13, 10
	.ascii "(C) Copyright Microsoft 1982"
_DATE:
	.ascii "         "			# $DATE (9 bytes)
	.byte 13, 10
	.byte 0

CERMSG:
	.byte 13, 10
	.ascii "Error detected in command line"
	.byte 13, 10
	.byte 0

	.section .data
	.globl MOVDAT, FREFLG, INITFG, ESCFLG, TWOBYT, STRTAB, ENDTAB, CMDOFF, FKCNUM
	.globl ACTPAG, F_EDPG, SEMFLG, F_CRET, F_INST, F_EDIT, DRWSCL, DRWFLG, DRWANG
	.globl MCLPTR, MCLLEN, MCLTAB, OCTAVE, BEATS, NOTELN, NOTE1L, MSCALE, MMODE
	.globl SNDTIC, SNDBAS, SNDOUT, KEYSW, PDIDS1, FPDVAR, LPDVAR, KYBQSZ
	.globl FOPTSZ

	.set FOPTSZ, 64		# size of file open options buffer
	.set KYBQSZ, 32		# size of keyboard queue
	# NMLINE = 25 (OEM.H): line terminator table has NMLINE+1 entries of 2 bytes

MOVDAT:
FREFLG:	.byte 0			# flag to print BYTES FREE message
INITFG:	.byte 0			# initialization complete flag
ESCFLG:	.byte 0			# escape sequence in progress flag
TWOBYT:	.byte 0			# save location for collecting two byte chars

# Keyboard support: SOFTKEY table (NMKEYF=10 entries of 16 bytes)
STRTAB:
	.ascii "LIST"		# FUNCTION 1
	.byte 0
	.skip 11
	.ascii "RUN"		# FUNCTION 2
	.byte 13, 0		# OFFSET CR,0
	.skip 11
	.ascii "LOAD"		# FUNCTION 3
	.byte 34, 0		# OFFSET QUOTE,0
	.skip 10
	.ascii "SAVE"		# FUNCTION 4
	.byte 34, 0
	.skip 10
	.ascii "CONT"		# FUNCTION 5
	.byte 13, 0
	.skip 10
	.ascii ","		# FUNCTION 6
	.byte 34		# OFFSET QUOTE
	.ascii "LPT1:"
	.byte 34, 0		# OFFSET QUOTE,0
	.skip 7
	.ascii "TRON"		# FUNCTION 7
	.byte 13, 0
	.skip 10
	.ascii "TROFF"		# FUNCTION 8
	.byte 13, 0
	.skip 9
	.ascii "KEY"		# FUNCTION 9
	.byte 0
	.skip 12
	.ascii "SCREEN 0,0,0"	# FUNCTION :
	.byte 13, 0
	.skip 2
ENDTAB:

CMDOFF:	.word 0			# index into ESCBUFF or STRTAB
FKCNUM:	.byte 6			# number of chars per function key to display on last line
ACTPAG:	.byte 0			# active page
F_EDPG:	.byte 0			# flag: 255 = program statement edit
SEMFLG:	.byte 0			# flag: non-zero indicates INPUT; (no CRLF at end of input)
F_CRET:	.byte 255		# zero indicates last character output was CR
F_INST:	.byte 0			# flag: 255 = insert mode
F_EDIT:	.byte 0			# flag: non-zero indicates INLIN active

# MACLNG (DRAW) variables
DRWSCL:	.byte 0			# DRAW scale
DRWFLG:	.byte 0			# DRAW option flags
DRWANG:	.byte 0			# DRAW translation angle
MCLPTR:	.long 0			# MAC LANG pointer (widened)
MCLLEN:	.byte 0			# string length
MCLTAB:	.long 0			# pointer to command table (widened)

# OEM-independent music locations
OCTAVE:	.byte 4			# PLAY octave
BEATS:	.byte 120		# beats (default 120 L4 notes per minute)
NOTELN:	.byte 4			# note length
NOTE1L:	.byte 4			# one (current) note length
MSCALE:	.byte 3			# note length scale factor (default 7/8 time)
MMODE:	.byte 0			# music mode (0 = foreground)
SNDTIC:	.byte 0			# sound/clock tick modulo counter
SNDBAS:	.long 0			# base address of sound queue cells (widened)
SNDOUT:	.long 0			# base address of sound queue buffer (widened)

# PDIRAM (single page, so PDIDS1 and FPDVAR coincide)
PDIDS1:
KEYSW:	.byte 0			# flag for softkey display

# PDURAM: page-dependent uninitialized RAM (scan driver)
FPDVAR:
LINLEN:	.skip 1			# number of characters in a line
LINCNT:	.skip 1			# number of lines
CRTWID:	.skip 1			# characters per line
WDOTOP:	.skip 1			# top line in window
WDOBOT:	.skip 1			# bottom line in window
WDOLFT:	.skip 1			# leftmost column in window
WDORGT:	.skip 1			# rightmost column plus one
WDTFLG:	.skip 1			# default width of SCRN:
LINTTB:	.skip 2*(25+1)	# line terminator table (2*(NMLINE+1))
TRMCUR:	.skip 4			# address of current terminator entry (widened)
FSTLIN:	.skip 1			# line number saved by SCNSTM call
FSTCOL:	.skip 1			# column saved as above
LSTLIN:	.skip 1			# line number saved by SCNSTM
LSTCOL:	.skip 1			# column saved as above
	.globl CSRX, CSRY
CSRY:	.skip 1			# current line (1-LINCNT). Must precede CSRX.
CSRX:	.skip 1			# current column (1-CRTWID)
LPDVAR:

# PIU RAM: page-independent uninitialized RAM
	.globl GRPACX, GRPACY, ATRBYT, GXPOS, GYPOS, MAXUPD, MINUPD, MAXDEL, MINDEL
	.globl ASPECT, CENCNT, CLINEF, CNPNTS, CPLOTF, CPCNT, CPCNT8, CRCSUM, CSTCNT
	.globl CSCLXY, CSAVEA, CSAVEM, CXOFF, CYOFF, LOHMSK, LOHDIR, LOHADR, LOHCNT
	.globl LFPROG, RTPROG, SKPCNT, MOVCNT, PDIREC, PUTFLG, QUEINP, QUEOUT
	.globl PSNLEN, QUELEN, SAVLEN, FILOPT, LP1DCB, KYBQDS, KYBQUE
	.globl MSWSIZ, MSWFLG, CSWSIZ, CSWFLG, NEWDS, COMDSC, CM1DCB, LSTIOB
	.globl TRPTBL, ONGSBF, SOFTKY, F_SUPR

GRPACX:	.skip 2			# previous X coordinate
GRPACY:	.skip 2			# previous Y coordinate
ATRBYT:	.skip 1			# attribute byte to store
GXPOS:	.skip 2			# X position of second coordinate
GYPOS:	.skip 2			# Y position of second coordinate
MAXUPD:	.skip 4			# address of major axis move update (widened)
MINUPD:	.skip 4			# address of minor axis move update (widened)
MAXDEL:	.skip 2			# largest delta for line
MINDEL:	.skip 2			# smaller of 2 deltas for line
ASPECT:	.skip 2			# aspect ratio
CENCNT:	.skip 2			# end circle point count
CLINEF:	.skip 1			# line-to-center flag
CNPNTS:	.skip 2			# 1/8 number of points in circle
CPLOTF:	.skip 1			# plot flag
CPCNT:	.skip 2			# 1/8 number of points in circle
CPCNT8:	.skip 2			# number of points in circle
CRCSUM:	.skip 2			# circle sum
CSTCNT:	.skip 2			# start count
CSCLXY:	.skip 1			# flag whether aspect was > 1
CSAVEA:	.skip 2			# ADVGRP C save area
CSAVEM:	.skip 1			# ADVGRP C save area
CXOFF:	.skip 2			# X offset from center save location
CYOFF:	.skip 2			# Y offset save location
LOHMSK:	.skip 1			# RAM save area for left overhang (LOHMSK and LOHDIR contiguous)
LOHDIR:	.skip 1
LOHADR:	.skip 2
LOHCNT:	.skip 2
LFPROG:	.skip 1			# PAINT: scan line already painted flags
RTPROG:	.skip 1
SKPCNT:	.skip 2			# skip count
MOVCNT:	.skip 2			# move count
PDIREC:	.skip 1			# paint direction
PUTFLG:	.skip 1			# whether doing PUT() or GET()
QUEINP:	.skip 2
QUEOUT:	.skip 2
PSNLEN:	.skip 2			# queue present length
QUELEN:	.skip 2			# maximum queue length
SAVLEN:	.skip 2			# used by BLOAD, BSAVE

# Device variables
FILOPT:	.skip FOPTSZ		# buffer for special-device open options
LP1DCB:	.skip 4*3		# LPT device control blocks (NMLPT=3, 4 bytes each)
KYBQDS:	.skip 16			# keyboard queue descriptor (4 pointers x 4 bytes, widened)
KYBQUE:	.skip KYBQSZ		# circular key queue
MSWSIZ:	.skip 2			# /M: value
MSWFLG:	.skip 1			# /M: exists flag
CSWSIZ:	.skip 2			# /C: value
CSWFLG:	.skip 1			# /C: exists flag
NEWDS:	.skip 2			# (segment value, unused in the flat model)
COMDSC:	.skip 18		# buffer used to communicate RS232 requests to OS
CM1DCB:	.skip 24*2		# COM device control blocks (NMCOMT=2)
LSTIOB:	.skip 1			# last RS232 unit accessed (0..15)

# Event trapping
TRPTBL:	.skip 5*14		# trap table: 5 bytes per event (flag byte + 4-byte GOSUB line pointer); 5*NUMTRP, NUMTRP=14
ONGSBF:	.skip 1			# some-event-happened flag (see NEWSTT)
SOFTKY:	.skip 1			# used by key trapping in GWSTS
F_SUPR:	.skip 1			# flag: non-zero = super shift expansion
