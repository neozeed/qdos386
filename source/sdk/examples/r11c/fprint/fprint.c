/* FPRINT: sprintf/snprintf of %f %e %g with flags, width and precision,
 * against tables/printf.h (expected text from Python's '%' operator, which
 * rounds the exact binary value, half-even).  Only failing rows are printed. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../check.h"
#include "../tables/printf.h"

static char buf[1024];
static char small[16];

static double make_d(unsigned long lo, unsigned long hi)
{
	unsigned char b[8];
	double d;
	int k;
	for (k = 0; k < 4; k++) {
		b[k] = (unsigned char) (lo >> (8 * k));
		b[k + 4] = (unsigned char) (hi >> (8 * k));
	}
	memcpy(&d, b, 8);
	return d;
}

int main(void)
{
	int i, bad = 0, n;
	int rc;

	for (i = 0; i < NFMT; i++) {
		const struct fmt_case *c = &fmt_cases[i];
		double x = make_d(c->lo, c->hi);
		rc = sprintf(buf, c->spec, x);
		n = (int) strlen(buf);
		if (strcmp(buf, c->want) != 0 || rc != n) {
			bad++;
			printf("FAIL row %d spec \"%s\": got \"%s\" (rc %d) want \"%s\"\n",
			       i, c->spec, buf, rc, c->want);
		}
	}
	printf("printf table: %d rows, %d bad\n", NFMT, bad);
	CHECK("printf table", bad == 0);

	/* snprintf truncates but returns the length it would have written */
	rc = snprintf(small, sizeof small, "%.20f", 1.0 / 3.0);
	CHECK("snprintf return length", rc == 22);
	CHECK("snprintf truncation", strlen(small) == 15 && small[0] == '0');
	CHECK("sprintf %g 100000", (sprintf(buf, "%g", 100000.0), strcmp(buf, "100000") == 0));
	CHECK("sprintf %g 1000000", (sprintf(buf, "%g", 1000000.0), strcmp(buf, "1e+06") == 0));
	CHECK("sprintf %e 0", (sprintf(buf, "%e", 0.0), strcmp(buf, "0.000000e+00") == 0));
	CHECK("sprintf %f -0", (sprintf(buf, "%.1f", -0.0), strcmp(buf, "-0.0") == 0));
	CHECK("sprintf %d still works", (sprintf(buf, "%d|%5d|%-4d|", 42, -7, 3), strcmp(buf, "42|   -7|3   |") == 0));
	CHECK("sprintf %s with precision", (sprintf(buf, "[%.2s]", "abcdef"), strcmp(buf, "[ab]") == 0));

	return finish("FPRINT");
}
