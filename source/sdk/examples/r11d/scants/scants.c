/* SCANTS: sscanf (and scanf from stdin) against the C89 conversions.  Every
 * result is checked in the guest.  The last check reads one line from stdin,
 * which the runner types after RUN SCANTS.AOU. */
#include <stdio.h>
#include <string.h>
#include "../check.h"

int main(void)
{
	int n, i, k, nn;
	unsigned int u;
	long lv;
	short sh;
	char word[16], set[16], ch;
	float f;
	double d;

	n = sscanf("42 abc", "%d %s", &i, word);
	CHECK("%d %s count", n == 2);
	CHECK("%d value", i == 42);
	CHECK("%s value", strcmp(word, "abc") == 0);

	n = sscanf("ff", "%x", &u);
	CHECK("%x", n == 1 && u == 255u);
	n = sscanf("0x1f", "%x", &u);
	CHECK("%x with 0x prefix", n == 1 && u == 31u);
	n = sscanf("010", "%i", &i);
	CHECK("%i octal prefix", n == 1 && i == 8);
	n = sscanf("0x10", "%i", &i);
	CHECK("%i hex prefix", n == 1 && i == 16);
	n = sscanf("17", "%o", &u);
	CHECK("%o", n == 1 && u == 15u);
	n = sscanf("-12", "%d", &i);
	CHECK("%d negative", n == 1 && i == -12);
	n = sscanf("+7", "%d", &i);
	CHECK("%d plus sign", n == 1 && i == 7);
	n = sscanf("123456789", "%ld", &lv);
	CHECK("%ld", n == 1 && lv == 123456789L);
	n = sscanf("-5", "%hd", &sh);
	CHECK("%hd", n == 1 && sh == -5);

	n = sscanf("Q", "%c", &ch);
	CHECK("%c", n == 1 && ch == 'Q');
	n = sscanf("abcdef", "%3c", word);
	CHECK("%3c reads three without skipping", n == 1 && word[0] == 'a' && word[2] == 'c');
	n = sscanf(" 7", "%c", &ch);
	CHECK("%c does not skip white space", n == 1 && ch == ' ');

	n = sscanf("5 6", "%*d %d", &i);
	CHECK("assignment suppression", n == 1 && i == 6);
	n = sscanf("abcdefgh", "%5s", word);
	CHECK("%5s width", n == 1 && strcmp(word, "abcde") == 0);
	n = sscanf("abcxyz", "%[a-c]", set);
	CHECK("%[a-c]", n == 1 && strcmp(set, "abc") == 0);
	n = sscanf("hello,world", "%[^,]", set);
	CHECK("%[^,]", n == 1 && strcmp(set, "hello") == 0);

	n = sscanf("3.25", "%f", &f);
	CHECK("%f", n == 1 && f == 3.25f);
	n = sscanf("1e3", "%lf", &d);
	CHECK("%lf exponent", n == 1 && d == 1000.0);
	n = sscanf("-2.5E-2", "%lf", &d);
	CHECK("%lf negative exponent", n == 1 && d == -0.025);
	n = sscanf("0.1 rest", "%lf", &d);
	CHECK("%lf rounds like strtod", n == 1 && d == 0.1);

	n = sscanf("12 34", "%d%n", &i, &nn);
	CHECK("%n counts characters", n == 1 && nn == 2);

	n = sscanf("", "%d", &i);
	CHECK("empty input before the first conversion is EOF", n == EOF);
	n = sscanf("abc", "%d", &i);
	CHECK("matching failure returns 0", n == 0);
	n = sscanf("12x", "%d,%d", &i, &k);
	CHECK("literal mismatch stops the scan", n == 1 && i == 12);
	n = sscanf("  7", " %d", &i);
	CHECK("white space in the format", n == 1 && i == 7);
	n = sscanf("%5", "%%%d", &i);
	CHECK("%% matches a percent sign", n == 1 && i == 5);
	n = sscanf("1 2 3", "%d %d", &i, &k);
	CHECK("stops after the requested conversions", n == 2 && i == 1 && k == 2);
	n = sscanf("12", "%d %d", &i, &k);
	CHECK("input ends after one conversion", n == 1);

	/* scanf from stdin: the runner types one line "3 4" */
	n = scanf("%d %d", &i, &k);
	CHECK("scanf from stdin count", n == 2);
	CHECK("scanf from stdin values", i == 3 && k == 4);

	return finish("SCANTS");
}
