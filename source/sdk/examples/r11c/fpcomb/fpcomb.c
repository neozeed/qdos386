/* FPCOMB: one program through the whole R11C floating-point path: parse with
 * strtod and atof, compute with sqrt, pow, floor, ceil, modf and frexp, format
 * with %f, %e, %g and sprintf, and write to stdout.  Output is fixed text, so
 * the same run twice must give the same bytes; the shell runs it twice. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "../check.h"

int main(void)
{
	char buf[128];
	char *end;
	double a, b, c, ip, fr;
	int e;

	static const char text[] = "  2.25e2xyz";

	a = strtod(text, &end);
	CHECK("strtod value", a == 225.0);
	CHECK("strtod end", end[0] == 'x');
	b = atof("0.5");
	CHECK("atof value", b == 0.5);

	c = sqrt(a) + pow(2.0, 10.0) - floor(-2.5) + ceil(-2.5);
	CHECK("combined value 15+1024+3-2", c == 1040.0);
	CHECK("sqrt exact", sqrt(a) == 15.0);

	fr = modf(-3.75, &ip);
	CHECK("modf integer", ip == -3.0);
	CHECK("modf fraction", fr == -0.75);
	fr = frexp(10.0, &e);
	CHECK("frexp", fr == 0.625 && e == 4);
	CHECK("ldexp", ldexp(fr, e) == 10.0);
	CHECK("fabs", fabs(-0.125) == 0.125);

	sprintf(buf, "%f", 1.0 / 3.0);
	CHECK("%f", strcmp(buf, "0.333333") == 0);
	sprintf(buf, "%e", 12345.678);
	CHECK("%e", strcmp(buf, "1.234568e+04") == 0);
	sprintf(buf, "%g", 0.0001);
	CHECK("%g", strcmp(buf, "0.0001") == 0);

	/* the fixed output, for the two-run comparison */
	printf("FPCOMB value %.6f exp %e gen %g\n", c, a, 1.0 / 3.0);
	printf("FPCOMB parts %.3f %.3f %.2f\n", ip, fr, b);
	printf("FPCOMB parse end offset %d errno %d\n", (int) (end - text), errno);

	return finish("FPCOMB");
}
