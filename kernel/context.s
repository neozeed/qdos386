/*
 * QDOS/386 R1 - architecture-specific assembly (shared by both platforms).
 *
 * Two saved-state formats exist for a job's stack:
 *   fmt 0 (cooperative): callee-saved registers pushed by yield_asm, then
 *                        the return address.  Restored by pop/ret.
 *   fmt 1 (interrupt):   the full register set pushed by pushal in the
 *                        IRQ0 or trap stub.  Restored by popal/iret.
 * Scheduler entry points return (saved_sp | fmt) in EAX; bit 0 selects the
 * restore path.  Stacks are 4-byte aligned, so bits 0-1 are free.
 *
 * C functions called from here use the GCC 1.40 leading-underscore names.
 */

.text

/* void load_descriptors(u32 gdtr_addr, u32 idtr_addr) */
.globl _load_descriptors
_load_descriptors:
	movl 4(%esp),%eax
	lgdt (%eax)
	movw $0x10,%ax
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss
	movw %ax,%fs
	movw %ax,%gs
	ljmp $0x08,$1f
1:
	movl 8(%esp),%eax
	lidt (%eax)
	ret

/* u32 read_cr0(void) */
.globl _read_cr0
_read_cr0:
	movl %cr0,%eax
	ret

/*
 * Common resume path.  EAX = saved_sp | fmt.
 * Interrupts are off on entry from an interrupt stub.  A cooperative frame
 * is resumed with IF set, since that job expects it.
 */
resume_dispatch:
	movl %eax,%ecx
	andl $1,%ecx
	andl $-4,%eax
	movl %eax,%esp
	cmpl $0,%ecx
	je resume_ctx
	popal
	iret
resume_ctx:
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	sti
	ret

/* IRQ0 (vector 0x20, interrupt gate) */
.globl _irq0_stub
_irq0_stub:
	pushal
	pushl %esp
	call _sched_irq_tick
	addl $4,%esp
	jmp resume_dispatch

/* Service trap (vector 0x30, interrupt gate). */
.globl _trap_stub
_trap_stub:
	pushal
	pushl %esp
	call _trap_dispatch
	addl $4,%esp
	jmp resume_dispatch

/*
 * int qtrap(fn, a, b, c): EAX=fn, EBX=a, ECX=b, EDX=c, INT 0x30.
 * EBX is callee-saved in cdecl, so it is saved here.
 */
.globl _qtrap
_qtrap:
	pushl %ebx
	movl 8(%esp),%eax
	movl 12(%esp),%ebx
	movl 16(%esp),%ecx
	movl 20(%esp),%edx
	int $0x30
	popl %ebx
	ret

/* Cooperative yield: callee-saved frame, scheduler pass, resume. */
.globl _yield_asm
_yield_asm:
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %esp
	call _sched_yield_sp
	addl $4,%esp
	jmp resume_dispatch

/* First code run by a fresh job (its frame returns here). */
.globl _job_trampoline
_job_trampoline:
	call _job_run
	hlt
