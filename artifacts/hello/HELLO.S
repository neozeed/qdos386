	.file	"HELLO.I"
gcc_compiled.:
.text
LC0:
	.ascii "Hello from guest-compiled C, argc=%d\12\0"
	.align 2
.globl _main
_main:
	pushl %ebp
	movl %esp,%ebp
	pushl 8(%ebp)
	pushl $LC0
	call _printf
	movl $7,%eax
	leave
	ret
