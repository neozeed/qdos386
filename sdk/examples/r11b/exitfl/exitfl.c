/* EXITFL: text written to stdout with no newline, then return from main.
 * The start code flushes the stdout buffer, so the text must appear. */
#include <stdio.h>

int main(void)
{
	printf("EXITFL: buffered text, no newline");
	return 0;
}
