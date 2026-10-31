# BIPRTU.ASM -> biprtu.S
# PRINT USING driver. Scans the USING string, prints fields from the value list.
#
# Stack widths: text and descriptor pointers are 4 bytes; flag and count words are 2 bytes;
# PUSHF/POPF are pushfd/popfd (4 bytes); return addresses are 4 bytes.
#
# Translation notes:
#  - Original "DB 276O" (an 8086 MOV SI,imm16 that skipped XOR AL,AL) is written as an explicit
#    fall-through: the fall-through path executes ADD AL,16 / INC BX without the XOR.
#  - Original "JZ SHORT $+3 / RET" pairs are written as conditional jumps around the RET.
#  - "??Lnnn" labels are written as Q_Q_Lnnn.
#  - FINNUM saves the 16-bit flags/digit word on the stack, pushes the 4-byte NOTSCI return
#    address, and keeps the full text pointer in EDX (originally DH:DL).

	.intel_syntax noprefix
	.include "oem.inc"
	.include "gwtok.inc"
	.include "tokens.inc"

	.set CSTRNG, 0134		# '\' starts a big string field (134 octal)
	.set CURNCY, 044		# '$' default currency (44 octal)

	.section .text
	.globl PRINUS

# COME HERE AFTER THE "USING" CLAUSE IN A PRINT STATEMENT IS RECOGNIZED.
PRINUS:
	call FRMCHK			# EVALUATE THE "USING" STRING
	call CHKSTR			# MAKE SURE IT IS A STRING
	call SYNCHR
	.byte 073			# MUST BE DELIMITED BY A SEMI-COLON
	xchg ebx, edx			# [D,E]=TEXT POINTER
	mov ebx, DWORD PTR FACLO	# GET POINTER TO "USING" STRING DESCRIPTOR
	jmp INIUS			# DONT POP OFF OR LOOK AT USFLG

REUSST:
	mov al, BYTE PTR USFLG		# DID WE PRINT OUT A VALUE LAST SCAN?
	or al, al
	jz FCERR3			# NO, GIVE ERROR
	pop edx				# [D,E]=POINTER TO "USING" STRING DESCRIPTOR
	xchg ebx, edx			# [D,E]=TEXT POINTER
INIUS:
	push ebx			# SAVE THE POINTER TO "USING" STRING DESCRIPTOR
	xor al, al			# more values in the value list
	mov BYTE PTR USFLG, al		# reset the flag that says values printed
	inc al				# turn the zero flag off: value list not ended
	pushfd				# save flag: whether the value list has ended
	push edx			# save the text pointer into the value list
	mov ch, BYTE PTR [ebx]		# [B]=LENGTH OF THE "USING" STRING
	inc ch				# Is the using string null?
	dec ch
FCERR3:
	jnz Q_Q_L000
	jmp FCERR			# IF SO, "ILLEGAL FUNCTION CALL"
Q_Q_L000:
	inc ebx				# POINTER AT THE "USING" STRING'S DATA
	mov ebx, DWORD PTR [ebx]
	jmp PRCCHR			# GO INTO THE LOOP TO SCAN THE "USING" STRING

# Big string field: \ ... \ with spaces between (the \\ field has 2 plus enclosed spaces)
BGSTRF:
	mov dl, ch			# SAVE THE "USING" STRING CHARACTER COUNT
	push ebx			# SAVE THE POINTER INTO THE "USING" STRING
	mov cl, 2			# THE \\ STRING FIELD HAS 2 PLUS NUMBER OF ENCLOSED SPACES WIDTH
LPSTRF:
	mov al, BYTE PTR [ebx]		# GET THE NEXT CHARACTER
	inc ebx				# ADVANCE THE POINTER AT THE "USING" STRING DATA
	cmp al, CSTRNG			# THE FIELD TERMINATOR?
	jnz Q_Q_L001
	jmp ISSTRF			# GO EVALUATE A STRING AND PRINT
Q_Q_L001:
	cmp al, ' '			# A FIELD EXTENDER?
	jnz NOSTRF			# IF NOT, ITS NOT A STRING FIELD
	inc cl				# INCREMENT THE FIELD WIDTH
	dec ch				# SEE IF THERE ARE MORE CHARACTERS
	jnz LPSTRF			# KEEP SCANNING FOR THE FIELD TERMINATOR

# Not a string field: restore the "USING" pointer and count and print the "\".
NOSTRF:
	pop ebx				# RESTORE THE POINTER INTO "USING" STRING'S DATA
	mov ch, dl			# RESTORE THE "USING" STRING CHARACTER COUNT
	mov al, CSTRNG			# RESTORE THE CHARACTER
