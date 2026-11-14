# forent.inc: layout of a FOR entry on the stack (32-bit port). Shared by gwmain.S (FOR, FNDFOR,
# LOOPER, POPGOF) and next86.S (NEXT). Included, so the offsets are assembly-time constants.
#
# Entry, low address first. B = entry base (pad byte). E = B+6 is the pointer FNDFOR returns.
#   B+0    pad byte (1)                    B+1    token $FOR (1)
#   B+2    loop-variable pointer (4)
#   E+0    text pointer of the matching NEXT (4)
#   E+4    sign of the increment (1)       E+5    type (1): minus = integer loop, plus = floating
#   E+6    step, floating (4)              E+10   upper value, floating (4)
#   E+14   line number of the FOR (2)      E+16   text pointer into the FOR statement (4)
#   Integer loops use the upper field for their values: E+6..9 unused, step (2) at E+10, upper (2) at E+12.
# Entry size 26 bytes (B..B+25). The next entry's base is E+20.
#
# Stack widths: pointers 4 bytes, line numbers 2, sign/type/pad 1 each, integer values 2.
# The two words pushed above the entry for NEXTS are 4 bytes each (like return addresses).
# FNDFOR starts at SP+8: FNDFOR's own return address (4) plus the return address of NEWSTT (4).
	.set FE_NXT, 0			# from E: NEXT text pointer (4)
	.set FE_SGN, 4			# from E: sign of increment (1)
	.set FE_TYP, 5			# from E: type byte (minus = integer) (1)
	.set FE_STP, 6			# from E: floating step (4)
	.set FE_UPP, 10			# from E: floating upper value (4)
	.set FE_ISTP, 10		# from E: integer step (2)
	.set FE_IUPP, 12		# from E: integer upper value (2)
	.set FE_LIN, 14			# from E: line number of the FOR (2)
	.set FE_FTX, 16			# from E: text pointer into the FOR statement (4)
	.set FE_NEXT, 20		# from E: distance to the next entry's base
	.set FE_VAR, 2			# from B: loop-variable pointer (4)
	.set FE_EB, 6			# from B to E
	.set FE_SIZE, 26		# bytes per entry, B..B+25
	.set FE_WORDS, 13		# GETSTK count (2-byte words) for a whole entry
	.set WHLSIZ, 10			# bytes of a WHILE entry after its token; with the pad byte and the token, 12 bytes
