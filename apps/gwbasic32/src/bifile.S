# bifile.S: SAVE and LOAD (Stage 4). Program files only; no sequential or random files yet.
#
# A program file is the program's stored image exactly as it is in memory: from TXTTAB up to VARTAB,
# which is every line with its 4-byte link and 2-byte line number, then the 4-byte zero end link. This
# is this port's own format, not the GW-BASIC binary or ASCII format, so the two are not interchangeable.
# The link fields in the file are stale; LOAD rebuilds them with CHEAD.
#
# LOAD refuses a file shorter than the end link before it touches the program ("Bad file mode", program
# unchanged). A file of the right length is read into the program area, then checked for line structure
# (CHKIMG); if that check fails it is "Bad file mode" and the program is left empty. The check is
# structural only: the text of a line is not parsed.
#
# Entry: a statement routine is entered with BX at the current character (see RUN), and must return with
# BX at the next character. File names are string expressions; GETFNAM copies one into FILNAMBUF.

	.intel_syntax noprefix
	.include "gwconst.inc"
	.include "gwtok.inc"
	.include "gio86u.inc"		# ASCCR, ASCLF, ASCTAB, ASCCTZ, PSEUDO_SAV

	.section .text
	.globl SAVE, LOAD, SAV_SOT, SAV_GPS, SAV_WID
	.set SAVBUFSZ, 4096
	.extern PLAT_FOPEN, PLAT_FSIZE, PLAT_FREAD, PLAT_FWRITE, PLAT_FCLOSE

# -------------------------------------------------------------------------------------
# SAVE "name": write the program image to the file.
# -------------------------------------------------------------------------------------
SAVE:
	call FRMEVL			# the file name expression at [BX]
	call GETFNAM			# EAX = zero-terminated name; BX preserved
	mov DWORD PTR SAVNAME, eax	# CHRGTR changes AL, so the name pointer is kept in memory
	mov BYTE PTR SAVMODE, 0		# binary image
	dec ebx
	call CHRGTR			# the next non-blank character: the end of the statement, or ","
	cmp al, ','
	jne sav_end
	call CHRGTR			# the mode letter after the comma
	cmp al, 'A'
	jne sav_notA
	mov BYTE PTR SAVMODE, 1		# ASCII listing
	call CHRGTR			# past the A
	jmp sav_end
sav_notA:
	cmp al, 'P'
	jne SNERR
	jmp ADVERR			# UNSUPPORTED: protected SAVE (the original's PROSAV)
sav_end:
	call FILEND			# the statement must end here
	push ebx			# BX is needed for the handle; the statement must return it
	mov ebx, DWORD PTR SAVNAME	# name
	mov edx, 1			# write (create or replace)
	call PLAT_FOPEN
	jc sav_open
	mov ebx, eax			# handle
	cmp BYTE PTR SAVMODE, 0
	jne sav_asc
	mov ecx, DWORD PTR VARTAB
	sub ecx, DWORD PTR TXTTAB	# the image, including the 4-byte end link
	mov edx, DWORD PTR TXTTAB
	call PLAT_FWRITE
	jc sav_write
	call PLAT_FCLOSE
	pop ebx
	ret
sav_write:
	# A failed write leaves the part written so far in the file, as the original does (the file is not
	# removed). The handle is closed first.
	push eax			# the platform error code
	call PLAT_FCLOSE
	pop eax
	jmp sav_error
sav_open:
	# EAX = the platform error code from the open (CF=1)
	jmp sav_error

# sav_error: EAX = the platform error code (gwconst.inc, PLAT_ERR_*). Raises the original's error for it:
# write-protected "Disk write protected" (70), disk full (61), root directory full "Too many files" (67),
# anything else "Device I/O Error" (57). Does not return.
sav_error:
	mov edx, ERRIOE
	cmp eax, PLAT_ERR_WPROT
	jne sav_e1
	mov edx, ERRDWP
	jmp ERROR
