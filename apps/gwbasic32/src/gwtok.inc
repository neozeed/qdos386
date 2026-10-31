# Embedded-constant token values, from GWMAIN.ASM lines 1728-1746.
# Shared by GWLIST, GWMAIN and later modules.
	.set OCTCON, 11		# embedded octal constant
	.set HEXCON, 12		# embedded hexadecimal constant
	.set DBLCON, 31		# double precision (8-byte) constant
	.set DBLCN1, DBLCON+1
	.set OPCNT, 4		# DBLDSP/SNGDSP/INTDSP dispatch count (gwdata.S defines the global copy)
	.set ONECON, 17		# first of 10 (0-9) single-byte integer special tokens (GWMAIN.ASM 1738)
	.set PRMSIZ, 100	# bytes for a user function definition block (GWDATA.ASM 1314)
	.set ERRID, 12		# "Illegal direct" error code (copy of gwdata_errors.inc; keep in sync)
	.set ERRADV, 73		# "Advanced feature" error code (copy of gwdata_errors.inc; keep in sync)
