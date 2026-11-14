# GWINIT.ASM -> gwinit.S
# System initialization: sets up the memory map and the stack, initializes the RAM
# flags, and prints the heading. The DOS and CP/M parts are replaced.
#
# Replaced or dropped (original is DOS / CP/M specific):
#  - Segment setup (MOV SS / DS / ES), CLI/STI, and the DOS exit vector: flat address space.
#  - CP/M version check.
#  - Command-line parsing for /M: /C: /F: /S: and the DOS segment size (SEGOFF, MAPCLC):
#    memory size is a build-time constant (MEMSIZE). The command line is empty, so
#    TEMP8 points at a zero byte and no program file is loaded.
#  - The copy CNSLEN+3 bytes from CONSTR to RAMLOW is a no-op here: the two share an
#    address (see gwdata.S), so it is omitted.
#
# Kept: the memory map (TXTTAB, STKLOW, FILTAB, MEMSIZ, TOPMEM, FRETOP, SP), the variable
# initialization, and the heading logic. Pointers are 32-bit, so PUSH/POP of a pointer is 4 bytes.

	.intel_syntax noprefix

	.section .text
	.globl INIT, CMDERR

# External routines (device layers in stage 4; others in stage 3):
#   SCNIPL, GWINI, SNDRST, GIOINI, STKINI, REASON, GETHED, STROUT, SKEYON,
#   OMERRR, SYSTME, INITSA.
# External data: HEDING, CERMSG (gwram.S / gwdata.S).

INIT:
	# Temporary stack in BUF, used during initialization (the original used BUF+128).
	mov ebx, offset BUF + 128
	mov MEMSIZ, ebx
	mov esp, ebx

	xor al, al
	mov BYTE PTR PROFLG, al
	mov BYTE PTR MSWFLG, al		# /M: flag (no effect: memory size is fixed)
	mov BYTE PTR CSWFLG, al		# /C: flag

	call SCNIPL			# screen editor initialization (stage 4)
	call GWINI			# OEM-specific initialization
	call SNDRST			# reset sound queue, disable speaker (stage 4)
	call GIOINI			# I/O initialization (stage 4)

	mov ebx, MEMSIZ
	mov TOPMEM, ebx
	mov ebx, offset KBUF - 1	# KBUF-1 holds a colon: direct input restarts OK
	mov BYTE PTR [ebx], 58		# ':'
	call STKINI			# set up the temporary stack (stage 3)

	xor al, al
	mov BYTE PTR CNTOFL, al
	mov BYTE PTR ENDBUF, al		# make sure overruns stop
	mov BYTE PTR DSEGZ, al		# DS-located zero
	mov BYTE PTR CHNFLG, al		# CHAIN and MERGE do not happen
	mov BYTE PTR MRGFLG, al
	mov BYTE PTR ERRFLG, al		# EDIT is not called on errors

	mov ebx, offset TEMPST
	mov TEMPPT, ebx
	mov ebx, offset PRMSTK		# parameter block chain
	mov PRMPRV, ebx

	# Memory size: the first byte past user memory (replaces the DOS SEGOFF call).
	mov ebx, offset TSTACK + MEMSIZE
	mov MEMSIZ, ebx
	mov MAXMEM, ebx			# maximum size for CLEAR

	# No command line: TEMP8 points at the DS zero byte, so there is no file.
	mov ebx, offset DSEGZ
	mov TEMP8, ebx

	# DONCMD: copy the command-line file name (empty) to BUF, then point TEMP8 at it.
	mov edx, offset BUF
	mov TEMP8, edx
NXTBYT:
	mov al, BYTE PTR [ebx]		# source byte
	xchg ebx, edx
	mov BYTE PTR [ebx], al		# destination byte
	inc ebx
	xchg ebx, edx
	inc ebx
	or al, al
	jnz NXTBYT

	# Memory map. MEMSIZ is lowered by one (top byte kept unused: VAL writes a zero past
	# a string), and the stack top (one lower again) is pushed as a 4-byte pointer.
	mov ebx, MEMSIZ
	dec ebx
	mov MEMSIZ, ebx
	dec ebx				# one lower is the stack top
	push ebx

	mov ebx, offset TSTACK		# program text starts at TSTACK; TXTTAB-1 (LSTVAR) holds CR
	mov TXTTAB, ebx			# bottom of memory (the 4-byte end link is written by INITSA)

	pop edx				# stack top (pointer)
	and edx, 0xFFFFFFFE		# even stack pointer (kept for fidelity)

	# Total free bytes / 8: DX - BX (error if negative).
	mov eax, edx
	sub eax, ebx
	jae 2f
	jmp OMERRR
2:
	mov ebx, eax
	shr ebx, 3
	cmp ebx, 512			# less than 512: use one eighth
	jb 3f
	mov ebx, 512
3:
	mov eax, edx			# subtract the stack size from top memory
	sub eax, ebx
	jae 4f
	jmp OMERRR
4:
	mov ebx, eax
	mov STKLOW, ebx			# lowest legal value for SP
	mov FILTAB, ebx			# initially there are no FDBs
	dec ebx
	mov BYTE PTR [ebx], 0		# string space is terminated by 0 for VAL
	dec ebx
	mov MEMSIZ, ebx			# highest byte used by strings

	xchg ebx, edx			# EBX = top of memory, EDX = MEMSIZ
	mov TOPMEM, ebx
	mov FRETOP, ebx			# REASON uses this
	mov esp, ebx			# set up the new stack
	mov SAVSTK, ebx
	mov ebx, TXTTAB
	xchg ebx, edx			# EBX = MEMSIZ, EDX = TXTTAB
	call REASON

	xor al, al
	mov BYTE PTR FREFLG, al		# clear: print the free bytes message

	mov al, 255			# if the heading is printed, show the function keys too
	mov BYTE PTR KEYSW, al
	call GETHED			# OEM heading; EBX = OEM heading pointer, flags as original
	jnz PRNTIT			# always print the heading
	push ebx			# keep the OEM heading pointer
	mov ebx, TEMP8
	mov al, BYTE PTR [ebx]		# test for a file on the command line
	pop ebx
	or al, al
	jz PRNTIT			# no program: print the heading
	mov BYTE PTR FREFLG, al		# inhibit the free bytes message
	xor al, al			# keys off if there is a program
	mov BYTE PTR KEYSW, al
	jmp PRNTND

PRNTIT:
	call STROUT			# print the OEM heading (EBX)
	mov ebx, offset HEDING		# "BASIC VERSION..." heading
	call STROUT
PRNTND:
	mov al, BYTE PTR KEYSW		# function key display switch
	or al, al
	jnz 5f
	jmp KEYSOF			# leave keys off
5:
	xor al, al
	mov BYTE PTR KEYSW, al
	call SKEYON			# function key display on
KEYSOF:
	mov al, 255
	mov BYTE PTR INITFG, al		# initialization complete: errors no longer exit to the OS
	jmp INITSA

# CMDERR: an error before initialization completes. Print the heading and the
# command-line error message, then exit to the OS.
CMDERR:
	mov ebx, offset HEDING
	call STROUT
	mov ebx, offset CERMSG
	call STROUT
	jmp SYSTME
