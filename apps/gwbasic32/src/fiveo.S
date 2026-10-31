# fiveo.S: WHILE/WEND, CHAIN, WRITE. Translated from FIVEO.ASM (GW-BASIC 5.0 features).
# CALL (machine code) is not in this port: its dispatch name maps to "Illegal function call"
# (see NOTES.md, Stage 3 FIVEO). Program lines are [link 4][line number 2][text...][0].
#
# WHILE entry on the stack (12 bytes, low address first):
#   [pad 1][$WHILE token 1][WEND text pointer 4][WHILE text pointer 4][line number 2]
# The pad and token are pushed as one word last (CH=token, CL=pad). Pointers are popped as 4 bytes.
	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "tokens.inc"
	.include "forent.inc"		# FE_SIZE (FOR entry size, 26) for the FNDWND skip

	.set WHLSKIP, 10			# FNDWND skips 10 bytes after the token (the WHILE entry body; gwfor.S steps WHLSIZ=10)
	.set FORSZC, FE_SIZE-1		# FNDWND skips FORSZC-1 = 24 bytes after the token (FOR entry, see forent.inc)
	.set CRLFSQ, 2			# bytes in a CR/LF sequence
	.set ERRWE, 30			# "WEND without WHILE" (gwdata_errors.inc; keep in sync)

	.text
	.globl WHILE, WEND, CHAIN, COMMON, CHNRET, SKPNAM, WRITE

# ----------------------------------------------------------------------------
# WHILE, WEND
# ----------------------------------------------------------------------------
WHILE:					# KEEP THE WHILE TEXT POINTER HERE
	mov DWORD PTR ENDFOR, ebx	# SAVE TEXT ADDRESS
	call WNDSCN			# SCAN FOR THE MATCHING WEND (ERRWH if none)
	call CHRGTR			# POINT AT CHARACTER AFTER WEND
	xchg ebx, edx			# POSITION OF MATCHING WEND
	call FNDWND			# SEE IF THERE IS A STACK ENTRY FOR THIS WHILE
	lahf
	add esp, 4			# GET RID OF THE NEWSTT ADDRESS ON THE STACK (4 bytes)
	sahf
	jnz WNOTOL			# IF NO MATCH NO NEED TO TRUNCATE THE STACK
	add ebx, ecx			# ELIMINATE EVERYTHING UP TO AND INCLUDING THE MATCHING WHILE ENTRY
	mov esp, ebx
	mov DWORD PTR SAVSTK, ebx
WNOTOL:	push WORD PTR CURLIN		# MAKE THE STACK ENTRY: line number (2)
	mov ebx, DWORD PTR ENDFOR
	push ebx			# WHILE text pointer (4)
	push edx			# WEND text pointer (4)
	jmp FNWEND			# FINISH USING WEND CODE (token word is pushed when the loop is entered)
WEND:	jz Q_L000
	jmp SNERR			# STATEMENT HAS NO ARGUMENTS
Q_L000:	xchg ebx, edx			# FIND MATCHING WHILE ENTRY ON STACK
	call FNDWND
	jnz WEERR			# MUST MATCH OR ELSE ERROR
	mov esp, ebx			# TRUNCATE STACK AT MATCH POINT
	mov DWORD PTR SAVSTK, ebx	# [EBX] POINTING INTO STACK ENTRY
	movzx edx, WORD PTR CURLIN	# REMEMBER WEND LINE #
	mov WORD PTR NXTLIN, dx		# IN NXTLIN
	add ebx, 4			# INDEX INTO STACK ENTRY: SKIP OVER TEXT POINTER OF WEND
	mov edx, DWORD PTR [ebx]	# [EDX]=TEXT POINTER OF WHILE
	add ebx, 4
	movzx ebx, WORD PTR [ebx]	# [EBX]=LINE NUMBER OF WHILE
	mov WORD PTR CURLIN, bx		# IN CASE OF ERROR OR CONTINUATION FIX CURLIN
	xchg ebx, edx			# GET TEXT POINTER OF WHILE FORMULA INTO [EBX]
