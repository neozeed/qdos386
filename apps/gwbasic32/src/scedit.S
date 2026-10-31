# SCNEDT.ASM -> scedit.S. Program and INPUT line input: PINLIN, QINLIN, INLIN, SINLIN (with the
# key loop INLLOP and the logical-line read-back INLRET), the INPUT semicolon scan SCNSEM, the
# BREAK reset EDTBRK, the LOAD ASCII line reader (DSKCHI), the SAVE ASCII output helpers OUTCH1,
# and the screen-editor entries EDIT and ERREDT.
#
# Port notes (see NOTES.md and platform_win32.S):
#  - Keys come from KEYIN (giokyb.S), which reads the platform boundary (PLAT_CHIN, PLAT_CHAVAIL).
#  - Echo goes through OUTDO (scrout.S). The logical line is read back into BUF by SCNRDL
#    (SCNDRV). SCNMRK sets up the cursor for the line (SCNDRV).
#  - Pointers held in BX, DI and SI as addresses are 32-bit. Pushed pointers are 4 bytes.
#    The REPE SCASB count in CX is ECX (32-bit).
#  - The INPUT statement is in GWMAIN.ASM (INPUT:), translated in gwmain_b2.S. It calls SINLIN
#    here. It is not in GIO86 and is not implemented in this file.
#  - UNSUPPORTED (Stage 4): the screen editor (EDIT, the auto-edit of a program line after an
#    error, and the help-key edit of KBUF). Those paths end in ADVERR. Their original code is kept.

	.intel_syntax noprefix

	.set CHREDT, 1			# EDiT
	.set CHRRET, 13			# RETurn
	.set CHRLNF, 10			# LiNeFeed
	.set CHRAPP, 14			# APPend
	.set BUFLEN, 255		# maximum line length (BUFLEN_ in gwdata.S)

	.section .text

# ---------------------------------------------------------------- PINLIN / QINLIN / INLIN / SINLIN
# ENTRY: PINLIN, QINLIN, INLIN, SINLIN
#
# PROGRAM STATEMENT INPUT
	.globl PINLIN
PINLIN:
	call DSKCHI			# Don't return if Loading ASCII File
	stc				# Indicate program statement input
	jmp INLIN0

# PRINT "?" BEFORE GETTING INPUT
	.globl QINLIN
QINLIN:
	mov al, '?'
	call OUTDO			# Output a "?"
	mov al, ' '
	call OUTDO			# Followed by a space

# INPUT STATEMENT
	.globl INLIN
INLIN:

# INPUT STATEMENT REDO
	.globl SINLIN
SINLIN:
	or al, al			# Indicate INPUT statement line input

INLIN0:
	sbb al, al			# If INPUT statement AL=0, else AL=-1
	mov BYTE PTR F_EDPG, al		# Save flag
	mov al, BYTE PTR F_EDIT		# Get old value of F.EDIT (in case of EDIT statement)
	mov BYTE PTR F_EDIT, 255	# Indicate executing INLIN
	call SCNMRK			# Set up for edit
					#   If AL=1 then move cursor to start of logical line
					#   which preceeds current.
					#   Else if ZF=1 then set up FSTPOS, LSTPOS
					#   if ZF=0 then FSTPOS=0,0; LSTPOS=max,max
	# The shadow screen can still hold text of an earlier, longer line to the right of the cursor. It
	# was checked and is not the cause of the stale-line bug (see NOTES.md, Stage 4); nothing is erased here.

# MAIN loop of editor
#
# WHILE EOL.NE.TRUE BEGIN
# ** GET A KEY
INLLOP:
	clc				# Signal for insert or overwrite cursor
	call STCTYP			# Set new cursor type
	call KEYIN			# AX=key value
# At this point we can have three classes of characters:
#   CF=1 indicates 2 byte char with 2 sub classes:
#       AH=FF indicates editor control function
#       AH<>FF indicates 2 byte character to echo
#   CF=0 indicates 1 or 3 byte key
#       AL=FF indicates a 3 byte key with scan code in DX - these are ignored
#       AL<>FF indicates a 1 byte key to echo, except if char is CR.
	jb INL2BY			# BRIF 2-byte character, check for editor controls
	cmp al, CHRRET
	jz INLRET			# BRIF CR, terminate line input
