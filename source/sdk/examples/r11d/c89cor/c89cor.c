/* C89COR: the C89 string, ctype, stdlib, limits and misc functions added or
 * completed in R11D, each checked inside the guest.  Expected values come
 * from the C89 text or from Python where noted. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include "../check.h"

/* compile-time checks: a negative array size is an error at compile time */
typedef char chk_schar_min[(SCHAR_MIN == -128) ? 1 : -1];
typedef char chk_schar_max[(SCHAR_MAX == 127) ? 1 : -1];
typedef char chk_uchar_max[(UCHAR_MAX == 255) ? 1 : -1];
typedef char chk_dbl_dig[(DBL_DIG == 15) ? 1 : -1];
typedef char chk_flt_mant[(FLT_MANT_DIG == 24) ? 1 : -1];
typedef char chk_long_max[(LONG_MAX == 2147483647L) ? 1 : -1];

static int sumv(int n, ...)
{
	va_list ap;
	int i, s = 0;
	va_start(ap, n);
	for (i = 0; i < n; i++)
		s += va_arg(ap, int);
	va_end(ap);
	return s;
}

static const char hay_abc[] = "abc";
static const char hay_hw[] = "hello world";
static const char hay_abcabc[] = "abcabc";
static const char hay_abcdef[] = "abcdef";
static const char hay_hello[] = "hello";
static const char hay_str1[] = "abc";

/* atexit handler: runs when main returns (its line is in the run log) */
static void on_exit_handler(void)
{
	printf("C89COR atexit handler ran\n");
}

static int cmp_ints(const void *p, const void *q)
{
	int x = *(const int *) p, y = *(const int *) q;
	return x < y ? -1 : (x > y);
}

