# biptrg.S: variable and array lookup (PTRGET, DIM, ISARY, NOTFDD, GETDEF, NPUTSB, IADAHL).
# Translated from BIPTRG.ASM (GW-BASIC 1983). Pointers are 32-bit; index arithmetic stays 16-bit
# as in the original (UMULT works on 16-bit values). Stack widths: pointer 4, value/flags 2.
	.intel_syntax noprefix
	.include "gwconst.inc"			# shared constants (tokens, error numbers)
	.include "oem.inc"

	.text
	.globl NOTFDD, NOTFNS, PTRGET, BSERR, PTRGT2, DIM, NOARYS, PTRGTN, PTRGTR, ERSFIN, ARYEXT, IADAHL

# DIMCON: after one dimensioned variable, COMMA ends it (more to come) or a terminator returns.
DIMCON:	dec ebx				# SEE IF COMMA ENDED THIS VARIABLE
	call CHRGTR
	jnz DIMC1			# (original: JNZ SHORT $+3)
	ret				# IF TERMINATOR, GOOD BYE
DIMC1:	call SYNCHR
	.byte 44			# MUST BE COMMA (DB OFFSET 44)
# DIM falls through from DIMCON after the comma and parses the next variable.
DIM:	mov ecx, offset DIMCON		# PLACE TO COME BACK TO (4-byte return address)
	push ecx
	mov al, 200			# MUST TURN THE MSB ON (DIMFLG)
	jmp PTRGT1

# PTRGET: read the variable name at the text pointer [EBX]; return pointer in [EDX], VALTYP set.
PTRGET:	xor al, al
PTRGT1:	mov BYTE PTR DIMFLG, al		# FLAG IT AS SUCH
	mov cl, BYTE PTR [ebx]		# GET FIRST CHARACTER IN [C]
PTRGT2:	call ISLET			# CHECK FOR LETTER (ISLET reads [EBX])
	jae Q_Q_L000
	jmp SNERR			# MUST HAVE A LETTER
Q_Q_L000:
	xor al, al
	mov ch, al			# ASSUME NO SECOND CHARACTER
	mov BYTE PTR NAMCNT, al		# ZERO NAMCNT
	inc ebx				# INCREMENT TEXT POINTER
	mov al, BYTE PTR [ebx]		# GET CHAR
	cmp al, '.'			# IS IT A DOT?
	jb NOSEC			# TOO SMALL FOR ANYTHING REASONABLE
	jz ISSEC			# "." IS VALID VAR CHAR
	cmp al, '9'+1			# TOO BIG FOR NUMERIC?
	jae PTRGT3			# YES
	cmp al, '0'			# IN RIGHT RANGE?
	jae ISSEC			# YES, WAS NUMERIC
PTRGT3:	call ISLET2			# SET CARRY IF NOT ALPHABETIC
	jb NOSEC			# ALLOW ALPHABETICS
ISSEC:	mov ch, al			# IT IS A NUMBER--SAVE IN B
	push cx				# SAVE [B,C] (value)
	mov ch, 255			# [B] COUNTS THE CHARACTERS PAST #2
	mov edx, offset NAMBUF-1	# THE PLACE TO PUT THE CHARACTERS
VMORCH:	or al, 128			# EXTRA CHARACTERS MUST HAVE THE HIGH BIT ON
	inc ch				# INCREASE THE CHARACTER COUNT
	mov edi, edx
	stosb				# AND STORE INTO THE BUFFER
	inc edx				# AND UPDATE THE BUFFER POINTER
	inc ebx				# INCREMENT TEXT POINTER
	mov al, BYTE PTR [ebx]		# GET CHAR
	cmp al, '9'+1			# TOO BIG?
	jae VMORC1			# YES
	cmp al, '0'			# IN RANGE FOR DIGIT
	jae VMORCH			# YES, VALID CHAR
