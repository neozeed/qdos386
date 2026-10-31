/*
 * ARGV.AOU - R13B: prints exactly the argument vector it was started with, one per line in brackets, then its tag
 * (built with -DTAG=... for the PATH-order tests, default "ARGV"), the current directory and PATH.  Exit status = argc.
 * With ARGV_WAIT set in its environment it then waits for one key (so its own session stays on the screen).
 */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

#ifndef TAG
#define TAG "ARGV"
#endif

int main(int argc, char **argv)
{
	int i;
	char cwd[80];
	const char *p = getenv("PATH");

	printf("%s argc=%d\n", TAG, argc);
	for (i = 0; i < argc; i++)
		printf("%s [%d]=[%s]\n", TAG, i, argv[i]);
	if (qd_getcwd(cwd, sizeof cwd) < 0)
		cwd[0] = 0;
	printf("%s cwd=%s path=%s\n", TAG, cwd, p ? p : "(none)");
	if (getenv("ARGV_WAIT") != 0) {
		printf("%s waiting for a key\n", TAG);
		fflush(stdout);
		(void) getchar();
	}
	return argc;
}