FNWEND:	call FRMEVL			# EVALUATE FORMULA
	push ebx			# SAVE TEXT POINTER
	call VSIGN			# GET IF TRUE OR FALSE
	pop ebx				# GET BACK WHILE TEXT POINTER
	jz FLSWHL			# GO BACK AT WEND IF FALSE
	mov cl, _WHILE			# COMPLETE WHILE ENTRY: pad and token (CH=token, CL=pad)
	mov ch, _WHILE
	push cx				# 2 bytes, pushed last (the low address of the entry)
	jmp NEWSTT
FLSWHL:	movzx ebx, WORD PTR NXTLIN	# SETUP CURLIN FOR WEND
	mov WORD PTR CURLIN, bx
	pop ebx				# TAKE OFF TEXT OF WEND AS NEW TEXT POINTER (4)
	pop ecx				# GET RID OF TEXT POINTER OF WHILE (4)
	pop cx				# TAKE OFF LINE NUMBER OF WHILE (2)
	jmp NEWSTT

# FNDWND: search the stack for a WHILE entry whose WEND text pointer matches [EDX].
# Returns ZF=1 and [EBX]=entry position (at the WEND pointer), [ECX]=WHLSKIP when found.
# FOR entries are skipped; anything else ends the search with ZF=0.
FNDWND:	mov ebx, 8			# SKIP OVER RETURN ADDRESS AND NEWSTT (4 bytes each)
	add ebx, esp
FNDWN2:	inc ebx
	mov al, BYTE PTR [ebx]		# GET THE ENTRY TYPE
	inc ebx
	mov ecx, _FOR
	cmp al, cl			# SEE IF ITS $FOR
	jnz FNDWN3
	mov ecx, FORSZC-1		# Yes, so skip over it (pointer already incremented once)
	add ebx, ecx
	jmp FNDWN2
FNDWN3:	mov ecx, _WHILE
	cmp al, cl
	jz FNDWW
	ret
FNDWW:	cmp DWORD PTR [ebx], edx	# SAME WEND?
	mov ecx, WHLSKIP		# (pointer already incremented once)
	jnz FNDWW2
	ret				# RETURN IF ENTRY MATCHES
FNDWW2:	add ebx, ecx
	jmp FNDWN2
WEERR:	mov dx, ERRWE
	jmp ERROR

# ----------------------------------------------------------------------------
# CHAIN
# CHAIN [MERGE]<file name>[,[<line number>][,ALL][,DELETE <range>]]
# Steps: 1 scan arguments; 2 scan program for COMMON; 3 squeeze unmarked simple variables;
# 4 copy string literals to string space; 5 move variables into string space; 6 load program;
# 7 move variables back (CHNRET); 8 run.
# ----------------------------------------------------------------------------
CHAIN:	xor al, al			# Assume no MERGE
	mov BYTE PTR MRGFLG, al
	mov BYTE PTR MDLFLG, al		# Also no MERGE w/ DELETE option
	mov al, BYTE PTR OPTFLG
	mov BYTE PTR TOPTFG, al		# SAVE OPTION BASE VALUE
	mov al, BYTE PTR OPTVAL		# SAVE OPTION VALUE FOR ARRAY BASE
	mov BYTE PTR TOPTVL, al
	mov al, BYTE PTR [ebx]		# Get current char
	cmp al, _MERGE			# Is it MERGE?
	jnz NTCHNM
	mov BYTE PTR MRGFLG, al		# Set MERGE flag
	inc ebx
NTCHNM:	dec ebx				# Rescan file name
	call CHRGTR
	call PRGFLI			# Evaluate file name and OPEN it
	push ebx
	mov ebx, 0
	mov WORD PTR CHNLIN, bx		# Assume no CHAIN line #
	pop ebx
	dec ebx
	call CHRGTR			# Scan char
	jnz Q_L001
	jmp NTCHAL			# No line number etc.
