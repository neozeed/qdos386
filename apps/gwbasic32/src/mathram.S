# Floating-point accumulators and math work buffers.
# Copied directive-for-directive from GWDATA.ASM lines 1437-1516 so the byte
# layout (and the overlapping LABEL WORD names) is identical to the original.
# Leading '$' in original names is written as '_' (GAS symbols cannot start with '$').
	.intel_syntax noprefix
	.section .bss
	.globl _DBUFF, _FMTCX, _FMTAX, _FMTAL, _ZLO, _ZX, _Z1LO, _Z1
	.globl _DFACL, DFACLO, _FACLO, FACLO, _FACM1, _FAC, FAC
	.globl _FLGOV, FLGOVC, OVCSTR, FLGSCN, _ARGLO, ARGLO, _ARG, ARG
	.globl _FBUFF, FBUFFR, FMLTT1, FMLTT2, _DPADR

_DBUFF:		.skip 9				# double-precision buffer
_FMTCX:		.skip 2				# format: digits right/left of point
_FMTAX:
_FMTAL:		.skip 3				# format spec byte(s)
_ZLO:		.skip 1
		.skip 6
_ZX:		.skip 1
_Z1LO:		.skip 7
_Z1:		.skip 1
		.skip 1				# [temporary least significant byte]
		.skip 8				# extra storage for D.P. multiply
_DFACL:
DFACLO:		.skip 4				# four lowest orders for double precision
_FACLO:
FACLO:		.skip 2				# 2-byte integer / low word. A STRING POINTER IS STORED HERE AS 4 BYTES
								# (widened): it overlaps _FACM1 and _FAC, which are dead while VALTYP is string.
								# NUMREL pushes only numeric FAC bytes; strings are pushed as 4-byte pointers.
_FACM1:		.skip 1				# [middle order of mantissa]
_FAC:
FAC:		.skip 2				# [high order of mantissa] [exponent/sign]
_FLGOV:
FLGOVC:		.skip 1				# overflow print flag
OVCSTR:		.skip 1				# place to store overflow flag after FIN
FLGSCN:		.skip 1				# flags input code executing for SCNVAL
		.skip 1				# [temporary least significant byte]
_ARGLO:
ARGLO:		.skip 7				# second argument for double precision
_ARG:
ARG:		.skip 1
_FBUFF:
FBUFFR:		.skip 13			# buffer for FOUT
		.skip 27			# (up to offset 40)
FMLTT1:		.skip 1				# FBUFFR+40: temp for FMULT
FMLTT2:		.skip 1				# FBUFFR+41: temp for FMULT
		.skip 2				# FBUFFR+42..42: remaining buffer bytes (total 43)

# Stored pointer, widened from 16-bit (DW) to 32-bit. Original: $DPADR in GWDATA.ASM line 1259.
	.section .bss
	.globl _DPADR
_DPADR:		.skip 4				# address of decimal point in _FBUFF
