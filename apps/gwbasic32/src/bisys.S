# bisys.S: SYSTEM and FILES (the original GIODSK.ASM SYSTEM and FILES, on the platform directory calls).
#
# SYSTEM: the rest of the statement must be empty (a syntax error otherwise). Then all files are closed
# (CLSALL), and the program exits to the operating system through PLAT_EXIT with status 0. The original
# goes through SYSTME, which also terminates the devices; this port's devices need no termination.
#
# FILES [spec]: lists the directory entries whose names match spec, or all entries if there is no spec.
# The layout is the original's (GIODSK.ASM FILES): the 8 name characters, a '.' when there is an extension
# (otherwise a space), then the 3 extension characters; the next entry starts 14 columns on, with a
# carriage return when the next entry would not fit the line width; a final CR/LF unless the cursor is at
# column 0. With no match the statement fails with "File not found". The spec is a string expression.
# Wildcards are MS-DOS style: '?' matches one character; '*' stands for the rest of its part (name or
# extension). A name with '*' and no '.' matches every extension. A drive prefix ("A:") is ignored.
#
# Entries come from PLAT_DIROPEN / PLAT_DIRNEXT / PLAT_DIRCLOSE (QDOS: the WIN1_ root; Win32: the current
# directory). Names longer than 8.3 are truncated to the field, as MS-DOS would show them.

	.intel_syntax noprefix
	.include "gwconst.inc"
	.include "gwtok.inc"

# -------------------------------------------------------------------------------------
# SYSTEM
# -------------------------------------------------------------------------------------
	.section .text
	.globl SYSTEM, FILES
SYSTEM:
	dec ebx
	call CHRGTR			# the next non-blank character must end the statement
	call FILEND
	call CLSALL			# close all files
	xor eax, eax			# exit status 0
	jmp PLAT_EXIT			# flushes the console and ends the job; does not return

# -------------------------------------------------------------------------------------
# FILES [spec]
# -------------------------------------------------------------------------------------
FILES:
	dec ebx
	call CHRGTR			# AL = the first non-blank character after FILES, BX at it
	or al, al
	jz fls_all
	cmp al, ':'
	je fls_all
	call FRMEVL			# the file specification: a string expression
	call GETFNAM			# EAX = zero-terminated spec; BX preserved
	mov DWORD PTR FLSPEC, eax	# CHRGTR changes AL, so the pointer is kept in memory
	dec ebx
	call CHRGTR
	call FILEND
	jmp fls_strip
fls_all:
	mov DWORD PTR FLSPEC, OFFSET fls_star	# "*.*"
fls_strip:
	# skip a drive prefix: the spec starts after its last ':'
	mov esi, DWORD PTR FLSPEC
	mov edx, esi
fls_scan:
	mov al, BYTE PTR [esi]
	or al, al
	jz fls_scanned
	cmp al, ':'
	jne fls_scan_next
	lea edx, [esi+1]
fls_scan_next:
	inc esi
	jmp fls_scan
fls_scanned:
	mov esi, edx
	mov edi, OFFSET FLPAT
	call build_fields
	mov DWORD PTR FLTXT, ebx	# the statement's text pointer; EBX is used by the directory calls
	mov DWORD PTR FLCOUNT, 0
	call PLAT_DIROPEN
	jc fls_nofile
fls_next:
	call PLAT_DIRNEXT		# CF=1 at the end; else EBX = name
	jc fls_done
	mov esi, ebx
	mov edi, OFFSET ENTFLD
	call build_fields
	mov esi, OFFSET FLPAT
	mov edi, OFFSET ENTFLD
	call field_match		# ZF=1 if the entry matches the pattern
	jne fls_next
	inc DWORD PTR FLCOUNT
	call fls_print
	jmp fls_next
fls_done:
	call PLAT_DIRCLOSE
	cmp DWORD PTR FLCOUNT, 0
	je fls_nofile
	call CRDONZ			# carriage return if not in column 0
	mov ebx, DWORD PTR FLTXT
	ret
fls_nofile:
	mov edx, ERRFNF			# no entry matches (or no listing): "File not found"
	jmp ERROR

# fls_print: the entry in ENTFLD, then the column step. All registers preserved except as noted.
fls_print:
	push ecx
	push esi
	mov esi, OFFSET ENTFLD
	mov ecx, 8