Q_L001:	call SYNCHR
	.byte 44			# Must be comma
	cmp al, 44			# Ommit line # (Use ALL for instance)
	jz NTLINF
	call FRMEVL			# Evaluate line # formula
	push ebx
	call FRQINT			# Force to int in [BX]
	mov WORD PTR CHNLIN, bx		# Save it for later
	pop ebx
	dec ebx
	call CHRGTR
	jz NTCHAL			# No ALL, i.e. preserve all vars across CHAIN
NTLINF:	call SYNCHR
	.byte 44			# Should be comma here
	cmp al, _DELETE			# Test for DELETE option
	jz CHMWDL
	call SYNCHR
	.byte 'A'			# Check for "ALL"
	call SYNCHR
	.byte 'L'
	call SYNCHR
	.byte 'L'
	jnz Q_L002
	jmp DNCMDA			# Goto step 3
Q_L002:	call SYNCHR
	.byte 44			# Force comma to appear
	cmp al, _DELETE			# Must be DELETE
	jz Q_L003
	jmp SNERR			# No, give error
Q_L003:	or al, al			# Flag to goto DNCMDA
CHMWDL:	pushfd				# Save ALL flag (4 bytes)
	mov BYTE PTR MDLFLG, al		# Set MERGE w/ DELETE
	call CHRGTR			# Get char after comma
	call SCNLIN			# Scan line range
	push ecx
	call DEPTR			# Change pointers back to numbers
	pop ecx
	pop dx				# Pop max line off stack (2)
	push ecx			# Save pointer to start of 1st line
	mov ebx, ecx			# Save pointer to start line
	mov DWORD PTR CMSPTR, ebx
	call FNDLIN			# Find the last line (DX=line)
	jae FCERRG			# Must have exact match on end of range
	mov edx, ebx			# [EDX] = pointer beyond the last line in the range
	mov DWORD PTR CMEPTR, ebx	# Save pointer to end line
	pop ebx				# Get back pointer to start of range
	cmp ebx, edx			# Make sure the start comes before the end
FCERRG:	jb Q_L004
	jmp FCERR			# If not, "Illegal function call"
Q_L004:	popfd				# Flag that says whether to go to DNCMDA
	jz Q_L005
	jmp DNCMDA			# "ALL" option was present
Q_L005:
NTCHAL:	movzx ebx, WORD PTR CURLIN	# Save current line number on stack (2)
	push ebx
	mov ebx, DWORD PTR TXTTAB	# Start searching for COMMONs at program start
	dec ebx				# Compensate for next instr
CLPSC1:	inc ebx				# Look at first char of next line
CLPSCN:	cmp DWORD PTR [ebx], 0		# Are we pointing to program end? (4-byte link)
	jnz Q_L006
	jmp CLPFIN			# Yes
Q_L006:	add ebx, 4			# Line number field
	mov dx, WORD PTR [ebx]		# Get line # in [DX]
	inc ebx
	mov WORD PTR CURLIN, dx		# Save current line # in CURLIN for errors
CSTSCN:	call CHRGTR			# Get statement type
AFTCOM:	or al, al
	jz CLPSC1			# EOL: scan next one
	cmp al, ':'			# Are we looking at colon
	jz CSTSCN			# Yes, get next statement
	cmp al, 254			# COMMONs preceded by 254
	jnz NOCOMM
	inc ebx				# Move ahead
	mov al, BYTE PTR [ebx]
	cmp al, _COMMON			# Is it a COMMON?
	jz DOCOMM
	dec ebx				# Back up the pointer
NOCOMM:	call CHRGTR			# Get first char of statement
	call DATA			# Skip over statement
	dec ebx				# Back up to rescan terminator
	jmp CSTSCN
