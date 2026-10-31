/*
 * QDOS/386 R11J - paging proof of concept, assembly part (QDOS_PAGING images only).
 *
 *   pg_load_cr3(dir), pg_read_cr3(), pg_read_cr2(), pg_enable()
 *   pg_exc_table: 20 exception stubs for the IDT (vectors 0..19, interrupt gates).
 *
 * Each stub leaves the same frame: pushal, vector, error code (0 when the CPU pushes
 * none), then the CPU's eip/cs/eflags.  pg_exception (paging.c) returns saved_sp | fmt
 * of the job to resume, exactly as the scheduler entry points do; the resume code below
 * is a copy of resume_dispatch in context.s (kept separate so context.s is unchanged).
 */
.text

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

/* CR0.PG on.  The code, stack, IDT and tables are identity-mapped, so execution
 * continues at the next instruction; the jump discards prefetched state. */
.globl _pg_enable
_pg_enable:
	movl %cr0,%eax
	orl $0x80000000,%eax
	movl %eax,%cr0
	jmp 1f
1:
	ret

pg_exc_common:
	pushal
	pushl %esp
	call _pg_exception
	addl $4,%esp
	movl %eax,%ecx
	andl $1,%ecx
	andl $-4,%eax
	movl %eax,%esp
	cmpl $0,%ecx
	je 2f
	popal
	iret
2:
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	sti
	ret

/* No error code from the CPU: push 0, then the vector. */
pg_exc0:	pushl $0
	pushl $0
	jmp pg_exc_common
pg_exc1:	pushl $0
	pushl $1
	jmp pg_exc_common
pg_exc2:	pushl $0
	pushl $2
	jmp pg_exc_common
pg_exc3:	pushl $0
	pushl $3
	jmp pg_exc_common
pg_exc4:	pushl $0
	pushl $4
	jmp pg_exc_common
pg_exc5:	pushl $0
	pushl $5
	jmp pg_exc_common
pg_exc6:	pushl $0
	pushl $6
	jmp pg_exc_common
pg_exc7:	pushl $0
	pushl $7
	jmp pg_exc_common
/* 8: #DF pushes an error code. */
pg_exc8:	pushl $8
	jmp pg_exc_common
pg_exc9:	pushl $0
	pushl $9
	jmp pg_exc_common
pg_exc10:	pushl $10
	jmp pg_exc_common
pg_exc11:	pushl $11
	jmp pg_exc_common
pg_exc12:	pushl $12
	jmp pg_exc_common
pg_exc13:	pushl $13
	jmp pg_exc_common
pg_exc14:	pushl $14
	jmp pg_exc_common
pg_exc15:	pushl $0
	pushl $15
	jmp pg_exc_common
pg_exc16:	pushl $0
	pushl $16
	jmp pg_exc_common
pg_exc17:	pushl $17
	jmp pg_exc_common
pg_exc18:	pushl $0
	pushl $18
	jmp pg_exc_common
pg_exc19:	pushl $0
	pushl $19
	jmp pg_exc_common
/* Vectors 20..31 are reserved and never raised by these CPUs: no stubs, gates not present. */

.data
.globl _pg_exc_table
_pg_exc_table:
	.long pg_exc0, pg_exc1, pg_exc2, pg_exc3, pg_exc4, pg_exc5, pg_exc6, pg_exc7
	.long pg_exc8, pg_exc9, pg_exc10, pg_exc11, pg_exc12, pg_exc13, pg_exc14, pg_exc15
	.long pg_exc16, pg_exc17, pg_exc18, pg_exc19