VMORC1:	call ISLET2			# AS ARE ALPHABETICS
	jae VMORCH
	cmp al, '.'			# DOTS ALSO OK
	jz VMORCH			# SO EAT IT
	mov al, ch			# CHECK FOR MAXIMUM COUNT
	cmp al, NAMLEN-1		# LIMITED TO SIZE OF NAMBUF ONLY
	jb Q_Q_L001
	jmp SNERR			# MUST BE BAD SYNTAX
Q_Q_L001:
	pop cx				# GET BACK THE STORED [B,C]
	mov BYTE PTR NAMCNT, al		# ALWAYS SET UP COUNT OF EXTRAS
	mov al, BYTE PTR [ebx]		# RESTORE TERMINATING CHAR
NOSEC:	cmp al, '%'+1			# NOT A TYPE INDICATOR
	jae TABTYP			# THEN DONT CHECK THEM
	mov edx, offset HAVTYP		# SAVE JUMPS BY USING RETURN ADDRESS
	push edx
	mov dh, 2			# CHECK FOR INTEGER
	cmp al, '%'
	jnz NS1
	ret
NS1:	inc dh				# CHECK FOR STRING
	cmp al, '$'
	jnz NS2
	ret
NS2:	inc dh				# CHECK FOR SINGLE PRECISION
	cmp al, '!'
	jnz NS3
	ret
NS3:	mov dh, 8			# ASSUME ITS DOUBLE PRECISION
	cmp al, '#'			# CHECK THE CHARACTER
	jnz NS4
	ret				# WHEN WE MATCH, SETUP VALTYP
NS4:	pop eax				# POP OFF NON-USED HAVTYP ADDRESS
TABTYP:	mov al, cl			# GET THE STARTING CHARACTER
	and al, 127			# GET RID OF THE USER-DEFINED FUNCTION BIT
	movzx edx, al			# BUILD A TWO BYTE OFFSET (DL=char, DH=0)
	push ebx			# SAVE THE TEXT POINTER
	mov ebx, offset DEFTBL-65	# SEE WHAT THE DEFAULT IS ("A"=65)
	add ebx, edx
	mov dh, BYTE PTR [ebx]		# GET THE TYPE OUT OF THE TABLE
	pop ebx				# GET BACK THE TEXT POINTER
	dec ebx				# NO MARKING CHARACTER
HAVTYP:	mov al, dh			# SETUP VALTYP
	mov BYTE PTR VALTYP, al
	call CHRGTR			# READ PAST TYPE MARKER
	mov al, BYTE PTR SUBFLG		# GET FLAG WHETHER TO ALLOW ARRAYS
	dec al				# IF SUBFLG=1, "ERASE" HAS CALLED
	jnz Q_Q_L002
	jmp ERSFIN			# PTRGET, AND SPECIAL HANDLING MUST BE DONE
Q_Q_L002:
	js Q_Q_L003
	jmp NOARYS			# NO ARRAYS ALLOWED
Q_Q_L003:
	mov al, BYTE PTR [ebx]		# GET CHAR BACK
	sub al, '('			# ARRAY PERHAPS (IF SUBFLG SET NEVER WILL MATCH)
	jz ISARY			# IT IS!
	sub al, '['-')'+1		# SEE IF LEFT BRACKET
	jz ISARY			# IF SO, OK SUBSCRIPT
NOARYS:	xor al, al			# ALLOW PARENS AGAIN
	mov BYTE PTR SUBFLG, al		# SAVE IN FLAG LOCATION
	push ebx			# SAVE THE TEXT POINTER
	mov al, BYTE PTR NOFUNS		# ARE FUNCTIONS ACTIVE?
	or al, al
	mov BYTE PTR PRMFLG, al		# INDICATE IF PARM1 NEEDS SEARCHING
	jz SNFUNS			# NO FUNCTIONS SO NO SPECIAL SEARCH
	movzx ebx, WORD PTR PRMLEN	# GET THE SIZE TO SEARCH
	mov edx, offset PARM1		# GET THE BASE OF THE SEARCH
	add ebx, edx			# [EBX]= PLACE TO STOP SEARCHING
	mov DWORD PTR ARYTA2, ebx	# SET UP STOPPING POINT
	xchg ebx, edx			# [EBX]=START [EDX]=END
	jmp LOPFND

