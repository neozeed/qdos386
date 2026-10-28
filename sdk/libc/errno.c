/* errno.c - the C89 error indicator (R11C).  One int per program; the
 * functions that set it are listed in docs/R11C-LIBM-MAP.md. */
#include <errno.h>
#include <string.h>

int errno = 0;

/* R11L: BSD error tables for the ported tools (sys_errlist[errno], valid below sys_nerr).  Same texts as strerror. */
int sys_nerr = 39;
char *sys_errlist[39] = {
	/*  0 */ "No error",
	/*  1 */ "Unknown error",
	/*  2 */ "No such file or directory",
	/*  3 */ "Unknown error",
	/*  4 */ "Unknown error",
	/*  5 */ "Input/output error",
	/*  6 */ "Unknown error",
	/*  7 */ "Unknown error",
	/*  8 */ "Unknown error",
	/*  9 */ "Bad file descriptor",
	/* 10 */ "Unknown error",
	/* 11 */ "Unknown error",
	/* 12 */ "Not enough memory",
	/* 13 */ "Permission denied",
	/* 14 */ "Unknown error",
	/* 15 */ "Unknown error",
	/* 16 */ "Resource busy",
	/* 17 */ "File exists",
	/* 18 */ "Unknown error",
	/* 19 */ "Unknown error",
	/* 20 */ "Unknown error",
	/* 21 */ "Unknown error",
	/* 22 */ "Invalid argument",
	/* 23 */ "Unknown error",
	/* 24 */ "Too many open files",
	/* 25 */ "Unknown error",
	/* 26 */ "Unknown error",
	/* 27 */ "Unknown error",
	/* 28 */ "No space left on device",
	/* 29 */ "Illegal seek",
	/* 30 */ "Read-only file system",
	/* 31 */ "Unknown error",
	/* 32 */ "Unknown error",
	/* 33 */ "Domain error",
	/* 34 */ "Result out of range",
	/* 35 */ "Unknown error",
	/* 36 */ "File name too long",
	/* 37 */ "Unknown error",
	/* 38 */ "Function not implemented",
};

/* The error messages for the codes this library sets.  No other code is
 * defined here, so every other value maps to the generic text. */
char *strerror(int errnum)
{
	switch (errnum) {
	case 0:
		return "No error";
	case EDOM:
		return "Domain error";
	case ERANGE:
		return "Result out of range";
	case ENOENT:
		return "No such file or directory";
	case EIO:
		return "Input/output error";
	case EBADF:
		return "Bad file descriptor";
	case ENOMEM:
		return "Not enough memory";
	case EACCES:
		return "Permission denied";
	case EBUSY:
		return "Resource busy";
	case EEXIST:
		return "File exists";
	case EINVAL:
		return "Invalid argument";
	case EMFILE:
		return "Too many open files";
	case ENOSPC:
		return "No space left on device";
	case ESPIPE:
		return "Illegal seek";
	case EROFS:
		return "Read-only file system";
	case ENAMETOOLONG:
		return "File name too long";
	case ENOSYS:
		return "Function not implemented";
	default:
		return "Unknown error";
	}
}

/* R11L: kernel error code -> errno.  Codes: CHAN_ERR_* (-6..-19), FAT_ERR_* (-40..-47), the R11L
 * write errors (-80..-86), MEM_ERR_NOMEM (-3).  Anything else is EIO. */
int qd_set_errno(int e)
{
	int v;
	switch (e) {
	case -1:  v = ENOSYS; break;          /* unknown trap: kernel without the write option */
	case -3:  v = ENOMEM; break;
	case -6:  v = EMFILE; break;
	case -7:  v = ENOENT; break;
	case -12: v = EBADF; break;
	case -13: v = EBADF; break;
	case -14: v = EINVAL; break;
	case -15: v = ENOSYS; break;
	case -18: v = ENOENT; break;
	case -44: v = ENOENT; break;
	case -46: v = ENAMETOOLONG; break;
	case -47: v = EINVAL; break;
	case -80: v = EROFS; break;
	case -81: v = ENOSPC; break;
	case -82: v = ENOSPC; break;
	case -83: v = EEXIST; break;
	case -84: v = EBUSY; break;
	case -87: v = ENOTDIR; break;
	case -88: v = EISDIR; break;
	case -89: v = ENOTEMPTY; break;
	case -90: v = ENAMETOOLONG; break;
	case -91: v = EINVAL; break;
	case -92: v = EXDEV; break;
	default:  v = EIO; break;
	}
	errno = v;
	return v;
}
