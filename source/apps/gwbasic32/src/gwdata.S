# GWDATA.ASM -> gwdata.S
# Interpreter constants, tables, messages and the DS: variables.
#
# Layout decisions (see NOTES.md, stage 3):
#  - The original keeps one 64K data segment. Program text, variables, strings and the
#    stack all live in it. Here the user memory is one .bss block at TSTACK, sized by the
#    compile-time MEMSIZE (passed with --defsym). Every pointer is a 32-bit address in the
#    flat space, so this block may be larger than 64K.
#  - The CS constant image and the DS RAM copy are one image. The original copies CONSTR
#    (code segment) to RAMLOW (data segment) at init. Here CONSTR and RAMLOW are the same
#    address, so GWINIT's copy is a self-copy and is harmless.
#  - Widened stored pointers (16 -> 32 bits): FILTAB, STKLOW, IOJUMP, CPMEXT (unchanged 4),
#    USRTAB entries, PTRFIL, TOPMEM, TXTTAB, OVERRI, DEVTBL/DEVPTR/DEVINI/DEVTRM, FREFDB,
#    CONTXT, MAXMEM, MEMSIZ, TEMPPT, FRETOP, TEMP3, TEMP8, ENDFOR, TEMP, SAVTXT, SAVSTK,
#    ERRTXT, TEMP2, OLDTXT, VARTAB, ARYTAB, STREND, DATPTR, PRMSTK, PRMPRV, ARYTA2, TEMP9,
#    NXTTXT, SAVFRE, CMEPTR, CMSPTR, LBUFF, PBUFF.
#    String descriptors: STRSIZ 3 -> 5 (length byte + 4-byte address). DSCPTR is still the
#    address field at offset 1 of DSCTMP. TEMPST and DSCTMP grow accordingly.
#    Tables of addresses: 2-byte entries -> 4-byte entries (FRCTBL, DBLDSP, SNGDSP, INTDSP).
#  - Line numbers stay 16-bit: DATLIN, ERRLIN, DOT, OLDLIN, AUTLIN, AUTINC, NXTLIN,
#    CHNLIN, CURLIN.
#  - The math scratch area (_DBUFF .. FBUFFR) is in mathram.S. Only the byte in front of
#    _DBUFF stays here (the original's "DOUBLE PRECISION BUFFER").

	.intel_syntax noprefix
	.include "oem.inc"
	.include "gio86u.inc"
	.include "msdosu.inc"

	.set STRSIZ, 5		# string descriptor: length(1) + address(4)
	.set NUMTMP_, 10	# NUMTMP from OEM.H
	.set PRMSIZ, 100	# bytes for a definition block
	.set BUFLEN_, 255
	.set KBFLEN, BUFLEN_ + (BUFLEN_ / 4)
	.set LPTLEN_, 132
	.set CLMWID_, 14
	.set LINLN_, 80
	.set NAMLEN_, 40

# ---------------------------------------------------------------- code-phase part
	.section .text
	.globl BEGCSG, START, JMPINI, _START
BEGCSG:
_START:
START:
JMPINI:
	jmp INIT		# INIT sets up the system, then jumps to READY

# ---------------------------------------------------------------- read-only tables
	.section .rodata
	.globl OPTAB, FRCTBL, DBLDSP, OPCNT, SNGDSP, INTDSP, INTXT, REDDY, BRKTXT

# Operator precedence (the routine addresses in the original were ADRP = empty here,
# so this table holds precedence bytes only: 12 operators).
OPTAB:
	.byte 121		# + (FADDT)
	.byte 121		# - (FSUBT)
	.byte 124		# * (FMULTT)
	.byte 124		# / (FDIVT)
	.byte 127		# ^ (FPWRT)
	.byte 80		# AND
	.byte 70		# OR
	.byte 60		# XOR (precedence)
	.byte 50		# EQV (precedence)
	.byte 40		# IMP (precedence)
	.byte 122		# MOD
	.byte 123		# IDIV

# Assignment conversion table: indexed by the 2-byte VALTYP index scaled to 4-byte entries.
# Entries: 0 = FRCDBL, 1 = pad (4 bytes), 2 = FRCINT, 3 = CHKSTR, 4 = FRCSNG
FRCTBL:
	.long FRCDBL
	.skip 4			# was DB 2 DUP(?): pad, widened with the entry size
	.long FRCINT
	.long CHKSTR
	.long FRCSNG

