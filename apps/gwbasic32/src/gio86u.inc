# GIO86U -> gio86u.inc. Device-independent I/O constants. (Macros dropped; ROFFST is
# expanded as: NAME = running offset, then the offset advances by SIZE.)
	.set ASCCTC, 3		# CTL-C
	.set ASCBS, 8
	.set ASCTAB, 9
	.set ASCLF, 10
	.set ASCCR, 13
	.set ASCCTS, 19		# CTL-S
	.set ASCCTZ, 26		# Control Z (end-of-file)
	.set ASCESC, 27
	.set ASCSPC, 32
	.set BINFID, 255	# files created by SAVE "fn"
	.set PROFID, 254	# files created by SAVE "fn",P
	.set BSVFID, 253	# files created by BSAVE "fn"
	.set PSEUDO_SAV, 0xFFFFFFFE	# pseudo-FDB of the ASCII SAVE output file (bifile.S: SAV_* routines)
	# Generalized I/O function offsets (each entry is 2 bytes)
	.set G_EOF, 0
	.set G_LOC, 2
	.set G_LOF, 4
	.set G_CLS, 6
	.set G_SWD, 8
	.set G_RND, 10
	.set G_OPN, 12
	.set G_SIN, 14
	.set G_SOT, 16
	.set G_GPS, 18
	.set G_GWD, 20
	.set G_SCW, 22
	.set G_GCW, 24
	.set G_BIN, 26
	.set G_BOT, 28