sav_e1:
	cmp eax, PLAT_ERR_DISKFULL
	jne sav_e2
	mov edx, ERRDFL
	jmp ERROR
sav_e2:
	cmp eax, PLAT_ERR_DIRFULL
	jne sav_e3
	mov edx, ERRTMF
	jmp ERROR
sav_e3:
	jmp ERROR

# SAVE "name",A: the program is listed to the file, as LIST does, and the file is closed with a CTL-Z,
# as the original's close of a sequential output file does. The lines go through the normal output
# routines to the file pseudo-FDB PSEUDO_SAV (SAVDSP_TBL, below). Entry: EBX = handle (in the file).
sav_asc:
	mov DWORD PTR SAVHND, ebx
	mov DWORD PTR SAVCNT, 0
	mov DWORD PTR SAVCOL, 0
	mov DWORD PTR SAVERR, 0
	mov DWORD PTR PTRFIL, PSEUDO_SAV
	call savasc_list
	mov al, ASCCTZ
	call OUTDO			# the end-of-file character
	call SAV_FLUSH			# write what is left in the buffer
	mov DWORD PTR PTRFIL, 0		# back to the screen
	mov ebx, DWORD PTR SAVHND
	call PLAT_FCLOSE
	mov eax, DWORD PTR SAVERR
	or eax, eax
	jnz sav_error			# a write failed: EAX = its platform code
	pop ebx
	ret

# savasc_list: print every line of the program, as LIST does (LIST4 in gwlist.S), to PTRFIL. Each record is
# a link (4 bytes), the line number (2 bytes) and the text ending in a zero. The link is zero at the end.
savasc_list:
	mov ebx, DWORD PTR TXTTAB
sal_line:
	mov ecx, DWORD PTR [ebx]	# link to the next line
	or ecx, ecx
	jz sal_done
	mov DWORD PTR SAVREC, ebx	# the record (its text is used after calls that clobber EBX)
	movzx ebx, WORD PTR [ebx+4]	# the line number
	call LINPRT			# print it as an integer without a leading space
	mov ebx, DWORD PTR SAVREC
	mov al, BYTE PTR [ebx+6]	# the first text byte
	cmp al, 9			# a tab is printed without a space before it
	je sal_text
	mov al, ' '
	call OUTDO
sal_text:
	mov ebx, DWORD PTR SAVREC
	add ebx, 6			# the text
	call BUFLIN			# unpack the line into BUF
	mov ebx, OFFSET BUF
	call LISPRT			# print the line
	call CRDO
	mov ebx, DWORD PTR SAVREC
	mov ebx, DWORD PTR [ebx]	# the next record
	jmp sal_line
sal_done:
	ret

# SAV_SOT: the file's output routine (G_SOT). AL = the byte. All registers preserved.
SAV_SOT:
	push eax
	push ebx
	push ecx
	push edx
	cmp DWORD PTR SAVERR, 0
	jne sot_col			# after a write error the bytes are dropped: the error is raised at the end
	mov ebx, DWORD PTR SAVCNT
	mov edx, OFFSET SAVBUF
	mov BYTE PTR [edx+ebx], al
	inc ebx
	mov DWORD PTR SAVCNT, ebx
	cmp ebx, SAVBUFSZ
	jb sot_col
	call SAV_FLUSH
sot_col:
	cmp al, ASCCR			# a carriage return starts column 0
	jne sot_notcr
	mov DWORD PTR SAVCOL, 0
	jmp sot_done
sot_notcr:
	cmp al, ASCLF
	je sot_done
	cmp al, ASCTAB
	jne sot_plain
	mov ebx, DWORD PTR SAVCOL	# a tab goes to the next multiple of 8
	add ebx, 8
	and ebx, 0xFFFFFFF8
	mov DWORD PTR SAVCOL, ebx
	jmp sot_done
sot_plain:
	inc DWORD PTR SAVCOL
