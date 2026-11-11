/* ldexp.c - x * 2^n (R11C libm).
 *
 * Normal results are exact (the exponent field is set).  Results in the
 * subnormal range are rounded once, to nearest-even, by a single multiply
 * by an exact power of two.  Overflow returns +/-HUGE_VAL (DBL_MAX) with
 * ERANGE; underflow to zero also sets ERANGE. */
#include <math.h>
#include <float.h>
#include <errno.h>

/* 2^k for -1074 <= k <= 1023, built by exact doubling/halving. */
static double pow2i(int k)
{
	double r = 1.0;

	while (k > 0) {
		r = r * 2.0;
		k--;
	}
	while (k < 0) {
		r = r * 0.5;
		k++;
	}
	return r;
}

double ldexp(double x, int n)
{
	union {
		double d;
		unsigned long w[2];
	} u;
	double m, v, r;
	int e, F, neg;

	if (x == 0.0)
		return x;
	neg = x < 0.0;
	m = frexp(x, &e);                  /* x = m * 2^e */
	e += n;
	if (e > 1024) {
		errno = ERANGE;
		return neg ? -HUGE_VAL : HUGE_VAL;
	}
	if (e >= -1021) {                  /* normal result: set the exponent */
		u.d = m;
		F = e + 1022;
		u.w[1] = (u.w[1] & 0x800fffffUL) | ((unsigned long) F << 20);
		return u.d;
	}
	errno = ERANGE;
	if (e < -1075)
		return neg ? -0.0 : 0.0;
	if (e == -1075) {                  /* 2^-1075 is half the least subnormal */
		r = fabs(m) > 0.5 ? pow2i(-1074) : 0.0;
		return neg ? -r : r;
	}
	v = m * pow2i(-1000);              /* exact: a normal number */
	r = v * pow2i(e + 1000);           /* the one rounding */
	return r;
}
