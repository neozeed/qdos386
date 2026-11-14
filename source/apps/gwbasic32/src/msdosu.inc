# MSDOSU -> msdosu.inc. MS-DOS constants and file-data-block (FDB) offsets.
# The FDB layout is used by the device layer (Stage 4) and by the interpreter where it
# reads file state. Offsets are computed the same way as ROFFST in the original.
	.set C_PRTO, 5		# printer output
	.set C_DCIO, 6		# direct console I/O
	.set C_REST, 13		# initialize BDOS
	.set C_SDRV, 14
	.set C_OPEN, 15
	.set C_CLOS, 16
	.set C_SEAR, 17
	.set C_DELE, 19
	.set C_READ, 20
	.set C_MAKE, 22
	.set C_RENA, 23
	.set C_GDRV, 25
	.set C_BUFF, 26
	.set C_RNDR, 33
	.set C_RNDW, 34
	.set C_RBR, 39
	.set C_PARS, 41
	.set FNAML, 11
	.set DATPSC, 128
	.set FCBSIZ, 38
	# FDB record (device-independent part)
	.set F_NUL1, 0
	.set F_NUM, 1
	.set F_NUL2, 2
	.set F_NEXT, 4		# thread for linked list, [MEMSIZ]=end
	.set F_MODE, 6
	.set MD_SQI, 1
	.set MD_SQO, 2
	.set MD_RND, 4
	.set MD_APP, 8
	.set F_FCB, 7		# File Control Block (38 bytes)
	.set F_CLOC, 45
	.set F_ORCT, 47
	.set F_BREM, 48
	.set F_CODE, 49
	.set FC_ASC, 0
	.set FC_BIN, 1
	.set F_BAKC, 50
	.set F_NUL3, 51
	.set F_DEV, 52
	.set F_WID, 53
	.set F_NUL4, 54
	.set F_FLGS, 55
	.set FL_BKC, 128	# 200 octal
	.set F_POS, 56
	.set FDBSIZ, 57
	.set DATOFS, 57		# OS interface buffer (DATPSC bytes)
	.set FD_SIZ, 185
	.set FD_PHY, 187
	.set FD_LOG, 189
	.set F_NUL5, 191
	.set FD_OPS, 192
	.set FD_DAT, 194
	# Offsets into the MS-DOS FCB
	.set FCB_DV, 7		# F_FCB+0
	.set FCB_FN, 8		# F_FCB+1
	.set FCB_FT, 16		# F_FCB+9
	.set FCB_EX, 19		# F_FCB+12
	.set FCB_RC, 21		# F_FCB+14
	.set FCB_FS, 23		# F_FCB+16
	.set FCB_DT, 29		# last definition wins: F_FCB+22 (the F_FCB+20 date line is overwritten)
	.set FCB_NR, 39		# F_FCB+32
	.set FCB_RN, 40		# F_FCB+33
