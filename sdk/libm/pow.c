/* pow.c - x raised to y (R11C libm).
 *
 * Domain: x > 0 with any finite y; x = 0 for y > 0 gives 0, for y < 0 it is a
 * domain error (EDOM, HUGE_VAL); x < 0 needs an integer y (else EDOM, 0).
 * Overflow gives +/-HUGE_VAL with ERANGE; underflow (a result below DBL_MIN,
 * including zero) sets ERANGE.  NaN and infinite arguments are not handled.
 *
 * Every non-trivial case is computed by qd_pow_core (xmath.s) as
 * 2^(y*log2 x) in the x87 extended format, rounded once to double.  The
 * error is far below half an ulp of the result for the tested range; the
 * measured figures are in docs/R11C-LIBM-MAP.md. */
#include <math.h>
#include <float.h>
#include <errno.h>

double qd_pow_core(double ax, double y, int *st);

static int is_odd_integer(double y)
{
	double h = y * 0.5;
	return fabs(y) < 1.0e15 && h != floor(h);
}

double pow(double x, double y)
{
	double r;
	int st = 0, neg = 0;

	if (y == 0.0)
		return 1.0;
	if (x == 0.0) {
		if (y > 0.0)
			return 0.0;
		errno = EDOM;
		return HUGE_VAL;
	}
	if (x < 0.0) {
		if (y != floor(y)) {
			errno = EDOM;
			return 0.0;
		}
		neg = is_odd_integer(y);
		x = -x;
	}
	if (x == 1.0)
		return neg ? -1.0 : 1.0;
	r = qd_pow_core(x, y, &st);
	if (st > 0 || r > DBL_MAX) {
		errno = ERANGE;
		r = HUGE_VAL;
	} else if (st < 0 || r < DBL_MIN) {
		errno = ERANGE;
		if (st < 0)
			r = 0.0;
	}
	return neg ? -r : r;
}
