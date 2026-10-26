/* void halt_with_asm(int status): EAX = status, HLT (i386VM ends the run). */
.text
.globl _halt_with_asm
_halt_with_asm:
	movl 4(%esp),%eax
	hlt
