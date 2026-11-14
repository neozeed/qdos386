# itsa.S -> ITSA86.ASM (INITSA, MAPINI, WORDS) and the exit path (SYSTME, GIOTRM, GWTERM).
#
# INITSA: the original moved DS and SS to the final memory map (MAPINI), wrote the end-of-program
# marker, checked the command line for a file to run, and then fell into READY.
#   - MAPINI: the segment move is not needed in the flat address space. INIT has already placed the
#     stack and the memory map, so MAPINI does nothing here.
#   - End-of-program marker: CR (0x0D) in the byte before the program text (TXTTAB-1), then a
#     four-byte zero link at TXTTAB (the original wrote a two-byte zero link). CHAIN and the
#     program walkers stop at a zero link.
#   - The command line is always empty in this port, so LRUN is not reached.
#
# SYSTME: return to the operating system. The original closed the files and terminated the
# devices, then exited through the MS-DOS vector. Here the console is flushed and the process ends.

	.intel_syntax noprefix
	.section .text
	.globl INITSA, MAPINI, SYSTME, GIOTRM, GWTERM

# ---------------------------------------------------------------- MAPINI
MAPINI:
	ret

# ---------------------------------------------------------------- INITSA
INITSA:
	call NODSKS			# no disk: nothing to do (BIMISC)
	call MAPINI
	mov ebx, DWORD PTR TXTTAB
	dec ebx
	mov BYTE PTR [ebx], 13		# CR before the program text, as the original's 0D
	mov ebx, DWORD PTR TXTTAB
	mov DWORD PTR [ebx], 0		# end-of-program link (four bytes)
	mov ebx, DWORD PTR TEMP8	# the command line
	mov al, BYTE PTR [ebx]
	or al, al
	jz GREADY			# no file named: go to READY
	jmp LRUN			# a file was named on the command line
GREADY:
	jmp READY

# ---------------------------------------------------------------- SYSTME
SYSTME:
	call GIOTRM
	call GWTERM
	xor eax, eax			# exit code 0
	jmp PLAT_EXIT

# GIOTRM: device termination. The console has nothing to restore on Win32 except that
# buffered output reaches the terminal. Files are closed by CLSALL (file adapter, later).
GIOTRM:
	call PLAT_FLUSH
	ret

# GWTERM: OEM termination. Nothing to do on Win32.
GWTERM:
	ret
