# platform_win32.S: the Win32 console and stdio boundary. This is the only place where the
# port calls the operating system for terminal I/O. The translated interpreter calls these
# routines instead of BIOS or DOS. A QDOS/386 port replaces this file; nothing else changes.
#
# Interface. Every routine preserves all registers and the flags unless stated otherwise.
#   PLAT_INIT     : get the standard handles. If stdin is a console, switch it to raw mode
#                   (no line input, no echo, no processed input) so that the interpreter's own
#                   line editor does the echo and Ctrl-C arrives as character 3 (BASIC break).
#   PLAT_CHOUT    : AL = character to write to the terminal (buffered; flushed when full).
#   PLAT_FLUSH    : write all buffered output now.
#   PLAT_CHIN     : blocking read of one character. Flushes output first. Returns AL = character
#                   with CF=0. At end of input returns AL=0 with CF=1 (flags are the only change).
#   PLAT_CHAVAIL  : non-blocking test. ZF=0 when PLAT_CHIN will not wait; ZF=1 otherwise.
#   PLAT_EXIT     : EAX = exit code. Flushes output and terminates the process. Does not return.
#
# Known limit: for a console, PLAT_CHAVAIL reports ready when any input event is queued, so a
# non-key event (for example a mouse or focus event) can make PLAT_CHIN wait for the next key.

	.intel_syntax noprefix

	.extern _GetStdHandle@4
	.extern _WriteFile@20
	.extern _ReadFile@20
	.extern _ExitProcess@4
	.extern _GetConsoleMode@8
	.extern _SetConsoleMode@8
	.extern _GetNumberOfConsoleInputEvents@8
	.extern _PeekNamedPipe@24
	.extern _CreateFileA@28
	.extern _GetFileSize@8
	.extern _CloseHandle@4
	.extern _GetLastError@0

	.set PLAT_OBSIZE, 4096
	.set PLAT_IBSIZE, 1024

	.section .text

# ---------------------------------------------------------------- PLAT_INIT
	.globl PLAT_INIT
PLAT_INIT:
	push eax
	push ebx
	push ecx
	push -10			# STD_INPUT_HANDLE
	call _GetStdHandle@4
	mov DWORD PTR plat_hin, eax
	push -11			# STD_OUTPUT_HANDLE
	call _GetStdHandle@4
	mov DWORD PTR plat_hout, eax
	# Raw mode for a console: clear LINE_INPUT (2), ECHO_INPUT (4), PROCESSED_INPUT (1).
	lea ebx, plat_mode
	push ebx
	push DWORD PTR plat_hin
	call _GetConsoleMode@8
	test eax, eax
	jz 1f				# not a console (pipe or file): nothing to change
	mov eax, DWORD PTR plat_mode
	and eax, 0xFFFFFFF8
	mov DWORD PTR plat_mode, eax
	push DWORD PTR plat_mode
	push DWORD PTR plat_hin
	call _SetConsoleMode@8
	mov BYTE PTR plat_isconsole, 1
1:	pop ecx
	pop ebx
	pop eax
	ret

# ---------------------------------------------------------------- PLAT_FLUSH
	.globl PLAT_FLUSH
PLAT_FLUSH:
	push eax
	push ebx
	push ecx
	push edx
	mov ecx, DWORD PTR plat_olen
	test ecx, ecx
	jz 1f
	push 0				# lpOverlapped
	push OFFSET plat_written	# lpNumberOfBytesWritten
	push ecx			# nNumberOfBytesToWrite
	push OFFSET plat_obuf		# lpBuffer
	push DWORD PTR plat_hout
	call _WriteFile@20
	mov DWORD PTR plat_olen, 0
1:	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# ---------------------------------------------------------------- PLAT_CHOUT
# AL = character. The buffer is written out when it is full.
	.globl PLAT_CHOUT
