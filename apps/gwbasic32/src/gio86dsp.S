# GIO86.ASM -> gio86dsp.S
# I/O dispatch (PTRDSP, SIDSP, SIDSP1, SIDSP2, TBLDSP, CDEVID), general queue routines
# (INITQ, PUTQ, GETQ, BUMPQP, NUMQ, LFTQ), PRINT helpers (PTRGPS, PTRWDC, PTRWID, ADRGET) and
# the register-restore epilogue (PDCBAX, PCBAX, PBAX), translated from GIO86.ASM. Also the
# dispatch tables and entries for file, printer and graphics devices that are out of scope for
# this stage (UNSUPPORTED entries jump to ADVERR, see the bottom of this file).
#
# Widths (CONVENTIONS.md):
#   pointers (BX, SI, DI, FDB and queue-descriptor addresses) are 32-bit, pushed and popped as 4 bytes;
#   values (AL, AH, AX, CX, DX, CH, CL, counts) stay 8-bit or 16-bit, pushed and popped as 2 bytes.
# Table widths:
#   DEVPTR (gwdata.S) holds the address of _DVPTR (scrout.S). _DVPTR entries are 4 bytes; the
#     index is the device number (0 = disk, 1 = KYBD, 2 = SCRN, 3 = CONS, 4..6 = LPT1..3). The
#     original scaled the index by 2; here it is scaled by 4.
#   Dispatch tables (SCNDSP, KYBDSP, CONDSP, ...) have 4-byte entries. A function code G_xxx
#     counts 2 per entry, so the byte offset is G_xxx * 2 (the original used G_xxx directly).
#   IOJUMP (gwdata.S) is a 4-byte jump vector.
# Queue descriptor (KYBQDS, gwram.S, 16 bytes): Q_PUT=0, Q_GET=4, Q_BUF=8, Q_END=12.
# No change to gwram.S was needed: KYBQDS is already 16 bytes.

	.intel_syntax noprefix
	.include "msdosu.inc"		# F_DEV, F_WID (FDB offsets)
	.include "gio86u.inc"		# G_xxx function codes (2 per dispatch entry)

# ---------------------------------------------------------------- constants
# Queue descriptor field offsets (GIO86.ASM: 8-byte descriptor of 2-byte fields; 16 bytes here)
	.set Q_PUT, 0			# points to the next empty byte in the queue
	.set Q_GET, 4			# points to the next byte to be fetched (oldest data)
	.set Q_BUF, 8			# points to the 1st byte of the physical buffer
	.set Q_END, 12			# points 1 byte beyond the end of the physical buffer

# Device ids (GIOTBL.ASM: $_KYBD = 0377 octal, $_LPT1 = 0374 octal). Same values as
# _KYBD and _LPT1 in scrout.S; defined locally so that they are immediates.
	.set DEVID_KYBD, 0377		# KYBD device id (-1)
	.set DEVID_LPT1, 0374		# LPT1 device id (-4)

	.section .text

# ---------------------------------------------------------------- queue routines
	.globl INITQ, PUTQ, GETQ, NUMQ, LFTQ, BUMPQP

# These routines are all called with SI pointing to a 16-byte queue descriptor
# (GIO86.ASM: 8-byte descriptor; Q_PUT=0, Q_GET=2, Q_BUF=4, Q_END=6 there).
#   Note: if Q.PUT(SI)=Q.GET(.SI), the queue is empty
#         Size of queue buffer is Q.END - Q.BUF
#         Maximum data in queue at any time is Q.END - Q.BUF - 1

# INITQ - initialize queue descriptor for empty queue
# Entry - SI points to 16 byte queue descriptor
#         BX points to 1st byte of queue buffer
#         [AX] = size of queue buffer
# Exit  - AX is used
#
INITQ:	mov DWORD PTR [esi+Q_PUT], ebx
	mov DWORD PTR [esi+Q_GET], ebx
	mov DWORD PTR [esi+Q_BUF], ebx
	movzx eax, ax			# AX is a 16-bit size; clear the upper half before adding the pointer
	add eax, ebx
	mov DWORD PTR [esi+Q_END], eax
	ret

# PUTQ - append data to back of queue
# Entry - SI points to 16 byte queue descriptor
#         [AL]=data to be put in queue
# Exit  - FLAGS.Z is true if queue is full
#
PUTQ:	mov ebx, DWORD PTR [esi+Q_PUT]
	mov BYTE PTR [ebx], al
	call BUMPQP			# advance PUT-POINTER BX
	cmp ebx, DWORD PTR [esi+Q_GET]
	je PQFULL			# brif queue is full (ignore new data)
	mov DWORD PTR [esi+Q_PUT], ebx	# save new PUT-POINTER