NOTFNS:	mov al, BYTE PTR PRMFLG		# HAS PARM1 BEEN SEARCHED
	or al, al
	jz SMKVAR			# IF SO, CREATE VARIABLE
	xor al, al			# FLAG PARM1 AS SEARCHED
	mov BYTE PTR PRMFLG, al
SNFUNS:	mov ebx, DWORD PTR ARYTAB	# STOPPING POINT IS [ARYTA2]
	mov DWORD PTR ARYTA2, ebx
	mov ebx, DWORD PTR VARTAB	# SET UP STARTING POINT
	jmp LOPFND

# PTRGTN: PTRGET without creating a variable (returns [EDX]=0, [AL]=0 if not found).
PTRGTN:	call PTRGET
PTRGTR:	ret				# DONT CHANGE THIS SEQUENCE AS RETURN ADDRESS IS CHECKED FOR

# VARNOT: exit for VARPTR and others (not found).
VARNOT:	xor al, al			# MAKE SURE [AL]=0
	xor edx, edx			# ZERO [D,E]
	pop ecx				# GET RID OF PUSHED [D,E]
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# PUT RETURN ADDRESS BACK ON STACK
	ret				# RETURN FROM PTRGET

SMKVAR:	pop ebx				# [EBX]= TEXT POINTER
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# [EBX]= RETURN ADDRESS
	push edx			# SAVE CURRENT VARIABLE TABLE POSITION
	mov edx, offset PTRGTR		# ARE WE RETURNING TO PTRGTN?
	cmp ebx, edx
	jz VARNOT			# YES.
	mov edx, offset RETVAR		# DID EVAL CALL US?
	cmp ebx, edx			# IF SO, DON'T MAKE A NEW VARIABLE
	pop edx				# RESTORE THE POSITION
	jz FINZER			# MAKE FAC ZERO (ALL TYPES) AND SKIP RETURN
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# PUT RETURN ADDRESS BACK
	push ebx			# PUT THE TEXT POINTER BACK
	push cx				# SAVE THE LOOKS (value)
	mov al, BYTE PTR VALTYP		# GET LENGTH OF SYMBOL TABLE ENTRY
	cmp al, 3			# string value is a 5-byte descriptor (STRSIZ 5), not 3
	jnz Q_Q_L020
	mov al, 5
Q_Q_L020:
	mov ch, al			# [B]=value size
	mov al, BYTE PTR NAMCNT		# INCLUDE EXTRA CHARACTERS IN SIZE
	add al, ch
	inc al				# AS WELL AS THE EXTRA CHARACTER COUNT
	mov cl, al			# [CL]=LENGTH OF THIS VARIABLE
	mov ch, BYTE PTR VALTYP		# [CH]=VALTYP: the record's first byte (the pop below writes DH)
	push cx				# SAVE VALTYP:LENGTH on the stack (value); value size is not the type
					# (a string is type 3 but a 5-byte descriptor, so size must not be stored)
	mov ch, 0			# [B]=0
	inc cx				# MAKE THE LENGTH INCLUDE THE LOOKS TOO
	inc cx
	inc cx
	mov ebx, DWORD PTR STREND	# THE CURRENT END OF STORAGE
	push ebx			# SAVE THIS #
	movzx ecx, cx
	add ebx, ecx			# ADD ON THE AMOUNT OF SPACE EXTRA NOW BEING USED
	pop ecx				# POP OFF HIGH ADDRESS TO MOVE
	push ebx			# SAVE NEW CANDIDATE FOR STREND
	call BLTU			# BLOCK TRANSFER (BX=dest high, CX=high, DX=low)
	pop ebx				# [EBX]=NEW STREND
	mov DWORD PTR STREND, ebx	# STORE SINCE WAS OK
	mov ebx, ecx			# GET BACK [EBX] POINTING AT THE END OF THE NEW VARIABLE
	mov DWORD PTR ARYTAB, ebx	# UPDATE THE ARRAY TABLE POINTER
