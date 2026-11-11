/* envx.c - QDOS/386 libc (R12B): setenv and unsetenv, extensions over the process environment (QT_ENV_SET).
 * A separate member, so programs that do not call them link the bytes they linked before. */
#include <stdlib.h>
#include "qdos386.h"

int setenv(const char *name, const char *value, int overwrite)
{
	char probe[2];

	if (name == NULL || value == NULL)
		return -1;
	if (!overwrite && (int) qtrap(0x35u, (unsigned int) name, (unsigned int) probe, sizeof probe) >= 0)
		return 0;                       /* present and not to be replaced */
	return (int) qtrap(0x36u, (unsigned int) name, (unsigned int) value, 0) == 0 ? 0 : -1;
}

int unsetenv(const char *name)
{
	int rc;

	if (name == NULL)
		return -1;
	rc = (int) qtrap(0x36u, (unsigned int) name, 0, 0);
	return (rc == 0 || rc == -7) ? 0 : -1;  /* removing an absent name is not an error */
}
