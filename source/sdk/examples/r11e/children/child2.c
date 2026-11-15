/* CHILD2: exits explicitly with 9 from a nested call; output is not flushed by
 * the program, so the exit path must flush it. */
#include <stdio.h>
#include <stdlib.h>

static void deep(void)
{
	printf("CHILD2 unflushed");
	exit(9);
}

int main(void)
{
	deep();
	return 0;
}
