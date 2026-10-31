# BIMISC.ASM -> bimisc.S
# Miscellaneous interpreter routines: stack-room checks (GETSTK, STKINI, OMERR, REASON),
# NEW / RUN / CLEAR support (SCRATCH, CLEARC, CLEAR), SYNCHR, the event-trap code (ON/OFF/STOP
# trap flags, GOTRP), RESTORE, STOP / END / CONT, TRON / TROFF, NULL, SWAP, ERASE, SUBDE and ISFLIO.
#
# Port notes (see NOTES.md, Stage 3 BIMISC):
#  - Pointers are 4 bytes on the stack and in memory; value words are 2 bytes.
#    The FNDFOR stopper stays a 2-byte zero word (SAVSTK = TOPMEM-2), as in the original.
#    The return address is 4 bytes.
#  - Trap entries are 5 bytes: flag byte, then the 4-byte GOSUB line pointer.
#  - CLI/STI are removed. The port has no interrupt-driven trap requests (Stage 4 polls).
#  - Out-of-scope items in this file are kept only where they are the original's control flow
#    (GRPRST and SNDINI are called as Stage 4 device stubs).

	.intel_syntax noprefix
	.include "oem.inc"

# Local copies of constants from other files. Keep in sync with the owners.
	.set ERRCN, 17			# "Can't continue" (gwdata_errors.inc)
	.set ERROM, 7			# "Out of memory" (gwdata_errors.inc)
	.set T_ON, 1			# trap on (BIMISC.ASM 343; also in gwsts.S)
	.set T_STOP, 2			# trap stop
	.set T_REQ, 4			# trap request

	.section .text

	.globl STOPRG, TON, TOFF, CLEARC, STOP, ISLET, ISLET2, STKINI, GETSTK
	.globl SCRATH, SCRTCH, STPEND, CONT, ENDST, GTMPRT, RUNC, ENDCON, RESTORE, CLEARC
	.globl RESFIN, STKERR, REASON, OMERR, OMERRR, NODSKS, DCOMPR, SYNCHR
	.globl ONTRP, OFFTRP, STPTRP, RSTTRP, REQTRP, SETTRP, FRETRP, INITRP, GOTRP
	.globl CTROPT, CTRLPT, NULL, SWAP, ERASE, POPAHT, CLEAR, SUBDE, ISFLIO
	.globl T_ON, T_STOP, T_REQ

# -------------------------------------------------------------------------------------
# GETSTK: make sure at least 2*NUMLEV bytes remain above the stack pointer after CL words
# (2 bytes each) are pushed. Call: mov cl,N / call GETSTK. Preserves [H,L] (EBX).
# Room exists when SP >= STKLOW + 2*count + 2*NUMLEV; otherwise OMERR.
# -------------------------------------------------------------------------------------
GETSTK:
	push ebx			# SAVE [H,L]
	mov ebx, DWORD PTR STKLOW	# in GIO86, the stack can grow down to STKLOW
	movzx ecx, cl			# CL = count of 2-byte entries
	add ebx, ecx
	add ebx, ecx			# SEE IF WE CAN HAVE THIS MANY
	add ebx, 2*NUMLEV		# [H,L] = candidate + margin
	cmp esp, ebx			# CF=1 if SP < candidate (no room); flags kept by POP
	pop ebx				# GET BACK ORIGINAL [H,L]
	jb OMERR
	ret

# OMERR fixes program links (starting at TXTTAB), resets SAVSTK to TOPMEM-2
# and issues an Out-of-Memory error message.
OMERR:
	call LINKER			# fix links in case OMERR came after a deleted line
	mov ebx, DWORD PTR TOPMEM	# eliminate all stack context to free some memory
	dec ebx				# MAKE SURE THE FNDFOR STOPPER IS SAVED
	dec ebx
	mov DWORD PTR SAVSTK, ebx	# PLACE STACK IS RESTORED FROM
OMERRR:
	mov edx, ERROM			# "OUT OF MEMORY"
	jmp ERROR

# REASON: make sure FRETOP is above [H,L]; garbage-collect if needed.
REASON:
	cmp DWORD PTR FRETOP, ebx	# CMP FRETOP,BX
	jb REASON1			# not enough room: collect garbage
	ret
