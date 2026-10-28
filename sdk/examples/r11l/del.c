/* DEL.AOU - R11L: delete files from the writable volume.   DEL NAME [NAME ...]
 * Exit status: the number of names that could not be deleted (0 = all deleted). */
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int i, failed = 0;

	if (argc < 2) {
		printf("usage: DEL NAME [NAME ...]\n");
		return 100;
	}
	for (i = 1; i < argc; i++) {
		if (remove(argv[i]) != 0) {
			printf("DEL: %s: %s\n", argv[i], strerror(errno));
			failed++;
		} else {
			printf("DEL: %s deleted\n", argv[i]);
		}
	}
	return failed;
}