# HERE TO PRINT THE CHARACTER IN [A] SINCE IT WASN'T PART OF ANY FIELD
NEWUCH:
	call PLSPRT			# IF A "+" CAME BEFORE, PRINT IT
	call OUTDO			# PRINT THE CHARACTER THAT WASN'T PART OF A FIELD

PRCCHR:
	xor al, al			# SET [D,E]=0 SO IF WE DISPATCH SOME FLAGS ARE ALREADY ZEROED
	mov dl, al
	mov dh, al			# DON'T PRINT "+" TWICE
PLSFIN:
	call PLSPRT			# ALLOW FOR MULTIPLE PLUSES IN A ROW
	mov dh, al			# SET "+" FLAG
	mov al, BYTE PTR [ebx]		# GET A NEW CHARACTER
	inc ebx
	cmp al, '!'			# CHECK FOR A SINGLE CHARACTER
	jnz Q_Q_L002
	jmp SMSTRF			# STRING FIELD
Q_Q_L002:
	cmp al, '#'			# CHECK FOR THE START OF A NUMERIC FIELD
	jz NUMNUM			# GO SCAN IT
	cmp al, '&'			# SEE IF ITS A VARIABLE LENGTH STRING FIELD
	jnz Q_Q_L003
	jmp VARSTR			# GO PRINT ENTIRE STRING
Q_Q_L003:
	dec ch				# ALL THE OTHER POSSIBILITIES REQUIRE AT LEAST 2 CHARACTERS
	jnz Q_Q_L004
	jmp REUSIN			# IF THE VALUE LIST IS NOT EXHAUSTED, GO REUSE "USING" STRING
Q_Q_L004:
	cmp al, '+'			# A LEADING "+" ?
	mov al, 8			# SETUP [D] WITH THE PLUS-FLAG ON (MOV does not change flags)
	jz PLSFIN			# CASE A NUMERIC FIELD STARTS
	dec ebx				# POINTER HAS ALREADY BEEN INCREMENTED
	mov al, BYTE PTR [ebx]		# GET BACK THE CURRENT CHARACTER
	inc ebx				# REINCREMENT THE POINTER
	cmp al, '.'			# NUMERIC FIELD WITH TRAILING DIGITS
	jz DOTNUM
	cmp al, '_'			# CHECK FOR LITERAL CHARACTER DECLARATION
	jnz Q_Q_L005
	jmp LITCHR
Q_Q_L005:
	cmp al, CSTRNG			# CHECK FOR A BIG STRING FIELD STARTER
	jz BGSTRF
	cmp al, BYTE PTR [ebx]		# SEE IF THE NEXT CHARACTER MATCHES THE CURRENT ONE
	jnz NEWUCH
	cmp al, CURNCY			# IS IT $$ ?
	jz DOLRNM
	cmp al, '*'			# IS IT ** ?
	jnz NEWUCH
	inc ebx				# CHECK FOR $
	mov al, ch			# SEE IF THE "USING" STRING IS LONG ENOUGH FOR **$
	cmp al, 2
	jb NOTSPC
	mov al, BYTE PTR [ebx]
	cmp al, CURNCY			# IS THE NEXT CHARACTER $ ?
NOTSPC:
	mov al, 32			# SET THE ASTERISK BIT (MOV does not change flags)
	jnz SPCNUM			# IF NOT THE SPECIAL CASE, DON'T SET THE DOLLAR SIGN FLAG
	dec ch				# COUNT THE $ AS PART OF THE FIELD
	inc dl				# INCREMENT THE FIELD WIDTH FOR THE FLOATING DOLLAR SIGN
	# Original "DB 276O" skipped XOR AL,AL here. Fall-through: AL=32+16, INC BX.
	add al, 16
	inc ebx
	jmp SPCNUM
DOLRNM:
	xor al, al			# CLEAR [A]
	add al, 16			# SET BIT FOR FLOATING DOLLAR SIGN FLAG
	inc ebx				# POINT BEYOND THE SPECIAL CHARACTERS
SPCNUM:
	inc dl				# SINCE TWO CHARACTERS SPECIFY THE FIELD SIZE
	add al, dh			# PUT NEW FLAG BITS IN [A]
	mov dh, al			# INTO [D]. THE PLUS FLAG MAY HAVE ALREADY BEEN SET