REASON1:
	mov al, BYTE PTR CHNFLG
	or al, al			# can't garbage collect if CHAINING
	jnz OMERR
	push cx				# SAVE ALL REGS (values 2 bytes, pointer 4)
	push dx
	push ebx
	call GARBA2			# DO A GARBAGE COLLECTION
	pop ebx
	pop dx
	pop cx
	cmp DWORD PTR FRETOP, ebx
	jb OMERR			# still no room: "out of memory", don't touch stack
	ret

# -------------------------------------------------------------------------------------
# NEW (NODSKS / SCRATH / SCRTCH) and RUN / CLEAR setup
# -------------------------------------------------------------------------------------
NODSKS:
	xor al, al			# no disk: AL=0, then falls into SCRATH (NEW), which sets up the program text
					# and variable space that INITSA relies on
# THE "NEW" COMMAND CLEARS THE PROGRAM TEXT AS WELL AS VARIABLE SPACE
SCRATH:
	jz SCRATHOK			# MAKE SURE THERE IS A TERMINATOR
	ret
SCRATHOK:
SCRTCH:
	mov ebx, DWORD PTR TXTTAB	# GET POINTER TO START OF TEXT
	call TOFF			# TURN OFF TRACE. SET [A]=0.
	mov BYTE PTR PROFLG, al		# NO LONGER A PROTECTED FILE
	mov BYTE PTR AUTFLG, al		# CLEAR AUTO MODE
	mov BYTE PTR PTRFLG, al		# SAY NO POINTERS EXIST
	mov DWORD PTR [ebx], 0		# SAVE THE 4-BYTE END-OF-PROGRAM LINK (a zero link)
	add ebx, 4
	mov DWORD PTR VARTAB, ebx	# NEW START OF VARIABLES
	mov ebx, DWORD PTR TXTTAB	# NEW/startup: text pointer one before the start, as before
	dec ebx
	jmp CLEARC			# and set up ARYTAB/STREND (INITSA relies on this)

# RUNC: start executing the first line (RUN with no line number, and the edit path in FINI).
RUNC:
	mov ebx, DWORD PTR TXTTAB	# any lines at all? (the first line's 4-byte link)
	cmp DWORD PTR [ebx], 0
	jnz RUNC1
	jmp READY			# empty program: nothing to run
RUNC1:
	dec ebx				# TXTTAB-1 holds CR: NEWSTT's GONZER takes it as the end of a line

# CLEARC initializes variable and array space (ARYTAB, STREND), resets the stack (falls into
# STKINI), and resets the trap, random-number and error state. [H,L] is preserved.
CLEARC:
	mov DWORD PTR TEMP, ebx		# SAVE [H,L] IN TEMP
	call INITRP			# INIT TRAP TABLE
	call GRPRST			# Stage 4 device stub (graphics reset)
	call INITRP			# Initialize trapping
	call SNDINI			# Stage 4 device stub (SOUND init)
	mov al, BYTE PTR MRGFLG		# DOING A CHAIN MERGE?
	or al, al
	jnz LEVDTB			# LEAVE DEFAULT TABLE ALONE
	xor al, al
	mov BYTE PTR OPTFLG, al		# INDICATE NO "OPTION" HAS BEEN SEEN
	mov BYTE PTR OPTVAL, al		# DEFAULT TO "OPTION BASE 0"
	mov ch, 26			# INITIALIZE THE DEFAULT VALTYPE TABLE
	mov ebx, offset DEFTBL
LOPDFT:
	mov BYTE PTR [ebx], 4		# SINGLE PRECISION
	inc ebx
	dec ch
	jnz LOPDFT
LEVDTB:
	mov edx, offset RNDCOP		# RESET THE RANDOM NUMBER GENERATOR
	mov ebx, offset RNDX		# SEED IN RNDX
	call MOVE
	xor eax, eax
	mov BYTE PTR ONEFLG, al		# RESET ON ERROR FLAG FOR RUNS
	mov ebx, eax			# BX=0: RESET ERROR LINE NUMBER (ONELIN=0)
	mov WORD PTR ONELIN, bx
	mov DWORD PTR OLDTXT, ebx	# MAKE CONTINUING IMPOSSIBLE
	mov WORD PTR ERRLIN, bx		# Clear error line number
	mov ebx, DWORD PTR MEMSIZ
	mov al, BYTE PTR CHNFLG		# ARE WE CHAINING?
	or al, al
	jnz GODFRE			# FRETOP IS GOOD, LEAVE IT ALONE
	mov DWORD PTR FRETOP, ebx	# FREE UP STRING SPACE
