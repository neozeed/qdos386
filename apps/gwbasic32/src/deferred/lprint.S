# GIO86.ASM LPRINT entry, translated and kept out of the build (deferred: printer output).
# NOT COMPILED: the Makefile builds only src/*.S. gio86prt.S carries the stub
# "LPRINT: jmp ADVERR"; to enable LPRINT, remove that stub and build this file instead.
# PRINT1 is the shared PRINT entry in gio86prt.S (exported there).
#
# PTRFIL is a 4-byte pointer in the port (gwdata.S), so the original WORD store is a DWORD store.

	.intel_syntax noprefix
	.section .text
	.globl LPRINT

LPRINT:	mov DWORD PTR PTRFIL, -1	# future output will go to Line Printer
	jmp PRINT1			# same as PRINT after setting the output file