int main(void)
{
	char buf[32];
	char *tok;
	int i, ok;
	div_t dv;
	ldiv_t ldv;
	struct tm tmv;

	/* ---- string.h ---- */
	memset(buf, 'x', sizeof buf);
	strcpy(buf, "abcdef");
	memmove(buf + 2, buf, 4);                   /* overlap forwards */
	CHECK("memmove overlap forward", strncmp(buf, "ababcd", 6) == 0);
	strcpy(buf, "abcdef");
	memmove(buf, buf + 2, 4);                   /* overlap backwards */
	CHECK("memmove overlap backward", strncmp(buf, "cdefef", 6) == 0);
	CHECK("memcmp unsigned", memcmp("\x80", "\x7f", 1) > 0);
	CHECK("memchr", memchr(hay_hello, 'l', 5) == (void *) (hay_hello + 2));
	CHECK("memchr absent", memchr("hello", 'z', 5) == NULL);
	CHECK("strncmp n=0", strncmp("a", "b", 0) == 0);
	CHECK("strcmp sign", strcmp("abc", "abd") < 0 && strcmp("b", "a") > 0);
	CHECK("strcoll is strcmp", strcoll(hay_str1, "abd") < 0 && strcoll("b", "b") == 0);
	strcpy(buf, "xxxxxxxx");
	strncpy(buf, "ab", 5);
	CHECK("strncpy pads with NUL", buf[0] == 'a' && buf[2] == '\0' && buf[4] == '\0' && buf[5] == 'x');
	strcpy(buf, "ab");
	strncat(buf, "cdefgh", 3);
	CHECK("strncat limit", strcmp(buf, "abcde") == 0);
	CHECK("strchr finds NUL", strchr(hay_abc, '\0') == &hay_abc[3]);
	CHECK("strchr absent", strchr("abc", 'z') == NULL);
	CHECK("strrchr last", strrchr(hay_abcabc, 'b') == &hay_abcabc[4]);
	CHECK("strstr empty needle", strstr(hay_abc, "") == (char *) hay_abc);
	CHECK("strstr found", strstr(hay_hw, "wor") == (char *) hay_hw + 6);
	CHECK("strspn", strspn("123abc", "0123456789") == 3);
	CHECK("strcspn", strcspn("abc,def", ",") == 3);
	CHECK("strpbrk", strpbrk(hay_abcdef, "ed") == (char *) hay_abcdef + 3);
	CHECK("strpbrk absent", strpbrk("abc", "xyz") == NULL);
	strcpy(buf, ",,alpha,,beta,");
	tok = strtok(buf, ",");
	CHECK("strtok first", tok != NULL && strcmp(tok, "alpha") == 0);
	tok = strtok(NULL, ",");
	CHECK("strtok second skips empties", tok != NULL && strcmp(tok, "beta") == 0);
	tok = strtok(NULL, ",");
	CHECK("strtok end", tok == NULL);
	strcpy(buf, "");
	CHECK("strtok empty string", strtok(buf, ",") == NULL);
	CHECK("strerror 0", strcmp(strerror(0), "No error") == 0);
	CHECK("strerror EDOM", strcmp(strerror(EDOM), "Domain error") == 0);
	CHECK("strerror ERANGE", strcmp(strerror(ERANGE), "Result out of range") == 0);
	CHECK("strerror unknown", strcmp(strerror(999), "Unknown error") == 0);
	{
		char *d = strdup("copy");
		CHECK("strdup", d != NULL && strcmp(d, "copy") == 0);
		free(d);
	}

	/* ---- ctype.h, over the whole C-locale domain ---- */
	ok = 1;
	for (i = 0; i < 128; i++) {
		int up = i >= 'A' && i <= 'Z', lo = i >= 'a' && i <= 'z', dg = i >= '0' && i <= '9';
		if (!!isupper(i) != up || !!islower(i) != lo || !!isdigit(i) != dg)
			ok = 0;
		if (!!isalpha(i) != (up || lo) || !!isalnum(i) != (up || lo || dg))
			ok = 0;
		if (!!isspace(i) != (i == ' ' || (i >= 9 && i <= 13)))
			ok = 0;
		if (isprint(i) != (i >= 32 && i <= 126) || iscntrl(i) != (i < 32 || i == 127))
			ok = 0;
		if (ispunct(i) != (isgraph(i) && !isalnum(i)))
			ok = 0;
	}
	CHECK("ctype classes 0..127", ok);
	CHECK("ctype EOF is false", !isalpha(EOF) && !isdigit(EOF) && tolower(EOF) == EOF && toupper(EOF) == EOF);
	CHECK("ctype high bytes false", !isalpha(200) && toupper(200) == 200 && tolower(200) == 200);
	CHECK("ctype case", toupper('q') == 'Q' && tolower('Q') == 'q' && toupper('1') == '1');
	CHECK("ctype isxdigit", isxdigit('f') && isxdigit('F') && !isxdigit('g'));

	/* ---- stdlib.h ---- */
	CHECK("abs", abs(-7) == 7 && abs(7) == 7);
	CHECK("labs", labs(-70000L) == 70000L);
	dv = div(-7, 2);
	CHECK("div truncates toward zero", dv.quot == -3 && dv.rem == -1);
	ldv = ldiv(7L, -2L);
	CHECK("ldiv", ldv.quot == -3L && ldv.rem == 1L);
	CHECK("atoi", atoi("  -42xyz") == -42);
	CHECK("atol", atol("123456789") == 123456789L);
	CHECK("strtol base 16", strtol("1A", NULL, 16) == 26);
	CHECK("strtol base 8 negative", strtol("-077", NULL, 8) == -63);
	CHECK("strtoul all ones", strtoul("ffffffff", NULL, 16) == 4294967295UL);
	CHECK("strtod", strtod("2.5", NULL) == 2.5 && atof("0.5") == 0.5);
	srand(1);
	CHECK("rand seed 1 first", rand() == 16838);
	CHECK("rand seed 1 second", rand() == 5758);
	CHECK("rand seed 1 third", rand() == 10113);
	srand(7);
	CHECK("srand restarts the sequence", rand() == 19564 && rand() == 9806);
	CHECK("RAND_MAX", RAND_MAX == 32767);
	{
		int a[8] = { 5, -2, 9, 0, 3, 3, -8, 1 };
		int sorted[8] = { -8, -2, 0, 1, 3, 3, 5, 9 };
		int key = 3, *hit, miss = 4;
		qsort(a, 8, sizeof a[0], cmp_ints);
		CHECK("qsort", memcmp(a, sorted, sizeof a) == 0);
		hit = bsearch(&key, a, 8, sizeof a[0], cmp_ints);
		CHECK("bsearch found", hit != NULL && *hit == 3);
		CHECK("bsearch missing", bsearch(&miss, a, 8, sizeof a[0], cmp_ints) == NULL);
	}
	CHECK("getenv has no environment", getenv("PATH") == NULL);
	CHECK("system(NULL) reports no processor", system(NULL) == 0);
	CHECK("system(cmd) fails", system("DIR") == -1);
	CHECK("mblen C locale", mblen("a", 1) == 1 && mblen("", 1) == 0 && mblen(NULL, 0) == 0);
	CHECK("atexit accepts a function", atexit(on_exit_handler) == 0);
	CHECK("atexit rejects NULL", atexit(NULL) != 0);

	/* ---- limits.h, float.h, stddef.h, stdarg.h, errno.h ---- */
	CHECK("CHAR_BIT", CHAR_BIT == 8);
	CHECK("SHRT_MAX", SHRT_MAX == 32767 && SHRT_MIN == -32768 && USHRT_MAX == 65535);
	CHECK("INT_MIN", INT_MIN == -2147483647 - 1 && INT_MAX == 2147483647 && UINT_MAX == 4294967295u);
	CHECK("ULONG_MAX", ULONG_MAX == 4294967295UL);
	CHECK("DBL_MAX finite", DBL_MAX > 1.0e308 && DBL_MIN < 1.0e-307);
	CHECK("DBL_EPSILON", DBL_EPSILON == 2.220446049250313e-16);
	CHECK("offsetof", offsetof(struct { char c; int i; }, i) == 4);
	CHECK("size_t sizes", sizeof(size_t) == 4 && sizeof(ptrdiff_t) == 4);
	CHECK("stdarg sum", sumv(4, 1, 2, 3, 4) == 10);
	errno = 0;
	CHECK("errno starts clear", errno == 0);
	errno = EDOM;
	CHECK("errno holds EDOM", errno == 33 && ERANGE == 34);
	errno = 0;

	/* ---- locale.h ---- */
	CHECK("setlocale C", setlocale(LC_ALL, "C") != NULL && strcmp(setlocale(LC_ALL, "C"), "C") == 0);
	CHECK("setlocale empty is C", strcmp(setlocale(LC_ALL, ""), "C") == 0);
	CHECK("setlocale other fails", setlocale(LC_ALL, "de_DE") == NULL);
	CHECK("localeconv decimal point", strcmp(localeconv()->decimal_point, ".") == 0);

	/* ---- signal.h: no handler is delivered ---- */
	CHECK("signal fails with SIG_ERR", signal(SIGINT, SIG_IGN) == SIG_ERR);
	CHECK("raise fails", raise(SIGINT) != 0);

	/* ---- time.h: arithmetic and formatting only ---- */
	CHECK("difftime", difftime(10, 3) == 7.0);
	memset(&tmv, 0, sizeof tmv);
	tmv.tm_mday = 1;
	tmv.tm_mon = 0;
	tmv.tm_year = 70;
	tmv.tm_wday = 4;
	if (strcmp(asctime(&tmv), "Thu Jan  1 00:00:00 1970\n") != 0)
		printf("asctime gave \"%s\"\n", asctime(&tmv));
	CHECK("asctime", strcmp(asctime(&tmv), "Thu Jan  1 00:00:00 1970\n") == 0);
	CHECK("time has no clock", time(NULL) == (time_t) -1);
	CHECK("clock has no clock", clock() == (clock_t) -1);
	CHECK("gmtime has no clock", gmtime(NULL) == NULL);

	/* ---- stdio: the files that cannot change fail ---- */
	CHECK("remove fails", remove("NOFILE.TXT") != 0);
	CHECK("rename fails", rename("A.TXT", "B.TXT") != 0);
	CHECK("tmpfile fails", tmpfile() == NULL);
	CHECK("tmpnam fails", tmpnam(buf) == NULL);

	/* ---- assert.h (a passing test; a failing one aborts the job) ---- */
	assert(1 + 1 == 2);

	return finish("C89COR");
}