GODFRE:
	xor al, al			# MAKE SURE [A] IS ZERO, CC'S SET
	call RESTORE			# RESTORE DATA
	mov ebx, DWORD PTR VARTAB	# GET START OF VARIABLE SPACE
	mov DWORD PTR ARYTAB, ebx	# SAVE IN START OF ARRAY SPACE
	mov DWORD PTR STREND, ebx	# AND END OF VARIABLE STORAGE
	mov al, BYTE PTR MRGFLG		# DOING CHAIN MERGE?
	or al, al
	jnz CLRCHK
	call CLSALL			# IF NOT, CLOSE FILES
CLRCHK:
	mov al, BYTE PTR NLONLY		# GET LOAD FLAGS
	and al, 1			# CURRENTLY LOADING?
	jnz CLRDS1			# YES, LEAVE OTHERS OPEN
	mov BYTE PTR NLONLY, al		# ENABLE CLOSE ALL
CLRDS1:

# STKINI resets the stack pointer, eliminating GOSUB and FOR context. String temporaries are
# freed, SUBFLG is reset, and a zero (non-FOR) stopper is put on the stack so FNDFOR always
# finds a non-FOR entry at the bottom. Exit: [A]=0, [D,E] preserved.
STKINI:
	pop ecx				# GET RETURN ADDRESS HERE (4 bytes)
	mov ebx, DWORD PTR TOPMEM
	dec ebx				# TAKE INTO ACCOUNT FNDFOR STOPPER
	dec ebx
	mov DWORD PTR SAVSTK, ebx	# MAKE SURE SAVSTK OK JUST IN CASE
	inc ebx				# INCREMENT BACK FOR SP
	inc ebx
STKERR:
	mov esp, ebx			# INITIALIZE STACK
	mov ebx, offset TEMPST
	mov DWORD PTR TEMPPT, ebx	# INITIALIZE STRING TEMPORARIES
	call CLROVC			# BACK TO NORMAL OVERFLOW PRINT MODE
	call FINPRT			# CLEAR PTRFIL, OTHER I/O FLAGS
	xor eax, eax			# ZERO OUT A
	mov ebx, eax			# ZERO OUT [H,L]
	mov WORD PTR PRMLEN, bx		# FLAG NO ACTIVE PARAMETERS
	mov BYTE PTR NOFUNS, al		# INDICATE NO USER FUNCTIONS ACTIVE
	mov WORD PTR PRMLN2, bx		# NO PARAMETERS BEING BUILT
	mov WORD PTR FUNACT, bx		# SET NUMBER OF FUNCTIONS ACTIVE TO 0
	mov DWORD PTR PRMSTK, ebx	# AND NO PARAMETER BLOCKS ON THE STACK
	mov BYTE PTR SUBFLG, al		# ALLOW SUBSCRIPTS
	push ebx				# PUT ZERO (NON $FOR,$GOSUB) ON THE STACK (2 bytes)
	push ecx			# PUT RETURN ADDRESS BACK ON
GTMPRT:
	mov ebx, DWORD PTR TEMP		# GET SAVED [H,L]
	ret

# -------------------------------------------------------------------------------------
# DCOMPR, SYNCHR
# -------------------------------------------------------------------------------------
DCOMPR:
	cmp ebx, edx
	ret

# SYNCHR: the byte following "call SYNCHR" is the token to match against the text at [BX].
# On a match, [BX] is advanced past it and the return address skips the byte. Otherwise SNERR.
SYNCHR:
	pop esi				# POP SI: address of the token byte
	mov edi, ebx			# MOV DI,BX
	cld
	cmpsb				# CMPSB: is the char the right one? (SI and DI advance)
	push esi			# PUSH SI: return past the token byte
	mov ebx, edi			# MOV BX,DI (text pointer past the matched char)
	jnz SYNERR			# GIVE ERROR IF CHARS DON'T MATCH
	mov al, BYTE PTR [ebx]		# GET IT
	cmp al, ':'			# IS IT END OF STATEMENT OR BIGGER
	jb SYNCON
	ret
SYNCON:
	jmp CHRCON			# REST OF CHRGET
SYNERR:
	jmp SNERR

# -------------------------------------------------------------------------------------
# TRAP ROUTINES: ON, OFF, STOP, INIT, REQUEST, FREE, RESET.
# Each trap entry is 5 bytes: flag byte at [BX], then the 4-byte GOSUB line pointer at [BX+1].
# -------------------------------------------------------------------------------------

