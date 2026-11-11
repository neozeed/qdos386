/* fabs.c - exact: clears the sign bit (R11C libm). */
#include <math.h>

double fabs(double x)
{
	union {
		double d;
		unsigned long w[2];       /* w[0] low word, w[1] high word */
	} u;

	u.d = x;
	u.w[1] &= 0x7fffffffUL;
	return u.d;
}
