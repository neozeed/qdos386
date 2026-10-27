/* FORMAT: printf family.  Checks the exact text and the return counts. */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../check.h"

static int fmt_s(char *b, const char *f, ...)
{
	va_list ap;
	int r;
	va_start(ap, f);
	r = vsprintf(b, f, ap);
	va_end(ap);
	return r;
}

static int fmt_v(char *b, size_t n, const char *f, ...)
{
	va_list ap;
	int r;
	va_start(ap, f);
	r = vsnprintf(b, n, f, ap);
	va_end(ap);
	return r;
}

int main(void)
{
	char b[300];
	int r;
	int r2;

	sprintf(b, "%d", -42);
	CHECK("%d negative", strcmp(b, "-42") == 0);
	sprintf(b, "[%5d]", 42);
	CHECK("%5d right aligned", strcmp(b, "[   42]") == 0);
	sprintf(b, "[%-5d]", 42);
	CHECK("%-5d left aligned", strcmp(b, "[42   ]") == 0);
	sprintf(b, "%05d", 42);
	CHECK("%05d zero padded", strcmp(b, "00042") == 0);
	sprintf(b, "%06d", -42);
	CHECK("%06d sign then zeros", strcmp(b, "-00042") == 0);
	sprintf(b, "%u", (unsigned int) 4294967295UL);
	CHECK("%u full range", strcmp(b, "4294967295") == 0);
	sprintf(b, "%x %X", 0xbeef, 0xbeef);
	CHECK("%x and %X", strcmp(b, "beef BEEF") == 0);
	sprintf(b, "%o", 8);
	CHECK("%o", strcmp(b, "10") == 0);
	sprintf(b, "%c%c", 'Z', 'q');
	CHECK("%c", strcmp(b, "Zq") == 0);
	sprintf(b, "[%s][%5s][%-5s]", "abc", "abc", "abc");
	CHECK("%s with width", strcmp(b, "[abc][  abc][abc  ]") == 0);
	sprintf(b, "%s", (const char *) NULL);
	CHECK("%s NULL prints (null)", strcmp(b, "(null)") == 0);
	sprintf(b, "100%%");
	CHECK("%% literal", strcmp(b, "100%") == 0);
	sprintf(b, "%ld %hd", 123456789L, 7);
	CHECK("%ld and %hd", strcmp(b, "123456789 7") == 0);
	sprintf(b, "%d", 0);
	CHECK("%d zero", strcmp(b, "0") == 0);
	sprintf(b, "%d", -2147483647 - 1);
	CHECK("%d INT_MIN", strcmp(b, "-2147483648") == 0);

	r = sprintf(b, "abc%d", 12);
	CHECK("sprintf returns length", r == 5 && strcmp(b, "abc12") == 0);

	r = snprintf(b, 4, "%s", "abcdef");
	CHECK("snprintf truncates, returns full length", r == 6 && strcmp(b, "abc") == 0);
	r = snprintf(b, 3, "ab");
	CHECK("snprintf exact fit", r == 2 && strcmp(b, "ab") == 0);
	strcpy(b, "keep");
	r = snprintf(b, 0, "xyz");
	CHECK("snprintf size 0 writes nothing", r == 3 && strcmp(b, "keep") == 0);
	r = fmt_v(b, 7, "%d-%s", 9, "nine");   /* 6 characters need a buffer of 7 */
	CHECK("vsnprintf length and text", r == 6 && strcmp(b, "9-nine") == 0);
	r2 = fmt_s(b, "%s=%d", "x", 5);
	CHECK("vsprintf length and text", r2 == 3 && strcmp(b, "x=5") == 0);

	sprintf(b, "%200d", 1);
	CHECK("wide field of 200", strlen(b) == 200 && b[199] == '1' && b[0] == ' ');

	r = printf("%s|%d\n", "printf", 1);
	CHECK("printf returns count", r == 9);
	r = fprintf(stdout, "%s\n", "fprintf to stdout");
	CHECK("fprintf stdout returns count", r == 18);
	r = fprintf(stderr, "%s\n", "fprintf to stderr");
	CHECK("fprintf stderr returns count", r == 18);
	r = fputs("fputs line\n", stdout);
	CHECK("fputs returns zero", r == 0);
	r = fputc('K', stdout);
	fputc('\n', stdout);
	CHECK("fputc returns the character", r == 'K');
	r = (int) fwrite("fwrite\n", 1, 7, stdout);
	CHECK("fwrite returns items", r == 7);
	r = fflush(stdout);
	CHECK("fflush stdout", r == 0);
	CHECK("setvbuf rejects caller buffer", setvbuf(stdout, b, _IOFBF, 16) == -1);
	CHECK("setvbuf accepts NULL unbuffered", setvbuf(stdout, NULL, _IONBF, 0) == 0);
	printf("unbuffered line\n");
	CHECK("setvbuf back to line mode", setvbuf(stdout, NULL, _IOLBF, 0) == 0);
	return finish("FORMAT");
}