# ** IF CURRENT_CHAR IS SPECIAL THREE BYTE CHAR
	cmp al, 254			# Test for 3-byte character
# ** *** IGNORE THIS CHAR
	jz INLLOP			# BRIF to ignore 3-byte character
	jmp INLOP0			# Not 3-bytes, just echo single byte
INL2BY:
# ** IF CURRENT_CHAR IS CONTROL_CHAR
	cmp ah, 255
	jnz INLOP0			# BRIF not editor control character, echo char
	cmp al, 127
	jz INLP0			# BRIF is a control character
	cmp al, ' '
	jnb INLOP0			# BRIF is not a control character
INLP0:
# ** IF CURRENT_CHAR IS <CR> RETURN CURRENT LOGICAL LINE
	cmp al, CHRRET
	jz INLRET			# BRIF is carriage return (EOL)
# ** IF (CURRENT_CHAR=<HELP> AND INPUT_MODE=PROGRAM) DO EDIT ON KBUF
	cmp al, CHREDT
	jnz INLOP0
	jmp INLHLP			# BRIF help char, do edit on KBUF
# Echo character in AX
INLOP0:
	or ah, ah
	jz INLOP3			# Single byte character
	xchg al, ah			# Output AH first
	call OUTDO			# Output a char
	xchg al, ah			# Restore AL
INLOP3:
	call OUTDO			# Output a char
	jmp INLLOP
# ** END

# SCAN FOR SEMICOLON
	.globl SCNSEM
SCNSEM:
	cmp al, ';'
	jnz SCNSMR			# BRIF not semicolon, return
	mov BYTE PTR SEMFLG, al
	jmp CHRGTR			# Skip semicolon and return
SCNSMR:
	ret

# ---------------------------------------------------------------- exit, return current logical line
# READ LOGICAL LINE INTO BUF
INLRET:
	mov ebx, offset BUF		# Put data into BUF
	mov ecx, 255			# CX=Max number of bytes to move
	test BYTE PTR F_EDPG, 255	# Set zero flag if program statement input
	pushfd
	mov dl, BYTE PTR CSRY		# (DH,DL) = (CSRX,CSRY)
	mov dh, BYTE PTR CSRX
	call SCNRDL			# Read the logical line into BUF
					# BX=address of last char in BUF plus one
# IF PROGRAM_STATEMENT_INPUT BEGIN
	popfd
	jz INCRTX			# BRIF not statement input
# ** WHILE POSN.NE.BEGIN_OF_LINE AND [POSN].NE.BLANK AND [POSN].NE.LF BEGIN
	mov ecx, ebx
	sub ecx, offset BUF		# CX=count of chars in BUF
	jz INCRTX			# BUF is empty
	push edi
	mov edi, ebx
	dec edi				# Start scan at last valid data
INLOT0:	mov al, ' '
# ** *** POSN=POSN-1
INLOT1:	std				# Scan backwards
	repe scasb			# This works for blank line ("," at BUF-1)
	inc edi				# [DI] = last valid data
	mov al, ' '
	cmp BYTE PTR [edi], al
	jz INLOT2			# BRIF more trailing blanks
	mov al, CHRLNF
	cmp al, BYTE PTR [edi]
	jnz INLOT3			# BRIF not more trailing linefeeds
INLOT2:	or ecx, ecx
	jnz INLOT1			# BRIF not at beginning of buffer
# ** *** END
# ** END
INLOT3:	mov ebx, edi
	pop edi
	inc ebx
INCRTX:	mov BYTE PTR [ebx], 0		# Terminate BUF
# UPDATE CURSOR POSN, RETURN
	test BYTE PTR SEMFLG, 255
	jnz INCRTF			# BRIF if INPUT<semicolon> statement
	mov ax, CHRAPP
	call OUTDO			# Move to end of logical
	mov BYTE PTR F_EDIT, 0
	call CRDO			# Move to first posn beyond this logical
INCRTF:	mov ebx, offset BUF-1		# Return BUF - 1
	pushfd
	call CLRFLG			# Clear miscellaneous status flags
	popfd
	ret

# ---------------------------------------------------------------- BREAK reset
# EDTBRK: routine which initializes when break detected. Called by the STOP code and by CNTCCN.
	.globl EDTBRK
