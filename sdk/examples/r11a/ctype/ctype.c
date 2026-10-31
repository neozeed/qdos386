/* CTYPE.AOU - R11A guest test: <ctype.h> in the C locale.
 * Assertions are expected values from the C89 definitions. */
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

static int passes = 0, fails = 0;

static void check(const char *name, int ok)
{
	printf("CHECK %s: %s\n", name, ok ? "ok" : "FAIL");
	if (ok) passes++; else fails++;
}

/* Count the ASCII characters (0..127) for which a class holds. */
static int count_class(int (*f)(int))
{
	int c, n = 0;
	for (c = 0; c <= 127; c++)
		if (f(c))
			n++;
	return n;
}

int main(int argc, char **argv)
{
	signed char sc = (signed char) 0xE9;   /* negative as char */
	int c;

	(void) argc;
	(void) argv;

	/* Counts over the ASCII range: fixed by the C locale definition. */
	check("count upper 26", count_class(isupper) == 26);
	check("count lower 26", count_class(islower) == 26);
	check("count alpha 52", count_class(isalpha) == 52);
	check("count digit 10", count_class(isdigit) == 10);
	check("count alnum 62", count_class(isalnum) == 62);
	check("count space 6", count_class(isspace) == 6);
	check("count xdigit 22", count_class(isxdigit) == 22);
	check("count punct 32", count_class(ispunct) == 32);
	check("count print 95", count_class(isprint) == 95);
	check("count graph 94", count_class(isgraph) == 94);
	check("count cntrl 33", count_class(iscntrl) == 33);

	/* Specific values, including the boundaries. */
	check("alpha A", isalpha('A') != 0);
	check("alpha z", isalpha('z') != 0);
	check("not alpha 0", isalpha('0') == 0);
	check("not alpha @", isalpha('@') == 0);
	check("not alpha [", isalpha('[') == 0);
	check("space tab", isspace('\t') != 0);
	check("space newline", isspace('\n') != 0);
	check("space vt", isspace('\v') != 0);
	check("space ff", isspace('\f') != 0);
	check("space cr", isspace('\r') != 0);
	check("not space nul", isspace('\0') == 0);
	check("space is not print", isprint(' ') != 0 && isgraph(' ') == 0);
	check("punct bang", ispunct('!') != 0);
	check("not punct space", ispunct(' ') == 0);
	check("cntrl DEL", iscntrl(127) != 0);
	check("xdigit f", isxdigit('f') != 0 && isxdigit('F') != 0);
	check("not xdigit g", isxdigit('g') == 0);
	check("tolower Q", tolower('Q') == 'q');
	check("toupper q", toupper('q') == 'Q');
	check("tolower digit unchanged", tolower('1') == '1');
	check("toupper punct unchanged", toupper('[') == '[');

	/* EOF and values outside unsigned char: false, and unchanged by case mapping. */
	check("EOF not alpha", isalpha(EOF) == 0);
	check("EOF not digit", isdigit(EOF) == 0);
	check("EOF not space", isspace(EOF) == 0);
	check("tolower EOF", tolower(EOF) == EOF);
	check("toupper EOF", toupper(EOF) == EOF);
	check("256 not alpha", isalpha(256) == 0);
	check("-2 not alpha", isalpha(-2) == 0);

	/* Signed-char edge case: a char with the high bit set. */
	check("signed char arg not alpha", isalpha(sc) == 0);
	check("unsigned char 0xE9 not alpha", isalpha((unsigned char) sc) == 0);
	check("unsigned char 0xE9 not upper", isupper((unsigned char) sc) == 0);
	check("0xA0 not space", isspace(0xA0) == 0);
	check("0x85 not space", isspace(0x85) == 0);
	check("tolower 0xC9 unchanged", tolower(0xC9) == 0xC9);

	/* Every byte: the mapping never leaves 0..255 and round-trips for letters. */
	for (c = 0; c <= UCHAR_MAX; c++) {
		if (isupper(c) && tolower(toupper(c)) != tolower(c))
			break;
		if (islower(c) && toupper(tolower(c)) != toupper(c))
			break;
	}
	check("case map round trip", c == UCHAR_MAX + 1);

	printf("CTYPE: %d checks passed, %d failed\n", passes, fails);
	if (fails == 0)
		printf("QDOS/386 R11A CTYPE PASS\n");
	else
		printf("QDOS/386 R11A CTYPE FAIL\n");
	return fails == 0 ? 0 : 1;
}
