/* errno.c - the C89 error indicator (R11C).  One int per program; the
 * functions that set it are listed in docs/R11C-LIBM-MAP.md. */
#include <errno.h>
#include <string.h>

int errno = 0;

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
	default:
		return "Unknown error";
	}
}