sot_done:
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# SAV_FLUSH: write the buffered bytes to the file. A failure is kept in SAVERR (the platform code, or 5).
SAV_FLUSH:
	push eax
	push ebx
	push ecx
	push edx
	mov ecx, DWORD PTR SAVCNT
	or ecx, ecx
	jz sfl_done
	cmp DWORD PTR SAVERR, 0
	jne sfl_drop
	mov ebx, DWORD PTR SAVHND
	mov edx, OFFSET SAVBUF
	call PLAT_FWRITE
	jnc sfl_drop
	or eax, eax
	jnz sfl_set
	mov eax, 5			# I/O error
sfl_set:
	mov DWORD PTR SAVERR, eax
sfl_drop:
	mov DWORD PTR SAVCNT, 0
sfl_done:
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# SAV_GPS: the column (G_GPS): [AH] = column, capped at 255. AL preserved.
SAV_GPS:
	mov ecx, DWORD PTR SAVCOL
	cmp ecx, 255
	jbe sg_ok
	mov ecx, 255
sg_ok:
	mov ah, cl
	ret

# SAV_WID: the width (G_GWD): [AH] = 255, the width of a file (no wrapping). AL preserved.
SAV_WID:
	mov ah, 255
	ret

# -------------------------------------------------------------------------------------
# LOAD "name" [,R]: replace the program with the file's image and clear the variables. Without ,R the
# interpreter returns to command level ("Ok"); with ,R it runs the program. Errors are raised before the
# program is changed, except a file that fails the check, which leaves an empty program.
# -------------------------------------------------------------------------------------
LOAD:
	call FRMEVL			# the file name expression at [BX]
	call GETFNAM			# EAX = name; BX preserved
	mov DWORD PTR LODNAME, eax	# CHRGTR changes AL, so the name pointer is kept in memory
	mov BYTE PTR LODRUNF, 0
	dec ebx
	call CHRGTR			# the next non-blank character
	cmp BYTE PTR [ebx], ','
	jne lod_end
	call CHRGTR
	cmp al, 'R'
	je lod_setr
	cmp al, 'r'
	jne SNERR
lod_setr:
	mov BYTE PTR LODRUNF, 1
	call CHRGTR			# past the R
lod_end:
	call FILEND
lod_open:
	mov ebx, DWORD PTR LODNAME
	xor edx, edx			# read
	call PLAT_FOPEN
	jc lod_open_err
	mov esi, eax			# ESI = handle
	mov ebx, esi
	call PLAT_FSIZE
	jc lod_io_h
	mov edi, eax			# EDI = size of the image
	cmp edi, 4			# at least the end link
	jb lod_bad_h
	mov eax, DWORD PTR TXTTAB
	add eax, edi
	cmp eax, DWORD PTR FRETOP	# the image must fit below the string space (no garbage collection)
	jae lod_om_h

	# The program is emptied before the read, so a failed read cannot leave a half-loaded program.
	mov ebx, DWORD PTR TXTTAB
	mov DWORD PTR [ebx], 0
	add ebx, 4
	mov DWORD PTR VARTAB, ebx
	mov ebx, esi
	mov ecx, edi
	mov edx, DWORD PTR TXTTAB
	call PLAT_FREAD
	jc lod_io_read
	mov ebx, esi
	call PLAT_FCLOSE

	mov ebx, DWORD PTR TXTTAB	# the image must be a list of lines, then the end link at its end
	mov ecx, ebx
	add ecx, edi
	call CHKIMG
	jc lod_bad_img

	mov eax, DWORD PTR TXTTAB
	add eax, edi
	mov DWORD PTR VARTAB, eax	# end of the program
	mov edx, DWORD PTR TXTTAB
	call CHEAD			# rebuild the links
	cmp BYTE PTR LODRUNF, 0
	jne lod_run
	mov ebx, OFFSET LODENDCR	# command level: a CR and a zero end link, as an empty program gives
	jmp CLEARC			# jump: CLEARC resets the stack and returns to NEWSTT

lod_run:
	jmp RUNC			# as RUN with no line number

