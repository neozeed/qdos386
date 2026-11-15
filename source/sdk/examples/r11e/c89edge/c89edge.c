/* C89EDGE: direct checks for the R11D functions that had no test naming them,
 * plus the write refusals of the read-only FAT16 stream model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include "../check.h"

static int fwd(const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);
	return n;
}

static int fwd_file(FILE *f, const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vfprintf(f, fmt, ap);
	va_end(ap);
	return n;
}

int main(void)
{
	char buf[64];
	char dst[32];
	FILE *f;
	struct tm tm;
	int n;
	char *p;
	void *m;

	/* strcat / strncat */
	strcpy(dst, "ab");
	strcat(dst, "cd");
	CHECK("strcat appends", strcmp(dst, "abcd") == 0);
	strcat(dst, "");
	CHECK("strcat with empty source", strcmp(dst, "abcd") == 0);
	strncat(dst, "efgh", 2);
	CHECK("strncat appends n characters", strcmp(dst, "abcdef") == 0);
	p = strcat(dst, "!");
	CHECK("strcat returns its destination", p == dst && strcmp(dst, "abcdef!") == 0);

	/* puts / putc / fputs */
	CHECK("puts returns a non-negative value", puts("C89EDGE puts line") >= 0);
	CHECK("putc returns the character", putc('x', stdout) == 'x');
	putchar('\n');
	CHECK("fputs to stdout", fputs("C89EDGE fputs line\n", stdout) >= 0);

	/* vprintf / vfprintf: va_list forwarding under GCC 1.40 */
	n = fwd("[%d-%s-%c]\n", 42, "xy", 'z');
	CHECK("vprintf forwards arguments and returns the count", n == 10);
	n = fwd_file(stdout, "[%5.2f|%x]\n", 3.5, 255);
	CHECK("vfprintf to stdout forwards arguments", n == 11);
	n = fwd_file(stdout, "");
	CHECK("vfprintf of empty format returns 0", n == 0);

	/* getc / ungetc / fscanf from a read-only file */
	f = fopen("LINES.TXT", "r");
	CHECK("open LINES.TXT", f != NULL);
	if (f != NULL) {
		int c = getc(f);
		CHECK("getc returns 'a'", c == 'a');
		CHECK("ungetc pushes back", ungetc(c, f) == 'a');
		CHECK("getc after ungetc", getc(f) == 'a');
		n = fscanf(f, "%5s", buf);
		CHECK("fscanf reads a word", n == 1 && strcmp(buf, "lpha") == 0);
		CHECK("fscanf then getc", getc(f) == '\n');
		rewind(f);
		CHECK("rewind resets position", ftell(f) == 0L);
		n = fscanf(f, "%5s", buf);
		CHECK("fscanf after rewind", n == 1 && strcmp(buf, "alpha") == 0);
		fclose(f);
	}

	/* setbuf before any I/O */
	f = fopen("LINES.TXT", "r");
	if (f != NULL) {
		static char sbuf[BUFSIZ];
		setbuf(f, sbuf);
		CHECK("setbuf then fgets", fgets(buf, sizeof buf, f) != NULL && strcmp(buf, "alpha\n") == 0);
		fclose(f);
	}

	/* writes on a read-only stream must fail and set the error flag */
	f = fopen("LINES.TXT", "r");
	if (f != NULL) {
		n = (int) fwrite("x", 1, 1, f);
		CHECK("fwrite to a read-only stream writes nothing", n == 0);
		CHECK("fwrite sets the error indicator", ferror(f) != 0);
		clearerr(f);
		CHECK("clearerr clears the error", ferror(f) == 0);
		CHECK("fputs to a read-only stream fails", fputs("x", f) == EOF);
		fclose(f);
	}

	/* perror / strerror */
	CHECK("strerror(EDOM) is a non-empty string", strlen(strerror(EDOM)) > 0);
	perror("C89EDGE perror");              /* writes to stderr; output checked by the log */

	/* time: struct tm functions only (no calendar clock) */
	memset(&tm, 0, sizeof tm);
	tm.tm_mday = 1;
	tm.tm_year = 70;
	tm.tm_wday = 4;
	CHECK("asctime of 1970-01-01", strcmp(asctime(&tm), "Thu Jan  1 00:00:00 1970\n") == 0);
	CHECK("difftime of 10 and 4", difftime((time_t) 10, (time_t) 4) == 6.0);
	CHECK("time reports no clock", time(NULL) == (time_t) -1);

	/* allocation failure */
	m = malloc(0x7fffffffu);
	CHECK("malloc of an impossible size is NULL", m == NULL);
	return finish("C89EDGE");
}
