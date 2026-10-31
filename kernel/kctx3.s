/*
 * QDOS/386 R16 - architecture assembly for the opt-in protected/SMP kernel (make qdos386-smp-pc, -DQDOS_R16).
 * Derived mechanically from kernel/kctx.s (unchanged, the ordinary kernel's file).  Three groups of changes:
 *   (a) CPL3: load kernel DS/ES after pushal; restore flat user segments (0x33) before returning to a CPL3 frame
 *       (frame CS has RPL 3); kx_user_enter starts a protected job with an iret to CS 0x2B / SS 0x33.
 *   (b) per-CPU: the stub-read words kx_lo/kx_hi/kx_entry/kx_resume_fmt become a per-CPU block kx_pc[2] selected by
 *       the task register (str): TR 0x18 (and 0x20, the #DF task) -> CPU 0, TR 0x38 -> CPU 1.  EBP holds the byte
 *       offset (0 or 16) from pushal until resume_dispatch; it is callee-saved, so the C handler preserves it.
 *       kx_pc layout: +0 lo, +4 hi, +8 entry, +12 resume_fmt.
 *   (c) the exact-ESP resume of KCTX-FIX2 is unchanged (resume_dispatch loads the full saved ESP, branches on fmt).
 * docs/R16-AUDIT.md, docs/R16-ARCHITECTURE.md.
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
 * Common resume path.  EAX = the saved stack pointer, complete (any value mod 4); _kx_resume_fmt = its frame format
 * (0 cooperative, 1 interrupt), set by the C function that returned EAX.
 * Interrupts are off on entry from an interrupt stub.  A cooperative frame
 * is resumed with IF set, since that job expects it.
 */
.globl resume_dispatch
resume_dispatch:
	movl %eax,%esp
	pushl %ebp
	call _bkl_leave
	popl %ebp
	cmpl $0,_kx_pc+12(%ebp)
	je resume_ctx
	testl $3,36(%esp)
	jz rd_k
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
rd_k:
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
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb i1s
	cmpl _kx_pc+4(%ebp),%eax
	jb i1k
i1s:	movl _kx_pc+8(%ebp),%esp
i1k:	pushl %eax
	call _bkl_enter
	call _input_irq1
	call _bkl_leave
	popl %esp
	testl $3,36(%esp)
	jz sr0
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
sr0:
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
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb i7s
	cmpl _kx_pc+4(%ebp),%eax
	jb i7k
i7s:	movl _kx_pc+8(%ebp),%esp
i7k:	pushl %eax
	call _bkl_enter
	call _irq7_dispatch
	call _bkl_leave
	popl %esp
	testl $3,36(%esp)
	jz sr1
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
sr1:
	popal
	iret

.globl _irq15_stub
_irq15_stub:
	pushal
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb ifs
	cmpl _kx_pc+4(%ebp),%eax
	jb ifk
ifs:	movl _kx_pc+8(%ebp),%esp
ifk:	pushl %eax
	call _bkl_enter
	call _irq15_dispatch
	call _bkl_leave
	popl %esp
	testl $3,36(%esp)
	jz sr2
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
sr2:
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
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb i0s
	cmpl _kx_pc+4(%ebp),%eax
	jb i0k
i0s:	movl _kx_pc+8(%ebp),%esp
i0k:	pushl %eax
	call _bkl_enter
	call _sched_irq_tick
	addl $4,%esp
	jmp resume_dispatch

/* Service trap (vector 0x30, interrupt gate). */
.globl _trap_stub
_trap_stub:
	pushal
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb trs
	cmpl _kx_pc+4(%ebp),%eax
	jb trk
trs:	movl _kx_pc+8(%ebp),%esp
trk:	pushl %eax
	call _bkl_enter
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
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 8f
	movl $16,%ebp
8:
	pushl %esp
	call _sched_yield_sp
	addl $4,%esp
	jmp resume_dispatch

/*
 * void kx_user_enter(unsigned int entry, unsigned int user_esp)  - protected jobs only (R16).
 * One-way iret to CPL3: flat user data segments, then iret with user CS 0x2B / SS 0x33, EFLAGS 0x202 (IF on,
 * IOPL 0).  TSS.ESP0 for the next kernel entry was set when this job was dispatched (schedule -> kx_set_esp0).
 */
.globl _kx_user_enter
_kx_user_enter:
	movl 4(%esp),%eax
	movl 8(%esp),%ecx
	cli
	movw $0x33,%dx
	movw %dx,%ds
	movw %dx,%es
	movw %dx,%fs
	movw %dx,%gs
	pushl $0x33
	pushl %ecx
	pushl $0x202
	pushl $0x2B
	pushl %eax
	iret

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
	str %bx
	xorl %eax,%eax
	cmpw $0x38,%bx
	jne 7f
	movl $16,%eax
7:
	movl %ecx,_kx_pc+8(%eax)
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
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
	movl %esp,%eax
	cmpl _kx_pc(%ebp),%eax
	jb exs
	cmpl _kx_pc+4(%ebp),%eax
	jb exk
exs:	movl _kx_pc+8(%ebp),%esp
exk:	pushl %eax
	call _bkl_enter
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
	movw $0x10,%dx
	movw %dx,%ds
	movw %dx,%es
	str %cx
	xorl %ebp,%ebp
	cmpw $0x38,%cx
	jne 9f
	movl $16,%ebp
9:
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

/* int kx_this_cpu(void): 0 if TR is the CPU0 main/#DF TSS, 1 if the CPU1 TSS (0x38). */
.globl _kx_this_cpu
_kx_this_cpu:
	str %ax
	andl $0xffff,%eax
	cmpl $0x38,%eax
	je 1f
	xorl %eax,%eax
	ret
1:	movl $1,%eax
	ret
