# GWFOR: FNDFOR / LOOPER / POPGOF (GWMAIN.ASM 300-371) and the FOR statement (GWMAIN.ASM 1425-1622,
# including NXTCON). Stack entries use the layout in forent.inc (read it first).
#
# Frame approach for the FOR entry: after LPFORM, GETSTK checks 13 words, then `sub esp, FE_SIZE`
# reserves the 26-byte entry at B = esp. Its base is kept in FORFRMB (a memory word), so every field
# store goes through a fixed base, not through ESP (calls in between push and pop, so ESP is
# unchanged, but FORFRMB makes that independent of the push discipline). Only mov/lea/xchg/lahf/sahf
# appear between the NEXT-text store and the LAHF pair, so the flags from CHRGTR reach NEXTS intact.
#
# Pointers: BX (text), EDX (loop variable), E = FNDFOR's returned pointer, all 4 bytes.
# Values: line numbers, integer step and upper, the sign/type bytes keep their original widths.

	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"
	.include "forent.inc"

	.section .bss
	.align 4
FORFRMB:	.skip 4				# base (B) of the FOR entry being built, valid only inside FOR

	.section .text
	.globl FNDFOR, FOR, NXTCON

# -------------------------------------------------------------------------------------------
# FNDFOR: find the FOR entry whose loop variable is EDX (EDX = 0 matches any; on a match EDX is
# the entry's variable). Exit: ZF=1 and EBX = E (the bottom of the entry, its NEXT text pointer);
# ZF=0 when there is no such entry.
# Stack at entry: [FNDFOR's return address (4)] [one more 4-byte return address or stand-in (4)] [entries]
# so the first entry base B is at SP+8.
# -------------------------------------------------------------------------------------------
FNDFOR:
	mov ebx, 8			# IGNORING NEWSTT AND THIS SUBROUTINE'S RETURN ADDRESSES (4 bytes each)
	add ebx, esp			# BX = B (the pad byte of the top entry)
LOOPER:
	inc ebx				# skip the pad byte (entries are token-first, 8086 used 2-byte entries)
	mov al, BYTE PTR [ebx]		# type of stack entry
	inc ebx
	cmp al, _WHILE
	jnz STKSRC
	add ebx, WHLSIZ			# WHILE entry: skip to the next entry base (WHLSIZ=10, see forent.inc)
	jmp LOOPER
STKSRC:
	cmp al, _FOR			# is this stack entry a FOR?
	jz FORFND
	ret				# no, so OK
FORFND:
	mov ecx, DWORD PTR [ebx]	# loop variable pointer of this FOR entry (4 bytes)
	add ebx, 4			# BX = E (the NEXT text pointer): the byte after the 4-byte variable pointer
	push ebx			# save E for POPGOF
	mov ebx, ecx			# BX = the entry's variable pointer
	or edx, edx			# "NEXT" statement without an argument?
	xchg ebx, edx			# (XCHG leaves the flags alone) DX gets the entry's variable
	jz POPGOF			# no argument: match any FOR entry
	xchg ebx, edx			# put the target back
	cmp ebx, edx
POPGOF:
	mov ecx, FE_NEXT		# distance from E to the next entry's base (20)
	pop ebx				# E
	jnz POPGOF2			# no match: keep scanning
	ret				# match: BX = E, ECX = FE_NEXT
POPGOF2:
	add ebx, ecx			# BX = next entry base
	jmp LOOPER

# -------------------------------------------------------------------------------------------
# FOR statement. The original is GWMAIN.ASM 1425-1606.
# -------------------------------------------------------------------------------------------
FOR:
	mov al, 100
	mov BYTE PTR SUBFLG, al		# DONT RECOGNIZE SUBSCRIPTED VARIABLES
	call PTRGET			# EDX = pointer to loop variable
	call SYNCHR
	.byte EQULTK			# SKIP OVER ASSIGNMENT "="
	push edx			# SAVE THE VARIABLE POINTER ON STACK
	mov DWORD PTR TEMP, edx		# AND IN TEMP (for later)
	mov al, BYTE PTR VALTYP		# REMEMBER THE LOOP VARIABLE TYPE
	push ax
	call FRMEVL			# GET THE START VALUE (BX = text pointer)
	pop ax				# REGET THE LOOP TYPE
	push ebx			# SAVE THE TEXT POINTER
	call DOCNVF			# FORCE CONVERSION TO LOOP TYPE
	mov ebx, offset FVALSV		# PLACE TO SAVE THE VALUE
	call MOVMF			# STORE FOR USE IN "NEXT" (MOVMF advances EBX by 4, flags kept)
	pop ebx				# GET BACK THE TEXT POINTER
	pop edx				# GET BACK THE VARIABLE POINTER
	pop ecx				# GET RID OF THE NEWSTT RETURN (4 bytes)
	push ebx			# SAVE THE TEXT POINTER (kept on the stack until LPFORM)
	call DATA			# SET BX=END OF STATEMENT
	mov DWORD PTR ENDFOR, ebx	# SAVE FOR COMPARISON
	# UNSURE: the original kept the variable pointer in DX across DATA. Here EDX is reloaded from TEMP
	# (TEMP holds the same pointer; the original also reads TEMP later in this statement).
	mov edx, DWORD PTR TEMP
	mov ebx, 4			# SET UP POINTER INTO STACK: the text pointer (4 bytes) is on top
	add ebx, esp			# BX = B, the first entry base
