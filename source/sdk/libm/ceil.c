/* ceil.c - smallest integer not less than x (R11C libm).  Exact. */
#include <math.h>

double ceil(double x)
{
	double ip, f;

	f = modf(x, &ip);
	if (f > 0.0)
		ip = ip + 1.0;
	return ip;
}
