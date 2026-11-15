/* CMP.AOU - R11L: compare two files byte for byte.   CMP A B
 * Exit status: 0 identical, 1 different (the first differing offset is printed), 2 a file is missing. */
#include <stdio.h>

int main(int argc, char **argv)
{
	FILE *a, *b;
	long off = 0;
	int ca, cb;

	if (argc != 3) {
		printf("usage: CMP A B\n");
		return 2;
	}
	a = fopen(argv[1], "r");
	b = fopen(argv[2], "r");
	if (a == NULL || b == NULL) {
		printf("CMP: cannot open %s\n", a == NULL ? argv[1] : argv[2]);
		return 2;
	}
	for (;;) {
		ca = fgetc(a);
		cb = fgetc(b);
		if (ca != cb) {
			printf("CMP %s %s differ at offset %ld\n", argv[1], argv[2], off);
			return 1;
		}
		if (ca == EOF)
			break;
		off++;
	}
	printf("CMP %s %s identical (%ld bytes)\n", argv[1], argv[2], off);
	return 0;
}