PQFULL:	ret

# GETQ - get next byte from front of queue
# Entry - SI points to 16 byte queue descriptor
# Exit  - FLAGS.Z is true if queue is empty
#         otherwise, [AL]=data to be put in queue
#         AX is used
#
GETQ:	mov ebx, DWORD PTR [esi+Q_GET]
	cmp ebx, DWORD PTR [esi+Q_PUT]
	je GQEMTY			# branch if queue is empty
	mov al, BYTE PTR [ebx]		# get next byte from queue
	call BUMPQP			# advance queue pointer BX
	mov DWORD PTR [esi+Q_GET], ebx	# save new GET-POINTER
	or ebx, ebx			# set NZ so caller knows AL has data (pointer is never 0)
GQEMTY:	ret

# BUMPQP - advance queue pointer BX by one byte, wrapping at Q_END back to Q_BUF
# Entry - SI points to 16 byte queue descriptor, BX = pointer
# Exit  - BX = advanced pointer
#
BUMPQP:	inc ebx
	cmp ebx, DWORD PTR [esi+Q_END]
	jb BUMPQX			# branch if no need to Wrap-Around
	mov ebx, DWORD PTR [esi+Q_BUF]	# reset pointer to start of buf
BUMPQX:	ret

# LFTQ - How many times can PUTQ be successfully called for queue SI
# Entry - SI points to 16 byte queue descriptor
# Exit  - [AX] = number of bytes free in queue (0..QueueSize - 1)
#
LFTQ:	mov eax, DWORD PTR [esi+Q_GET]
	dec eax
	sub eax, DWORD PTR [esi+Q_PUT]
	jmp NUMQ1

# NUMQ - How many times can GETQ be successfully called for queue SI
# Entry - SI points to 16 byte queue descriptor
# Exit  - [AX] = number of bytes of data in queue (0..QueueSize - 1)
#
NUMQ:	mov eax, DWORD PTR [esi+Q_PUT]
	sub eax, DWORD PTR [esi+Q_GET]
NUMQ1:	jae NUMQX			# brif PUT-POINTER exceeds GET-POINTER
	add eax, DWORD PTR [esi+Q_END]
	sub eax, DWORD PTR [esi+Q_BUF]
NUMQX:	ret

# ---------------------------------------------------------------- dispatch to I/O routines
	.globl PTRDSP, SIDSP, SIDSP1, SIDSP2, TBLDSP, CDEVID, ERDNA2

# Dispatch to I/O routine for file with FDB=[PTRFIL]
# Entry - [AH]=function code (G.OPN, G.CLS, etc.)
#         [PTRFIL]=points to file's FDB
# Exit  - All registers preserved except those changed by device driver
#
PTRDSP:	push esi
	mov esi, DWORD PTR PTRFIL
	call SIDSP
	pop esi
	ret

# SIDSP - Dispatch to I/O routine for file with FDB=[SI]
# Entry - [AH]=function code (G.OPN, G.CLS, etc.)
#         [SI]=points to file's FDB
# Exit  - All registers preserved except those changed by device driver
#
SIDSP:	push edi
	push esi
	push ax
	pushfd
	cmp esi, PSEUDO_SAV		# the ASCII SAVE output file: its own table (SAVDSP_TBL)
	je SIDSP_SAV
	inc esi
	jz LPDSP			# branch if Line Printer Pseudo FDB
	dec esi
	jnz SIDSP1			# branch if not Keyboard/CRT Pseudo FDB
#        MOVI    DI,$CODE+2*<^O400-$.KYBD>
	mov edi, DEVID_KYBD		# MOV DI,OFFSET $_KYBD
	neg edi				# Change neg. dev. # to 0 based offset
	and edi, 0377			# AND DI,377O
	shl edi, 2			# ASM86 cann't multiply externals by 2 (entries are 4 bytes: *4)
	jmp SIDSP2
LPDSP:
#        MOVI    DI,$CODE+2*<^O400-$.LPT1>   ;SI=0
	mov edi, DEVID_LPT1		# MOV DI,OFFSET $_LPT1 (SI=0 here, after INC)
	neg edi				# Change neg. dev. # to 0 based offset
	and edi, 0377			# AND DI,377O
	shl edi, 2			# entries are 4 bytes: *4
	jmp SIDSP2
SIDSP1:	mov al, BYTE PTR [esi+F_DEV]	# (AL) = device number
	call CDEVID			# [DI]=device dispatch table offset
