/* EXITX: exit(5) from a nested call with output still buffered, and an atexit
 * handler that must run.  The shell must print "exit status 5" and continue. */
#include <stdio.h>
#include <stdlib.h>

static void handler(void)
{
	printf("EXITX atexit ran\n");
}

static void nested(void)
{
	printf("EXITX buffered text, no newline");
	exit(5);
}

int main(void)
{
	if (atexit(handler) != 0)
		return 1;
	nested();
	return 0;
}