# Double precision, single precision and integer dispatch (4-byte entries).
DBLDSP:
	.long DADD
	.long DSUB
	.long DMULT
	.long DDIV
	.long DCOMP
	.set OPCNT, 4		# (($-DBLDSP)/2)-1 in the original; 5 entries, so 4
SNGDSP:
	.long FADD
	.long FSUB
	.long FMULT
	.long FDIV
	.long FCOMP
INTDSP:
	.long IADD
	.long ISUB
	.long IMULT
	.long INTDIV
	.long ICOMP

# Error messages (generated)
	.globl ERRTAB
	.include "gwdata_errors.inc"

# Text constants for messages
	.globl INTXT, REDDY, BRKTXT
INTXT:
	.ascii " in "
	.byte 0
REDDY:
	.ascii "Ok"
	.byte 255		# flag as not being user input
	.byte 13, 10		# ACRLF
	.byte 0
BRKTXT:
	.ascii "Break"
	.byte 0

# ---------------------------------------------------------------- data segment
	.section .data
	.globl BEGDSG, CPMMEM, RAMLOW, CONSTR, ENDCNS
	.globl NUMCON, DSEGZ, RNDCOP, RNDX, _RNDX, ENDPRG, STAINP, USRTAB, RNDCNT
	.globl NULCNT, MSDCCF, CTLCAD, DINTAD, LSTCHR, ERRFLG, LPTLST, LPTPOS, PRTFLG
	.globl NLPPOS, LPTSIZ, DAYSPM, CLMLST, RUBSW, CNTOFL, PTRFIL, TOPMEM, CURLIN
	.globl TXTTAB, OVERRI, CSRTYP, CSRFLG, DEVTBL, DEVPTR, DEVINI, DEVTRM, SAVKEY
	.globl SAVKYF, FREFDB, DATSTR, IOJUMP, FILTAB, STKLOW, FILMOD, NLONLY, RUNFLG
	.globl F_PCOM, DATIME, CPMEXT, FILNAM, FILDEV, FILNM, FILEXT, NAMCNT, NAMBUF, NAMTMP
	.globl FILNA2, CPMVRN, CPMREA, CPMWRI, KBUF, BUFMIN, BUF, ENDBUF, TTYPOS, DIMFLG
	.globl VALTYP, _VALTP, DORES, _OPRTYP, OPRTYP, DONUM, CONTXT, CONSAV, CONTYP, CONLO
	.globl MAXMEM, MEMSIZ, TEMPPT, TEMPST, DSCTMP, DSCPTR, FRETOP, TEMP3, TEMP8
	.globl ENDFOR, DATLIN, SUBFLG, SARYFL, FLGINP, USFLG, TEMP, PTRFLG, AUTFLG
	.globl AUTLIN, AUTINC, SAVTXT, SAVSTK, ERRLIN, DOT, ERRTXT, ONELIN, ONEFLG
	.globl SAVSEG, TEMP2, OLDLIN, OLDTXT, VARTAB, ARYTAB, STREND, DATPTR, DEFTBL
	.globl PRMLEN, PRMSTK, PRMPRV, PRMLN2, PARM1, PARM2, PRMFLG, ARYTA2, NOFUNS
	.globl TEMP9, FUNACT, INPPAS, NXTTXT, NXTFLG, FVALSV, NXTLIN, OPTVAL, OPTFLG
	.globl TOPTVL, TOPTFG, TEMPA, SAVFRE, PROFLG, MRGFLG, MDLFLG, CMEPTR, CMSPTR
	.globl CHNFLG, CHNLIN, SWPTMP, TRCFLG, EXPAF, EXPTMP, TEMPB, RECRD, LBUFF, PBUFF
	.globl PGTFLG, TSTACK

# Start of the data segment. The original puts the CP/M control block below offset 256.
BEGDSG:
	.skip 2
CPMMEM:
	.skip 254

# ---- constant image (code segment in the original, copied to RAM at init)
RAMLOW:
CONSTR:
	.byte 0xEA		# JMP far (inter-segment direct jump, unused in the flat model)
	.byte 0, 0, 0, 0	# the four address bytes of that jump
	.byte 0