# TURN TRAP ON
ONTRP:
	mov al, BYTE PTR [ebx]
	and al, T_REQ			# LEAVE REQUEST BIT
	or al, T_ON			# ADD ON BIT
	cmp al, BYTE PTR [ebx]
	mov BYTE PTR [ebx], al
	jz ONTRP0			# NO CHANGE IN STATUS
	and al, T_REQ
	jnz SETTP2			# DELAYED TRAP REQUEST
ONTRP0:
	ret

# TURN TRAP OFF
OFFTRP:
	mov al, BYTE PTR [ebx]
	mov BYTE PTR [ebx], 0
	jmp FRECHK			# FREE OUTSTANDING REQUEST

# STOP TRAP
STPTRP:
	mov al, BYTE PTR [ebx]
	lahf				# PUSH PSW
	xchg al, ah
	push ax
	xchg al, ah
	or al, T_STOP
	mov BYTE PTR [ebx], al
	pop ax				# POP PSW
	xchg al, ah
	sahf
FRECHK:
	xor al, T_ON+T_REQ		# STOP OR OFF ACTIVE REQUEST?
	jz FRETP2			# YES, FREE ACTIVE REQUEST COUNT
	ret

# RESET STOP ON TRAP
RSTTRP:
	mov al, BYTE PTR [ebx]
	and al, T_ON+T_REQ
	cmp al, BYTE PTR [ebx]
	mov BYTE PTR [ebx], al
	jnz SETCHK			# IF NEW STATUS, CHECK FOR SETTRP
	ret

# REQUEST TRAP
REQTRP:
	mov al, BYTE PTR [ebx]
	and al, T_ON
	jz REQTPX			# TRAP NOT ON
	mov al, BYTE PTR [ebx]
	or al, T_REQ
	cmp al, BYTE PTR [ebx]
	jz REQTPX			# NO CHANGE
	mov BYTE PTR [ebx], al
SETCHK:
	xor al, T_ON+T_REQ
	jz SETTP2			# GO SET TRAP GLOBAL FLAG
REQTPX:
	ret

# SET TRAP GLOBAL FLAG
SETTRP:
SETTP2:
	mov al, BYTE PTR ONGSBF
	inc al
	mov BYTE PTR ONGSBF, al
	ret

# FREE TRAP
FRETRP:
	mov al, BYTE PTR [ebx]
	and al, T_ON+T_STOP
	cmp al, BYTE PTR [ebx]
	mov BYTE PTR [ebx], al
	jnz FRETP2			# DECR GLOBAL FLAG ALSO
FRETP1:
	ret
FRETP2:
	mov al, BYTE PTR ONGSBF
	sub al, 1
	jb FRETP1
	mov BYTE PTR ONGSBF, al
	ret

# INITIALIZE TRAP TABLE (NUMTRP entries of 5 bytes, all off, GOSUB pointer cleared)
INITRP:
	mov ebx, offset TRPTBL		# [H,L]=TRAP TABLE ADDRESS
	mov ch, NUMTRP			# [B]=NUMBER OF TRAPS
	xor eax, eax
INITP0:
	mov BYTE PTR [ebx], al		# TRAP OFF
	mov DWORD PTR [ebx+1], eax	# CLEAR GOSUB ADDR TOO
	add ebx, 5
	dec ch
	jnz INITP0
	mov BYTE PTR ONGSBF, al
	ret

# GO TO TRAP IF ANY ENABLED AND REQUESTED
GOTRP:
	mov al, BYTE PTR ONEFLG
	or al, al
	jz GOTRPOK
	ret				# CAN'T TRAP FROM ERROR TRAP
GOTRPOK:
	push ebx			# SAVE TEXT POINTER
	mov bx, WORD PTR CURLIN
	mov al, bh
	and al, bl
	inc al
	jz GOTRPX			# CAN'T TRAP FROM DIRECT STATEMENT

# CHECK FOR ENABLED REQUESTS
	mov ebx, offset TRPTBL
	mov ch, NUMTRP			# [H,L]=TRAP TABLE ADDRESS, [B]=# OF TRAPS
GOTRPL:
	mov al, BYTE PTR [ebx]
	cmp al, T_ON+T_REQ
	jz GOTRP2			# ENABLED AND REQUESTED
GOTRP1:
	add ebx, 5			# NEXT ENTRY (5 bytes)
	dec ch
	jnz GOTRPL
GOTRPX:
	pop ebx				# RESTORE TEXT POINTER
	ret