NUMNUM:
	inc dl				# INCREMENT THE NUMBER OF DIGITS BEFORE THE DECIMAL POINT
	mov cl, 0			# SET THE NUMBER OF DIGITS AFTER THE DECIMAL POINT = 0
	dec ch				# SEE IF THERE ARE MORE CHARACTERS
	jz ENDNUS			# IF NOT, WE ARE DONE SCANNING THIS NUMERIC FIELD
	mov al, BYTE PTR [ebx]		# GET THE NEW CHARACTER
	inc ebx
	cmp al, '.'			# DO WE HAVE TRAILING DIGITS?
	jz AFTDOT
	cmp al, '#'			# MORE LEADING DIGITS ?
	jz NUMNUM
	cmp al, 054			# DOES HE WANT A COMMA EVERY THREE DIGITS?
	jnz FINNUM			# NO MORE LEADING DIGITS, CHECK FOR ^^^
	mov al, dh			# TURN ON THE COMMA BIT
	or al, 64
	mov dh, al
	jmp NUMNUM

# A "." starts a numeric field only if followed by "#"
DOTNUM:
	mov al, BYTE PTR [ebx]		# GET THE CHARACTER THAT FOLLOWS
	cmp al, '#'			# IS THIS A NUMERIC FIELD?
	mov al, '.'			# IF NOT, GO BACK AND PRINT "."
	jz Q_Q_L006
	jmp NEWUCH
Q_Q_L006:
	mov cl, 1			# INITIALIZE THE NUMBER OF DIGITS AFTER THE DECIMAL POINT
	inc ebx
AFTDOT:
	inc cl				# INCREMENT THE NUMBER OF DIGITS AFTER THE DECIMAL POINT
	dec ch				# SEE IF THE "USING" STRING HAS MORE CHARACTERS
	jz ENDNUS
	mov al, BYTE PTR [ebx]		# GET THE NEXT CHARACTER
	inc ebx
	cmp al, '#'			# MORE DIGITS AFTER THE DECIMAL POINT?
	jz AFTDOT

# CHECK FOR THE "^^^^" THAT INDICATES SCIENTIFIC NOTATION
FINNUM:
	push dx				# SAVE [D]=FLAGS AND [E]=LEADING DIGITS (2 bytes)
	mov edx, offset NOTSCI		# PLACE TO GO IF ITS NOT SCIENTIFIC NOTATION
	push edx			# this becomes the return address (4 bytes)
	mov edx, ebx			# REMEMBER THE TEXT POINTER IN CASE ITS NOT SCIENTIFIC
	cmp al, '^'			# IS THE FIRST CHARACTER "^" ?
	jz Q_Q_L007
	ret
Q_Q_L007:
	cmp al, BYTE PTR [ebx]		# IS THE SECOND CHARACTER "^" ?
	jz Q_Q_L008
	ret
Q_Q_L008:
	inc ebx
	cmp al, BYTE PTR [ebx]		# IS THE THIRD CHARACTER "^" ?
	jz Q_Q_L009
	ret
Q_Q_L009:
	inc ebx
	cmp al, BYTE PTR [ebx]		# IS THE FOURTH CHARACTER "^" ?
	jz Q_Q_L010
	ret
Q_Q_L010:
	inc ebx
	mov al, ch			# WERE THERE ENOUGH CHARACTERS FOR "^^^^" (IT TAKES FOUR)
	sub al, 4
	jnb Q_Q_L011
	ret
Q_Q_L011:
	pop edx				# POP OFF THE NOTSCI RETURN ADDRESS (4 bytes)
	pop dx				# GET BACK [D]=FLAGS [E]=LEADING DIGITS
	mov ch, al			# MAKE [B]=NEW CHARACTER COUNT
	inc dh				# TURN ON THE SCIENTIFIC NOTATION FLAG
	inc ebx
	jmp ENDNUS

NOTSCI:
	xchg ebx, edx			# RESTORE THE OLD TEXT POINTER
	pop dx				# GET BACK [D]=FLAGS [E]=LEADING DIGITS

ENDNUS:
	mov al, dh			# IF THE LEADING PLUS FLAG IS ON
	dec ebx
	inc dl				# INCLUDE LEADING "+" IN NUMBER OF DIGITS
	and al, 8			# DON'T CHECK FOR A TRAILING SIGN
	jnz ENDNUM			# ALL DONE WITH THE FIELD IF THERE IS A LEADING PLUS
	dec dl				# NO LEADING PLUS, DON'T INCREMENT THE DIGITS BEFORE THE POINT
	mov al, ch
	or al, al			# SEE IF THERE ARE MORE CHARACTERS
	jz ENDNUM			# IF NOT, STOP SCANNING
	mov al, BYTE PTR [ebx]		# GET THE CURRENT CHARACTER
	sub al, '-'			# TRAIL MINUS?
	jz SGNTRL			# SET THE TRAILING SIGN FLAG
	cmp al, -2			# "+"-"-" : A TRAILING PLUS?
	jnz ENDNUM			# IF NOT, WE ARE DONE SCANNING
	mov al, 8			# TURN ON THE POSITIVE="+" FLAG