EDTBRK:
	mov BYTE PTR CSRTYP, 0		# Indicate cursor off
	call SETCSR
	call CLRFLG			# Clear editor flags
	push ax
	mov al, CHRAPP
	mov BYTE PTR F_EDIT, al
	call OUTDO			# Move cursor to end of logical line
	mov BYTE PTR F_EDIT, 0		# Reset edit mode
	call CRDO			# Move cursor to next physical line
	pop ax
	mov BYTE PTR AUTFLG, 0		# Reset AUTO mode
	ret

# CLRFLG: routine which clears flags
CLRFLG:
	mov BYTE PTR F_EDIT, 0		# No longer in INLIN
	mov BYTE PTR F_EDPG, 0		# Not program edit
	mov BYTE PTR F_INST, 0		# Not insert mode
	mov BYTE PTR SEMFLG, 0		# Not INPUT; statement
	ret

# ---------------------------------------------------------------- LOAD ASCII line input
# PROGRAM LINE INPUT FROM DISK. Called by PINLIN. Only a LOAD statement (a file open as PTRFIL)
# takes this path; the normal keyboard path returns.
DSKCHI:
	call ISFLIO			# Set FLAGS.NZ if PTRFIL points to active file
	jnz ISLOAD			# BRIF LOAD statement
	ret				# If so use special screen io
ISLOAD:
	pop eax				# Discard Return Address (4 bytes)
	mov cl, BUFLEN			# Setup the maximum character count
	mov ebx, offset BUF		# Place we are going to store the line
LOPBUF:	call INCHR			# Get a character from the file (will call indskc and handle eof)
	jc DSKEOF			# end of the MERGE file
	mov BYTE PTR [ebx], al		# Store the character
	cmp al, 13			# Is it the end (a CR)
	jnz INOTCR			# Not a [CR]
	cmp BYTE PTR [ebx-1], 10	# Preceeded by a line feed?
	jz LOPBUF			# Yes, ignore the [CR]
	jmp FINLIN			# No, this is the end of a line
INOTCR:
	or cl, cl
	je LTLONG			# Branch if line is too long to fit in BUF
	cmp al, 10			# LEADING LINE FEEDS MUST BE IGNORED
	jnz INOTLF
	cmp cl, BUFLEN			# CL=BUFLEN if this is the 1st char on the line
	jz LOPBUF			# Branch if this was a leading line-feed
INOTLF:	inc ebx				# ADVANCE THE POINTER
	dec cl
	jmp LOPBUF			# DO NEXT CHAR
	dec cl
FINLIN:
	mov BYTE PTR [ebx], 0
	mov ebx, offset BUF-1		# POINT AT BUFMIN
	clc				# a line was read (PINLIN's caller tests CF)
	ret

# END OF THE MERGE FILE. A last line without its CR is still a line. With nothing read, the file is
# finished: the return address of PINLIN's caller is discarded and READY closes the file and prints Ok.
DSKEOF:
	cmp ebx, offset BUF
	jne DSKEOF_LINE
	pop eax				# the return address into the caller of PINLIN (ISLOAD left it on the stack)
	jmp READY
DSKEOF_LINE:
	mov BYTE PTR [ebx], 0
	mov ebx, offset BUF-1
	clc
	ret

LTLONG:	jmp LBOERR			# Report LINE BUFFER OVERFLOW error (This program wasn't created by BASIC)

# ---------------------------------------------------------------- SAVE ASCII output helpers
# PROGRAM LISTING CHAR OUTPUT TO DISK (CONVERT <LF> TO <LF><CR>).
# Called by the LIST/SAVE output path. The character goes to OUTDO (scrout.S) in every case.
	.globl OUTCH1
OUTCH1:
	cmp al, CHRLNF
	jz OUTCH0			# IS LF
	jmp OUTDO
OUTCH0:
	call ISFLIO
	jnz OUTCH2			# branch if outputting to file
	call OUTDO
	ret

OUTCH2:
	call OUTDO
	mov al, CHRRET
	call OUTDO
	mov al, CHRLNF			# (original: returns with AL = LF and no further output)
	ret

# ---------------------------------------------------------------- EDIT (UNSUPPORTED: screen editor)
# EDIT COMMAND. UNSUPPORTED: screen editor (EDIT) not in Stage 4. The original body follows and
# is unreachable until the screen editor is ported.
	.globl EDIT
