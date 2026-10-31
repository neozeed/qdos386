/* modf.c - splits x into integer and fraction parts, exactly (R11C libm).
 * The integer part is found by clearing the fraction bits of the
 * significand, so no rounding happens.  Both parts carry the sign of x,
 * including a zero fraction (modf(-1.0) is -0.0 in the fraction). */
#include <math.h>

double modf(double x, double *ip)
{
	union {
		double d;
		unsigned long w[2];
	} u, t, z;
	int e, k;

	u.d = x;
	e = (int) ((u.w[1] >> 20) & 0x7ffUL) - 1023;
	if (e < 0) {                       /* |x| < 1: integer part is a signed zero */
		t.w[0] = 0UL;
		t.w[1] = u.w[1] & 0x80000000UL;
	} else if (e >= 52) {              /* already an integer (or huge) */
		t.d = u.d;
	} else {
		k = 52 - e;                    /* number of fraction bits */
		t.d = u.d;
		if (k >= 32) {
			t.w[0] = 0UL;
			t.w[1] &= ~((1UL << (k - 32)) - 1UL);
		} else {
			t.w[0] &= ~((1UL << k) - 1UL);
		}
	}
	*ip = t.d;
	if (x == t.d) {                    /* no fraction: a zero with the sign of x */
		z.w[0] = 0UL;
		z.w[1] = u.w[1] & 0x80000000UL;
		return z.d;
	}
	return x - t.d;                    /* exact */
}
