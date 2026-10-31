/*
 * FROZEN COPY (KCTX-FIX2): the kernel/kctx.s of R12A..R14 / R14A (sha256 5714e2db... as a file under its old name),
 * linked only by the frozen r12a-pc .. r14-pc and r13b-epoch targets so that those releases rebuild byte-identical.
 * It contains the misaligned-ESP resume defect (resume_dispatch: andl $-4; docs/KCTX-FIX1-AUDIT.md).  Do not link it
 * into a new kernel: the current context switcher is kernel/kctx.s.
 *
 * QDOS/386 R12A - architecture assembly for QDOS_DYNPROC kernels.  Derived from context.s (unchanged, still used by
 * every other image) and paging.s; see docs/R12A-INTERRUPT-STACKS.md.
 *
 * KERNEL ENTRY RULE.  Every entry stub (IRQ0, IRQ1, IRQ7, IRQ15, the service trap, exceptions 0..19 except #DF) pushes
 * the CPU frame and pushal on whatever stack is current - for a process running its own code, its PRIVATE user stack -
 * and then, if ESP is not inside the current job's kernel stack [kx_lo, kx_hi), moves ESP to kx_entry, a point on the
 * job's kernel stack (shared kernel heap, mapped in every directory).  The C handler gets the frame pointer and runs on
 * the kernel stack, so a CR3 load in the scheduler never removes the stack it runs on.  Jobs without a private stack
 * (SYSTEM, IDLE, template jobs) have kx_lo = 0, kx_hi = 0xFFFFFFFF: no switch, exactly the context.s behaviour.
 * The resume path sets ESP to the saved frame of the job chosen (its directory is already active) and pops it.
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

/*
 * IRQ1 (vector 0x21, keyboard).  The body reads the i8042 and sends the EOI;
 * it never schedules, so it returns by popal/iret.
 */
.globl _irq1_stub
_irq1_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb i1s
	cmpl _kx_hi,%eax
	jb i1k
i1s:	movl _kx_entry,%esp
i1k:	pushl %eax
	call _input_irq1
	popl %esp
	popal
	iret

/*
 * IRQ7 (vector 0x27, master) and IRQ15 (vector 0x2F, slave).  The C
 * dispatchers in kernel/irq.c read the in-service registers and send an EOI
 * only for a genuine interrupt.  A spurious IRQ7 or IRQ15 returns by iret
 * after the dispatcher has decided on the EOI.  These handlers never
 * schedule: after the call, popl %esp returns to the frame (on whatever stack it was pushed), then popal/iret.
 */
.globl _irq7_stub
_irq7_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb i7s
	cmpl _kx_hi,%eax
	jb i7k
i7s:	movl _kx_entry,%esp
i7k:	pushl %eax
	call _irq7_dispatch
	popl %esp
	popal
	iret

.globl _irq15_stub
_irq15_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb ifs
	cmpl _kx_hi,%eax
	jb ifk
ifs:	movl _kx_entry,%esp
ifk:	pushl %eax
	call _irq15_dispatch
	popl %esp
	popal
	iret

/* Software injection (guest tests only): int 0x27 and int 0x2F. */
.globl _sw_int27
_sw_int27:
	int $0x27
	ret

.globl _sw_int2f
_sw_int2f:
	int $0x2f
	ret

/* IRQ0 (vector 0x20, interrupt gate) */
.globl _irq0_stub
_irq0_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb i0s
	cmpl _kx_hi,%eax
	jb i0k
i0s:	movl _kx_entry,%esp
i0k:	pushl %eax
	call _sched_irq_tick
	addl $4,%esp
	jmp resume_dispatch

/* Service trap (vector 0x30, interrupt gate). */
.globl _trap_stub
_trap_stub:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb trs
	cmpl _kx_hi,%eax
	jb trk
trs:	movl _kx_entry,%esp
trk:	pushl %eax
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

/*
 * unsigned int irq_save(void): returns EFLAGS and clears IF.
 * void irq_restore(unsigned int flags): sets IF again only if the saved
 * flags had it set, so nested critical sections restore correctly.
 */
.globl _irq_save
_irq_save:
	pushfl
	popl %eax
	cli
	ret

.globl _irq_restore
_irq_restore:
	movl 4(%esp),%eax
	testl $0x200,%eax
	jz 1f
	sti
1:	ret

/*
 * Cooperative yield: callee-saved frame, scheduler pass, resume.
 * Interrupts are masked for the whole pass.  Without this, the timer could
 * run the scheduler while this job's cooperative frame is half built (R2 gap).
 * resume_ctx sets IF again before returning to the job.
 */
.globl _yield_asm
_yield_asm:
	cli
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

/* ---- R10: x87 state (80387 instructions; see kernel/fpu.c) ---------------
 * All four use only the 80387 instruction set and no SSE.  The 108-byte
 * FNSAVE image is saved by fpu_save and restored by fpu_restore. */

/* void fpu_fninit(void): the clean control word, empty stack. */
.globl _fpu_fninit
_fpu_fninit:
	fninit
	ret

/* void fpu_save(u32 *area): FNSAVE, which also reinitialises the FPU. */
.globl _fpu_save
_fpu_save:
	movl 4(%esp),%eax
	fnsave (%eax)
	ret

/* void fpu_restore(u32 *area): FRSTOR. */
.globl _fpu_restore
_fpu_restore:
	movl 4(%esp),%eax
	frstor (%eax)
	ret