LPFORM:
	call LOOPER			# MUST HAVE VARIABLE POINTER IN EDX
	jnz NOTOL			# IF NO MATCHING ENTRY, DON'T ELIMINATE ANYTHING
	add ebx, ecx			# ECX = FE_NEXT: BX = base of the entry after the match
	push edx			# SAVE THE VARIABLE POINTER
	mov edx, DWORD PTR [ebx-4]	# FOR text pointer of the matching entry (E+16 .. E+19)
	push ebx			# SAVE THE STACK POINTER FOR THE COMPARISON
	mov ebx, DWORD PTR ENDFOR	# GET ENDING TEXT POINTER FOR THIS "FOR"
	cmp ebx, edx			# SEE IF THEY MATCH
	pop ebx				# GET BACK THE STACK POINTER
	pop edx				# GET BACK THE VARIABLE POINTER
	jnz LPFORM			# KEEP SEARCHING IF NO MATCH
	pop edx				# GET BACK THE TEXT POINTER
	mov esp, ebx			# DO THE ELIMINATION
	mov DWORD PTR SAVSTK, ebx	# UPDATE SAVED STACK
	mov ebx, edx			# [BX] = text pointer (the original's XCHG BX,DX)
	jmp FORGO
NOTOL:
	pop ebx				# GET BACK THE TEXT POINTER
FORGO:
	mov ecx, FE_WORDS		# MAKE SURE A WHOLE ENTRY (13 words) IS AVAILABLE
	call GETSTK			# (checks only; BX preserved)
	sub esp, FE_SIZE		# RESERVE THE 26-BYTE ENTRY: B = ESP
	mov DWORD PTR FORFRMB, esp
	mov edi, esp
	mov BYTE PTR [edi], 0		# pad byte (unused)
	mov BYTE PTR [edi+1], _FOR	# token
	mov eax, DWORD PTR TEMP
	mov DWORD PTR [edi+FE_VAR], eax	# loop variable pointer (B+2)
	mov eax, DWORD PTR ENDFOR
	mov DWORD PTR [edi+FE_EB+FE_FTX], eax	# text pointer into FOR = ENDFOR (E+16)
	mov ax, WORD PTR CURLIN
	mov WORD PTR [edi+FE_EB+FE_LIN], ax	# line number of the FOR (E+14)

	call SYNCHR
	.byte _TO			# "TO" IS NECESSARY
	call GETYPR			# SEE WHAT TYPE THIS VALUE HAS
	jnz TOSTR1
	jmp TMERR			# GIVE STRINGS A "TYPE MISMATCH"
TOSTR1:
	jb TONUM			# branch if numeric (JNAE in the original)
	jmp TMERR			# AS WELL AS DOUBLE-PRECISION
TONUM:
	pushfd				# SAVE THE INTEGER/FLOATING FLAG (sign flag = integer)
	call FRMEVL			# EVALUATE THE TARGET VALUE FORMULA
	popfd				# POP OFF THE FLAG
	push ebx			# SAVE THE TEXT POINTER
	js INTFOR			# minus = integer FOR
	jmp SNGFOR			# positive means single precision FOR-loop

