/* STREAM: read-only WIN1_ files through the buffered C89 stream layer.
 * Fixtures on the volume: LINES.TXT ("alpha\nbeta\n\ngamma", 17 bytes),
 * EMPTY.TXT (0 bytes), LONG.TXT (200 x, newline, "end\n"). */
#include <stdio.h>
#include <string.h>
#include "../check.h"

int main(void)
{
	FILE *f;
	char buf[64];
	char tmp[8];
	size_t n;
	int total;
	int i;

	f = fopen("NOPE.TXT", "r");
	CHECK("missing file is NULL", f == NULL);
	CHECK("write mode w refused", fopen("LINES.TXT", "w") == NULL);
	CHECK("write mode a refused", fopen("LINES.TXT", "a") == NULL);
	CHECK("write mode r+ refused", fopen("LINES.TXT", "r+") == NULL);

	f = fopen("LINES.TXT", "r");
	CHECK("open LINES.TXT", f != NULL);
	if (f == NULL)
		return finish("STREAM");
	CHECK("fgets line 1", fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "alpha\n") == 0);
	CHECK("ftell after line 1 is 6", ftell(f) == 6L);
	CHECK("fgets line 2", fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "beta\n") == 0);
	CHECK("fgets empty line", fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "\n") == 0);
	CHECK("fgets last line without newline", fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "gamma") == 0);
	CHECK("fgets at end is NULL", fgets(buf, sizeof buf, f) == NULL);
	CHECK("feof after end", feof(f) != 0);
	CHECK("ferror clear at end", ferror(f) == 0);
	clearerr(f);
	CHECK("clearerr clears eof", feof(f) == 0);

	CHECK("fseek to 0", fseek(f, 0L, SEEK_SET) == 0 && ftell(f) == 0L);
	n = fread(tmp, 1, 5, f);
	CHECK("fread 5 bytes", n == 5 && memcmp(tmp, "alpha", 5) == 0);
	CHECK("fseek SEEK_CUR +1", fseek(f, 1L, SEEK_CUR) == 0 && ftell(f) == 6L);
	CHECK("getc after seek is b", getc(f) == 'b');
	CHECK("ungetc returns its char", ungetc('X', f) == 'X');
	CHECK("ftell after ungetc is 6", ftell(f) == 6L);
	CHECK("pushed-back char read first", fgetc(f) == 'X');
	CHECK("ungetc then ungetc fails", ungetc('a', f) == 'a' && ungetc('b', f) == EOF);
	CHECK("single pushback then continues", fgetc(f) == 'a' && fgetc(f) == 'e');
	CHECK("fseek discards pushback", ungetc('Q', f) == 'Q' && fseek(f, 0L, SEEK_SET) == 0 && fgetc(f) == 'a');
	CHECK("SEEK_END refused", fseek(f, 0L, SEEK_END) == -1);
	CHECK("negative seek refused", fseek(f, -1L, SEEK_SET) == -1);
	CHECK("fseek to 15", fseek(f, 15L, SEEK_SET) == 0);
	n = fread(buf, 1, 10, f);
	CHECK("short fread at end", n == 2 && feof(f) != 0);
	CHECK("fread of zero items", fread(buf, 1, 0, f) == 0);
	fclose(f);

	f = fopen("EMPTY.TXT", "r");
	CHECK("open empty file", f != NULL);
	if (f != NULL) {
		CHECK("empty file fgets is NULL", fgets(buf, sizeof buf, f) == NULL);
		CHECK("empty file fgetc is EOF with feof", fgetc(f) == EOF && feof(f) != 0);
		CHECK("empty file ftell 0", ftell(f) == 0L);
		fclose(f);
	}

	f = fopen("LONG.TXT", "r");
	CHECK("open long-line file", f != NULL);
	if (f != NULL) {
		total = 0;
		i = 0;
		while (fgets(buf, 64, f) != NULL) {
			int len = (int) strlen(buf);
			total += len;
			i++;
			if (len > 63)
				total = -1000000;
		}
		CHECK("long lines split at buffer size", total == 205);
		CHECK("long file has chunks", i >= 4);
		CHECK("eof after long file", feof(f) != 0);
		fclose(f);
	}

	for (i = 0; i < 20; i++) {
		f = fopen("LINES.TXT", "r");
		if (f == NULL)
			break;
		fclose(f);
	}
	CHECK("20 open/close cycles", i == 20);

	f = fopen("LINES.TXT", "r");
	if (f != NULL) {
		CHECK("rewind after reading", fgets(buf, sizeof buf, f) != NULL);
		rewind(f);
		CHECK("rewind returns to start", ftell(f) == 0L && fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "alpha\n") == 0);
		fgets(buf, sizeof buf, f);
		fgets(buf, sizeof buf, f);
		fgets(buf, sizeof buf, f);
		CHECK("feof then rewind clears it", fgets(buf, sizeof buf, f) == NULL && feof(f) != 0);
		rewind(f);
		CHECK("rewind clears feof", feof(f) == 0);
		fclose(f);
	}
	CHECK("fclose of stdout keeps it open", fclose(stdout) == 0);
	CHECK("stdout still writable", fputs("", stdout) == 0);
	return finish("STREAM");
}
