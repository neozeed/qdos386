# NEXT86: NEXT, NEXTS, NEXTC and the loop-continuation code (NEXT86.ASM, whole module).
# The FOR entry layout is in forent.inc. After FNDFOR returns E, the chop MOV SP,E leaves the
# entry's pad, token and variable (B..B+5) below SP; NEXTC/FINNXT re-create the variable word by
# pushing it (the placeholder push), and NXTCON pushes the token word again.
# Stack widths: pointers 4 bytes (variable, text, line pointer, upper pointer); flags stand-ins 4 bytes;
# the sign word 2 bytes (AH = sign); integer values 2 bytes.

	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"
	.include "forent.inc"

	.section .text
	.globl NEXT, NEXTS

NF1:	jmp NFERR
NEXT:
	lahf				# FLAGS TO AH
	xchg ah, al
	push eax			# stand-in (4 bytes)
	mov al, 1
	jmp NEXTS1
NEXTS:	xor al, al			# FLAG THAT "FOR IS USING "NEXT
NEXTS1:	mov BYTE PTR NXTFLG, al
	pop eax				# (stand-in)
	xchg al, ah
	sahf
	mov edx, 0			# NO VARIABLE YET (MOV keeps the flags)
NEXTC:
	mov DWORD PTR NXTTXT, ebx	# SAVE STARTING TEXT POINTER
	jz NEXTC1
	call PTRGET			# EDX = loop variable pointer, EBX advanced
NEXTC1:
	mov DWORD PTR TEMP, ebx		# SAVE TEXT POINTER (TEMP is the text pointer in NEXT)
	call FNDFOR			# TRY TO FIND A "FOR" ENTRY (EBX = E, EDX = its variable)
	jnz NF1				# NEXT W/O FOR ERROR
	mov esp, ebx			# SETUP STACK BY CHOPPING HERE (B..B+5 are discarded)
	mov esi, DWORD PTR NXTTXT
	cmp DWORD PTR [ebx+FE_NXT], esi	# COMPARE TEXT POINTERS AT THE START OF THIS NEXT
	jnz NF1
	push edx			# PLACEHOLDER: re-creates the entry's variable pointer below E
	mov ah, BYTE PTR [ebx+FE_SGN]	# sign of the increment
	push ax				# (AH = sign)
	push edx			# loop variable pointer, popped again below
	add ebx, FE_STP			# BX = step field (E+6)
	test BYTE PTR [ebx-1], 0x80	# TYPE BYTE (E+5): minus means integer
	js INTNXT			# IF INTEGER HANDLE SEPARATELY
	mov ecx, 2			# two words
	cld				# DF=0 SO MOVSW WILL INCREMENT
	mov esi, ebx
	mov edi, offset _FACLO
	rep movsw			# GET STEP INTO FAC (4 bytes)
	pop ebx				# LOOP VARIABLE POINTER (popped as the temporary)
	push esi			# UPPER VALUE POINTER (E+10)
	push ebx			# LOOP VARIABLE POINTER
	test BYTE PTR NXTFLG, 0xFF
	jnz NXTDO
	mov esi, offset FVALSV		# FETCH INITIAL VALUE
	sub edi, 4			# POINT DI BACK TO FACLO
	mov ecx, 2			# TWO WORD MOVE
	rep movsw
	xor al, al			# CONTINUE "NEXT" WITH INITIAL VALUE
	jz NXTDOA
NXTDO:	call FADDS			# FAC = FAC + loop variable (EBX = its pointer)
NXTDOA:	pop edi				# POP POINTER TO LOOP VARIABLE
	mov esi, offset _FACLO		# WILL MOVE FAC TO THERE
	mov ecx, 2
	cld
	rep movsw
	pop esi				# GET ENTRY POINTER (upper value pointer)
	mov dx, WORD PTR [esi]		# upper value, low word
	mov cx, WORD PTR [esi+2]	# upper value, high word
	add esi, 4			# LINE NUMBER POINTER (E+14)
	push esi			# SAVE ENTRY POINTER
	call FCOMP			# COMPARE THE NOS. (AL: -1 if FAC less, 0 if equal, 1 if greater)
	jmp FINNXT

INTNXT:
	add ebx, 4			# SKIP 4 DUMMY BYTES: BX = integer step (E+10)
	mov cx, WORD PTR [ebx]		# FETCH STEP TO CX
	add ebx, 2			# BX = integer upper value (E+12)
	pop esi				# PTR TO LOOP VARIABLE
	mov dx, WORD PTR [esi]
	test BYTE PTR NXTFLG, 0xFF	# SEE IF "FOR IS USING "NEXT CODE
	jnz INXTDO			# NO, JUST CONTINUE
	mov dx, WORD PTR FVALSV		# GET THE INITIAL VALUE
	jmp IFORIN			# CONTINUE FIRST ITERATION CHECK
INXTDO:	add dx, cx
	jo OV1
IFORIN:
	mov WORD PTR [esi], dx		# STORE THE LOOP VARIABLE
	push dx				# SAVE THE VALUE OF THE LOOP VAR.
	mov dx, WORD PTR [ebx]		# INTEGER UPPER VALUE (E+12)
	add ebx, 2			# LINE NUMBER POINTER (E+14)
	pop ax
	push ebx			# SAVE ENTRY POINTER
	call ICMPA			# COMPARE (AX) WITH (DX)

FINNXT:
	pop ebx				# POP "FOR ENTRY" PTR (line pointer E+14)
	pop cx				# GET SGN OF INCREMENT (CH) (the placeholder remains on the stack)
	sub al, ch			# SUBTRACT THE INCREMENT'S SIGN FROM THAT OF (CURRENT - FINAL)
	mov dx, WORD PTR [ebx]		# LINE NUMBER OF THE FOR (E+14)
	mov ecx, DWORD PTR [ebx+2]	# TEXT POINTER INTO THE FOR (E+16)
	lea ebx, [ebx+6]		# BX = end of the entry (next entry base, E+20); LEA keeps the flags
	jz LOOPDN			# JUMP IF DONE
	mov WORD PTR CURLIN, dx		# STORE LINE NO.
	mov ebx, ecx			# BX = text pointer into the FOR
	jmp NXTCON

LOOPDN:
	mov esp, ebx			# ELIMINATE FOR ENTRY & SAVE UPDATED STACK
	mov DWORD PTR SAVSTK, ebx
	mov ebx, DWORD PTR TEMP		# RESTORE TEXT PTR
	cmp BYTE PTR [ebx], 0x2C	# COMMA? (054 octal)
	jnz NS1				# "NEXT
LOOPD1:
	mov BYTE PTR NXTFLG, 0xFF	# Set NXTFLG non-zero since NEXT couldn't have been called by FOR
	call CHRGTR			# SKIP COMMA
	mov edx, esp			# [EDX]=[ESP] which will never match any VARPTR (see FNDFOR)
	call NEXTC			# PROCESS NEXT NEXT
OV1:	jmp OVERR
NS1:	jmp NEWSTT