# TRAP REQUEST FOUND
GOTRP2:
	push cx				# SAVE STATE IN CASE ADDR IS ZERO
	mov edx, DWORD PTR [ebx+1]	# DE=TRAP GOSUB ADDR (4 bytes)
	test edx, edx			# GOSUB ADDR NOT SPECIFIED, TRY NEXT TRAP
	pop cx				# RESTORE COUNTER (POP does not change flags)
	jz GOTRP1
	push edx			# PUT GOSUB ADDR ON STACK
	push ebx			# PUT FLAG ADDR ON STACK
	call FRETRP			# FREE TRAP REQUEST
	call STPTRP			# PUT STOP ON TRAP
	mov cl, 5			# MAKE SURE ROOM IN STACK: 2 pointers = 4 words, plus 1 margin word
	call GETSTK
	pop ecx				# BC=TRAP FLAG ADDRESS
	pop edx				# DE=GOSUB ROUTINE ADDR
	pop ebx				# HL=TEXT POINTER (pushed at entry)
	pop esi				# drop the return address of GOTRP (original XTHL sequence)
	jmp GOSUB2			# GOSUB TO ADDR IN DE, FLAG ADDR IN BC, TEXT PTR IN HL

# -------------------------------------------------------------------------------------
# RESTORE, STOP, END
# -------------------------------------------------------------------------------------
RESTORE:
	xchg ebx, edx			# SAVE [H,L] IN [D,E]
	mov ebx, DWORD PTR TXTTAB
	jz BGNRST			# RESTORE DATA POINTER TO BEGINNING OF PROGRAM
	xchg ebx, edx			# TEXT POINTER BACK TO [H,L]
	call LINGET			# GET THE FOLLOWING LINE NUMBER
	push ebx			# SAVE TEXT POINTER
	call FNDLIN			# FIND THE LINE NUMBER; line pointer returned in ECX
	mov ebx, ecx			# GET POINTER TO LINE IN [H,L]
	pop edx				# TEXT POINTER BACK TO [D,E]
	jb RESTORE1
	jmp USERR			# SHOULD HAVE FOUND LINE
RESTORE1:
BGNRST:
	dec ebx				# INITIALIZE DATPTR TO [TXTTAB]-1
RESFIN:
	mov DWORD PTR DATPTR, ebx	# READ FINISHES COME TO RESFIN
	xchg ebx, edx			# GET THE TEXT POINTER BACK
	ret

STOP:
	jz STOPRG			# RETURN IF NOT CONTROL-C AND MAKE SURE "STOP"
	ret				# STATEMENTS HAVE A TERMINATOR
STOPRG:
	inc al
	jmp CONSTP
ENDST:
	jz ENDSTOK			# MAKE SURE "END" STATEMENTS HAVE A TERMINATOR
	ret
ENDSTOK:
	xor al, al			# clear ONEFLG: we aren't within an error-trapping routine
	mov BYTE PTR ONEFLG, al
ENDS1:
	pushfd				# PRESERVE CONDITION CODES OVER CALL TO CLSALL
	jnz ENDS1A
	call CLSALL
ENDS1A:
	popfd				# RESTORE CONDITION CODES
CONSTP:
	mov DWORD PTR SAVTXT, ebx	# SAVE FOR "CONTINUE"
	mov ebx, offset TEMPST		# RESET STRING TEMP POINTER
	mov DWORD PTR TEMPPT, ebx	# SAVE IN CASE ^C PRINT USING
	jmp ENDSPOP			# (original skipped the next instruction with an MVI-opcode trick)
STPEND:
	or al, 0377			# SET NON-ZERO TO FORCE PRINTING OF BREAK MESSAGE
ENDSPOP:
	pop ecx				# POP OFF NEWSTT ADDRESS
ENDCON:
	mov bx, WORD PTR CURLIN		# SAVE CURLIN
	push ebx				# SAVE LINE TO PRINT
	pushfd				# SAVE THE MESSAGE FLAG (zero means don't print "BREAK")
	mov al, bl
	and al, bh			# SEE IF IT WAS DIRECT
	inc al
	jz DIRIS			# IF NOT SET UP FOR CONTINUE
	mov WORD PTR OLDLIN, bx		# SAVE OLD LINE #
	mov ebx, DWORD PTR SAVTXT	# GET POINTER TO START OF STATEMENT
	mov DWORD PTR OLDTXT, ebx	# SAVE IT