RNDCNT:
	.byte 0, 0		# RNDCNT (label at image offset 6)
	.byte 0xC3		# RET: kludge to make CHRGET work
NUMCON:
	.byte 30		# CONCON: fake token forces CHRGET to re-scan the embedded constant
	.byte 16		# CONCN2: second kind of embedded constant
DSEGZ:
	.byte 0			# data-segment-located zero
RNDCOP:
	.byte 82, 199, 79, 128	# copy of the RND seed (octal 122,307,117,200): between 0 and 1
RNDX:
_RNDX:
	.byte 82, 199, 79, 128	# last random number generated (same initial value)
STAINP:
	.byte 0xE4, 0x00	# IN AL,0 (not reached in the flat model)
	.byte 0xCB		# far return
ENDPRG:
	.byte 58		# ":" then four zero bytes: fake end of program for RESUME NEXT
	.byte 0, 0, 0, 0

# ---- initialized RAM copied from the code segment
USRTAB:
	.rept 10
	.long 65535		# undefined user function (was DW 65535)
	.endr
NULCNT:	.byte 1			# number of nulls to print after CRLF
MSDCCF:	.byte 0			# Ctrl-C flag set by Ctrl-C interrupt handler
CTLCAD:	.skip 4			# pre-BASIC Ctrl-C interrupt vector (device)
DINTAD:	.skip 4			# BASIC disk error interrupt vector (device)
LSTCHR:	.byte 0			# used by SCNSOT to remember last char out
ERRFLG:	.byte 0			# saves the error number so EDIT can run on "SN" error
LPTLST:	.byte 0			# last line printer operation (0 = linefeed)
LPTPOS:	.byte 0			# position of LPT print head
PRTFLG:	.byte 0			# whether output goes to LPT
NLPPOS:	.byte 112		# last column beyond which no more comma fields: LNCMPS
LPTSIZ:	.byte 132		# default line printer width: LPTLEN
DAYSPM:	.byte 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
CLMLST:	.byte 56		# position of last comma column: NCMPOS
RUBSW:	.byte 0			# rubout switch
CNTOFL:	.byte 0			# suppress output flag
PTRFIL:	.long 0			# pointer to data block of current file (widened)
TOPMEM:	.long TSTACK + 100	# top location for the stack (widened)
CURLIN:	.long 65534		# current line number (65534 during INIT; value fits 16 bits)
TXTTAB:	.long TSTACK	# beginning of program text (widened); INIT sets it again
OVERRI:	.long OVRMSG		# address of the overflow message (widened)
CSRTYP:	.byte 0
CSRFLG:	.byte 0
DEVTBL:	.long _DVTBL		# device name table (widened)
DEVPTR:	.long _DVPTR
DEVINI:	.long _DVINI
DEVTRM:	.long _DVTRM
SAVKEY:	.byte 0			# second byte of a 2-byte sequence
SAVKYF:	.byte 0			# flag: SAVKEY active
FREFDB:	.long 0			# FDB to release if an open fails (widened)
DATSTR:
ENDCNS:

# ---- GIO86 uninitialized data

IOJUMP:	.skip 4			# I/O jump vector (widened)
FILTAB:	.skip 4			# points to 1st FDB (= STKLOW if no FDBs; widened)
STKLOW:	.skip 4			# lowest legal value of SP (widened)
FILMOD:	.skip 1
NLONLY:	.skip 1
RUNFLG:	.skip 1
F_PCOM:	.skip 1			# flags comma terminator for number print
DATIME:	.skip 5			# days(2), hours(1), minutes(1), seconds(1)
CPMEXT:	.skip 4			# exit jump vector (set to DS:0 at INIT)

# ---- RAM variables (LOW SEGMENT)
NAMCNT:	.skip 1			# characters beyond #2 in a variable name
NAMBUF:	.skip NAMLEN_ - 2	# storage for chars beyond #2 (used in PTRGET)
NAMTMP:	.skip 4			# temp storage during name save at INDLOP (widened: holds a text pointer)
# DIRTMP (CPMWRM+128) is a CP/M buffer address; the flat model has no equivalent.
FILNA2:	.skip 16		# used by name code
	.skip 17		# used for FILES in the SCP version