EDIT:
	jmp ADVERR			# UNSUPPORTED: screen editor (EDIT) not in Stage 4
	call LINSPC			# LINE NUMBER IN DE
	jnz EFCERR			# STATEMENT MUST HAVE ENDED
EREDIT:	pop ebx				# REMOVE NEWSTT (OR CALL FROM MAIN) RETURN
	mov WORD PTR DOT, dx
	call FNDLIN			# FIND LINE
	jnb EUSERR			# LINE DOES NOT EXIST
	mov ebx, ecx			# CX=LINE PTR (UNSURE: FNDLIN's pointer width; assumed ECX)
ERED2:	inc ebx
	inc ebx
	mov dx, WORD PTR [ebx]		# DE=LINE NUMBER
	inc ebx
	inc ebx
ERED3:	push ebx
	mov BYTE PTR F_EDIT, 1		# SET FLAG INDICATING IN EDIT MODE
					# (FORCES OPEN LINES DURING LIST)
					# (AND TELLS INLIN TO START AT BEGIN OF LOGICAL)
	movzx ebx, dx			# XCHG BX,DX (the pointer is on the stack)
ERED4:	call LINPRT			# PRINT THE LINE NUMBER
	pop ebx
	cmp BYTE PTR [ebx], 9		# LINE START WITH TAB?
	jz ERED5
	mov al, ' '
	call OUTDO			# NO, PUT SPACE OUT
# ENTRY FOR EDIT ON KBUF IS HERE
ERED5:
	mov BYTE PTR F_EDIT, 1		# SET FLAG INDICATING IN EDIT MODE
	call BUFLIN			# PUT LINE IN BUF AND SET UP HLPBFA
	mov ebx, offset BUF
	call LISPRT			# PRINT THE LINE AND SET UP HLPCSR
	movzx ebx, BYTE PTR CSRY
	mov bh, BYTE PTR CSRX
	jmp MAIN
EFCERR:	jmp FCERR			# Indirect jump to FCERR
EUSERR:	jmp USERR			# Indirect jump to USERR

# Help key edit. UNSUPPORTED: the help key during INPUT is echoed (no edit). During a program
# statement input it would edit KBUF with the screen editor, which is not in Stage 4.
INLHLP:
	test BYTE PTR F_EDPG, 255
	jnz INLHL0			# BRIF do help edit on program statement
# Help during input statement
	jmp INLOP3			# Just output char

# DO EDIT ON DOT (UNSUPPORTED: screen editor)
INLHL0:
	jmp ADVERR			# UNSUPPORTED: help-key edit of KBUF (screen editor not in Stage 4)
	pop eax				# Remove call to this routine (4 bytes)

# See if edit on error line number
	mov dx, WORD PTR ERRLIN
	call ERRED2			# If exists, do edit on error line number

# See if edit on current line number
	mov dx, WORD PTR CURLIN
	call ERRED2			# If exists, do edit on current line number
# ERRLIN AND CURLIN ARE DIRECT, DO EDIT ON KBUF
	call DEPTR			# Remove line pointers from program
	mov ebx, offset KBUF
	push ebx
	dec ebx
	call SCNEXT			# Remove any line pointers from KBUF
	pop ebx
	jmp ERED5

# ---------------------------------------------------------------- ERREDT (automatic edit for errors)
# ERREDT is called by READY after a syntax error. A direct-mode error (ERRLIN = 65535) returns
# to the caller as in the original. A program-line error would enter the screen editor, which is
# UNSUPPORTED in Stage 4: ERRED2 ends in ADVERR for that case (the original's JMP EREDIT).
	.globl ERREDT
ERREDT:
	mov BYTE PTR ERRFLG, al		# Reset the flag to call edit
	mov dx, WORD PTR ERRLIN		# Get the line number
ERRED2:
	cmp dx, 65535			# See if it was direct
	jz ERRED3			# Go back if direct
	or dx, dx
	jz ERRED3			# Go back if zero
	# The original enters the screen editor (EREDIT) on the bad line. There is no screen editor in
	# this stage, so the line is not loaded for editing and nothing is executed. The error message
	# and "Ok" have already been printed by then. Printing "Advanced Feature" here was wrong: it
	# re-ran the stale text pointer and reported a second, false error (see NOTES.md).
	ret
ERRED3:
	ret