ZEROER:	dec ebx				# ZERO BACKWARDS TO [EDX] (START OF VARIABLE)
	mov BYTE PTR [ebx], 0
	cmp ebx, edx
	jnz ZEROER
	pop dx				# VALTYP:size word (value)
	mov BYTE PTR [ebx], dh		# VALTYP IS IN HIGH ORDER
	inc ebx
	pop dx				# looks (value)
	mov WORD PTR [ebx], dx
	inc ebx				# PUT DESCRIPTION OF THIS VARIABLE INTO MEMORY
	call NPUTSB			# SAVE THE EXTRA CHARACTERS IN THE NAME
	xchg ebx, edx			# POINTER AT VARIABLE INTO [EDX]
	inc edx				# POINT AT THE VALUE
	pop ebx				# RESTORE THE TEXT POINTER
	ret

# MAKE ALL TYPES ZERO AND SKIP RETURN
FINZER:	call _DZERO			# clear the entire FAC (math package)
	call GETYPR			# SEE IF ITS A STRING
	jnz POPHR2			# IF NOT, DONE
	mov ebx, offset DSEGZ		# ZERO IN THE DATA SEGMENT
	mov DWORD PTR FACLO, ebx	# POINTING AT A ZERO (4-byte pointer)
POPHR2:	pop ebx				# GET THE TEXT POINTER
	ret				# RETURN FROM EVAL

# ISARY: array reference. Indices are pushed (2 bytes each); DIMFLG/VALTYP word is pushed as a value.
ISARY:	push ebx			# SAVE DIMFLG AND VALTYP FOR RECURSION
	movzx ebx, WORD PTR DIMFLG
	pop esi				# XTHL
	xchg esi, ebx
	push esi				# TEXT POINTER BACK INTO [EBX]; DIMFLG word on the stack
	mov dh, al			# SET # DIMENSIONS =0
	dec al				# SARYFL=^O377 - array element has been scanned
	mov BYTE PTR SARYFL, al
INDLOP:	push dx				# SAVE NUMBER OF DIMENSIONS
	push cx				# SAVE LOOKS
	mov edx, offset NAMCNT		# POINT AT THE AREA TO SAVE
	mov esi, edx
	mov al, BYTE PTR [esi]		# GET LENGTH
	or al, al			# IS IT ZERO?
	jz SHTNAM			# YES, SHORT NAME
	xchg ebx, edx			# SAVE THE TEXT POINTER IN [EDX]
	add al, 2			# WE WANT SMALLEST INT .GE.(NAMCNT+1)/2
	rcr al, 1
	mov cl, al			# SEE IF THERE IS ROOM TO SAVE THIS STUFF
	call GETSTK
	mov al, cl			# RESTORE COUNT OF PUSHES
LPPSNM:	mov cl, BYTE PTR [ebx]		# GET VALUES TO PUSH
	inc ebx
	mov ch, BYTE PTR [ebx]
	inc ebx
	push cx				# AND DO THE SAVE (value)
	dec al				# [A] TIMES
	jnz LPPSNM
	push ebx			# SAVE THE ADDRESS TO STORE TO (pointer)
	mov al, BYTE PTR NAMCNT		# SAVE THE NUMBER OF BYTES FOR A COUNT
	push ax
	xchg ebx, edx			# RESTORE THE TEXT POINTER
	call INTIDX			# EVALUATE INDICE: [EDX]=index (16-bit)
	pop ax				# COUNT TELLING HOW MUCH TO RESTORE
	mov DWORD PTR NAMTMP, ebx	# SAVE THE TEXT POINTER (4-byte; see NOTES: NAMTMP widened)
	pop ebx				# THE PLACE TO RESTORE TO
	add al, 2			# CALCULATE BYTE POPS AGAIN
	rcr al, 1
LPLNAM:	pop cx
	dec ebx
	mov BYTE PTR [ebx], ch
	dec ebx
	mov BYTE PTR [ebx], cl
	dec al				# LOOP [A] TIMES POPING NAME BACK INTO NAMBUF
	jnz LPLNAM
	mov ebx, DWORD PTR NAMTMP
	jmp LNGNAM			# WAS LONG ONE
