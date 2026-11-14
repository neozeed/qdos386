# gwconst.inc: constants used by more than one source file. Plain .set values, not .globl, so each file
# that includes this gets its own copy and no symbol is exported. Error numbers are the GW-BASIC message
# numbers (gwdata_errors.inc holds the messages); the token numbers are the embedded-constant tokens
# (gwtok.inc holds the rest, including DBLCON).
	.set PTRCON, 13			# line reference constant (4-byte pointer)
	.set LINCON, 14			# line number constant (2 bytes)
	.set IN2CON, 15			# single byte integer constant (two bytes with the token)
	.set INTCON, 28			# 16-bit integer constant
	.set ERRSN,  2			# Syntax error
	.set ERRFC,  5			# Illegal function call
	.set ERRBS,  9			# Subscript out of range
	.set ERRDD, 10			# Duplicate Definition (redimensioned array)
	.set ERRMMM, 66			# Direct statement in file (original numbering; ERROR 66 prints it). gwdata_errors.inc
						# labels it 65: its labels around 64-67 are one off (the message table itself is right)
	.set ERROM,  7			# Out of memory
	.set ERRFNF, 53			# File not found
	.set ERRBFM, 54			# Bad file mode
	.set ERRIOE, 57			# Device I/O Error
	.set ERRIFN, 64			# Bad file name
	.set ERRTMF, 67			# Too many files (also the root directory full on SAVE)
	.set ERRDFL, 61			# Disk full (original GIODSK DERDFL)
	.set ERRDWP, 70			# Disk write protected (original DSKERR table, "Disk write protected")
# Platform file error codes (platform_qdos.S, platform_win32.S): what PLAT_FOPEN / PLAT_FWRITE leave in EAX
# on CF=1. The write codes are the Win32 ones the platform layer uses, so both platforms share the mapping.
	.set PLAT_ERR_WPROT, 19		# ERROR_WRITE_PROTECT: kernel -80 (write-protected or read-only)
	.set PLAT_ERR_DISKFULL, 112	# ERROR_DISK_FULL: kernel -81, or a write that moved no bytes
	.set PLAT_ERR_DIRFULL, 4	# ERROR_TOO_MANY_OPEN_FILES here: kernel -82 (root directory full)