SIDSP2:	popfd
	pop ax				# restore caller's [AX]
	call TBLDSP			# function [AH] on device [DI]
	pop esi
	pop edi
	ret

# SIDSP_SAV: the ASCII SAVE output file. Same epilogue as SIDSP2, with SAVDISP in place of TBLDSP.
SIDSP_SAV:
	popfd
	pop ax				# restore caller's [AX]
	call SAVDISP
	pop esi
	pop edi
	ret

# SAVDISP - call the SAVDSP_TBL routine for function [AH]. The routine gets the caller's AL.
# The table has one longword per G code, at byte offset 2*G (the same layout as LPTDSP): G_CLS=6 is
# entry 3, G_SOT=16 entry 8, G_GPS=18 entry 9, G_GWD=20 entry 10. Other codes are ADVERR.
SAVDISP:
	push eax
	movzx eax, ah
	shl eax, 1
	add eax, OFFSET SAVDSP_TBL
	mov eax, DWORD PTR [eax]
	mov DWORD PTR SAVJMP, eax
	pop eax				# the caller's AX again (AH is the function code)
	call DWORD PTR SAVJMP
	ret

	.globl SAVDSP_TBL
SAVDSP_TBL:
	.long ADVERR			# G_EOF
	.long ADVERR			# G_LOC
	.long ADVERR			# G_LOF
	.long ADVERR			# G_CLS (not dispatched: SAVE closes the file itself)
	.long ADVERR			# G_SWD
	.long ADVERR			# G_RND
	.long ADVERR			# G_OPN
	.long ADVERR			# G_SIN
	.long SAV_SOT			# G_SOT: output one byte
	.long SAV_GPS			# G_GPS: column
	.long SAV_WID			# G_GWD: width
	.long ADVERR			# G_SCW
	.long ADVERR			# G_GCW
	.long ADVERR			# G_BIN
	.long ADVERR			# G_BOT

	.section .bss
	.lcomm SAVJMP, 4

	.section .text
# TBLDSP - Dispatch to I/O routine for device-table [DI]
# Entry - [AH]=function code (G.OPN, G.CLS, etc.)
#         DI = 4 * device id (0=disk, 4=keyboard etc.)
# Exit  - All registers preserved except those changed by device driver
#
TBLDSP:	push edi
	pushfd
	push ax
	mov eax, DWORD PTR DEVPTR	# AX points to 1st entry in dispatch table
	add edi, eax			# DI points to dispatch table pointer for device
	mov edi, DWORD PTR [edi]	# DI points to dispatch table for device
	or edi, edi			# If entry is 0, then
	jz ERDNA2			#   Error - device not available
	pop ax				# restore function code
	push ax
	xchg ah, al
	cbw				# [AX]=function code
	cwde				# [EAX]=function code (32-bit)
	shl eax, 1			# byte offset: G codes count 2 per 4-byte entry (G*2)
	add edi, eax			# Add function code offset to dispatch
	mov eax, DWORD PTR [edi]	# Get address of routine
	mov DWORD PTR IOJUMP, eax
	pop ax
	popfd
	pop edi
	jmp DWORD PTR IOJUMP		# jump to routine at address [IOJUMP]

# CDEVID - convert device id to dispatch table address
# Entry - [AL]=device id (0..n for disk, -1..-n for non-disk devices)
# Exit  - [DI]=DEVPTR offset for device (0 for disk, 4 for KYBD, etc.)
#         FLAGS.Z is true if device=disk
#         All other registers are preserved.
#
CDEVID:	push ax
	or al, al
	js CDEV1			# must be a special device
	xor al, al			# (AL) = 0 for disks
	jmp CDEV2
CDEV1:	neg al				# (AL) = - device number for special
CDEV2:	pushfd				# save Z=disk
	cbw				# [AX]=device number
	movzx edi, ax
	shl edi, 2			# * 4 (entries are 4 bytes; the original was * 2)
	popfd				# restore Z=disk
	pop ax
	ret

# ERDNA2 - device not available error
ERDNA2:	jmp DERDNA

# ---------------------------------------------------------------- PRINT helpers
	.globl PTRGPS, PTRWDC, PTRWID, ADRGET

# PTRGPS - get column position for file PTRFIL
#
# Exit  - [AH] = [AL] = column position
#         FLAGS.Z is true if at column 0
#         All other registers are preserved
#
PTRGPS:	mov ah, G_GPS			# MOV AH,LOW OFFSET G_GPS: Get column position request-code
	call PTRDSP			# [AH]=position
	mov al, ah			# [AL]=position
	or al, al
	ret