DOCOMM:	call CHRGTR			# Get thing after COMMON
	jz AFTCOM
NXTCOM:	push ebx			# Save text pointer (4)
	mov al, 1			# Call PTRGET to search for array
	mov BYTE PTR SUBFLG, al
	call PTRGTN			# This subroutine in F3 scans variables
	jz FNDAAY			# Found array
	mov al, ch			# Try finding array with COMMON bit set
	or al, 128
	mov ch, al
	xor al, al			# Set zero CC
	call ERSFIN			# Search array table
	mov al, 0			# Clear SUBFLG in all cases
	mov BYTE PTR SUBFLG, al
	jnz NTFN2T			# Not found, try simple
	mov al, BYTE PTR [ebx]		# Get terminator, should be "("
	cmp al, '('
	jnz SCNSMP			# Must be simple then
	pop eax				# Get rid of saved text pointer
	jmp COMADY			# Already was COMMON, ignore it
NTFN2T:	mov al, BYTE PTR [ebx]		# Get terminator
	cmp al, '('			# Array specifier?
	pop edx				# (EDX := saved text pointer)
	jz SKPCOM			# Yes, undefined array: skip it
	push edx			# No, resave pointer to start of variable
SCNSMP:	pop ebx				# Rescan variable name for start
	call PTRGTN			# Evaluate as simple
	or edx, edx			# If var not found, [EDX]=0
	jnz COMFNS			# Found it
	mov al, ch			# Try to find in COMMON
	or al, 128			# Set COMMON bit
	mov ch, al
	mov edx, offset COMPT2		# push on return address
	push edx
	mov edx, offset PTRGTR		# address to common return point
	push edx
	mov al, BYTE PTR VALTYP		# Must have VALTYP in [DH]
	mov dh, al
	jmp NOARYS			# Search symbol table
COMPT2:	or edx, edx			# Found?
	jz SKPCOM			# No, just skip over this variable
COMFNS:	push ebx			# Save text pointer
	mov ecx, edx			# Get pointer to var in [ECX]
	mov ebx, offset BCKUCM		# Loop back here
	push ebx
CBAKBL:	dec ecx				# Point at first char of rest
LPBKNC:	mov esi, ecx
	mov al, BYTE PTR [esi]		# Back up until plus byte
	dec ecx
	or al, al
	jns Q_L007
	jmp LPBKNC
Q_L007:					# Now point to 2nd char of var name
	mov esi, ecx
	mov al, BYTE PTR [esi]		# set COMMON bit
	or al, 128
	mov edi, ecx
	stosb
	ret				# done
FNDAAY:	mov BYTE PTR SUBFLG, al		# Array found, clear SUBFLG
	mov al, BYTE PTR [ebx]		# Make sure really array spec
	cmp al, '('			# Really an array?
	jnz SCNSMP			# No, scan as simple
	pop esi				# XTHL
	xchg esi, ebx
	push esi			# Save text pointer, get rid of saved text pointer
BAKCOM:	dec ecx				# Point at last char of name extension
	dec ecx
	call CBAKBL			# Back up before variable and mark as COMMON
BCKUCM:	pop ebx				# Restore text pointer
SKPCOM:	dec ebx				# Rescan terminator
	call CHRGTR
	jnz Q_L008
	jmp AFTCOM			# End of COMMON statement
Q_L008:	cmp al, '('			# End of COMMON array spec?
	jnz CHKCST			# No, should be comma
COMADY:	push ebx
	call CHRGTR			# Fetch char after paren
	cmp al, ')'
	jz COMRPN			# Only right paren follows
	pop ebx
	call EVAL			# Possible number of dimensions (compiler compatible)
	call GETYPR
	jnz Q_L009
	jmp FCERR			# Dimensions argument cannot be string
