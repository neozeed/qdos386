/* trans.c - QDOS/386 libm (R11D): exp, log, log10, trigonometric, inverse
 * trigonometric, hyperbolic and fmod functions over the x87 kernels in
 * xtrans.s.  Domain and range errors set errno as C89 requires: EDOM for a
 * domain error (the result is 0), ERANGE for an overflow (HUGE_VAL) or an
 * underflow (0).  Accuracy is measured, not claimed: see docs/R11D-LIBM-MAP.md. */
#include <math.h>
#include <float.h>
#include <errno.h>

double qd_x_exp(double x, int *st);
double qd_x_log(double x, int base);
double qd_x_trig(double x, int op, int *st);
double qd_x_atan2(double y, double x);

/* 2^k for |k| <= 1000: exact (repeated squaring of powers of two). */
static double pow2i(int k)
{
	double p = 1.0, b = k >= 0 ? 2.0 : 0.5;
	unsigned int m = k >= 0 ? (unsigned int) k : (unsigned int) -k;
	while (m != 0) {
		if (m & 1)
			p = p * b;
		b = b * b;
		m >>= 1;
	}
	return p;
}

/* e * 2^n with one rounding (only the final step can enter the subnormal range). */
static double scale2(double e, int n)
{
	while (n > 1000) {
		e = e * pow2i(1000);
		n -= 1000;
	}
	while (n < -1000) {
		e = e * pow2i(-1000);
		n += 1000;
	}
	return e * pow2i(n);
}

/* exp: Cody-Waite reduction x = n*ln2 + r with ln2 split so that n*LN2_HI is
 * exact; r is then |r| <= ~0.35 and the x87 kernel computes exp(r) to about
 * one ulp, so the error does not grow with |x| as it did with x*log2e alone. */
#define LN2_HI 6.93147180369123816490e-01
#define LN2_LO 1.90821492927058770002e-10
#define LOG2E  1.44269504088896338700e+00

double exp(double x)
{
	int st = 0, n;
	double nd, r, e, res;
	if (x != x)
		return x;
	if (x > 709.8) {
		errno = ERANGE;
		return HUGE_VAL;
	}
	if (x < -745.2) {
		errno = ERANGE;
		return 0.0;
	}
	nd = x * LOG2E;
	n = (int) (nd < 0.0 ? nd - 0.5 : nd + 0.5);
	r = (x - (double) n * LN2_HI) - (double) n * LN2_LO;
	e = qd_x_exp(r, &st);
	res = scale2(e, n);
	if (res > DBL_MAX) {
		errno = ERANGE;
		return HUGE_VAL;
	}
	if (res != 0.0 && res < DBL_MIN) {
		errno = ERANGE;
	}
	return res;
}

double log(double x)
{
	if (x < 0.0) {
		errno = EDOM;
		return 0.0;
	}
	if (x == 0.0) {
		errno = ERANGE;
		return -HUGE_VAL;
	}
	return qd_x_log(x, 0);
}

double log10(double x)
{
	if (x < 0.0) {
		errno = EDOM;
		return 0.0;
	}
	if (x == 0.0) {
		errno = ERANGE;
		return -HUGE_VAL;
	}
	return qd_x_log(x, 1);
}

/* Trigonometric functions take radians.  |x| >= 2^63 cannot be reduced by the
 * x87: EDOM and 0 (documented). */
static double trig(double x, int op)
{
	int st = 0;
	double r;
	if (fabs(x) >= 9223372036854775808.0) {
		errno = EDOM;
		return 0.0;
	}
	r = qd_x_trig(x, op, &st);
	if (st != 0) {
		errno = EDOM;
		return 0.0;
	}
	return r;
}

double sin(double x)
{
	return trig(x, 0);
}

double cos(double x)
{
	return trig(x, 1);
}

double tan(double x)
{
	return trig(x, 2);
}

double atan(double x)
{
	return qd_x_atan2(x, 1.0);
}

double atan2(double y, double x)
{
	if (x == 0.0 && y == 0.0) {
		errno = EDOM;
		return 0.0;
	}
	return qd_x_atan2(y, x);
}

/* asin(x) = atan2(x, sqrt((1-x)(1+x))) keeps accuracy near |x| = 1.  The
 * domain is |x| <= 1. */
double asin(double x)
{
	if (fabs(x) > 1.0) {
		errno = EDOM;
		return 0.0;
	}
	return qd_x_atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}

double acos(double x)
{
	if (fabs(x) > 1.0) {
		errno = EDOM;
		return 0.0;
	}
	return qd_x_atan2(sqrt((1.0 - x) * (1.0 + x)), x);
}

/* Hyperbolic functions from exp.  For |x| < 2^-26 the result is x (the
 * correction term is below half an ulp); tanh saturates for |x| > 22.
 * sinh and cosh overflow above 709.78 (exp(709.78) is the largest exp). */
/* sinh(x) for |x| < 1 by its Taylor series: (e^x - e^-x)/2 cancels badly
 * near zero, the series does not. */
static double sinh_small(double x)
{
	double term = x, sum = x, x2 = x * x;
	int k;
	for (k = 1; k < 40; k++) {
		term = term * x2 / ((2.0 * k) * (2.0 * k + 1.0));
		sum = sum + term;
		if (fabs(term) <= fabs(sum) * 1.0e-17)
			break;
	}
	return sum;
}

double sinh(double x)
{
	double e, ax = fabs(x), r;
	if (ax < 1.0)
		return sinh_small(x);
	if (ax > 709.78) {
		errno = ERANGE;
		return x < 0.0 ? -HUGE_VAL : HUGE_VAL;
	}
	e = exp(ax);
	r = 0.5 * (e - 1.0 / e);
	return x < 0.0 ? -r : r;
}

double cosh(double x)
{
	double e, ax = fabs(x);
	if (ax > 709.78) {
		errno = ERANGE;
		return HUGE_VAL;
	}
	e = exp(ax);
	return 0.5 * (e + 1.0 / e);
}

double tanh(double x)
{
	double ax = fabs(x), e, r;
	if (ax < 1.0) {
		double c = cosh(x);
		return sinh_small(x) / c;
	}
	if (ax > 22.0)
		return x < 0.0 ? -1.0 : 1.0;
	e = exp(2.0 * ax);
	r = (e - 1.0) / (e + 1.0);
	return x < 0.0 ? -r : r;
}

/* fmod: exact remainder with the sign of x.  y == 0 is a domain error.
 * Exact in IEEE double arithmetic without extended precision: t is |y| scaled
 * by a power of two so that t <= r < 2t; then r - t is exact (Sterbenz) and
 * halving t is exact.  Cost is proportional to the exponent difference. */
double fmod(double x, double y)
{
	double r, ay, t;
	if (y == 0.0) {
		errno = EDOM;
		return 0.0;
	}
	if (x != x || y != y)                   /* NaN propagates */
		return x + y;
	if (x - x != 0.0) {                     /* x infinite: domain error */
		errno = EDOM;
		return 0.0;
	}
	r = x < 0.0 ? -x : x;
	ay = y < 0.0 ? -y : y;
	if (r < ay)
		return x;
	t = ay;
	while (t * 2.0 <= r)
		t = t * 2.0;
	while (t >= ay) {
		if (r >= t)
			r = r - t;
		t = t * 0.5;
	}
	return x < 0.0 ? -r : r;
}