DIRIS:
	xor al, al
	mov BYTE PTR CNTOFL, al		# FORCE OUTPUT
	call FINLPT
	call CRDONZ			# PRINT CR IF TTYPOS .NE. 0
	popfd				# GET BACK ^C FLAG
	mov ebx, offset BRKTXT		# "BREAK"
	jz ENDCONPRT
	jmp ERRFIN			# CALL STROUT AND FALL INTO READY
ENDCONPRT:
	jmp STPRDY			# POP OFF LINE NUMBER & FALL INTO READY

# -------------------------------------------------------------------------------------
# CTRLPT, DDT, CONT, NULL, TRON, TROFF
# -------------------------------------------------------------------------------------
CTROPT:
	mov al, CONTO			# PRINT AN ^O.
CTRLPT:
	push ax				# SAVE CURRENT CHAR
	sub al, 3			# CONTROL-C?
	jnz NTCTCT
	mov BYTE PTR CNTOFL, al		# RESET ^O FLAG
NTCTCT:
	mov al, '^'			# PRINT UP-ARROW.
	call OUTDO
	pop ax				# GET BACK CONTROL CHAR.
	add al, 0100			# MAKE PRINTABLE
	call OUTDO
	mov al, 0377			# MARK LINE AS NOT FOR INPUT
	call OUTDO
	jmp CRDO			# AND THEN SEND CRLF

CONT:
	mov ebx, DWORD PTR OLDTXT	# A STORED TEXT POINTER OF ZERO IS SETUP BY STKINI
	or ebx, ebx			# AND INDICATES THERE IS NOTHING TO CONTINUE
RESERR:
	mov edx, ERRCN			# "CAN'T CONTINUE"
	jnz CONTOK
	jmp ERROR
CONTOK:
	mov dx, WORD PTR OLDLIN		# SET UP OLD LINE # AS CURRENT LINE #
	mov WORD PTR CURLIN, dx
	ret

NULL:
	call GETBYT
	jz NULLOK			# MAKE SURE THERE IS A TERMINATOR
	ret
NULLOK:
	inc al				# CODE AT CRDO EXPECTS AT LEAST 1
	mov BYTE PTR NULCNT, al		# CHANGE NUMBER OF NULLS
	ret

TON:
	mov al, 0377			# "MVI A," NON-ZERO QUANTITY (nonzero is all that matters)
	jmp TRCSET
TOFF:
	xor al, al			# MAKE [A]=0 FOR NO TRACE
TRCSET:
	mov BYTE PTR TRCFLG, al		# UPDATE THE TRACE FLAG
	ret

# -------------------------------------------------------------------------------------
# SWAP, ERASE
# -------------------------------------------------------------------------------------
SWAP:
	call PTRGET			# [D,E]=POINTER AT VALUE #1
	push edx			# SAVE THE POINTER AT VALUE #1
	push ebx			# SAVE THE TEXT POINTER
	mov ebx, offset SWPTMP		# TEMPORARY STORE LOCATION
	call VMOVE			# SWPTMP=VALUE #1
	mov ebx, DWORD PTR ARYTAB	# GET ARYTAB SO CHANGE CAN BE NOTED
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# GET THE TEXT POINTER BACK AND SAVE CURRENT [ARYTAB]
	call GETYPR
	push ax				# SAVE THE TYPE OF VALUE #1
	call SYNCHR
	.byte 44			# variables delimited by a comma
	call PTRGET			# [D,E]=POINTER AT VALUE #2
	pop ax
	mov ch, al			# [B]=TYPE OF VALUE #1
	call GETYPR			# [A]=TYPE OF VALUE #2
	cmp al, ch			# MAKE SURE THEY ARE THE SAME
	jz SWAP5
	jmp TMERR			# IF NOT, "TYPE MISMATCH" ERROR
SWAP5:
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# [H,L]=OLD [ARYTAB]; SAVE THE TEXT POINTER
	xchg ebx, edx			# [D,E]=OLD [ARYTAB]
	push ebx			# SAVE THE POINTER AT VALUE #2
	mov ebx, DWORD PTR ARYTAB	# GET NEW [ARYTAB]
	cmp ebx, edx
	jnz GFCERR			# IF ITS CHANGED, ERROR
	pop edx				# [D,E]=POINTER AT VALUE #2
	pop ebx				# [H,L]=TEXT POINTER
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# SAVE THE TEXT POINTER ON THE STACK
	push edx			# SAVE THE POINTER AT VALUE #2
	call VMOVE			# TRANSFER VALUE #2 INTO VALUE #1'S OLD POSITION
	pop ebx				# [H,L]=POINTER AT VALUE #2
	mov edx, offset SWPTMP		# LOCATION OF VALUE #1
	call VMOVE			# TRANSFER SWPTMP=VALUE #1 INTO VALUE #2'S OLD POSITION
	pop ebx				# GET THE TEXT POINTER BACK
	ret