Q_L009:	jmp COMRP1
COMRPN:	pop edx
COMRP1:	call SYNCHR
	.byte ')'			# Right paren should follow
	jnz Q_L010
	jmp AFTCOM			# End of COMMON
Q_L010:
CHKCST:	call SYNCHR
	.byte 44			# Force comma to appear here
	jmp NXTCOM			# Get next COMMON variable

# Step 3 - squeeze unmarked simple variables and arrays
CLPFIN:	pop bx				# Restore previous CURLIN (2)
	mov WORD PTR CURLIN, bx
	mov edx, DWORD PTR ARYTAB	# End of simple var squeeze to [EDX]
	mov ebx, DWORD PTR VARTAB	# Start of simps
CLPSLP:	cmp ebx, edx			# Are we done?
	jz DNCMDS			# Yes, done with simps
	push ebx			# Save where this simp is
	mov cl, BYTE PTR [ebx]		# Get VALTYP
	cmp cl, 3			# string value is a 5-byte descriptor (STRSIZ 5)
	jnz Q_L020
	mov cl, 5
Q_L020:
	inc ebx
	inc ebx
	mov al, BYTE PTR [ebx]		# Get COMMON bit
	or al, al			# Set minus if COMMON
	pushfd				# Save indicator
	and al, 127			# Clear COMMON bit
	mov BYTE PTR [ebx], al		# Save back
	inc ebx
	call IADAHL			# Skip over rest of var name
	movzx ecx, cl			# Skip VALTYP bytes
	add ebx, ecx
	popfd				# Get indicator whether to delete
	pop ecx				# Pointer to where var started
	jns Q_L011
	jmp CLPSLP
Q_L011:	push ecx			# This is where we will resume scanning vars later
	call VARDLS			# Delete variable
	mov ebx, DWORD PTR ARYTAB	# Now correct ARYTAB by # of bytes deleted
	add ebx, edx			# Add negative difference between old and new
	mov DWORD PTR ARYTAB, ebx	# Save new ARYTAB
	xchg ebx, edx			# To [EDX]
	pop ebx				# Get current place back
	jmp CLPSLP

VARDLS:	xchg ebx, edx			# Point to where var ends
VARDL1:	mov ebx, DWORD PTR STREND	# One beyond last byte to move
DLSVLP:	cmp ebx, edx			# Done?
	mov esi, edx
	mov al, BYTE PTR [esi]		# Grab byte
	mov edi, ecx
	stosb				# Move down
	lahf
	inc edx				# Increment pointers
	sahf
	lahf
	inc ecx
	sahf
	jnz DLSVLP
	mov edx, ecx			# [EDX] = [ECX] - [EBX] (difference, 32-bit)
	sub edx, ebx
	dec edx				# Correct # of bytes
	dec ecx				# Moved one too far
	mov ebx, ecx			# Get new STREND
	mov DWORD PTR STREND, ebx
	ret

DNCMDS:	mov edx, DWORD PTR STREND	# Limit of array search
CLPAKP:	cmp ebx, edx			# Done?
	jz DNCMDA			# Yes
	push ebx			# Save pointer to VALTYP
	inc ebx				# Move down to COMMON bit
	inc ebx
	mov al, BYTE PTR [ebx]		# Get it
	or al, al			# Set CC's
	pushfd				# Save COMMON indicator
	and al, 127			# Clear COMMON bit
	mov BYTE PTR [ebx], al		# Save back
	inc ebx				# Point to length of array
	call IADAHL			# Add length of var name
	mov cl, BYTE PTR [ebx]		# Get length of array in [CX]
	inc ebx
	mov ch, BYTE PTR [ebx]
	inc ebx
	movzx ecx, cx
	add ebx, ecx			# [EBX] now points after array
	popfd				# Get back COMMON indicator
	pop ecx				# Get pointer to start of array
	jns Q_L012
	jmp CLPAKP			# COMMON, don't delete!
