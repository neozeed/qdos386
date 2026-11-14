# qdos_main.S: process entry for the QDOS/386 build (GNU as 1.38, AT&T syntax). It replaces src/main.S.
# crt0's _start calls _main(argc, argv); the arguments are not used. START (gwdata.S) runs INIT and
# READY; the interpreter ends only through PLAT_EXIT, which calls qd_exit, and that unwinds to
# crt0. So _main does not return.
	.text
	.globl _main
_main:
	call PLAT_INIT
	jmp START