GFCERR:
	jmp FCERR			# GIVE A FUNCTION CALL ERROR

# ERASE: PTRGET (with SUBFLG=1) returns CX = start of the array entry and DX = its length.
ERASE:
	mov al, 1
	mov BYTE PTR SUBFLG, al		# THAT THIS IS "ERASE" CALLING PTRGET
	call PTRGET			# GO FIND OUT WHERE TO ERASE
	jnz GFCERR			# PTRGET DID NOT FIND VARIABLE!
	push ebx			# SAVE THE TEXT POINTER
	mov BYTE PTR SUBFLG, al		# ZERO OUT SUBFLG TO RESET "ERASE" FLAG
	mov ebx, ecx			# [B,C]=START OF ARRAY TO ERASE
	dec ecx				# BACK UP TO THE FRONT
	dec ecx				# NO VALUE TYPE WITHOUT LENGTH=2
	dec ecx				# BACK UP ONE MORE
LPBKNM:
	mov esi, ecx
	mov al, BYTE PTR [esi]		# GET A CHARACTER. ONLY THE COUNT HAS HIGH BIT=0
	dec ecx				# SO LOOP UNTIL WE SKIP OVER THE COUNT
	or al, al			# SKIP ALL THE EXTRA CHARACTERS
	js LPBKNM
	dec ecx
	dec ecx
	movzx edx, dx			# DX = entry length (16 bits)
	add ebx, edx			# [H,L]=THE END OF THIS ARRAY ENTRY
	xchg ebx, edx			# [D,E]=END OF THIS ARRAY
	mov ebx, DWORD PTR STREND	# [H,L]=LAST LOCATION TO MOVE UP
ERSLOP:
	cmp ebx, edx			# SEE IF THE LAST LOCATION IS GOING TO BE MOVED
	mov esi, edx
	mov al, BYTE PTR [esi]		# DO THE MOVE
	mov edi, ecx
	stosb
	lea edx, [edx+1]		# UPDATE THE POINTERS (LEA keeps the flags of CMP)
	lea ecx, [ecx+1]
	jnz ERSLOP			# MOVE THE REST
	dec ecx
	mov ebx, ecx			# SETUP THE NEW STORAGE END POINTER
	mov DWORD PTR STREND, ebx
	pop ebx				# GET BACK THE TEXT POINTER
	mov al, BYTE PTR [ebx]		# SEE IF MORE ERASURES NEEDED
	cmp al, 44			# ADDITIONAL VARIABLES DELIMITED BY COMMA
	jz ERASEMORE
	ret				# ALL DONE IF NOT
ERASEMORE:
	call CHRGTR
	jmp ERASE

# POPAHT: pop PSW and the text pointer (used by callers that saved flags with LAHF)
POPAHT:
	pop ax				# POP PSW
	xchg al, ah
	sahf
	pop ebx				# GET THE TEXT POINTER
	ret

# -------------------------------------------------------------------------------------
# ISLET: test for a letter. Carry on = not a letter; carry off = a letter.
# -------------------------------------------------------------------------------------
ISLET:
	mov al, BYTE PTR [ebx]
ISLET2:
	cmp al, 'A'
	jae ISLETOK
	ret				# IF LESS THAN "A", RETURN EARLY
ISLETOK:
	cmp al, 'Z'+1
	cmc
	ret

# -------------------------------------------------------------------------------------
# CLEAR [[a][,b [,c]]]
#   a is ignored (pre-FIVEO memory management)
#   b is the highest memory location available to BASIC
#   c is the number of bytes used for the stack
# -------------------------------------------------------------------------------------
GCLERC:
	jmp CLEARC			# GO TO CLEARC

CLEAR:
	lahf				# PUSH PSW
	xchg al, ah
	push ax
	xchg al, ah
	xor al, al			# Since FILTAB is reset below, close all files (even file 0,
	mov BYTE PTR NLONLY, al		# because of DYNAMIC FDB's)
	call CLSALL
	pop ax				# POP PSW
	xchg al, ah
	sahf
	jz GCLERC			# IF NO FORMULA JUST CLEAR
	cmp al, 44			# ALLOW NO STRING SPACE
	jz CSKPCM
	call INTID2			# Get dummy integer parameter into [D,E]
	dec ebx
	call CHRGTR			# SEE IF ITS THE END
	jz GCLERC
