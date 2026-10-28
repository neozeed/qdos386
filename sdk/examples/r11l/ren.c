/* REN.AOU - R11L: rename a file on the writable volume.   REN OLD NEW
 * Exit status: 0 done, 1 failed (the reason is printed). */
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("usage: REN OLD NEW\n");
		return 1;
	}
	if (rename(argv[1], argv[2]) != 0) {
		printf("REN: %s -> %s: %s\n", argv[1], argv[2], strerror(errno));
		return 1;
	}
	printf("REN: %s -> %s\n", argv[1], argv[2]);
	return 0;
}