SGNTRL:
	add al, 4			# TURN ON THE TRAILING SIGN FLAG
	add al, dh			# INCLUDE WITH OLD FLAGS
	mov dh, al
	dec ch				# ACCOUNT FOR THE TRAILING SIGN
ENDNUM:
	pop ebx				# THE OLD TEXT POINTER
	popfd				# FLAG: WHETHER THERE ARE MORE VALUES IN THE VALUE LIST
	jz FLDFIN			# IF NOT, WE ARE DONE WITH THE "PRINT"
	push cx				# SAVE [B]=# OF CHARACTERS REMAINING AND [C]=TRAILING DIGITS
	push dx				# SAVE [D]=FLAGS AND [E]=LEADING DIGITS
	call FRMEVL			# READ A VALUE FROM THE VALUE LIST
	pop dx				# [D]=FLAGS & [E]=# OF LEADING DIGITS
	pop cx				# [B]=# CHARACTERS LEFT, [C]=NUMBER OF TRAILING DIGITS
	push cx				# SAVE [B] FOR ENTERING SCAN AGAIN
	push ebx			# SAVE THE TEXT POINTER
	mov ch, dl			# [B]=# OF LEADING DIGITS
	mov al, ch			# MAKE SURE THE TOTAL NUMBER OF DIGITS
	add al, cl			# DOES NOT EXCEED TWENTY-FOUR
	cmp al, 25
	jb Q_Q_L012
	jmp FCERR			# IF SO, "ILLEGAL FUNCTION CALL"
Q_Q_L012:
	mov al, dh			# [A]=FLAG BITS
	or al, 128			# TURN ON THE "USING" BIT
	call PUFOUT			# PRINT THE VALUE
	call DSOUT			# PRINT FROM THE DATA SEGMENT
FNSTRF:
	pop ebx				# GET BACK THE TEXT POINTER
	dec ebx				# SEE WHAT THE TERMINATOR WAS
	call CHRGTR
	stc				# SET FLAG THAT CRLF IS DESIRED
	jz CRDNUS			# END-OF-STATEMENT: VALUE LIST ENDED, PRINT CRLF
	mov BYTE PTR USFLG, al		# FLAG THAT VALUE HAS BEEN PRINTED
	cmp al, 073			# A SEMI-COLON?
	jz SEMUSN			# A LEGAL DELIMITER
	cmp al, 054			# A COMMA ?
	jz Q_Q_L013
	jmp SNERR			# THE DELIMETER WAS ILLEGAL
Q_Q_L013:
SEMUSN:
	call CHRGTR			# IS THERE ANOTHER VALUE?
CRDNUS:
	pop cx				# [B]=CHARACTERS REMAINING IN "USING" STRING
	xchg ebx, edx			# [D,E]=TEXT POINTER
	pop ebx				# POINT AT THE "USING" STRING DESCRIPTOR
	push ebx			# RESAVE IT
	pushfd				# SAVE THE FLAG THAT SAYS WHETHER THE VALUE LIST TERMINATED
	push edx			# SAVE THE TEXT POINTER

# SINCE FRMEVL MAY HAVE FORCED GARBAGE COLLECTION, USE THE NUMBER OF CHARACTERS ALREADY
# SCANNED AS AN OFFSET TO THE "USING" STRING'S DATA TO GET A NEW POINTER.
	mov al, BYTE PTR [ebx]		# GET THE "USING" STRING'S LENGTH
	sub al, ch			# SUBTRACT THE NUMBER OF CHARACTERS ALREADY SCANNED
	inc ebx				# POINT AT THE DATA POINTER
	movzx edx, al			# DOUBLE BYTE OFFSET (MOV DH,0 / MOV DL,AL)
	mov ebx, DWORD PTR [ebx]	# THE "USING" STRING'S DATA
	add ebx, edx			# ADD ON THE OFFSET TO GET THE NEW POINTER
CHKUSI:
	mov al, ch			# [A]=THE NUMBER OF CHARACTERS LEFT TO SCAN
	or al, al			# SEE IF THERE ARE ANY LEFT
	jz Q_Q_L014
	jmp PRCCHR			# IF SO, KEEP SCANNING
Q_Q_L014:
	jmp FINUSI			# SEE IF THERE ARE MORE VALUES

REUSIN:
	call PLSPRT			# PRINT A "+" IF NECESSARY
	call OUTDO			# PRINT THE FINAL CHARACTER
