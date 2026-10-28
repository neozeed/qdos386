/* SUM.AOU - R11L: CRC-32 (IEEE 802.3, as zlib.crc32) and size of files.   SUM NAME [NAME ...]
 * Prints "SUM NAME crc=xxxxxxxx size=N" per file (the host judge compares the lines with zlib.crc32).
 * Exit status: the number of files that could not be read. */
#include <stdio.h>

static unsigned long table[256];

static void init_table(void)
{
	unsigned long c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++)
			c = (c & 1ul) ? (0xEDB88320ul ^ (c >> 1)) : (c >> 1);
		table[n] = c;
	}
}

int main(int argc, char **argv)
{
	unsigned char buf[512];
	int i, failed = 0;

	if (argc < 2) {
		printf("usage: SUM NAME [NAME ...]\n");
		return 100;
	}
	init_table();
	for (i = 1; i < argc; i++) {
		FILE *fp = fopen(argv[i], "r");
		unsigned long crc = 0xFFFFFFFFul, size = 0;
		size_t n, k;

		if (fp == NULL) {
			printf("SUM %s missing\n", argv[i]);
			failed++;
			continue;
		}
		while ((n = fread(buf, 1, sizeof buf, fp)) > 0) {
			for (k = 0; k < n; k++)
				crc = table[(crc ^ buf[k]) & 0xFFul] ^ (crc >> 8);
			size += n;
		}
		fclose(fp);
		printf("SUM %s crc=%08lx size=%lu\n", argv[i], crc ^ 0xFFFFFFFFul, size);
	}
	return failed;
}
