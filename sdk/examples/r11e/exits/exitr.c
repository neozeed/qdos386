/* EXITR: returns 6 from main with output still buffered; the stdio flush and the
 * atexit handler must run on the return path too. */
#include <stdio.h>
#include <stdlib.h>

static void handler(void)
{
	printf("EXITR atexit ran\n");
}

int main(void)
{
	atexit(handler);
	printf("EXITR returns 6");
	return 6;
}
