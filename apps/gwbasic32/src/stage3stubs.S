# stage3stubs.S: Stage 3i link stubs (see NOTES.md, Stage 3 integration).
# Nothing here is a working Stage 4 routine. Names the Stage 3 modules call but that are not yet
# implemented. Statements and functions outside scope, and the Stage 4 input and file routines,
# jump to ADVERR ("Advanced feature") so an unimplemented path fails loudly. Output and init routines
# are silent returns. Device status routines return zero.

	.intel_syntax noprefix
	.include "gwconst.inc"		# ERRMMM as an immediate (was a memory load from address 65)
	.section .text

	.globl BLOAD
BLOAD:
	jmp ADVERR			# not implemented in this stage

	.globl BSAVE
BSAVE:
	jmp ADVERR			# not implemented in this stage

	.globl CIRCLE
CIRCLE:
	jmp ADVERR			# not implemented in this stage

	.globl CLOSE
CLOSE:
	jmp ADVERR			# not implemented in this stage

	.globl DRAW
DRAW:
	jmp ADVERR			# not implemented in this stage

	.globl FIELD
FIELD:
	jmp ADVERR			# not implemented in this stage

	.globl KILL
KILL:
	jmp ADVERR			# not implemented in this stage

	.globl LSET
LSET:
	jmp ADVERR			# not implemented in this stage

	.globl RSET
RSET:
	jmp ADVERR			# not implemented in this stage

	.globl MOTOR
MOTOR:
	jmp ADVERR			# not implemented in this stage

	.globl NAME
NAME:
	jmp ADVERR			# not implemented in this stage

	.globl OPEN
OPEN:
	jmp ADVERR			# not implemented in this stage

	.globl PAINT
PAINT:
	jmp ADVERR			# not implemented in this stage

	.globl POINT
POINT:
	jmp ADVERR			# not implemented in this stage

	.globl PRESET
PRESET:
	jmp ADVERR			# not implemented in this stage

	.globl PSET
PSET:
	jmp ADVERR			# not implemented in this stage

	.globl RESET
RESET:
	jmp ADVERR			# not implemented in this stage

	.globl WIDTHS
WIDTHS:
	jmp ADVERR			# not implemented in this stage

	.globl LOC
LOC:
	jmp ADVERR			# not implemented in this stage

	.globl LOF
LOF:
	jmp ADVERR			# not implemented in this stage

	.globl EOF
EOF:
	jmp ADVERR			# not implemented in this stage

	.globl MKI_
MKI_:
	jmp ADVERR			# not implemented in this stage

	.globl MKS_
MKS_:
	jmp ADVERR			# not implemented in this stage

	.globl MKD_
MKD_:
	jmp ADVERR			# not implemented in this stage

	.globl CVI
CVI:
	jmp ADVERR			# not implemented in this stage

	.globl CVS
CVS:
	jmp ADVERR			# not implemented in this stage

	.globl CVD
CVD:
	jmp ADVERR			# not implemented in this stage

	.globl JISFN
JISFN:
	jmp ADVERR			# not implemented in this stage

	.globl KLENFN
KLENFN:
	jmp ADVERR			# not implemented in this stage

	.globl KPOSFN
KPOSFN:
	jmp ADVERR			# not implemented in this stage

	.globl KTNFN
KTNFN:
	jmp ADVERR			# not implemented in this stage

	.globl GETPTR
GETPTR:
	jmp ADVERR			# not implemented in this stage

	.globl LINGT3
LINGT3:
	jmp ADVERR			# not implemented in this stage

	.globl LPOS
LPOS:
	jmp ADVERR			# not implemented in this stage

	.globl FILGET
# FILGET: file-number lookup for PRINT, WRITE and similar statements. Entry: EBX = text pointer
# (CHRGTR style), CL = mode. Without '#' the screen is the file (PTRFIL=0) and EBX is unchanged.
# A '#' names a disk file: file I/O is deferred to the Win32 file adapter, so it stops with ADVERR.
FILGET:
	push ebx
	dec ebx				# the text pointer is on the item's first character (as in PRINT's
	call CHRGTR			# NEWCHR and WRITE), so back up before reading: CHRGTR then returns that
					# first character without consuming it. Reading past it (CHRGTR alone) saw
					# the character after "A" in PRINT A#, and wrongly sent it to ADVERR.
	cmp al, '#'
	pop ebx				# restore the text pointer (pop does not change the flags)
	jz FILGET_HASH
	mov DWORD PTR PTRFIL, 0		# screen / keyboard pseudo-FDB
	ret
