/*
 * QDOS/386 R5 - application start (application-owned; not part of the kernel).
 *
 * The loader creates a job whose entry is _start.  The job's stack is the
 * kernel's 4 KiB heap stack, already set up; _start must not touch ESP
 * beyond its own frame, must not clear BSS (the loader does that), and must
 * not halt.  It calls main, passes the result to QT_EXIT (0x18) and returns,
 * so the kernel ends the job (job_run -> job_exit).
 */
.text
.globl _start
_start:
	pushl %ebp
	movl %esp,%ebp
	call _main
	movl %eax,%ebx		/* a = exit status */
	movl $0x18,%eax		/* QT_EXIT */
	int $0x30
	leave
	ret