CSKPCM:
	call SYNCHR
	.byte 44
	jz GCLERC
	mov edx, DWORD PTR TOPMEM	# Use current top of memory as default
	cmp al, 44
	jz CLEARS			# SHOULD FINISH THERE
	call GETMPM			# GET MEMORY SIZE PARAMETER (an offset into the workspace, 16-bit)
	add edx, OFFSET TSTACK		# rebase to an address: the top of memory is TSTACK+n
	push ebx			# save text pointer
	mov ebx, DWORD PTR MAXMEM	# [H,L]=highest byte available to BASIC
	cmp ebx, edx
	jae CLEARM
	jmp FCERR			# branch if user tried to get more
CLEARM:
	pop ebx				# restore text pointer
CLEARS:
	dec ebx				# BACK UP
	call CHRGTR			# GET CHAR
	push edx			# SAVE NEW HIGH MEM
	jz CDFSTK			# USE SAME STACK SIZE
	call SYNCHR
	.byte 44
	jz CDFSTK
	call GETMPM			# GET STACK SIZE PARAMETER
	dec ebx
	call CHRGTR
	jz CLEARSZ
	jmp SNERR
CLEARSZ:
CLEART:
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# SAVE TEXT POINTER
	push ebx			# SAVE CANDIDATE FOR TOPMEM
	mov ebx, 2*NUMLEV+20		# CHECK STACK SIZE IS REASONABLE
	cmp ebx, edx
	jae GOMERR
	pop ebx				# [H,L]=candidate for TOPMEM
	call SUBDE			# [D,E]=[H,L]-[D,E]=new stack bottom
	jb GOMERR			# WANTED MORE THAN TOTAL!
	push ebx			# SAVE CANDIDATE TOPMEM (BX is unchanged by SUBDE)
	mov ebx, DWORD PTR VARTAB	# TOP LOCATION IN USE
	mov ecx, 20			# LEAVE BREATHING ROOM
	add ebx, ecx
	cmp ebx, edx			# ROOM?
	jae GOMERR			# NO, DON'T EVEN CLEAR
	xchg ebx, edx			# NEW STACK BASE IN [H,L]
	mov DWORD PTR STKLOW, ebx	# set bottom of stack limit
	mov DWORD PTR FILTAB, ebx	# set FDB chain base (no files are open)
	dec ebx
	mov BYTE PTR [ebx], 0		# set string terminator so VAL$ will always work
	dec ebx
	mov DWORD PTR MEMSIZ, ebx	# SET UP NEW STACK BOTTOM
	pop ebx				# HL=highest RAM available to BASIC
	mov DWORD PTR TOPMEM, ebx	# SAVE IT, IT MUST BE OK
	pop ebx				# REGAIN THE TEXT POINTER
	jmp GCLERC			# GO CLEAR

GETMPM:
	call FRMQNT			# EVALUATE FORMULA
	or dx, dx			# Memory size = 0?
	jnz GETMPM1
	jmp FCERR			# Yes, error
GETMPM1:
	movzx edx, dx			# size as an unsigned 16-bit value (UNSURE: see NOTES, CLEAR size)
	ret

GOMERR:
	jmp OMERR			# GIVE OM ERROR

# CLEAR default stack size = current stack size
CDFSTK:
	mov edx, DWORD PTR TOPMEM	# FIGURE OUT CURRENT STACK SIZE
	sub edx, DWORD PTR STKLOW	# SUB DX,STKLOW
	jmp CLEART

# [D,E]=[H,L]-[D,E]
SUBDE:
	mov eax, ebx
	sub eax, edx
	mov edx, eax
	ret

# ISFLIO: zero flag set if there is no current file pointer. [H,L] preserved.
ISFLIO:
	push ebx			# SAVE [H,L]
	mov ebx, DWORD PTR PTRFIL	# GET FILE POINTER
	or ebx, ebx			# NO ZERO?
	jnz isf_set			# a file pointer: a file is current
	cmp BYTE PTR MRGACT, 0		# no file pointer: a MERGE in progress is also a file (ZF=0 then)
isf_set:
	pop ebx				# RESTORE [H,L] (POP does not change the flags)
	ret