SHTNAM:	call INTIDX			# EVALUATE IT
	xor al, al			# MAKE SURE NAMCNT=0
	mov BYTE PTR NAMCNT, al
LNGNAM:	mov al, BYTE PTR OPTVAL		# SEE WHAT THE OPTION BASE IS
	or al, al
	jz OPTB0			# IF BASE 0 DO NOTHING
	or dx, dx			# CHECK FOR 0 SUBSCRIPT (illegal in BASE 1)
	jnz Q_Q_L006
	jmp BSERR
Q_Q_L006:
	dec dx				# ADJUST SUBSCRIPT
OPTB0:	pop cx				# POP OFF THE LOOKS
	pop ax				# POP PSW (dims word)
	xchg al, ah
	sahf				# [A] = NUMBER OF DIMENSIONS SO FAR
	xchg ebx, edx			# [EDX]=TEXT POINTER, [BX]=INDICE (16-bit)
	pop esi				# XTHL (DIMFLG word)
	xchg si, bx
	push si				# PUT THE INDICE ON THE STACK (2 bytes: POPDIM pops a 2-byte index)
	push ebx				# RESAVE VALTYP AND DIMFLG
	xchg ebx, edx			# [EBX]=TEXT POINTER
	inc al				# INCREMENT # OF DIMENSIONS
	mov dh, al			# [DH]=NUMBER OF DIMENSIONS
	mov al, BYTE PTR [ebx]		# GET TERMINATING CHARACTER
	cmp al, 44			# A COMMA SO MORE INDICES FOLLOW?
	jnz Q_Q_L007
	jmp INDLOP			# IF SO, READ MORE
Q_Q_L007:
	cmp al, ')'			# EXPECTED TERMINATOR?
	jz DOCHRT			# DO CHRGET FOR NEXT ONE
	cmp al, ']'			# BRACKET?
	jz Q_Q_L008
	jmp SNERR			# NO, GIVE ERROR
Q_Q_L008:
DOCHRT:	call CHRGTR
SUBSOK:	mov DWORD PTR TEMP2, ebx	# SAVE THE TEXT POINTER
	pop ebx				# [BX]= VALTYP & DIMFLG (value word)
	mov WORD PTR DIMFLG, bx		# SAVE VALTYP AND DIMFLG
	mov dl, 0			# WHEN [DX] IS POPPED INTO PSW, ZERO FLAG NOT SET
	push dx				# SAVE NUMBER OF DIMENSIONS
	jmp LOPFD0
ERSFIN:	push ebx			# SAVE THE TEXT POINTER
	lahf				# PUSH PSW
	xchg al, ah
	push ax
	xchg al, ah			# SAVE A DUMMY NUMBER OF DIMENSIONS WITH ZF SET
LOPFD0:	mov ebx, DWORD PTR ARYTAB	# [EBX]=PLACE TO START THE SEARCH
	jmp LOPFD1

# ARYEXT: variable found in the table (entry at [EBX]).
ARYEXT:	mov al, BYTE PTR DIMFLG		# SEE IF CALLED BY "DIM"
	or al, al			# ZERO MEANS NO
	jz Q_Q_L009
	jmp DDERR			# REDIMENSIONED VARIABLE
Q_Q_L009:
	pop ax				# POP PSW
	xchg al, ah
	sahf				# [A]=NUMBER OF DIMENSIONS
	mov ecx, ebx			# SET [CX] TO POINT AT NUMBER OF DIMENSIONS
	jnz Q_Q_L010
	jmp POPHRT			# "ERASE" IS DONE AT THIS POINT
Q_Q_L010:
	sub al, BYTE PTR [ebx]		# MAKE SURE THE NUMBER GIVEN NOW AND WHEN THE ARRAY WAS SET UP ARE THE SAME
	jnz Q_Q_L011
	jmp GETDEF			# JUMP OFF AND READ THE INDICES
Q_Q_L011:
BSERR:	mov dx, ERRBS			# "SUBSCRIPT OUT OF RANGE"
	jmp ERROR

