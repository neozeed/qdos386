/* MATHX: the R11D libm functions (exp log log10 sin cos tan asin acos atan
 * atan2 sinh cosh tanh fmod) against tables/mathx.h, whose reference is
 * Python's math module.  Each function's worst error in ulp is printed; the
 * checks use the bound in docs/R11D-LIBM-MAP.md.  Domain and range errors are
 * checked separately, with errno. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "../check.h"
#include "../tables/mathx.h"

static const char *names[14] = { "exp", "log", "log10", "sin", "cos", "tan", "asin",
                                 "acos", "atan", "atan2", "sinh", "cosh", "tanh", "fmod" };

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

/* distance of got from want, in units of the ulp of want */
static double ulps(double got, double want)
{
	double d = got - want;
	double u;
	int e;
	if (d == 0.0)
		return 0.0;
	(void) frexp(want, &e);
	if (want == 0.0)
		e = -1022;
	u = ldexp(1.0, e - 53);
	return (d < 0.0 ? -d : d) / u;
}

int main(void)
{
	double worst[14];
	int cnt[14], bad[14];
	int i, f, fails = 0;
	double bound = 2.0;   /* ulp; see docs/R11D-LIBM-MAP.md for the measured figures */

	for (f = 0; f < 14; f++) {
		worst[f] = 0.0;
		cnt[f] = 0;
		bad[f] = 0;
	}
	for (i = 0; i < NMATHX; i++) {
		const struct mathx_row *r = &mathx_rows[i];
		double x = make_d(r->xlo, r->xhi);
		double y = make_d(r->ylo, r->yhi);
		double want = make_d(r->rlo, r->rhi);
		double got = 0.0, u;
		errno = 0;
		switch (r->fn) {
		case 0: got = exp(x); break;
		case 1: got = log(x); break;
		case 2: got = log10(x); break;
		case 3: got = sin(x); break;
		case 4: got = cos(x); break;
		case 5: got = tan(x); break;
		case 6: got = asin(x); break;
		case 7: got = acos(x); break;
		case 8: got = atan(x); break;
		case 9: got = atan2(x, y); break;
		case 10: got = sinh(x); break;
		case 11: got = cosh(x); break;
		case 12: got = tanh(x); break;
		default: got = fmod(x, y); break;
		}
		u = ulps(got, want);
		cnt[r->fn]++;
		if (u > worst[r->fn])
			worst[r->fn] = u;
		if (u > bound) {
			bad[r->fn]++;
			fails++;
			if (bad[r->fn] <= 3)
				printf("FAIL %s row %d x=%.17g y=%.17g got %.17g want %.17g (%.2f ulp)\n",
				       names[r->fn], i, x, y, got, want, u);
		}
	}
	for (f = 0; f < 14; f++)
		printf("MATHX %-6s rows %3d worst %.2f ulp bad %d\n", names[f], cnt[f], worst[f], bad[f]);
	CHECK("libm table within the bound", fails == 0);

	/* exact results */
	CHECK("exp(0) = 1", exp(0.0) == 1.0);
	CHECK("log(1) = 0", log(1.0) == 0.0);
	CHECK("log10(1000) = 3", log10(1000.0) == 3.0);
	CHECK("sin(0) = 0", sin(0.0) == 0.0);
	CHECK("cos(0) = 1", cos(0.0) == 1.0);
	CHECK("atan2(0,1) = 0", atan2(0.0, 1.0) == 0.0);
	CHECK("fmod exact", fmod(7.5, 2.0) == 1.5 && fmod(-7.5, 2.0) == -1.5);
	CHECK("tanh(0) = 0", tanh(0.0) == 0.0);

	/* domain and range errors: the value and errno, as C89 requires */
	errno = 0;
	CHECK("log(0) gives -HUGE_VAL", log(0.0) == -HUGE_VAL);
	CHECK("log(0) sets ERANGE", errno == ERANGE);
	errno = 0;
	CHECK("log(-1) gives 0", log(-1.0) == 0.0);
	CHECK("log(-1) sets EDOM", errno == EDOM);
	errno = 0;
	CHECK("exp(1000) gives HUGE_VAL", exp(1000.0) == HUGE_VAL);
	CHECK("exp(1000) sets ERANGE", errno == ERANGE);
	errno = 0;
	CHECK("exp(-1000) gives 0", exp(-1000.0) == 0.0);
	CHECK("exp(-1000) sets ERANGE", errno == ERANGE);
	errno = 0;
	CHECK("asin(2) gives 0", asin(2.0) == 0.0);
	CHECK("asin(2) sets EDOM", errno == EDOM);
	errno = 0;
	CHECK("acos(-2) sets EDOM", (acos(-2.0), errno == EDOM));
	errno = 0;
	CHECK("atan2(0,0) sets EDOM", (atan2(0.0, 0.0), errno == EDOM));
	errno = 0;
	CHECK("sin(1e300) sets EDOM (not reduced)", (sin(1e300), errno == EDOM));
	errno = 0;
	CHECK("fmod(1,0) sets EDOM", (fmod(1.0, 0.0), errno == EDOM));
	errno = 0;
	CHECK("cosh(1000) sets ERANGE", (cosh(1000.0), errno == ERANGE));

	return finish("MATHX");
}