Q_L012:	push ecx			# Save so we can resume
	call VARDLS			# Delete the array
	xchg ebx, edx			# Returns with STREND in [EBX], so put in [EDX]
	pop ebx				# Get back pointer to the next array
	jmp CLPAKP			# Check next array

# Step 4 - copy literals into string space
DNCMDA:	mov ebx, DWORD PTR VARTAB	# Look at simple strings
CSVAR:	mov edx, DWORD PTR ARYTAB	# Limit of search
	cmp ebx, edx			# Done?
	jz CAYVAR			# Yes
	call SKPNAM			# Skip name, returns Z if was a string
	jnz CSKPVA			# Skip this var, not string
	call CDVARS			# Copy this guy into string space if necessary
	xor al, al			# CDVARS has already incremented [EBX]
CSKPVA:	movzx edx, al			# Add length of VALTYP
	add ebx, edx
	jmp CSVAR
CAYVA2:	pop ecx				# Adjust stack
CAYVAR:	mov edx, DWORD PTR STREND	# New limit of search
	cmp ebx, edx			# Done?
	jz DNCCLS			# Yes
	call SKPNAM			# Skip over name
	push ax				# Save VALTYP
	mov cl, BYTE PTR [ebx]		# Get length of array
	inc ebx
	mov ch, BYTE PTR [ebx]		# Into [CX]
	inc ebx
	pop ax				# Get back VALTYP
	push ebx			# Save pointer to array element
	movzx ecx, cx
	add ebx, ecx			# Point after array
	cmp al, 3			# String array?
	jnz CAYVA2			# No, look at next one
	mov DWORD PTR TEMP3, ebx	# Save pointer to end of array
	pop ebx				# Get back pointer to array start
	mov cl, BYTE PTR [ebx]		# Pick up number of DIMs
	movzx ecx, cl			# Make double with high zero
	add ebx, ecx			# Go past DIMS
	add ebx, ecx
	inc ebx				# One more to account for # of DIMs
CAYSTR:	mov edx, DWORD PTR TEMP3	# Get end of array
	cmp ebx, edx			# See if at end of array
	jz CAYVAR			# Get next array
	mov ecx, offset CAYSTR		# Do next str in array
	push ecx			# Save branch address on stack
CDVARS:	mov al, BYTE PTR [ebx]		# Get length of array entry
	inc ebx				# Also pick up pointer into [EDX] (4 bytes)
	mov edx, DWORD PTR [ebx]		# Get data pointer
	add ebx, 4
	or al, al			# Set CC's on length
	jnz Q_L013
	ret				# Ignore null strings
Q_L013:	push ebx			# Save where we are
	mov ebx, DWORD PTR VARTAB	# Is string in program text or disk buffers?
	cmp ebx, edx			# Compare
	pop ebx				# Restore where we are
	jnb Q_L014
	ret				# No, must be in string space
Q_L014:	push ebx			# Save where we are again
	mov ebx, DWORD PTR TXTTAB	# Is it in buffers?
	cmp ebx, edx			# Test
	pop ebx				# Restore where we are
	jb Q_L015
	ret				# In buffers, do nothing
Q_L015:	push ebx			# Save where we are for nth time
	sub ebx, 5			# Point to start of descriptor (5 bytes: length + 4-byte pointer)
	push ebx			# Save pointer to start
	call STRCPY			# Copy string into DSCTMP
	pop ebx				# Destination in [EBX], source in [EDX]
	mov ch, 5			# # of bytes to move (5-byte string descriptor)
	call MOVE1			# Move em
	pop ebx				# Where we are
	ret

