/*
 * QDOS/386 R1 (PC) - kernel entry and port I/O.
 * kstart must be the first object in the link so it sits at 0x100000.
 */
.text
.globl kstart
kstart:
	cli                       /* IF is inherited from the boot path; keep it clear until the scheduler runs */
	movl $0x7FFFF0,%esp
	cld
	movl $_bss_start,%edi
	movl $_bss_end,%ecx
	subl %edi,%ecx
	xorl %eax,%eax
	rep
	stosb
	call _main
	movl $0,%eax
	call _pc_halt_done
1:
	hlt
	jmp 1b

/* void outb(unsigned int port, unsigned int val) */
.globl _outb
_outb:
	movl 4(%esp),%edx
	movb 8(%esp),%al
	outb %al,%dx
	ret

/* unsigned int inb(unsigned int port) */
.globl _inb
_inb:
	movl 4(%esp),%edx
	xorl %eax,%eax
	inb %dx,%al
	ret

.globl _pc_halt_done
_pc_halt_done:
	ret