FILNAM:	.skip 33		# directory buffer for CP/M reads
	# Aliases of FILNAM come after its definition: GNU as 1.38 leaves an alias of a later label undefined.
	FILDEV = FILNAM
	FILNM = FILNAM + 1
	FILEXT = FILNM + 11 - 3	# FNAML = 11
CPMVRN:	.skip 1			# CP/M version number
CPMREA:	.skip 1			# CP/M read call
CPMWRI:	.skip 1			# CP/M write call
	.byte 58		# ":" a colon for restarting input
KBUF:	.skip KBFLEN		# krunch buffer
	.skip 2			# room for the 4-byte zero link after a direct statement (CRUNCH writes it)
BUFMIN:	.byte 44		# a comma (preload): INPUT data pointer starts on a comma
BUF:	.skip BUFLEN_ + 1	# type-in buffer; direct statements execute from here
	.skip 2			# allow for a single quote in a big line
ENDBUF:	.skip 1			# place to stop big lines
TTYPOS:	.skip 1			# terminal position
DIMFLG:	.skip 1			# whether a pointer is wanted for DIM
_VALTP:
VALTYP:	.skip 1			# type indicator: 0 numeric, 1 string (VALTYP and DIMFLG are adjacent)
_OPRTYP:
OPRTYP:				# operator number (zero-size alias of DORES)
DORES:	.skip 1			# whether crunch can crunch reserved words
DONUM:	.skip 1			# crunch flag: 0 numbers allowed, 1 numbers by LINGET, -1 disallowed
CONTXT:	.skip 4			# saved text pointer used by CHRGET (widened)
CONSAV:	.skip 1			# saved token for a constant after CHRGET
CONTYP:	.skip 1			# saved constant valtype
CONLO:	.skip 8			# saved constant value (4) plus extra for double precision
MAXMEM:	.skip 4			# maximum size of the data area (widened)
MEMSIZ:	.skip 4			# highest location in memory (widened)
TEMPPT:	.skip 4			# pointer at first free temp descriptor (widened)
TEMPST:	.skip STRSIZ * NUMTMP_	# NUMTMP temp descriptors of STRSIZ bytes
DSCTMP:	.skip 1			# string functions build their answer descriptor here: length byte
DSCPTR:	.skip 4			# address field of DSCTMP (DSCTMP+1, 4 bytes)
FRETOP:	.skip 4			# top of string free space (widened)
TEMP3:	.skip 4			# end of string arrays in GC; temp in FRMEVL/FOUT (widened)
TEMP8:	.skip 4			# used by garbage collection (widened)
ENDFOR:	.skip 4			# saved text pointer at end of FOR (widened)
DATLIN:	.skip 2			# data line number, for errors
SUBFLG:	.skip 1			# subscripted variable allowed flag
SARYFL:	.skip 1			# scanned-array flag (set by PTRGET for array element)
USFLG:
FLGINP:	.skip 1			# flags whether we are doing INPUT or READ
TEMP:	.skip 4			# temp for statement code (pointers, widened)
PTRFLG:	.skip 1			# 0 if no line numbers converted to pointers
AUTFLG:	.skip 1			# AUTO command in progress
AUTLIN:	.skip 2			# current line being inserted by AUTO
AUTINC:	.skip 2			# the AUTO increment
SAVTXT:	.skip 4			# text pointer saved for RESUME (widened)
SAVSTK:	.skip 4			# stack saved for error recovery (widened)
ERRLIN:	.skip 2			# line number of the last error
DOT:	.skip 2			# current line for EDIT and LIST
ERRTXT:	.skip 4			# text pointer for RESUME (widened)
ONELIN:	.skip 4			# line pointer to GOTO on error (widened: holds a 4-byte line pointer)
ONEFLG:	.skip 1			# 1 while executing an error-trap routine
SAVSEG:	.skip 2			# segment save area (unused in the flat model)
TEMP2:	.skip 4			# formula evaluator temp (widened)
OLDLIN:	.skip 2			# old line number (after ^C, STOP or END)
OLDTXT:	.skip 4			# old text pointer (widened)
VARTAB:	.skip 4			# start of simple variable space (widened)
ARYTAB:	.skip 4			# beginning of the array table (widened)
STREND:	.skip 4			# end of storage in use (widened)
DATPTR:	.skip 4			# pointer to DATA (widened)
DEFTBL:	.skip 26		# default VALTYP for each letter of the alphabet