PLAT_CHOUT:
	push eax			# saved AL is at [esp+12] after the three pushes below
	push ebx
	push ecx
	push edx
	mov ecx, DWORD PTR plat_olen
	cmp ecx, PLAT_OBSIZE
	jb 1f
	call PLAT_FLUSH			# preserves all registers
	xor ecx, ecx
	mov al, BYTE PTR [esp+12]	# reload the character from the saved EAX
1:	lea ebx, plat_obuf
	mov BYTE PTR [ebx+ecx], al
	inc ecx
	mov DWORD PTR plat_olen, ecx
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# ---------------------------------------------------------------- plat_fill (internal)
# One ReadFile into the input buffer. Sets plat_ilen to 0 at end of input.
plat_fill:
	push eax
	push ebx
	push ecx
	push edx
	push 0				# lpOverlapped
	push OFFSET plat_read		# lpNumberOfBytesRead
	push PLAT_IBSIZE		# nNumberOfBytesToRead
	push OFFSET plat_ibuf		# lpBuffer
	push DWORD PTR plat_hin
	call _ReadFile@20
	mov DWORD PTR plat_ipos, 0
	test eax, eax
	jz 1f				# read failed: end of input
	mov eax, DWORD PTR plat_read
	mov DWORD PTR plat_ilen, eax
	jmp 2f
1:	mov DWORD PTR plat_ilen, 0
2:	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# ---------------------------------------------------------------- PLAT_CHIN
# Returns AL = next character with CF=0, or AL=0 with CF=1 at end of input.
# Line ends: a console sends CR for Enter, and piped text may send CR LF or LF. LF is returned
# as CR (the BASIC line terminator), and the LF of a CR LF pair is dropped.
		.globl PLAT_CHIN
PLAT_CHIN:
		push ebx
		push ecx
		push edx
		call PLAT_FLUSH			# the prompt must be visible before we wait
1:		mov ecx, DWORD PTR plat_ipos
		cmp ecx, DWORD PTR plat_ilen
		jb 2f
		call plat_fill
		xor ecx, ecx
		cmp DWORD PTR plat_ilen, 0
		jz 3f				# end of input
2:		lea ebx, plat_ibuf
		mov al, BYTE PTR [ebx+ecx]
		inc ecx
		mov DWORD PTR plat_ipos, ecx
		cmp al, 10			# LF?
		jne 4f
		cmp BYTE PTR plat_lastcr, 0
		je 5f				# LF alone: a line end, returned as CR
		mov BYTE PTR plat_lastcr, 0	# LF after CR: the second half of CR LF, dropped
		jmp 1b
5:		mov al, 13
4:		mov dl, 0
		cmp al, 13
		jne 6f
		mov dl, 1
6:		mov BYTE PTR plat_lastcr, dl
		clc
		jmp 7f
3:		xor eax, eax
		stc
7:		pop edx
		pop ecx
		pop ebx
		ret

# ---------------------------------------------------------------- PLAT_CHAVAIL
	.globl PLAT_CHAVAIL
PLAT_CHAVAIL:
	push eax
	push ebx
	push ecx
	push edx
	mov ecx, DWORD PTR plat_ipos
	cmp ecx, DWORD PTR plat_ilen
	jb plat_av_yes			# characters already buffered
	mov DWORD PTR plat_avail, 0
	cmp BYTE PTR plat_isconsole, 0
	jz plat_av_pipe
	# Console: any queued input event counts as ready (see the known limit above).
	push OFFSET plat_avail
	push DWORD PTR plat_hin
	call _GetNumberOfConsoleInputEvents@8
	cmp DWORD PTR plat_avail, 0
	jnz plat_av_yes
	jmp plat_av_no
plat_av_pipe:
	# Redirected input (pipe or file): PLAT_CHIN blocks for data and reports end of input, so the
	# poll reports ready. (PeekNamedPipe reported no bytes for a piped stdin under Wine, which
	# left the editor polling forever.)
	jmp plat_av_yes
plat_av_no:
	xor eax, eax
	test eax, eax			# ZF=1: not ready
	jmp plat_av_end