# NOTFDD: variable not found; build an entry at [EBX] (end of table). Falls into GETDEF.
NOTFDD:	mov al, BYTE PTR VALTYP		# GET VALTYP OF NEW VAR
	mov BYTE PTR [ebx], al		# PUT DOWN THE VARIABLE TYPE
	inc ebx
	movzx edx, al			# [DX]=SIZE OF ONE VALUE (VALTYP)
	cmp al, 3			# string elements are 5 bytes (STRSIZ 3->5)
	jnz NFD1
	mov dl, 5
NFD1:	pop ax				# POP PSW
	xchg al, ah
	sahf				# [A]=NUMBER OF DIMENSIONS
	jnz Q_Q_L012
	jmp PTRRNZ			# CALLED BY CHAIN, JUST RETURN NON-ZERO
Q_Q_L012:
	mov BYTE PTR [ebx], cl		# PUT DOWN THE DESCRIPTOR
	inc ebx
	mov BYTE PTR [ebx], ch
	call NPUTSB			# STORE THE EXTRA CHARACTERS IN THE TABLE
	inc ebx
	mov cl, al			# [C]=NUMBER OF TWO BYTE ENTRIES
	call GETSTK			# GET SPACE FOR DIMENSION ENTRIES
	inc ebx				# SKIP OVER THE SIZE LOCATIONS
	inc ebx
	mov DWORD PTR TEMP3, ebx	# POINTS AT THE NUMBER OF DIMENSIONS
	mov BYTE PTR [ebx], cl		# STORE THE NUMBER OF DIMENSIONS
	inc ebx
	mov al, BYTE PTR DIMFLG		# CALLED BY DIMENSION?
	rcl al, 1			# SET CARRY IF SO
	mov al, cl			# [A]=NUMBER OF DIMENSIONS
LOPPTA:	jb POPDIM
	lahf
	push ax
	mov al, BYTE PTR OPTVAL		# GET THE OPTION BASE
	xor al, 11			# MAP 0 TO 11 AND 1 TO 10
	mov cl, al			# [CX]=DEFAULT DIMENSION
	mov ch, 0
	pop ax
	sahf
	jae NOTDIM			# DEFAULT DIMENSIONS TO TEN
POPDIM:	pop cx				# POP OFF AN INDICE INTO [CX]
	lahf
	inc cx				# ADD ONE TO IT FOR THE ZERO ENTRY
	sahf
NOTDIM:	mov BYTE PTR [ebx], cl		# PUT THE MAXIMUM DOWN
	lahf
	push ax				# SAVE THE NUMBER OF DIMENSIONS AND DIMFLG (CARRY)
	inc ebx
	mov BYTE PTR [ebx], ch
	inc ebx
	call UMULT			# [DX]=[CX]*[DX]
	pop ax
	sahf				# GET THE NUMBER OF DIMENSIONS AND DIMFLG (CARRY) BACK
	dec al				# DECREMENT THE NUMBER OF DIMENSIONS LEFT
	jnz LOPPTA
	lahf
	push ax				# SAVE DIMFLG (CARRY)
	mov ch, dh			# [CX]=SIZE
	mov cl, dl
	xchg ebx, edx			# [EDX]=START OF VALUES, [BX]=SIZE
	movzx ebx, bx
	add ebx, edx			# [EBX]=END OF VALUES (no 16-bit carry check in flat memory; REASON checks FRETOP)
	call REASON			# SEE IF THERE IS ROOM FOR THE VALUES
	mov DWORD PTR STREND, ebx	# UPDATE THE END OF STORAGE
