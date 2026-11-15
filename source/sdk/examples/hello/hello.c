/*
 * SDK example: hello.  Writes one line to the console and exits with 0.
 * Build:  qdos386-gcc -o SDKHEL.aout hello.c
 */
#include "qdos386.h"

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	if (qd_puts("Hello from the QDOS/386 SDK (qdos386-gcc, slot 1).\n") < 0)
		return 1;
	return 0;
}