FINUSI:
	pop ebx				# POP OFF THE TEXT POINTER
	popfd				# POP OFF THE INDICATOR OF WHETHER THE VALUE LIST HAS ENDED
	jz Q_Q_L015
	jmp REUSST			# IF NOT, REUSE THE "USING" STRING
Q_Q_L015:
FLDFIN:
	jnb Q_Q_L016
	call CRDO			# IF NOT COMMA OR SEMI-COLON, ENDED THE VALUE LIST: PRINT CRLF
Q_Q_L016:
	pop esi				# XTHL: SI=DESCRIPTOR, BX=TEXT POINTER
	xchg esi, ebx
	push esi			# SAVE THE TEXT POINTER
	call FRETM2			# FINALLY FREE THE DESCRIPTOR (BX)
	pop ebx				# GET BACK THE TEXT POINTER
	jmp FINPRT			# ZERO [PTRFIL]

# A literal character in the USING string, preceded by "_"
LITCHR:
	call PLSPRT			# PRINT PREVIOUS "+" IF ANY
	dec ch				# DECREMENT COUNT FOR ACTUAL CHARACTER
	mov al, BYTE PTR [ebx]		# FETCH LITERAL CHARACTER
	inc ebx
	call OUTDO
	jmp CHKUSI			# GO SEE IF USING STRING ENDED

# Variable length string field "&"
VARSTR:
	mov cl, 0			# SET LENGTH TO 0 TO FLAG VARIABLE LENGTH
	jmp ISSTR1

# "!" single character string field
SMSTRF:
	mov cl, 1			# SET THE FIELD WIDTH TO 1
	jmp ISSTR1

ISSTRF:
	pop eax				# DISCARD THE POINTER SAVED IN BGSTRF (4 bytes)
ISSTR1:
	dec ch				# DECREMENT THE "USING" STRING CHARACTER COUNT
	call PLSPRT			# PRINT A "+" IF ONE CAME BEFORE THE FIELD
	pop ebx				# TAKE OFF THE TEXT POINTER
	popfd				# FLAG: WHETHER THERE ARE MORE VALUES IN THE VALUE LIST
	jz FLDFIN			# IF THERE ARE NO MORE VALUES THEN WE ARE DONE
	push cx				# SAVE [B]=NUMBER OF CHARACTERS YET TO BE SCANNED
	call FRMEVL			# READ A VALUE
	call CHKSTR			# MAKE SURE ITS A STRING
	pop cx				# [C]=FIELD WIDTH
	push cx				# RESAVE [B]
	push ebx			# SAVE THE TEXT POINTER
	mov ebx, DWORD PTR FACLO	# GET A POINTER TO THE DESCRIPTOR
	mov ch, cl			# [B]=FIELD WIDTH
	mov cl, 0			# SET UP FOR "LEFT$"
	mov al, ch			# GET FIELD WIDTH IN [A]
	push ax				# SAVE THE FIELD WIDTH FOR SPACE PADDING
	mov al, ch			# SEE IF VARIABLE LENGTH FIELD
	or al, al
	jz Q_Q_L017
	call LEFTUS
Q_Q_L017:
	call STRPRT			# PRINT THE STRING
	mov ebx, DWORD PTR FACLO	# SEE IF IT NEEDS TO BE PADDED
	pop ax				# [A]=FIELD WIDTH
	or al, al			# IF FIELD LENGTH IS 0 MUST BE "&": NO TRAILING SPACES
	jnz Q_Q_L018
	jmp FNSTRF
Q_Q_L018:
	sub al, BYTE PTR [ebx]		# [A]=AMOUNT OF PADDING NEEDED
	mov ch, al
	mov al, ' '			# SETUP THE PRINT CHARACTER
	inc ch				# DUMMY INCREMENT OF NUMBER OF SPACES
UPRTSP:
	dec ch				# SEE IF MORE SPACES
	jnz Q_Q_L019
	jmp FNSTRF			# NO, GO SEE IF THE VALUE LIST ENDED
Q_Q_L019:
	call OUTDO			# PRINT A SPACE
	jmp UPRTSP

# PLSPRT: print a pending "+" (DH bit). The bit is always set when a "+" was seen.
PLSPRT:
	push ax				# SAVE THE CURRENT CHARACTER
	mov al, dh			# CHECK THE PLUS BIT
	or al, al
	mov al, '+'			# SETUP TO PRINT THE PLUS (MOV does not change flags)
	jz Q_Q_L020
	call OUTDO			# PRINT IT IF THE BIT WAS SET
Q_Q_L020:
	pop ax				# GET BACK THE CURRENT CHARACTER
	ret
