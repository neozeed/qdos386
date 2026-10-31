# call86.S -> CALL86.ASM: the CALL and CALLS statements.
#
# The argument parsing is the original: the subroutine name is a simple variable (PTRGET), its
# value is an integer address (TEMPA), and the parameter list is a list of simple variables
# pushed as pointers. Parameters are pointers in this port (4 bytes), so the original's
# "push segment" for CALL is kept as a zero placeholder (the flat address space has no segments).
#
# UNSUPPORTED: the final step, transferring control to a machine-code routine at TEMPA
# (CALLST's far jump and the return to CALLRT), is not available in the 32-bit port. The
# instruction sequence is kept below as a comment. The statement reaches ADVERR after the
# parameter list has been parsed, so a syntax error in CALL is still reported as the original would.

	.intel_syntax noprefix
	.section .text
	.globl CALLS, CALLSL

CALLSL:
	clc				# clear carry: CALL (push the segment placeholder)
	jmp CALLS1

CALLS:
	stc				# set carry: CALLS (no segment placeholder)
CALLS1:
	pushfd				# save the CALL/CALLS flag
	call PRODIR			# do not allow CALL in direct mode of a protected file
	mov BYTE PTR SUBFLG, 128	# scan only a simple var (octal 200): scan and search symbol table
	call PTRGET
	mov BYTE PTR SARYFL, 0		# clear scanned-array-element flag
	push ebx			# save text pointer
	mov ebx, edx			# get pointer to var in BX
	call VMOVFM			# load variable into FAC
	call FRQINT			# make it an integer (BX = value)
	mov WORD PTR TEMPA, bx		# save the subroutine address
	mov cl, 32			# get max # of parameters
	call GETSTK			# see if there is that much stack space
	pop ebx				# get back text pointer
	call CHRGT2			# eat character after var name
	jz CALLST			# end of statement, no parameter list
	call SYNCHR			# check for open paren
	.byte 40			# (

GETPAR:
	push DWORD PTR ARYTAB		# save pointer to start of array var data
	call PTRGET			# scan parameter variable
	pop ecx				# [ECX] = old value of ARYTAB
	cmp ecx, DWORD PTR ARYTAB	# if = old, no undefined simples were referenced
	je NONEWS			# branch if new simple var not encountered
	cmp BYTE PTR SARYFL, 0		# zero if no array elements have been parsed by PTRGET
	jz NONEWS
	jmp FCERR			# undefined scalars can't follow array elements (addresses change)

NONEWS:
	popfd				# restore carry = CALL/CALLS flag
	jb SHTPRM			# branch if CALLS (no segment placeholder)
	push 0				# segment placeholder (flat address space; the original pushed DS)
SHTPRM:
	push edx			# save pointer of the parameter
	pushfd				# re-save CALL/CALLS flag
	mov al, BYTE PTR [ebx]		# get terminator
	cmp al, 44			# comma?
	jnz ENDPAR			# no, must be end of the parameter list
	call CHRGTR			# scan next char
	jmp GETPAR			# scan next parameter

ENDPAR:
	call SYNCHR			# check for terminating right paren
	.byte 41			# )

CALLST:
	mov DWORD PTR TEMP, ebx		# save text pointer
	popfd				# discard CALL/CALLS flag
	# The original pushed CS, the CALLRT return address, the subroutine segment and address, then
	# did a far return. Native machine-code execution is not supported in this port.
	jmp ADVERR			# UNSUPPORTED: native machine-code CALL (see the header)

# CALLRT: return point of the machine-code routine. Not reached while CALLST is unsupported.
CALLRT:
	mov ebx, DWORD PTR TEMP		# get back text pointer
	ret
