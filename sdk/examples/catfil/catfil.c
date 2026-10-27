/* CATFIL.AOU - R9 proof: read a FAT16 file through the C library.
 *
 *   - a missing name and a write mode are refused (fopen returns NULL);
 *   - pass 1: fread in 100-byte chunks counts the file and keeps its first 16
 *     bytes; afterwards feof is set, ftell equals the count, and a further
 *     fread returns 0;
 *   - pass 2: fseek back to the start, the first 16 bytes match pass 1, and a
 *     full re-read counts the same total;
 *   - a relative seek (SEEK_SET 2, SEEK_CUR 3) lands at 5 and fgetc reads the
 *     byte there, which must equal the byte at offset 5 from pass 1.
 */
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *fp;
	char chunk[100];
	char head[16];
	char again_head[16];
	unsigned long total = 0, again = 0;
	unsigned int got;
	int c, i, fails = 0;
	int at5 = -1;

	(void) argc;
	(void) argv;

	if (fopen("NOPE.TXT", "r") != NULL)
		fails++;
	if (fopen("README.TXT", "w") != NULL)
		fails++;

	fp = fopen("README.TXT", "r");
	if (fp == NULL) {
		printf("QDOS/386 R9 FILECAT FAIL (open)\n");
		return 91;
	}

	/* Pass 1. */
	memset(head, 0, sizeof head);
	while ((got = (unsigned int) fread(chunk, 1, sizeof chunk, fp)) != 0) {
		if (total == 0)
			memcpy(head, chunk, got < 16 ? got : 16);
		if (total <= 5 && total + got > 5)
			at5 = (unsigned char) chunk[5 - total];
		total += got;
	}
	if (!feof(fp) || ferror(fp))
		fails++;
	if (ftell(fp) != (long) total)
		fails++;
	if (fread(chunk, 1, 1, fp) != 0)      /* EOF is sticky until a seek */
		fails++;

	/* Pass 2. */
	if (fseek(fp, 0L, SEEK_SET) != 0 || ftell(fp) != 0L)
		fails++;
	if (fread(again_head, 1, 16, fp) != 16 || memcmp(again_head, head, 16) != 0)
		fails++;
	if (fseek(fp, 0L, SEEK_SET) != 0)
		fails++;
	while ((got = (unsigned int) fread(chunk, 1, sizeof chunk, fp)) != 0)
		again += got;
	if (again != total)
		fails++;

	/* Relative seek. */
	if (fseek(fp, 2L, SEEK_SET) != 0 || fseek(fp, 3L, SEEK_CUR) != 0)
		fails++;
	if (ftell(fp) != 5L)
		fails++;
	c = fgetc(fp);
	if (c == EOF || c != at5)
		fails++;
	fclose(fp);

	printf("CATFIL: README.TXT %lu bytes; second pass %lu\n", total, again);
	printf("CATFIL: first line: ");
	fp = fopen("README.TXT", "r");
	if (fp != NULL) {
		for (i = 0; i < 60; i++) {
			c = fgetc(fp);
			if (c == EOF || c == '\n')
				break;
			putchar(c);
		}
		fclose(fp);
	}
	putchar('\n');

	if (fails == 0 && total != 0) {
		printf("QDOS/386 R9 FILECAT PASS\n");
		return 0;
	}
	printf("QDOS/386 R9 FILECAT FAIL (%d)\n", fails);
	return 100 + fails;
}