/* void fpu_probe(u32 *out): out[0] low word <- FNSTCW, out[1] low word <-
 * FNSTSW, out[2] <- FILD m32 of 12345 then FISTP m32 (out[3] holds the
 * result).  The caller preloads sentinels so a no-op FPU is visible. */
.globl _fpu_probe
_fpu_probe:
	movl 4(%esp),%eax
	fnstcw (%eax)
	fnstsw 4(%eax)
	movl $12345,8(%eax)
	fildl 8(%eax)
	fistpl 12(%eax)
	ret

/* ---- R12A additions ------------------------------------------------------------------------------------------- */

.globl _pg_load_cr3
_pg_load_cr3:
	movl 4(%esp),%eax
	movl %eax,%cr3
	ret

.globl _pg_read_cr3
_pg_read_cr3:
	movl %cr3,%eax
	ret

.globl _pg_read_cr2
_pg_read_cr2:
	movl %cr2,%eax
	ret

.globl _pg_enable
_pg_enable:
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0
	jmp kxpe1
kxpe1:
	ret

/* void kx_ltr(unsigned int selector) */
.globl _kx_ltr
_kx_ltr:
	movl 4(%esp),%eax
	ltr %ax
	ret

/*
 * void kx_call_user(entry, user_top, unsigned int *kx_entry_slot)
 * Called on the job's kernel stack (proc_entry).  Records the kernel entry point just below the current frames (in the
 * job record and in kx_entry), switches to the private user stack and calls the image's _start.  When _start returns
 * (crt0 returns to the kernel), ESP comes back from EBP to the kernel stack and job_run continues to job_exit there.
 */
.globl _kx_call_user
_kx_call_user:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl 16(%ebp),%edx
	leal -32(%esp),%ecx
	andl $-16,%ecx
	cli
	movl %ecx,(%edx)
	movl %ecx,_kx_entry
	movl 12(%ebp),%esp
	sti
	call *8(%ebp)
	cli
	movl %ebp,%esp
	subl $12,%esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	sti
	ret

/* Exceptions 0..19 (vector 8 is replaced by a task gate in descriptors.c).  Frame: pushal, vector, error code (0 when
 * the CPU pushes none), eip, cs, eflags; then the kernel-stack switch as for the other entries. */
kx_exc_common:
	pushal
	movl %esp,%eax
	cmpl _kx_lo,%eax
	jb exs
	cmpl _kx_hi,%eax
	jb exk
exs:	movl _kx_entry,%esp
exk:	pushl %eax
	call _pg_exception
	addl $4,%esp
	jmp resume_dispatch

kx_exc0:	pushl $0
	pushl $0
	jmp kx_exc_common
kx_exc1:	pushl $0
	pushl $1
	jmp kx_exc_common
kx_exc2:	pushl $0
	pushl $2
	jmp kx_exc_common
kx_exc3:	pushl $0
	pushl $3
	jmp kx_exc_common
kx_exc4:	pushl $0
	pushl $4
	jmp kx_exc_common
kx_exc5:	pushl $0
	pushl $5
	jmp kx_exc_common
kx_exc6:	pushl $0
	pushl $6
	jmp kx_exc_common
kx_exc7:	pushl $0
	pushl $7
	jmp kx_exc_common
kx_exc8:	pushl $8
	jmp kx_exc_common
kx_exc9:	pushl $0
	pushl $9
	jmp kx_exc_common
kx_exc10:	pushl $10
	jmp kx_exc_common
kx_exc11:	pushl $11
	jmp kx_exc_common
kx_exc12:	pushl $12
	jmp kx_exc_common
kx_exc13:	pushl $13
	jmp kx_exc_common
kx_exc14:	pushl $14
	jmp kx_exc_common
kx_exc15:	pushl $0
	pushl $15
	jmp kx_exc_common
kx_exc16:	pushl $0
	pushl $16
	jmp kx_exc_common
kx_exc17:	pushl $17
	jmp kx_exc_common
kx_exc18:	pushl $0
	pushl $18
	jmp kx_exc_common
kx_exc19:	pushl $0
	pushl $19
	jmp kx_exc_common

/*
 * #DF task (kx_tss_df: own 4 KiB stack, template directory, IF off).  The CPU pushed an error code.  After
 * proc_double_fault returns, iret goes back to the interrupted task (the main TSS, possibly rewritten to resume in
 * kx_df_kill).  The next #DF resumes this task after the iret, hence the jump back to the entry.
 */
.globl _kx_df_entry
_kx_df_entry:
	addl $4,%esp
	call _proc_double_fault
	iret
	jmp _kx_df_entry

/* Resumed after a user-stack overflow, on the job's kernel stack: build a frame and end the job.  Every task switch
 * sets CR0.TS; clear it, or the scheduler's FNSAVE would raise #NM. */
.globl _kx_df_kill
_kx_df_kill:
	clts
	pushal
	pushl %esp
	call _proc_df_kill
	addl $4,%esp
	jmp resume_dispatch

.data
.globl _pg_exc_table
_pg_exc_table:
	.long kx_exc0, kx_exc1, kx_exc2, kx_exc3, kx_exc4, kx_exc5, kx_exc6, kx_exc7
	.long kx_exc8, kx_exc9, kx_exc10, kx_exc11, kx_exc12, kx_exc13, kx_exc14, kx_exc15
	.long kx_exc16, kx_exc17, kx_exc18, kx_exc19