lod_open_err:
	cmp eax, 2			# ERROR_FILE_NOT_FOUND
	je lod_nf
	cmp eax, 3			# ERROR_PATH_NOT_FOUND
	je lod_nf
	mov edx, ERRIOE
	jmp ERROR
lod_nf:
	mov edx, ERRFNF
	jmp ERROR

lod_io_h:				# error with the handle open
	mov ebx, esi
	call PLAT_FCLOSE
	mov edx, ERRIOE
	jmp ERROR
lod_bad_h:
	mov ebx, esi
	call PLAT_FCLOSE
	mov edx, ERRBFM
	jmp ERROR
lod_om_h:
	mov ebx, esi
	call PLAT_FCLOSE
	mov edx, ERROM
	jmp ERROR

lod_io_read:				# the read failed part way: close, and leave the program empty
	mov ebx, esi
	call PLAT_FCLOSE
	call lod_make_empty
	mov edx, ERRIOE
	jmp ERROR
lod_bad_img:				# the file was read but is not an image: leave the program empty
	call lod_make_empty
	mov edx, ERRBFM
	jmp ERROR

# lod_make_empty: the program becomes empty (the zero end link at TXTTAB, VARTAB after it).
lod_make_empty:
	mov ebx, DWORD PTR TXTTAB
	mov DWORD PTR [ebx], 0
	add ebx, 4
	mov DWORD PTR VARTAB, ebx
	ret

# -------------------------------------------------------------------------------------
# GETFNAM: the string result of the last formula (FACLO) becomes a zero-terminated file name in
# FILNAMBUF. Returns EAX = the name. BX is preserved. The string temporary is freed.
# (Not named FILNAM: gwdata.S defines a global data label FILNAM, the CP/M directory buffer, and a call from
# another object file would resolve to that data label and run into it.)
# -------------------------------------------------------------------------------------
	.globl GETFNAM
GETFNAM:
	push ebx
	call FRESTR			# checks that it is a string; frees the temporary; BX = descriptor
	movzx ecx, BYTE PTR [ebx]	# length
	mov esi, DWORD PTR [ebx+1]	# the characters
	or ecx, ecx
	jz fil_bad
	mov edi, OFFSET FILNAMBUF
	cld
	rep movsb
	mov BYTE PTR [edi], 0
	mov eax, OFFSET FILNAMBUF
	pop ebx
	ret
fil_bad:
	mov edx, ERRIFN			# an empty name: "Bad file name"
	jmp ERROR

	.globl FILEND
# FILEND: AL is the next non-blank character. It must end the statement (0 or ':'), or it is a syntax error.
FILEND:
	or al, al
	jz fe_ok
	cmp al, ':'
	jne SNERR
fe_ok:
	ret

# -------------------------------------------------------------------------------------
# CHKIMG: check that the image from BX to CX (exclusive) has the structure CHEAD walks: each line has a
# nonzero 4-byte link, a 2-byte line number, and text ending in a zero byte (embedded constants are
# skipped by their size, as in CHEAD). The image ends with a zero link exactly at CX. CF=1 if not.
# ECX is preserved; EAX, EBX and EDX are used.
# -------------------------------------------------------------------------------------
CHKIMG:
	push esi
chk_line:
	lea edx, [ebx+4]
	cmp edx, ecx
	ja chk_bad			# no room for the link
	cmp DWORD PTR [ebx], 0
	je chk_end
	lea edx, [ebx+6]		# first text byte (after link and line number)
	cmp edx, ecx
	ja chk_bad
	lea esi, [ebx+5]		# scan from one before the text, as CHEAD does
chk_scan:
	inc esi
	cmp esi, ecx
	jae chk_bad
	movzx eax, BYTE PTR [esi]
	or al, al
	jz chk_eol
	cmp al, DBLCON+1		# an ordinary character?
	jae chk_scan
	cmp al, 11
	jb chk_scan
	sub al, 11			# an embedded constant: skip its data
	movzx eax, al
	movzx eax, BYTE PTR CZCSIZ[eax]
	add esi, eax
	jmp chk_scan
