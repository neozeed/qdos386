/* FILETEST.AOU - R11A guest test: read-only stdio on the WIN1_ volume.
 * Reads LINES.TXT, whose bytes are exactly:
 *   "alpha\n" "beta\n" "\n" "gamma"     (17 bytes, no final newline)
 */
#include <stdio.h>
#include <string.h>

static int passes = 0, fails = 0;

static void check(const char *name, int ok)
{
	printf("CHECK %s: %s\n", name, ok ? "ok" : "FAIL");
	if (ok) passes++; else fails++;
}

int main(int argc, char **argv)
{
	FILE *fp;
	char line[32];
	int i, c;
	char blob[8];

	(void) argc;
	(void) argv;

	/* open and refuse */
	check("fopen read-only ok", (fp = fopen("LINES.TXT", "r")) != NULL);
	if (fp == NULL) {
		printf("QDOS/386 R11A FILETEST FAIL\n");
		return 1;
	}
	check("fopen missing file is NULL", fopen("NOSUCH.TXT", "r") == NULL);
	check("fopen write refused", fopen("LINES.TXT", "w") == NULL);
	check("fopen NULL name refused", fopen(NULL, "r") == NULL);
	check("fopen too-long name refused",
	      fopen("ABCDEFGHIJKLMNOPQRSTUVWXYZ.TXT", "r") == NULL);

	/* fgets: lines kept with their newline, a blank line, and a final line
	 * without newline, then end of file */
	check("fgets alpha", fgets(line, sizeof line, fp) != NULL && strcmp(line, "alpha\n") == 0);
	check("fgets beta", fgets(line, sizeof line, fp) != NULL && strcmp(line, "beta\n") == 0);
	check("fgets blank", fgets(line, sizeof line, fp) != NULL && strcmp(line, "\n") == 0);
	check("fgets last without newline", fgets(line, sizeof line, fp) != NULL && strcmp(line, "gamma") == 0);
	check("fgets at end is NULL", fgets(line, sizeof line, fp) == NULL);
	check("feof after end", feof(fp) != 0);
	check("ferror clear", ferror(fp) == 0);
	clearerr(fp);
	check("clearerr clears eof", feof(fp) == 0);

	/* fgets truncation: buffer of 4 keeps 3 chars and the terminator */
	check("seek start", fseek(fp, 0L, SEEK_SET) == 0 && ftell(fp) == 0L);
	check("fgets truncates to n-1", fgets(blob, 4, fp) != NULL && strcmp(blob, "alp") == 0);
	check("ftell after fgets", ftell(fp) == 3L);
	check("fgets n=0 is NULL", fgets(blob, 0, fp) == NULL);

	/* getc / fgetc / ungetc / ftell agree */
	fseek(fp, 0L, SEEK_SET);
	check("getc first", getc(fp) == 'a');
	check("ftell 1", ftell(fp) == 1L);
	check("ungetc returns char", ungetc('A', fp) == 'A');
	check("ftell after ungetc", ftell(fp) == 0L);
	check("read pushed-back char", fgetc(fp) == 'A');
	check("ftell back at 1", ftell(fp) == 1L);
	check("next char after pushback", fgetc(fp) == 'l');
	check("ftell 2", ftell(fp) == 2L);

	/* only one pushback; EOF cannot be pushed */
	check("ungetc EOF refused", ungetc(EOF, fp) == EOF);
	check("ungetc first ok", ungetc('x', fp) == 'x');
	check("ungetc second refused", ungetc('y', fp) == EOF);
	check("pushback read x", fgetc(fp) == 'x');
	check("continue after pushback", fgetc(fp) == 'p');

	/* fseek discards pushback */
	check("ungetc before seek", ungetc('Q', fp) == 'Q');
	check("seek clears pushback", fseek(fp, 0L, SEEK_SET) == 0);
	check("first char after seek", fgetc(fp) == 'a');
	check("SEEK_END refused", fseek(fp, 0L, SEEK_END) == -1);
	check("seek back ok", fseek(fp, 0L, SEEK_SET) == 0);

	/* fread across the whole file in two calls */
	check("fread 5 bytes", fread(blob, 1, 5, fp) == 5 && memcmp(blob, "alpha", 5) == 0);
	check("fread rest", fread(blob, 1, 8, fp) == 8 && memcmp(blob, "\nbeta\n\ng", 8) == 0);
	c = 0;
	for (i = 0; i < 20; i++) {
		if (fgetc(fp) == EOF) {
			c = 1;
			break;
		}
	}
	check("reads reach EOF", c == 1 && feof(fp) != 0);
	check("fgetc at EOF is EOF", fgetc(fp) == EOF);
	fclose(fp);

	/* repeated open and close: channels and heap blocks are released */
	for (i = 0; i < 20; i++) {
		FILE *again = fopen("LINES.TXT", "r");
		if (again == NULL)
			break;
		if (fgets(line, sizeof line, again) == NULL || strcmp(line, "alpha\n") != 0) {
			fclose(again);
			break;
		}
		fclose(again);
	}
	check("20 open/read/close cycles", i == 20);
	fp = fopen("LINES.TXT", "rb");
	check("reopen after cycles (rb)", fp != NULL && fgets(line, sizeof line, fp) != NULL);
	if (fp != NULL)
		fclose(fp);

	printf("FILETEST: %d checks passed, %d failed\n", passes, fails);
	if (fails == 0)
		printf("QDOS/386 R11A FILETEST PASS\n");
	else
		printf("QDOS/386 R11A FILETEST FAIL\n");
	return fails == 0 ? 0 : 1;
}
