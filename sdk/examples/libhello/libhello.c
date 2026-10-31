/* LIBHEL.AOU - R9 proof: malloc/free, strings, printf, argc/argv, exit status.
 * Ordinary C89 with the SDK headers; no traps are written by hand. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	char *buf;
	char line[64];
	int i, fails = 0;

	printf("LIBHEL argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("LIBHEL argv[%d] = \"%s\"\n", i, argv[i]);
	if (argc < 1 || argv[0] == NULL)
		fails++;

	buf = (char *) malloc(64);
	if (buf == NULL)
		return 90;
	strcpy(buf, "malloc ok");
	strcat(buf, ", strcat ok");
	if (strcmp(buf, "malloc ok, strcat ok") != 0)
		fails++;
	printf("LIBHEL %s (%u bytes)\n", buf, (unsigned int) strlen(buf));
	free(buf);

	sprintf(line, "%d|%u|%x|%X|%o|%c|%5d|%-4d|%04d|%s|%%",
	        -42, 7u, 255u, 255u, 8u, 'Z', 42, 7, 9, "end");
	if (strcmp(line, "-42|7|ff|FF|10|Z|   42|7   |0009|end|%") != 0)
		fails++;
	printf("LIBHEL %s\n", line);

	if (fails == 0) {
		printf("QDOS/386 R9 LIBHELLO PASS\n");
		return argc;
	}
	printf("QDOS/386 R9 LIBHELLO FAIL (%d)\n", fails);
	return 100 + fails;
}
