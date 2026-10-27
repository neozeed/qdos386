/*
 * QDOS/386 R8 SDK - crt0 (copy of apps/common/app_start.s; see docs/R8-APP-ABI.md).
 *
 * The loader creates a job whose entry is _start.  The job's stack is the
 * kernel's heap stack, already set up; _start must not touch ESP beyond its
 * own frame, must not clear BSS (the loader does that), and must not halt.
 *
 * R8 startup ABI (docs/R8-APP-ABI.md):
 *   1. QT_APP_ARGS (0x1F) returns argc in EAX and stores the argv array
 *      address in the word named by a.  A job with no argument block has
 *      argc 0 and argv 0.
 *   2. main(argc, argv) is called with the C stack convention of GCC 1.40:
 *      argc at 8(%ebp) of main, argv at 12(%ebp).
 *   3. The return value of main is the exit status, passed to QT_EXIT (0x18),
 *      which the kernel reports through the job exit record.
 *
 * This variant is linked when libc.a is present (see bin/qdos386-gcc).
 * It calls the libc flush so that buffered stdout reaches the console.
 *
 * Legacy R5/R7 applications link the same way and simply ignore the
 * arguments; main(void) is still a valid definition.
 */
.text
.globl _start
_start:
	pushl %ebp
	movl %esp,%ebp
	subl $8,%esp		/* -4(%ebp): argv array address (written by the kernel) */
	leal -4(%ebp),%ebx	/* a = where the kernel writes argv */
	movl $0x1F,%eax		/* QT_APP_ARGS */
	int $0x30		/* eax = argc */
	pushl -4(%ebp)		/* argv */
	pushl %eax		/* argc (first argument, lowest address) */
	call _main
	addl $8,%esp
	pushl %eax		/* status: runs the atexit functions, flushes stdio, records the status */
	call _qd_main_exit
	addl $4,%esp
	leave
	ret			/* the job ends with its entry point */