ZERITA:	dec ebx				# ZERO THE NEW ARRAY
	mov BYTE PTR [ebx], 0
	cmp ebx, edx
	jnz ZERITA
	xor al, al			# MAKE SURE [AL]=0
	inc cx				# ADD ONE TO THE SIZE TO INCLUDE THE NUMBER OF DIMENSIONS BYTE
	mov dh, al			# [DH]=ZERO
	mov ebx, DWORD PTR TEMP3	# POINTER AT THE NUMBER OF DIMENSIONS
	mov dl, BYTE PTR [ebx]		# [DL]=NUMBER OF DIMENSIONS
	add dx, dx			# TWO TIMES THE NUMBER OF DIMENSIONS
	add dx, cx			# ADD ON THE SIZE: TOTAL NUMBER OF BYTES USED
	sub ebx, 2			# BACK UP TO THE LOCATION FOR THE SIZE
	mov WORD PTR [ebx], dx		# PUT DOWN THE SIZE
	add ebx, 2
	pop ax
	sahf				# GET BACK DIMFLG (CARRY)
	jb FINNOW

# GETDEF: [EBX] points at the number of dimensions (the table is: ndims byte, then one 2-byte
# maximum per dimension, then the values). The subscripts are 2-byte items on the stack, top first
# (the same order DIM pops them in), so each is popped with a 2-byte POP. The element offset is
#   CURTOL = CURTOL * MAX(dimension) + SUBSCRIPT   (over the dimensions, in table order)
# and the value is at (values base) + CURTOL * (size of one element).
# Values base = address of the ndims byte + 1 + 2 * ndims (NOTFDD lays the values out there).
GETDEF:	movzx ecx, BYTE PTR [ebx]	# ECX = number of dimensions
	mov esi, ebx			# ESI -> the number-of-dimensions byte (values base is computed from it)
	inc ebx				# EBX -> the first dimension maximum (2 bytes)
	xor edi, edi			# EDI = CURTOL (running element index)
GDLOOP:	movzx edx, WORD PTR [ebx]	# EDX = maximum for this dimension
	add ebx, 2			# next dimension maximum
	pop ax				# the subscript for this dimension (2 bytes)
	movzx eax, ax
	cmp eax, edx			# subscript must be below the maximum
	jae BSERR
	imul edi, edx			# CURTOL = CURTOL * MAXIMUM
	add edi, eax			# CURTOL = CURTOL + SUBSCRIPT
	dec ecx
	jnz GDLOOP
	mov al, BYTE PTR VALTYP		# size of one element
	movzx ecx, al
	cmp al, 3			# string element: 5-byte descriptor
	jnz GDESZ
	mov ecx, 5
GDESZ:	imul edi, ecx			# byte offset of the element
	movzx ecx, BYTE PTR [esi]	# number of dimensions again
	lea edx, [esi+ecx*2+1]		# values base = ndims byte + 1 + 2 * ndims
	add edx, edi			# [EDX] = pointer to the element (returned as in the original)
FINNOW:	mov ebx, DWORD PTR TEMP2	# REGET THE TEXT POINTER
	ret
PTRRNZ:	stc				# RETURN WITH NON-ZERO IN [A]
	sbb al, al			# AND CONDITION CODES SET
	pop ebx				# RESTORE TEXT POINTER
	ret

# LONG VARIABLE NAME SUBROUTINES. After the 2-char name the count of extra chars is stored,
# then the characters with the high bit set.
IADAHL:	mov al, BYTE PTR [ebx]		# GET THE CHARACTER COUNT
	inc ebx
ADDAHL:	push cx				# ADD [A] TO [EBX]
	movzx ecx, al
	add ebx, ecx
	pop cx
	ret

# NPUTSB: store the "long" name at [EBX]. Preserves CX, EDX and flags.
NPUTSB:	push cx
	push edx
	lahf
	push ax
	mov edx, offset NAMCNT		# POINT AT DATA TO SAVE
	mov esi, edx
	mov al, BYTE PTR [esi]		# GET THE COUNT
	mov ch, al
	inc ch				# [B]= NUMBER OF BYTES TO SAVE
SLPLNG:	mov esi, edx
	mov al, BYTE PTR [esi]		# FETCH STORE VALUE
	inc edx
	inc ebx				# MOVE UP TO STORE NAME INTO TABLE
	mov BYTE PTR [ebx], al		# DO THE STORE
	dec ch				# AND REPEAT [B] TIMES
	jnz SLPLNG
	pop ax
	sahf
	pop edx
	pop cx
	ret