# PTRWDC - get line width and last comma column for file PTRFIL
# Exit  - [CH] = line width
#         [AH] = [CL] = last comma column
#         All other registers are preserved
#
PTRWDC:	call PTRWID			# [AH]=file PTRFIL's line width
	mov ch, ah			# [CH]=file width
PTRWDL:	sub ah, 14			# T.B.S. eventually, use G.GCW (LOW 14D)
	jae PTRWDL
	add ah, 28			# LOW 28D
	neg ah
	add ah, ch
	mov cl, ah			# [CL]=last comma column
	ret

# PTRWID - get line width for file PTRFIL
# Exit  - [AH] = line width
#         All other registers are preserved
#
PTRWID:	push esi			# save caller's SI
	mov esi, DWORD PTR PTRFIL
	cmp esi, PSEUDO_SAV		# the ASCII SAVE file is not an FDB: ask its device (SAV_WID)
	je PTRWD1
	inc esi
	jz PTRWD1			# if LPT Pseudo FDB, use Device width
	dec esi
	jz PTRWD1			# if KYBD Pseudo FDB, use Device width
	mov ah, BYTE PTR [esi+F_WID]	# else use FDB width
	pop esi				# restore caller's SI
	ret
PTRWD1:	pop esi				# restore caller's SI
	mov ah, G_GWD			# MOV AH,LOW OFFSET G_GWD: Get file width function code
	jmp PTRDSP			# return with [AH]=device width

# ADRGET - parse 16 bit expression
# Entry - [BX]=text pointer
# Exit  - [DX]=result (0..65535)
#         [BX]=updated text pointer
#         AX used, other registers preserved.
#
ADRGET:	push cx
	call FRMEVL
	push ebx			# text pointer (4 bytes)
	call FRQINT			# Make Unsigned 16 bits (value in BX)
	pop edx				# text pointer comes back in EDX
	xchg ebx, edx			# Offset in [DX], text pointer in [BX]
	pop cx
	ret

# ---------------------------------------------------------------- register-restore epilogue
	.globl PDCBAX, PCBAX, PBAX

# PDCBAX - POP DX,CX,BX,AX and RET. Callers push AX (2), BX (4, pointer), CX (2), DX (2)
# in that order, so the pops are 2, 2, 4, 2 bytes (KYBINI in giokyb.S pushes the same way).
PDCBAX:	pop dx
PCBAX:	pop cx
PBAX:	pop ebx				# BX is a pointer: 4 bytes
	pop ax
	ret

# ---------------------------------------------------------------- UNSUPPORTED entries
# Out of scope for this stage. Each entry jumps to ADVERR ("Advanced feature"). The original
# bodies are kept in the comments or cited by line in GIO86.ASM / GIODSK.ASM / GIOLPT.ASM /
# GENGRP.ASM (original source tree, not built).

# DEVBIN - block input from a file on a device (BSAVE/BLOAD binary I/O).
# Original (GIO86.ASM 1120):
#   DEVBIN: OR CX,CX / JZ RET27 / MOV AH,G_SIN / CALL PTRDSP / JB RET27 /
#           store AL at [BX], INC BX, DEC CX, JMP DEVBIN
	.globl DEVBIN
DEVBIN:	jmp ADVERR			# UNSUPPORTED: block input (BSAVE/BLOAD binary file I/O); no file I/O in this stage

# DEVBOT - block output to a file or device (BSAVE and block PUT).
# Original (GIO86.ASM 1144):
#   DEVBOT: OR CX,CX / JZ RET27 / load [BX] / CALL OUTDO / INC BX / DEC CX / JMP DEVBOT
	.globl DEVBOT
DEVBOT:	jmp ADVERR			# UNSUPPORTED: block output (BSAVE/BLOAD binary file I/O); no file I/O in this stage

# INCHSI - get next byte from file SI (CTL Z = end-of-file). Original: GIO86.ASM 978-1000.
# The keyboard branch (SI=0) goes to INCHRI in the original; file branch calls SIDSP with G_SIN.
	.globl INCHSI
INCHSI:	jmp ADVERR			# UNSUPPORTED: sequential file input (INPUT#, LINE INPUT#); no file input in this stage

# BCHRSI - backup sequential input file. Original (GIO86.ASM 1084):
#   BCHRSI: MOV BYTE PTR F_BAKC[SI],AL / OR BYTE PTR F_FLGS[SI],FL_BKC / RET
	.globl BCHRSI
