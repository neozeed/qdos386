/* ENTER.AOU - R11L minimal source entry.  The guest has no editor; this is the smallest tool that lets a
 * person (or a script feeding the console) create a text file on the writable volume.
 *
 *   ENTER [-a] FILE      copy console lines into FILE (created or truncated; -a appends) until a line that
 *                        holds only a period.  Input is echoed; Backspace edits the current line.
 *
 * Lines are stored with LF.  A line cannot consist of a single period (that ends the entry), and a line is
 * at most 250 characters (longer input is cut).  Exit status: 0 done, 1 usage, 2 cannot open FILE,
 * 3 write or close error.  Classification: guest-native (runs inside QDOS/386; standard C89 + stdio). */
#include <stdio.h>
#include <string.h>

#define LINE_MAX_CH 250

int main(int argc, char **argv)
{
	FILE *fp;
	char line[LINE_MAX_CH + 2];
	const char *name = NULL;
	int append = 0, i, c, n, eof = 0, bad = 0;
	long bytes = 0, lines = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0)
			append = 1;
		else if (argv[i][0] == '-' || name != NULL)
			name = NULL, i = argc + 1;
		else
			name = argv[i];
	}
	if (name == NULL) {
		printf("usage: ENTER [-a] FILE   (end the text with a line holding only a period)\n");
		return 1;
	}
	fp = fopen(name, append ? "a" : "w");
	if (fp == NULL) {
		printf("ENTER: cannot open %s for writing\n", name);
		return 2;
	}
	for (;;) {
		n = 0;
		for (;;) {
			c = getchar();
			if (c == EOF) {
				eof = 1;
				break;
			}
			if (c == '\n')
				break;
			if (c == 8 || c == 127) {
				if (n > 0) {
					n--;
					fputs("\b \b", stdout);
				}
				continue;
			}
			if ((c >= 32 && c < 127) || c == '\t') {
				if (n < LINE_MAX_CH) {
					line[n++] = (char) c;
					putchar(c);
				}
			}
		}
		putchar('\n');
		line[n] = '\0';
		if (eof && n == 0)
			break;
		if (!eof && n == 1 && line[0] == '.')
			break;
		line[n++] = '\n';
		if (fwrite(line, 1, (size_t) n, fp) != (size_t) n)
			bad = 1;
		bytes += n;
		lines++;
		if (eof)
			break;
	}
	if (fclose(fp) != 0)
		bad = 1;
	if (bad) {
		printf("ENTER: write error on %s\n", name);
		return 3;
	}
	printf("ENTER: %ld lines, %ld bytes -> %s\n", lines, bytes, name);
	return 0;
}