fp_name:
	mov al, BYTE PTR [esi]
	call OUTDO
	inc esi
	dec ecx
	jnz fp_name
	mov al, BYTE PTR [esi]		# the first extension character decides the separator
	cmp al, ' '
	jne fp_dot
	mov al, ' '
	jmp fp_sep
fp_dot:
	mov al, '.'
fp_sep:
	call OUTDO
	mov ecx, 3
fp_ext:
	mov al, BYTE PTR [esi]
	call OUTDO
	inc esi
	dec ecx
	jnz fp_ext
	call PTRWID			# AH = line width
	mov ch, ah
	call PTRGPS			# AL = column
	add al, 14
	cmp al, ch
	jae fp_crlf
	mov al, ' '
	call OUTDO
	jmp fp_out
fp_crlf:
	call CRDO
fp_out:
	pop esi
	pop ecx
	ret

# build_fields: ESI = zero-terminated name or pattern; EDI = 11-byte field (8 name, 3 extension, space
# padded). Letters are made upper case. In a pattern '*' fills the rest of its part with '?'. Characters
# beyond the 8 name or 3 extension positions are ignored. Clobbers EAX, ECX, EDX, ESI.
build_fields:
	push edi
	mov ecx, 11
	mov al, ' '
	cld
	rep stosb
	pop edi
	xor ecx, ecx			# index into the field
	xor edx, edx			# DH = 1 once the '.' has been seen; DL bit 0 = '*' seen in the name
bf_loop:
	mov al, BYTE PTR [esi]
	or al, al
	jz bf_end
	cmp al, '.'
	je bf_dot
	cmp al, '*'
	je bf_star
	call bf_store
	inc esi
	jmp bf_loop
bf_dot:
	test dh, dh
	jnz bf_dot_again
	mov dh, 1
	mov ecx, 8
bf_dot_again:
	inc esi
	jmp bf_loop
bf_star:
	test dh, dh
	jnz bf_star_ext
	or dl, 1
	mov ebp, 8			# the name part ends at 8
	jmp bf_fill
bf_star_ext:
	mov ebp, 11			# the extension part ends at 11
bf_fill:
	cmp ecx, ebp
	jae bf_fill_done
	mov BYTE PTR [edi+ecx], '?'
	inc ecx
	jmp bf_fill
bf_fill_done:
	# skip the rest of the source part, leaving ESI at a '.' or at the terminating zero
bf_skip:
	mov al, BYTE PTR [esi]
	or al, al
	jz bf_loop
	cmp al, '.'
	je bf_loop
	inc esi
	jmp bf_skip
bf_end:
	# a name with '*' and no extension matches every extension ("A*" is "A*.*" in MS-DOS)
	test dl, 1
	jz bf_out
	test dh, dh
	jnz bf_out
	mov BYTE PTR [edi+8], '?'
	mov BYTE PTR [edi+9], '?'
	mov BYTE PTR [edi+10], '?'
bf_out:
	ret

# bf_store: store AL (upper cased) at field[ECX] if ECX is inside the current part.
bf_store:
	test dh, dh
	jnz bfs_ext
	cmp ecx, 8
	jae bfs_no
	jmp bfs_put
bfs_ext:
	cmp ecx, 11
	jae bfs_no
bfs_put:
	cmp al, 'a'
	jb bfs_w
	cmp al, 'z'
	ja bfs_w
	sub al, 32
bfs_w:
	mov BYTE PTR [edi+ecx], al
	inc ecx
bfs_no:
	ret

# field_match: ESI = pattern field, EDI = entry field. ZF=1 if they match ('?' matches any character).
field_match:
	mov ecx, 11
fm_loop:
	mov al, BYTE PTR [esi]
	cmp al, '?'
	je fm_next
	cmp al, BYTE PTR [edi]
	jne fm_no
fm_next:
	inc esi
	inc edi
	dec ecx
	jnz fm_loop
	xor eax, eax			# ZF=1
	ret
fm_no:
	or eax, 1			# ZF=0
	ret

	.section .data
fls_star:
	.asciz "*.*"

	.section .bss
	.lcomm FLSPEC, 4
	.lcomm FLTXT, 4
	.lcomm FLCOUNT, 4
	.lcomm FLPAT, 11
	.lcomm ENTFLD, 11