INTFOR:
	call FRCINT			# COERCE THE FINAL VALUE (BX = integer)
	mov edi, DWORD PTR FORFRMB
	mov WORD PTR [edi+FE_EB+FE_IUPP], bx	# integer upper value (E+12)
	pop ebx				# text pointer
	mov dx, 1			# DEFAULT THE STEP TO BE 1
	mov al, BYTE PTR [ebx]		# SEE WHAT CHARACTER IS NEXT
	cmp al, STEPTK			# IS THERE A "STEP" CLAUSE?
	jnz INTSTP
	call GETINT			# IF SO, READ THE STEP INTO DX
INTSTP:
	mov edi, DWORD PTR FORFRMB
	mov WORD PTR [edi+FE_EB+FE_ISTP], dx	# integer step (E+10)
	push ebx			# SAVE THE TEXT POINTER
	mov bx, dx			# ISIGN works on the integer in BX
	call ISIGN			# SIGN OF THE STEP INTO AL
	pop ebx				# POP OFF THE TEXT POINTER
	jmp SIGNOK

SNGFOR:
	call FRCSNG
	call MOVRF			# UPPER VALUE in DX:CX (low word DX)
	pop ebx				# REGAIN TEXT POINTER
	mov edi, DWORD PTR FORFRMB
	mov WORD PTR [edi+FE_EB+FE_UPP], dx		# floating upper, low word (E+10)
	mov WORD PTR [edi+FE_EB+FE_UPP+2], cx		# floating upper, high word (E+12)
	mov cx, 0x8100			# 1.0 (LOW 201O*256)
	mov dh, cl
	mov dl, dh			# DX:CX = 1.0
	mov al, BYTE PTR [ebx]		# GET TERMINATING CHARACTER
	cmp al, STEPTK			# DO WE HAVE "STEP" ?
	mov al, 1			# DEFAULT SIGN OF THE STEP (MOV keeps the flags)
	jnz SNGONE			# no step: use 1.0
	call FRMCHK			# DON'T NEED TO CHECK THE TYPE
	push ebx
	call FRCSNG
	call MOVRF			# SET UP THE REGISTERS: step in DX:CX
	call SIGN			# SIGN OF THE STEP (AL)
	pop ebx
SNGONE:
	mov edi, DWORD PTR FORFRMB
	mov WORD PTR [edi+FE_EB+FE_STP], dx		# floating step, low word (E+6)
	mov WORD PTR [edi+FE_EB+FE_STP+2], cx		# floating step, high word (E+8)

# Common tail for both types: AL = sign of the step, BX = text pointer.
SIGNOK:
	or al, al			# IS THE SIGN OF THE STEP ZERO?
	jnz NT0STP			# No, then the step is not zero.
	mov al, 2			# force an infinite loop (ANSI): sign 2 can never match
NT0STP:
	mov cl, al			# CL = SIGN OF STEP
	call GETYPR			# AL = type byte (VALTYP-3: minus for integer)
	mov edi, DWORD PTR FORFRMB
	mov BYTE PTR [edi+FE_EB+FE_SGN], cl		# sign (E+4)
	mov BYTE PTR [edi+FE_EB+FE_TYP], al		# type (E+5)
	dec ebx				# MAKE SURE THE "FOR" ENDED PROPERLY
	call CHRGTR
	jz FORSTMT_OK
	jmp SNERR
FORSTMT_OK:
	call NXTSCN			# SCAN UNTIL THE MATCHING "NEXT" IS FOUND
	call CHRGTR			# FETCH FIRST CHARACTER OF "NEXT"
	mov edi, DWORD PTR FORFRMB
	mov DWORD PTR [edi+FE_EB+FE_NXT], ebx		# NEXT text pointer (E+0)
	mov ax, WORD PTR NXTLIN
	mov WORD PTR CURLIN, ax		# MAKE IT THE CURRENT LINE
	mov BYTE PTR [edi+1], _FOR	# FINISH UP "FOR" (token; the pad byte is at B+0)
	# The flags from CHRGTR must reach NEXTS: only mov/lahf/xchg/push between here and the jump.
	lahf
	xchg al, ah
	push eax			# stand-in 1 (4 bytes)
	xchg al, ah
	lahf
	xchg al, ah
	push eax			# stand-in 2 (4 bytes); NEXTS pops this one
	xchg al, ah
	jmp NEXTS			# GO EXECUTE "NEXT" WITH NXTFLG ZERO

# NXTCON: put the FOR token word on the stack (the caller has pushed the rest of the entry)
# and continue with the next statement. Falls into NEWSTT in the original.
NXTCON:
	mov ch, _FOR			# PUT A 'FOR' TOKEN ONTO THE STACK
	push cx
	jmp NEWSTT

