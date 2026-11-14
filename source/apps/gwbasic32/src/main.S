# main.S: process entry for the Win32 build. The interpreter's own startup is START (gwdata.S),
# which runs INIT (gwinit.S), then INITSA (itsa.S), which falls into READY. Terminal I/O uses the
# platform layer (platform_win32.S). The process ends only through SYSTME, which calls PLAT_EXIT.
	.intel_syntax noprefix
	.section .text
	.globl _mainCRTStartup
_mainCRTStartup:
	call PLAT_INIT
	jmp START