chk_eol:
	lea ebx, [esi+1]		# the next line starts after the zero
	jmp chk_line
chk_end:
	lea edx, [ebx+4]
	cmp edx, ecx
	jne chk_bad
	pop esi
	clc
	ret
chk_bad:
	pop esi
	stc
	ret

	.section .data
# The end of an empty program as NEWSTT sees it: a CR (the line end), then a zero link. LOAD without ,R
# returns to command level through it. Pointing at TXTTAB-1 would not do: NEWSTT would follow the first
# link of a non-empty program.
LODENDCR:	.byte 13
	.long 0

	.section .bss
	.lcomm FILNAMBUF, 256
	.lcomm LODRUNF, 1
	.lcomm LODNAME, 4
	.lcomm SAVNAME, 4
	.lcomm SAVMODE, 1
	.lcomm SAVHND, 4
	.lcomm SAVCNT, 4
	.lcomm SAVCOL, 4
	.lcomm SAVERR, 4
	.lcomm SAVREC, 4
	.lcomm SAVBUF, 4096

	.section .text

# -------------------------------------------------------------------------------------
# MERGE "name": merge an ASCII program file into the program. Direct mode only in this build (a MERGE
# inside a running program is UNSUPPORTED). The file is then read a line at a time by the line input:
# PINLIN -> DSKCHI -> INCHR, with ISFLIO true while MRGACT is set, so each line is stored as if it had
# been typed (a line without a number is a direct statement in file, error 65). At the end of the file
# DSKEOF returns to command level through READY, and READY closes the file (MRG_END) and prints "Ok".
# An error ends the merge the same way, through ERROR -> READY.
# -------------------------------------------------------------------------------------
	.globl MERGE
MERGE:
	cmp WORD PTR CURLIN, 65535	# direct mode only (CURLIN = 65535)
	jne ADVERR
	call FRMEVL			# the file name expression at [BX]
	call GETFNAM			# EAX = name; BX preserved
	mov DWORD PTR MRGNAME, eax	# CHRGTR changes AL, so the name pointer is kept in memory
	dec ebx
	call CHRGTR			# the next non-blank character (as LOAD does), for FILEND
	call FILEND
	mov ebx, DWORD PTR MRGNAME
	xor edx, edx			# read
	call PLAT_FOPEN
	jc lod_open_err			# the same open-error mapping as LOAD
	mov DWORD PTR MRGHDL, eax
	mov BYTE PTR MRGACT, 1
	jmp MAIN

# MRG_END: close the MERGE file, if one is open. Called by READY, so every end of a MERGE closes it.
	.globl MRG_END
MRG_END:
	cmp BYTE PTR MRGACT, 0
	je mrg_end_r
	push ebx
	mov ebx, DWORD PTR MRGHDL
	call PLAT_FCLOSE
	pop ebx
	mov BYTE PTR MRGACT, 0
mrg_end_r:
	ret

# INCHR: the next byte of the MERGE file into AL. CF=1 at the end of the file (a short read, or a CTL-Z).
# EBX, ECX and EDX are preserved: DSKCHI keeps its line pointer and count in them.
	.globl INCHR
INCHR:
	push ebx
	push ecx
	push edx
	mov ebx, DWORD PTR MRGHDL
	mov ecx, 1
	mov edx, OFFSET MRGCH
	call PLAT_FREAD			# one byte; a short read is ERROR_HANDLE_EOF (38) with CF=1
	jc inchr_fail
	mov al, BYTE PTR MRGCH
	cmp al, 26			# CTL-Z ends the file (DOS text convention)
	je inchr_eof
	pop edx
	pop ecx
	pop ebx
	clc
	ret
inchr_fail:
	cmp eax, 38
	je inchr_eof
	pop edx
	pop ecx
	pop ebx
	mov edx, ERRIOE
	jmp ERROR			# a read error: the message, then READY closes the file
inchr_eof:
	pop edx
	pop ecx
	pop ebx
	stc
	ret
