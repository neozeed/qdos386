/*
 * QDOS/386 R1 - architecture-specific assembly.
 *
 * Two saved-state formats exist for a job's stack:
 *   fmt 0 (CTX): callee-saved registers pushed by yield_asm, then return
 *                address.  Restored by a pop/ret sequence.
 *   fmt 1 (IRQ): full general register set pushed by irq0_stub (pushal).
 *                Restored by popal/iretl, so EFLAGS, EIP and CS come from
 *                the interrupt frame.
 *
 * Both schedulers (cooperative yield and timer preemption) return
 *   (saved_sp | fmt) in EAX.  resume_dispatch uses bit 0 to choose the
 *   restore path.  Stacks are 4-byte aligned, so bits 0-1 are free.
 *
 * C functions referenced from here use the GCC 1.40 leading-underscore
 * convention.
 */

.text

/* void load_descriptors(u32 gdtr_addr, u32 idtr_addr)  -- flat GDT + IDT */
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

/* void halt_with(int status): EAX=status, HLT (ends the run) */
.globl _halt_with
_halt_with:
	movl 4(%esp),%eax
	hlt

/*
 * Common resume path.  EAX = saved_sp | fmt.
 * Interrupts are disabled here on entry from an ISR, and are enabled
 * before a fmt-0 job resumes, since that job expects IF=1.
 */
resume_dispatch:
	movl %eax,%ecx
	andl $1,%ecx		/* ECX = format (1 = IRQ) */
	andl $-4,%eax
	movl %eax,%esp
	cmpl $0,%ecx
	je resume_ctx
	popal			/* restores EDI..EAX, skipping saved ESP */
	iret			/* restores EIP, CS, EFLAGS (IF) */
resume_ctx:
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	sti
	ret

/*
 * Timer preemption entry (IRQ0, vector 0x20, interrupt gate).
 * Saves the complete interrupted state, lets the scheduler choose the
 * next job, and resumes that job's saved state.
 */
.globl _irq0_stub
_irq0_stub:
	pushal
	pushl %esp
	call _sched_irq_tick
	addl $4,%esp
	jmp resume_dispatch

/*
 * Cooperative yield.  Saves callee-saved registers as a fmt-0 frame, lets the
 * scheduler choose the next job and resumes it.  Returns when the calling job
 * is next resumed.
 */
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

/*
 * First code run by a new job.  A fresh job's frame returns here; the job's
 * entry point is then called, and its return reaches the termination path.
 */
.globl _job_trampoline
_job_trampoline:
	call _job_run
	hlt			/* job_run never returns; keep the stack-top defined */