BCHRSI:	jmp ADVERR			# UNSUPPORTED: backup of sequential file input; no file input in this stage

# INCHR (the next byte of the MERGE file) is in bifile.S.

# INIFDB - allocate and link a file-data-block for OPEN. Original: GIO86.ASM 1683-1712.
# Only OPEN reaches it (KYBOPN, CONOPN, SCNOPN in the device tables), and OPEN is ADVERR.
	.globl INIFDB
INIFDB:	jmp ADVERR			# UNSUPPORTED: OPEN (file-data-block allocation); OPEN is ADVERR (stage3stubs.S)

# DSKDSP - disk dispatch table (GIODSK.ASM 51). The entry in _DVPTR (scrout.S) is a table of
# 15 function entries, so the entries are ADVERR rather than a single jump. No disk device exists
# in this stage because OPEN is ADVERR.
	.section .rodata
	.globl DSKDSP
DSKDSP:
	.long ADVERR			# test EOF for file opened to this device (UNSUPPORTED)
	.long ADVERR			# LOC (UNSUPPORTED)
	.long ADVERR			# LOF (UNSUPPORTED)
	.long ADVERR			# perform special CLOSE functions for this device (UNSUPPORTED)
	.long ADVERR			# set device width (UNSUPPORTED)
	.long ADVERR			# GET/PUT random record from/to this device (UNSUPPORTED)
	.long ADVERR			# perform special OPEN functions for this device (UNSUPPORTED)
	.long ADVERR			# input 1 byte from file opened on this device (UNSUPPORTED)
	.long ADVERR			# output 1 byte to file opened on this device (UNSUPPORTED)
	.long ADVERR			# POS (UNSUPPORTED)
	.long ADVERR			# get device width (UNSUPPORTED)
	.long ADVERR			# set device comma width (UNSUPPORTED)
	.long ADVERR			# get device comma width (UNSUPPORTED)
	.long ADVERR			# block input from file opened on this device (UNSUPPORTED)
	.long ADVERR			# block output to file opened on this device (UNSUPPORTED)

# LPTDSP - line printer dispatch table (GIOLPT.ASM 37). Printer output is deferred (see
# gio86prt.S, LPRINT). 15 function entries, all ADVERR.
	.globl LPTDSP
LPTDSP:
	.long ADVERR			# test EOF (UNSUPPORTED: printer)
	.long ADVERR			# LOC (UNSUPPORTED: printer)
	.long ADVERR			# LOF (UNSUPPORTED: printer)
	.long ADVERR			# perform special CLOSE functions (UNSUPPORTED: printer)
	.long ADVERR			# set device width (UNSUPPORTED: printer)
	.long ADVERR			# GET/PUT random record (UNSUPPORTED: printer)
	.long ADVERR			# perform special OPEN functions (UNSUPPORTED: printer)
	.long ADVERR			# input 1 byte (UNSUPPORTED: printer)
	.long ADVERR			# output 1 byte (UNSUPPORTED: printer)
	.long ADVERR			# POS (UNSUPPORTED: printer)
	.long ADVERR			# get device width (UNSUPPORTED: printer)
	.long ADVERR			# set device comma width (UNSUPPORTED: printer)
	.long ADVERR			# get device comma width (UNSUPPORTED: printer)
	.long ADVERR			# block input (UNSUPPORTED: printer)
	.long ADVERR			# block output (UNSUPPORTED: printer)

	.section .text

# LPTINI - called by GIOINI (DOALDV) for each printer during BASIC initialization. Original
# (GIOLPT.ASM 73): sets the LPT device control block to width 80, position 0, flags 0.
# Not ADVERR: an ADVERR here would fail the whole initialization. Printer state is not used
# in this stage, so the call returns with nothing to do.
	.globl LPTINI
LPTINI:	ret

# LPTTRM - called by GIOTRM (DOALDV). Original (GIOLPT.ASM 54): RET.
	.globl LPTTRM
LPTTRM:	ret

# GRPINI - center the graphics cursor. Original (GENGRP.ASM 555-567):
#   GRPINI: CALL GRPSIZ / X=C+1 / SHR X,1 -> GRPACX / Y=D+1 / SHR Y,1 -> GRPACY / RET
# UNSUPPORTED: needs GRPSIZ (OEM screen pixel size, not in the source tree). Its only caller in
# src/ is SCNCLR (scrout.S), which has no caller in src/.
	.globl GRPINI
GRPINI:	jmp ADVERR			# UNSUPPORTED: graphics cursor (needs OEM GRPSIZ); graphics is out of scope
