/* CHILD1: prints a line and returns 7 from main (exit status 7). */
#include <stdio.h>

int main(int argc, char **argv)
{
	printf("CHILD1 argc %d\n", argc);
	return 7;
}
