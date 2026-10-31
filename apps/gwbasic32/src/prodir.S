# prodir.S -> GIODSK.ASM PROCHK and PRODIR (protected-file check for direct mode).
#
# PRODIR: when the current line is the direct line (CURLIN = 0FFFFH, so CURLIN+1 = 0), run PROCHK.
# PROCHK: a protected file (LOAD "name",P) may not be listed, edited or saved in direct mode.
#         PROFLG is nonzero then, and the original gives "Illegal function call".
# CURLIN is a 16-bit line number; BX is saved and restored because it is a pointer register.

	.intel_syntax noprefix
	.section .text
	.globl PROCHK, PRODIR

PRODIR:
	push ebx			# save [H,L]
	mov bx, WORD PTR CURLIN		# get current line #
	inc bx				# direct? (if BX=0, direct)
	pop ebx				# restore [H,L] (POP does not change the flags)
	jz PROCHK
	ret

PROCHK:
	pushfd				# save flags
	mov al, BYTE PTR PROFLG		# is this a protected file?
	or al, al			# set cc's
	jnz FCERRA			# yes, give error
	popfd				# restore flags
	ret

FCERRA:
	jmp FCERR
