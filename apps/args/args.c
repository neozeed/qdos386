/*
 * QDOS/386 R8 application: ARGS.AOU.
 *
 * Prints argc and each argv string on the console, validates the argument
 * vector against the R8 ABI and exits with a known status:
 *   valid   -> exit status = argc (argv[0] counts), so RUN ARGS.AOU one
 *              "two words" exits with status 3;
 *   invalid -> exit status 100 + the failing check number (1..5).
 *
 * Checks:
 *   1  argc is at least 1 and at most 16
 *   2  argv is not NULL
 *   3  argv[argc] is NULL (the array is NULL-terminated)
 *   4  every argv[i] for i < argc is non-NULL and non-empty for i == 0
 *   5  the strings are NUL-terminated within the 432-byte limit (strlen bounded)
 */
#include "appcalls.h"

static int con;

static void say(const char *s)
{
	unsigned int n = 0;
	while (s[n] != 0)
		n++;
	qtrap(APP_QT_CH_WRITE, (unsigned int) con, (unsigned int) s, n);
}

static void say_num(int v)
{
	char buf[12];
	int i = 0, neg = 0;
	unsigned int u;

	if (v < 0) {
		neg = 1;
		u = (unsigned int) -v;
	} else {
		u = (unsigned int) v;
	}
	do {
		buf[i++] = (char) ('0' + (u % 10));
		u /= 10;
	} while (u != 0);
	if (neg)
		buf[i++] = '-';
	while (i > 0) {
		char c[2];
		i--;
		c[0] = buf[i];
		c[1] = 0;
		say(c);
	}
}

/* Bounded length: returns the length, or -1 if no NUL within the limit. */
static int bounded_len(const char *s)
{
	int n;
	for (n = 0; n < (int) APP_ARG_STR_MAX; n++)
		if (s[n] == 0)
			return n;
	return -1;
}

int main(int argc, char **argv)
{
	int i, len, fail = 0;
	char **end;

	con = qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (con <= 0)
		return 99;

	say("ARGS argc = ");
	say_num(argc);
	say("\n");

	if (argc < 1 || argc > (int) APP_ARG_MAX) {
		fail = 1;
	} else if (argv == 0) {
		fail = 2;
	} else if (argv[argc] != 0) {
		fail = 3;
	} else {
		for (i = 0; i < argc && fail == 0; i++) {
			if (argv[i] == 0) {
				fail = 4;
				break;
			}
			len = bounded_len(argv[i]);
			if (len < 0) {
				fail = 5;
				break;
			}
			if (i == 0 && len == 0) {
				fail = 4;
				break;
			}
			say("ARGS argv[");
			say_num(i);
			say("] = \"");
			say(argv[i]);
			say("\"\n");
		}
	}

	qtrap(APP_QT_CH_CLOSE, (unsigned int) con, 0, 0);
	if (fail != 0)
		return 100 + fail;
	return argc;
}
