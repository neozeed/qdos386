/* FPARSE: strtod and atof against the table tables/parse.h.  The expected
 * values are Python's float() on the C89 numeric prefix (correctly rounded),
 * the end offset and errno follow the C89 grammar.  Checks are counted per
 * table row; only failing rows are printed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../check.h"
#include "../tables/parse.h"

static void bits_of(double d, unsigned long *lo, unsigned long *hi)
{
	unsigned char b[8];
	unsigned long w[2];
	memcpy(b, &d, 8);
	w[0] = (unsigned long) b[0] | ((unsigned long) b[1] << 8) |
	       ((unsigned long) b[2] << 16) | ((unsigned long) b[3] << 24);
	w[1] = (unsigned long) b[4] | ((unsigned long) b[5] << 8) |
	       ((unsigned long) b[6] << 16) | ((unsigned long) b[7] << 24);
	*lo = w[0];
	*hi = w[1];
}

int main(void)
{
	int i, bad = 0, n = 0;
	unsigned long lo, hi;

	for (i = 0; i < NPARSE; i++) {
		const struct parse_case *c = &parse_cases[i];
		char *end = 0;
		double d;
		int ok;

		errno = 0;
		d = strtod(c->s, &end);
		bits_of(d, &lo, &hi);
		ok = (lo == c->lo && hi == c->hi) &&
		     (end - c->s == c->end) && (errno == c->err);
		n++;
		if (!ok) {
			bad++;
			printf("FAIL row %d \"%s\": got %08lx%08lx end %d errno %d, want %08lx%08lx end %d errno %d\n",
			       i, c->s, hi, lo, (int) (end - c->s), errno,
			       c->hi, c->lo, c->end, c->err);
		}
	}
	printf("strtod table: %d rows, %d bad\n", n, bad);
	CHECK("strtod table", bad == 0);

	/* atof is strtod with no end pointer */
	CHECK("atof 2.5", atof("2.5") == 2.5);
	CHECK("atof -0.125", atof("-0.125") == -0.125);
	CHECK("atof 1e3", atof("1e3") == 1000.0);
	CHECK("atof 0.1 exact double", atof("0.1") == 0.1);

	/* the requested errno outcomes */
	errno = 0;
	(void) strtod("1e400", 0);
	CHECK("strtod overflow errno", errno == ERANGE);
	errno = 0;
	(void) strtod("1e-400", 0);
	CHECK("strtod underflow errno", errno == ERANGE);
	errno = 0;
	(void) strtod("1.5", 0);
	CHECK("strtod no errno", errno == 0);

	return finish("FPARSE");
}