plat_av_yes:
	mov eax, 1
	test eax, eax			# ZF=0: ready
plat_av_end:
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# ---------------------------------------------------------------- file routines (Stage 4)
# Program files are read and written whole with the Win32 file API. A name is a zero-terminated
# string, relative to the process's current directory.
#   PLAT_FOPEN  : EBX = name, EDX = 0 to read (the file must exist), nonzero to create or replace it
#                 for writing. CF=0 and EAX = handle, or CF=1 and EAX = Win32 error code.
#   PLAT_FSIZE  : EBX = handle. CF=0 and EAX = size in bytes (2 GB or more is an error), or CF=1.
#   PLAT_FREAD  : EBX = handle, ECX = count, EDX = buffer. CF=0 only if all COUNT bytes were read.
#                 A short read is CF=1 with EAX = 38 (ERROR_HANDLE_EOF).
#   PLAT_FWRITE : EBX = handle, ECX = count, EDX = buffer. CF=0 only if all bytes were written.
#   PLAT_FCLOSE : EBX = handle. Always returns.
# EBX, ECX, EDX, ESI, EDI and EBP are preserved (FCLOSE may change ECX and EDX); EAX carries results.
	.globl PLAT_FOPEN, PLAT_FSIZE, PLAT_FREAD, PLAT_FWRITE, PLAT_FCLOSE
PLAT_FOPEN:
	push ecx
	push edx
	push 0				# hTemplateFile
	push 128			# FILE_ATTRIBUTE_NORMAL
	test edx, edx
	jnz plat_fo_w
	push 3				# OPEN_EXISTING
	push 0				# lpSecurityAttributes
	push 1				# FILE_SHARE_READ
	push 0x80000000			# GENERIC_READ
	jmp plat_fo_go
plat_fo_w:
	push 2				# CREATE_ALWAYS
	push 0				# lpSecurityAttributes
	push 0				# no sharing while the file is being written
	push 0x40000000			# GENERIC_WRITE
plat_fo_go:
	push ebx			# lpFileName
	call _CreateFileA@28
	cmp eax, -1			# INVALID_HANDLE_VALUE
	je plat_fo_err
	pop edx
	pop ecx
	clc
	ret
plat_fo_err:
	call _GetLastError@0
	pop edx
	pop ecx
	stc
	ret

PLAT_FSIZE:
	push ecx
	push edx
	mov DWORD PTR plat_fhigh, 0
	push OFFSET plat_fhigh		# lpFileSizeHigh
	push ebx			# hFile
	call _GetFileSize@8
	cmp eax, -1
	je plat_fs_err
	cmp DWORD PTR plat_fhigh, 0
	jne plat_fs_big
	test eax, eax
	js plat_fs_big			# 2 GB or more
	pop edx
	pop ecx
	clc
	ret
plat_fs_big:
	mov eax, 223			# ERROR_FILE_TOO_LARGE
	jmp plat_fs_fail
plat_fs_err:
	call _GetLastError@0
plat_fs_fail:
	pop edx
	pop ecx
	stc
	ret

PLAT_FREAD:
	push ecx
	push edx
	push 0				# lpOverlapped
	push OFFSET plat_fxfer		# lpNumberOfBytesTransferred
	push ecx			# nNumberOfBytesToRead
	push edx			# lpBuffer
	push ebx			# hFile
	call _ReadFile@20
	jmp plat_fx_done

PLAT_FWRITE:
	push ecx
	push edx
	push 0				# lpOverlapped
	push OFFSET plat_fxfer		# lpNumberOfBytesTransferred
	push ecx			# nNumberOfBytesToWrite
	push edx			# lpBuffer
	push ebx			# hFile
	call _WriteFile@20

