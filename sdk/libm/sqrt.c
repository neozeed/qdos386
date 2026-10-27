/* sqrt.c - square root (R11C libm).  Correctly rounded for x >= 0 (the x87
 * fsqrt at 53-bit precision); x < 0 is a domain error: EDOM, result 0. */
#include <math.h>
#include <errno.h>

double qd_sqrt_raw(double x);

double sqrt(double x)
{
	if (x < 0.0) {
		errno = EDOM;
		return 0.0;
	}
	if (x == 0.0)
		return x;
	return qd_sqrt_raw(x);
}