# Step 5 - move stuff up into string space
DNCCLS:	call GARBA2			# Get rid of unused strings
	mov ebx, DWORD PTR STREND	# Load end of vars
	mov ecx, ebx			# Into [ECX]
	mov edx, DWORD PTR VARTAB	# Start of simps into [EDX]
	mov ebx, DWORD PTR ARYTAB
	sub ebx, edx			# Get length of simps in [EBX]
	mov DWORD PTR TEMP9, ebx	# Save here
	mov ebx, DWORD PTR FRETOP	# Destination of high byte
	mov DWORD PTR SAVFRE, ebx	# Save FRETOP to restore later
	call BLTUC			# Move stuff up
	mov ebx, ecx			# Now adjust top of memory below saved vars
	dec ebx				# One lower to be sure
	mov DWORD PTR FRETOP, ebx	# Update FRETOP to reflect new value
	mov al, BYTE PTR MDLFLG		# MERGE w/ DELETE?
	or al, al
	jz NTMDLT			# No
	mov ebx, DWORD PTR CMSPTR	# Start of lines to delete
	mov ecx, ebx			# Into [ECX]
	mov ebx, DWORD PTR CMEPTR	# End of lines to delete
	call DEL			# Delete the lines
	mov DWORD PTR ARYTAB, ebx	# ***also set up ARYTAB and STREND
	mov DWORD PTR STREND, ebx	# in case we get error in CHAIN
	call LINKER			# Re-link lines just in case

# Step 6 - load new program
NTMDLT:	mov al, 1			# Set CHAIN flag
	mov BYTE PTR CHNFLG, al
	mov al, BYTE PTR MRGFLG		# MERGEing?
	or al, al
	jz Q_L016
	jmp OKGETM			# Do MERGE
Q_L016:	jmp CHNENT			# Jump to LOAD code

# Step 7 - move stuff back down
CHNRET:	mov al, BYTE PTR TOPTVL
	mov BYTE PTR OPTVAL, al		# RESTORE OPTION BASE VALUE
	mov al, BYTE PTR TOPTFG
	mov BYTE PTR OPTFLG, al		# RESTORE OPTION FLG
	xor al, al			# Clear CHAIN, MERGE flags
	mov BYTE PTR CHNFLG, al
	mov BYTE PTR MRGFLG, al
	mov ebx, DWORD PTR VARTAB	# Get current VARTAB
	mov ecx, ebx			# Into [ECX]
	mov ebx, DWORD PTR TEMP9	# Get length of simps
	add ebx, ecx			# Add to present VARTAB to get new ARYTAB
	mov DWORD PTR ARYTAB, ebx
	mov ebx, DWORD PTR FRETOP	# Where to start moving
	inc ebx				# One higher
	xchg ebx, edx			# Into [EDX]
	mov ebx, DWORD PTR SAVFRE	# Last byte to move
	mov DWORD PTR FRETOP, ebx	# Restore FRETOP from this
MVBKVR:	cmp ebx, edx			# Done?
	mov esi, edx
	mov al, BYTE PTR [esi]		# Move byte down
	mov edi, ecx
	stosb
	lahf
	inc edx				# Increment pointers
	sahf
	lahf
	inc ecx
	sahf
	jnz MVBKVR
	dec ecx				# Point to last var byte
	mov ebx, ecx			# [EBX]=last var byte
	mov DWORD PTR STREND, ebx	# This is new end
	xor al, al
	mov BYTE PTR NLONLY, al		# allow all files to be closed
	call FINPRT			# close file zero and reset PTRFIL to 0
	xor al, al
	call RESTORE			# Make sure DATA is valid by doing RESTORE
	mov dx, WORD PTR CHNLIN		# Get CHAIN line # in [DX]
	mov ebx, DWORD PTR TXTTAB	# Get prog start
	dec ebx				# Point at zero before program
	or dx, dx			# line number zero?
	jnz Q_L017
	jmp NEWSTT			# line #=0, go...
Q_L017:	call FNDLIN			# Try to find destination line
	jb Q_L018
	jmp USERR			# Not there...
Q_L018:	dec ecx				# Point to zero on previous line
	mov ebx, ecx			# Make text pointer for NEWSTT
	jmp NEWSTT

