#include <stdio.h>
#include <stdlib.h>
static void h(void) { printf("handler\n"); }
int main(void)
{
	printf("start\n");
	if (atexit(h) != 0) printf("atexit failed\n");
	return 3;
}