# User-defined function parameter information
PRMSTK:	.skip 4			# previous definition block on stack (widened)
PRMLEN:	.skip 2			# number of bytes in the active table
PARM1:	.skip PRMSIZ		# active parameter definition table
PRMPRV:	.skip 4			# previous parameter block pointer (widened)
PRMLN2:	.skip 2			# size of parameter block being built
PARM2:	.skip PRMSIZ		# parameters being made
PRMFLG:	.skip 1			# PTRGET: PARM1 has been searched
ARYTA2:	.skip 4			# stopping point for simple search (widened; DWORD per NOTES)
NOFUNS:	.skip 1			# zero if no functions active
TEMP9:	.skip 4			# GC temp chained through parameter blocks (widened)
FUNACT:	.skip 2			# count of active functions
INPPAS:	.skip 1			# INPUT scanning first (0) or second time
NXTTXT:	.skip 4			# text pointer at start of NEXT (widened)
NXTFLG:	.skip 1			# zero if FOR uses NEXT code to check empty loop
FVALSV:	.skip 4			# loop variable start value (ANSI order of evaluation)
NXTLIN:	.skip 2			# line number during the NEXT scan
OPTVAL:	.skip 1			# OPTION BASE value (0 or 1)
OPTFLG:	.skip 1			# OPTION BASE has been scanned
TOPTVL:	.skip 1			# temp: OPTVAL during CHAIN
TOPTFG:	.skip 1			# temp: OPTFLG during CHAIN
TEMPA:	.skip 2			# misc temp used by CALL and LIST (IBMRES uses it as a byte)
SAVFRE:	.skip 4			# FRETOP saved by CHAIN (widened)
PROFLG:	.skip 1			# loaded a protected file without password
MRGFLG:	.skip 1			# CHAIN with MERGE in progress
MDLFLG:	.skip 1			# CHAIN with MERGE and DELETE in progress
# MERGE (ASCII program file, read a line at a time through INCHR; see bifile.S)
	.globl MRGACT, MRGHDL, MRGNAME, MRGCH
MRGACT:	.skip 1			# nonzero while a MERGE file is being read (ISFLIO reports a file)
MRGCH:	.skip 1			# the byte INCHR last read
MRGHDL:	.skip 4			# platform handle of the MERGE file
MRGNAME:	.skip 4			# pointer to the MERGE file name (during the open)
CMEPTR:	.skip 4			# pointer to end line to delete (widened)
CMSPTR:	.skip 4			# pointer to start line to delete (widened)
CHNFLG:	.skip 1			# CHAIN in progress
CHNLIN:	.skip 2			# destination line in the new program
SWPTMP:	.skip 8			# value of first SWAP variable (4), room for double precision (4 more)
TRCFLG:	.skip 1			# zero: no trace in progress

# ---- math RAM temporaries (EXPAF/EXPTMP are used by FOUT; the rest is in mathram.S)
EXPAF:	.skip 1			# exponent adjustment factor for E format
EXPTMP:	.skip 8			# FAC save area for field overflow test
TEMPB:				# used by FIELD (zero-size)
RECRD:	.skip 2			# record number
LBUFF:	.skip 4			# logical buffer address (widened)
PBUFF:	.skip 4			# physical buffer address (widened)
PGTFLG:	.skip 1			# PUT/GET flag (non-zero = PUT)
	.skip 1			# DOUBLE PRECISION BUFFER (byte in front of mathram _DBUFF)

# ---- user memory: program text, variables, strings, stack. Sized at build time.
	.section .bss
	.globl LSTVAR, TSTACK
# LSTVAR: the zero byte in front of the program text (the original's last variable byte).
# INIT sets TXTTAB = TSTACK (the program text start). LSTVAR = TSTACK-1 holds the CR that
# INITSA writes before the program text (the original's byte at TXTTAB-1).
LSTVAR:	.skip 1
TSTACK:
	.skip MEMSIZE