# SKPNAM: skip a variable's name at [EBX]. Returns VALTYP in [AL] with ZF set if it is a string.
SKPNAM:	mov al, BYTE PTR [ebx]		# Get VALTYP
	inc ebx				# Point to length of long var name
	inc ebx
	inc ebx
	push ax				# Save VALTYP
	call IADAHL			# Move past long variable name
	pop ax				# Get back VALTYP
	cmp al, 3			# String?
	ret

COMMON:	jmp DATA

# ----------------------------------------------------------------------------
# WRITE
# ----------------------------------------------------------------------------
WRITE:	mov cl, MD_SQO			# Setup output file
	call FILGET
WRTCHR:	dec ebx
	call CHRGTR			# Get another character
	jz WRTFIN			# Done with WRITE
WRTMLP:	call FRMEVL			# Evaluate formula
	push ebx			# Save the text pointer
	call GETYPR			# See if we have a string
	jz WRTSTR			# We do
	call FOUT			# Convert to a string
	call STRLIT			# Literalize string
	mov ebx, DWORD PTR FACLO	# Get pointer to string descriptor
	inc ebx				# Point to address field (4 bytes)
	mov edx, DWORD PTR [ebx]
	mov esi, edx
	mov al, BYTE PTR [esi]		# Is number positive?
	cmp al, ' '			# Test
	jnz WRTNEG			# No, must be negative
	inc edx
	mov DWORD PTR [ebx], edx	# Store the adjusted address
	dec ebx
	dec BYTE PTR [ebx]		# Adjust length of string (length < 255 so OK)
WRTNEG:	call STRPRT			# Print the number
NXTWRV:	pop ebx				# Get back text pointer
	dec ebx				# Back up pointer
	call CHRGTR			# Get next char
	jz WRTFIN			# end
	cmp al, 59			# Semicolon?
	jz WASEMI			# Was one
	call SYNCHR
	.byte 44			# Only possible left is comma
	dec ebx				# to compensate for later CHRGET
WASEMI:	call CHRGTR			# Fetch next char
	mov al, 44			# put out comma
	call OUTDO
	jmp WRTMLP			# Back for more
WRTSTR:	mov al, 34			# put out double quote
	call OUTDO			# Send it
	call STRPRT			# print the string
	mov al, 34			# Put out another double quote
	call OUTDO			# Send it
	jmp NXTWRV			# Get next value
WRTFIN:	push ebx			# Save text pointer
	mov ebx, DWORD PTR PTRFIL	# See if disk file
	or ebx, ebx
	jz NTRNDW			# No
	push ebx			# Save FDB pointer
	mov ecx, F_DEV
	add ebx, ecx			# EBX points at Device Entry in FDB
	mov al, BYTE PTR [ebx]		# [AL]=device id
	or al, al			# if disk [AL] will be 0..n
	pop ebx
	jns Q_L019
	jmp NTRNDW			# branch if special device ([AL] is negative)
Q_L019:	push ebx
	mov ecx, F_MODE
	add ebx, ecx			# EBX points to File Mode Byte in FDB
	mov al, BYTE PTR [ebx]		# [AL]=file mode
	pop ebx				# Restore FDB pointer
	cmp al, MD_RND			# Random?
	jnz NTRNDW			# NO
	call CMPFBC			# See how many bytes left (DX)
	movzx edx, dx
	sub ebx, edx			# subtract
	mov edx, -CRLFSQ		# Subtract bytes in <cr>
	add ebx, edx			# (sets CF as the 16-bit original did for ebx >= 2)
	jae NTRNDW			# Not enough, give error eventually
NXTWSP:	mov al, ' '			# Put out spaces
	call OUTDO			# Send space
	dec ebx				# Count down
	jnz NXTWSP
NTRNDW:	pop ebx				# Restore [EBX]
	call CRDO			# Do crlf
	jmp FINPRT
