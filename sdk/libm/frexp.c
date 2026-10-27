/* frexp.c - x = m * 2^e with 0.5 <= |m| < 1 (R11C libm).  Exact. */
#include <math.h>

double frexp(double x, int *exp)
{
	union {
		double d;
		unsigned long w[2];
	} u;
	int e;

	u.d = x;
	if (x == 0.0) {
		*exp = 0;
		return x;
	}
	e = (int) ((u.w[1] >> 20) & 0x7ffUL);
	if (e == 0) {                      /* subnormal: scale up by 2^54 first */
		u.d = x * 18014398509481984.0;
		e = (int) ((u.w[1] >> 20) & 0x7ffUL);
		*exp = e - 1022 - 54;
	} else {
		*exp = e - 1022;
	}
	u.w[1] = (u.w[1] & 0x800fffffUL) | (1022UL << 20);
	return u.d;
}