FILGET_HASH:
	jmp ADVERR			# UNSUPPORTED: '#' file number (disk file I/O deferred)

	.globl FILIND
FILIND:
	jmp ADVERR			# not implemented in this stage

	.globl FILINP
FILINP:
	jmp ADVERR			# not implemented in this stage

	.globl PRGFIN
PRGFIN:
	ret				# finish output of a file: no file is open in this build (file adapter deferred)

	.globl PRGFLI
PRGFLI:
	ret				# no file in progress in this build (file adapter deferred)

	.globl DLINE
DLINE:
	jmp ADVERR			# not implemented in this stage


	.globl LRUN
LRUN:
	jmp ADVERR			# not implemented in this stage

	.globl OKGETM
OKGETM:
	jmp ADVERR			# not implemented in this stage

	.globl CHNENT
CHNENT:
	jmp ADVERR			# not implemented in this stage

	.globl CMPFBC
CMPFBC:
	jmp ADVERR			# not implemented in this stage

	.globl FIXINP
FIXINP:
	jmp ADVERR			# not implemented in this stage

	.globl NAMSCN
NAMSCN:
	jmp ADVERR			# not implemented in this stage

	.globl FINLPT
FINLPT:
	ret				# print any left-over LPRINT output: no printer in this build (LPRINT deferred)

	.globl DONOTE
DONOTE:
	ret				# Stage 4 device stub: no output / no effect

	.globl DPUTG
DPUTG:
	ret				# Stage 4 device stub: no output / no effect

	.globl FKYFMT
FKYFMT:
	ret				# Stage 4 device stub: no output / no effect

	.globl GETFBC
GETFBC:
	ret				# Stage 4 device stub: no output / no effect

	.globl GRPRST
GRPRST:
	ret				# Stage 4 device stub: no output / no effect

	.globl SETFBC
SETFBC:
	ret				# Stage 4 device stub: no output / no effect

	.globl NULOPM
NULOPM:
	ret				# Stage 4 device stub: no output / no effect

	.globl PEKFLT
PEKFLT:
	ret				# Stage 4 device stub: no output / no effect

	.globl POKFLT
POKFLT:
	ret				# Stage 4 device stub: no output / no effect

	.globl POLCOM
POLCOM:
	xor al, al			# Stage 4 device stub: no key, no event
	ret

	.globl POLLEV
POLLEV:
	xor al, al			# Stage 4 device stub: no key, no event
	ret

	.globl RDSTIK
RDSTIK:
	xor al, al			# Stage 4 device stub: no key, no event
	ret

	.globl RDTRIG
RDTRIG:
	xor al, al			# Stage 4 device stub: no key, no event
	ret

	.globl DEVGDAT
DEVGDAT:
	xor cx, cx
	xor dx, dx
	ret				# Stage 4: date (year, month, day) = 0

	.globl DEVGTIM
DEVGTIM:
	xor cx, cx
	xor dx, dx
	ret				# Stage 4: time = 0

	.globl DEVSDAT
DEVSDAT:
	xor al, al			# Stage 4: date accepted
	ret

	.globl DEVSTIM
DEVSTIM:
	xor al, al			# Stage 4: time accepted
	ret


	.section .text
	.globl CLSALL
CLSALL:
	ret				# close all files: none can be open in this build (file adapter deferred)

	.globl DIRDO
# DIRDO: a direct statement (a line without a number) runs. From a file it is an error
# ("Direct statement in file"); from the terminal (PTRFIL=0) it runs at NEWSTT.
DIRDO:	call ISFLIO			# ZF=0 when input or output is a file
	jnz DIRDO_FILE
	jmp NEWSTT
DIRDO_FILE:
	mov edx, ERRMMM			# "Direct statement in file" (as the other error callers: the whole EDX)
	jmp ERROR


	.section .text

	.globl FINPRT
FINPRT:
	# Clear PTRFIL so the console (0) is the output again: after LLIST, PTRFIL is the printer, and
	# without this the error message (STKERR calls FINPRT) goes to the printer, whose ADVERR starts
	# the error again, forever. There is no file to close here: the printer and OPEN are unsupported.
	mov DWORD PTR PTRFIL, 0
	ret

	.globl FKYADV
FKYADV:
	ret				# function-key line: the status line is outside the window (SCNSWI sets WDOBOT=LINCNT-1), so
				# the key labels stay on the shadow screen and nothing is drawn on the terminal
