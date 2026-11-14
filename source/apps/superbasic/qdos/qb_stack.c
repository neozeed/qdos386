/*
 * qb_stack.c - a private stack for the interpreter's C recursion.
 *
 * Each nested DEFine call and each nested expression costs C stack in the core (about 1 KB per call level and 0.6 KB per level of
 * parentheses, GCC 1.40 -O).  The job stack the kernel gives a job is 64 KiB, so the interpreter runs on a bigger one.
 *
 * WHERE THE STACK LIVES.  It must be a block of the shared kernel heap (qd_alloc), like the job stacks.  The first version used a
 * BSS array of the program's own window and the guest died with a triple fault the first time two jobs ran: the scheduler loads
 * the next job's page directory while it is still running on the interrupted job's stack, and a paged job's window is private
 * (not mapped in the other directories).  Hence the heap: it is mapped identically everywhere, and pg_user_range_ok() accepts it.
 * The cost is that the stack comes out of the one ~0.9 MB kernel heap, so the default is 256 KiB.
 *
 * The block is painted with a known word before use; the high-water mark is the lowest word that no longer has the pattern.  The
 * lowest QB_GUARD words are a guard: if they change the stack overflowed (reported by the drivers).  qb_depth_limit is the core's
 * SB_MAX_CALL_DEPTH, derived from the size obtained: (size - 100 KiB) / 1 KiB, the 100 KiB being headroom for a typed line of
 * 255 characters nested in parentheses (about 76 KiB), libc and the kernel's trap and interrupt frames.
 *
 * qb_call_on_stack is a few lines of x86 (GCC 1.40 passes top-level asm through): it keeps the old ESP in EBP, which every C
 * function preserves.
 */
#include "qdos386.h"
#include "qb.h"

#define PAINT       0xA5A5A5A5u
#define QB_GUARD    16u                  /* words */

int qb_depth_limit = 25;

static unsigned int *qb_stk;
static unsigned int qb_words;

asm(".text\n"
    ".globl _qb_call_on_stack\n"
    "_qb_call_on_stack:\n"
    "\tpushl %ebp\n"
    "\tmovl %esp,%ebp\n"
    "\tmovl 8(%ebp),%ecx\n"             /* fn   */
    "\tmovl 12(%ebp),%edx\n"            /* arg  */
    "\tmovl 16(%ebp),%eax\n"            /* top  */
    "\tmovl %eax,%esp\n"
    "\tpushl %edx\n"
    "\tcall *%ecx\n"
    "\tmovl %ebp,%esp\n"
    "\tpopl %ebp\n"
    "\tret\n");

int qb_call_on_stack(int (*fn)(void *), void *arg, unsigned int top);

int qb_stack_call(unsigned int kib, int (*fn)(void *), void *arg)
{
	unsigned int i, bytes, top;
	int r;

	if (kib > QB_STACK_MAX_KIB)
		kib = QB_STACK_MAX_KIB;
	for (;;) {
		if (kib < QB_STACK_MIN_KIB)
			return -1;
		qb_stk = (unsigned int *) qd_alloc(kib * 1024u);
		if (qb_stk != 0)
			break;
		kib /= 2;
	}
	bytes = kib * 1024u;
	qb_words = bytes / 4u;
	for (i = 0; i < qb_words; i++)
		qb_stk[i] = PAINT;
	qb_depth_limit = (int) ((bytes - 100u * 1024u) / 1024u);
	if (qb_depth_limit < 4)
		qb_depth_limit = 4;
	top = ((unsigned int) qb_stk + bytes - 64u) & ~15u;
	r = qb_call_on_stack(fn, arg, top);
	return r;
}

unsigned int qb_stack_used(void)
{
	unsigned int i = QB_GUARD;

	if (qb_stk == 0)
		return 0;
	while (i < qb_words && qb_stk[i] == PAINT)
		i++;
	return (qb_words - i) * 4u;
}

unsigned int qb_stack_size(void)
{
	return qb_stk == 0 ? 0 : (qb_words - QB_GUARD) * 4u;
}

void qb_stack_free(void)
{
	if (qb_stk != 0)
		qd_free(qb_stk);
	qb_stk = 0;
}

int qb_stack_intact(void)
{
	unsigned int i;

	for (i = 0; i < QB_GUARD && qb_stk != 0; i++)
		if (qb_stk[i] != PAINT)
			return 0;
	return 1;
}