# Common end of FREAD and FWRITE: EAX = BOOL result; [ESP] = saved EDX, [ESP+4] = saved ECX.
plat_fx_done:
	test eax, eax
	jz plat_fx_err
	mov eax, DWORD PTR plat_fxfer
	cmp eax, DWORD PTR [esp+4]
	jne plat_fx_short
	pop edx
	pop ecx
	clc
	ret
plat_fx_err:
	call _GetLastError@0
	jmp plat_fx_fail
plat_fx_short:
	mov eax, 38			# ERROR_HANDLE_EOF: fewer bytes than asked for
plat_fx_fail:
	pop edx
	pop ecx
	stc
	ret

PLAT_FCLOSE:
	push ecx
	push edx
	push ebx			# hObject
	call _CloseHandle@4
	pop edx
	pop ecx
	ret

# ---------------------------------------------------------------- PLAT_EXIT
# EAX = exit code. Does not return.
	.globl PLAT_EXIT
PLAT_EXIT:
	push eax
	call PLAT_FLUSH			# preserves EAX
	pop eax
	push eax
	call _ExitProcess@4

# ---------------------------------------------------------------- directory listing (FILES)
# The current directory, every entry except "." and "..". Same contract as platform_qdos.S:
#   PLAT_DIROPEN : CF=0 with the listing open, or CF=1 (EAX = 2) if there is none.
#   PLAT_DIRNEXT : CF=0 with EBX = the entry name (zero-terminated, as Windows stores it), or CF=1 at the end.
#   PLAT_DIRCLOSE: closes the listing.
# Clobbers EAX, ECX, EDX. EBX, ESI, EDI and EBP are preserved.
	.extern _FindFirstFileA@8
	.extern _FindNextFileA@8
	.extern _FindClose@4
	.globl PLAT_DIROPEN, PLAT_DIRNEXT, PLAT_DIRCLOSE
PLAT_DIROPEN:
	push offset plat_finddata	# lpFindFileData (WIN32_FIND_DATAA)
	push offset plat_dirpat		# "*"
	call _FindFirstFileA@8
	cmp eax, -1			# INVALID_HANDLE_VALUE: no entries
	je plat_do_fail
	mov DWORD PTR plat_dirh, eax
	mov DWORD PTR plat_dirfirst, 1
	clc
	ret
plat_do_fail:
	mov eax, 2			# ERROR_FILE_NOT_FOUND
	stc
	ret

PLAT_DIRNEXT:
	cmp DWORD PTR plat_dirfirst, 0
	je plat_dn_more
	mov DWORD PTR plat_dirfirst, 0
	jmp plat_dn_have
plat_dn_more:
	push offset plat_finddata
	push DWORD PTR plat_dirh
	call _FindNextFileA@8
	test eax, eax
	jz plat_dn_end
plat_dn_have:
	lea ebx, [plat_finddata+44]	# cFileName
	cmp BYTE PTR [ebx], '.'		# skip "." and ".."
	je plat_dn_more
	clc
	ret
plat_dn_end:
	mov eax, 18			# ERROR_NO_MORE_FILES
	stc
	ret

PLAT_DIRCLOSE:
	push DWORD PTR plat_dirh
	call _FindClose@4
	ret

	.section .bss
	.lcomm plat_hin, 4
	.lcomm plat_dirh, 4
	.lcomm plat_dirfirst, 4
	.lcomm plat_finddata, 320
	.section .data
plat_dirpat:
	.asciz "*"
	.section .bss
	.lcomm plat_hout, 4
	.lcomm plat_mode, 4
	.lcomm plat_isconsole, 1
	.lcomm plat_written, 4
	.lcomm plat_read, 4
	.lcomm plat_avail, 4
	.lcomm plat_olen, 4
	.lcomm plat_obuf, PLAT_OBSIZE
	.lcomm plat_ilen, 4
	.lcomm plat_ipos, 4
	.lcomm plat_lastcr, 1
	.lcomm plat_ibuf, PLAT_IBSIZE
	.section .bss
	.lcomm plat_fhigh, 4
	.lcomm plat_fxfer, 4
