/* FMATH: floor, ceil, fabs, frexp, modf, sqrt against tables/libm.h, and
 * ldexp and pow.  Expected values come from Python's math module.  floor,
 * ceil, fabs, frexp, modf and sqrt must be exact; pow is checked against its
 * documented bound (see docs/R11C-LIBM-MAP.md) and the largest observed error
 * in ulp is printed. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "../check.h"
#include "../tables/libm.h"

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

static int same(double a, unsigned long lo, unsigned long hi)
{
	double b = make_d(lo, hi);
	return memcmp(&a, &b, 8) == 0;
}

/* distance of got from want in units of the ulp of want */
static double ulps(double got, double want)
{
	double d = got - want;
	double u;
	int e;
	if (d == 0.0)
		return 0.0;
	(void) frexp(want, &e);
	u = ldexp(1.0, e - 53);	/* ulp of want: want = m * 2^e, 0.5 <= m < 1 */
	return (d < 0.0 ? -d : d) / u;
}

int main(void)
{
	int i, badf = 0, bade = 0, badm = 0, nm = 0;
	int ii, badp = 0, np = 0;
	double worst = 0.0, worst_ulp = 0.0;
	double got, fr, ip;
	int e;

	for (i = 0; i < NMATH; i++) {
		const struct math_row *r = &math_rows[i];
		double x = make_d(r->xlo, r->xhi);
		nm++;
		if (!same(floor(x), r->flo, r->fhi) ||
		    !same(ceil(x), r->clo, r->chi) ||
		    !same(fabs(x), r->alo, r->ahi)) {
			badf++;
			printf("FAIL floor/ceil/fabs row %d x=%.17g\n", i, x);
		}
		got = frexp(x, &e);
		if (!same(got, r->mlo, r->mhi) || e != r->fm_e) {
			badm++;
			printf("FAIL frexp row %d x=%.17g e=%d\n", i, x, e);
		}
		fr = modf(x, &ip);
		if (!same(ip, r->ilo, r->ihi) || !same(fr, r->frlo, r->frhi)) {
			badm++;
			printf("FAIL modf row %d x=%.17g\n", i, x);
		}
		if (r->has_sqrt) {
			if (!same(sqrt(x), r->slo, r->shi)) {
				bade++;
				printf("FAIL sqrt row %d x=%.17g\n", i, x);
			}
		}
	}
	printf("floor/ceil/fabs: %d rows, %d bad; frexp/modf: %d bad; sqrt: %d bad\n",
	       nm, badf, badm, bade);
	CHECK("floor ceil fabs table", badf == 0);
	CHECK("frexp modf table", badm == 0);
	CHECK("sqrt table", bade == 0);

	/* ldexp: exact scalings and the range rules */
	CHECK("ldexp 1.5 4", ldexp(1.5, 4) == 24.0);
	CHECK("ldexp -3 -1", ldexp(-3.0, -1) == -1.5);
	CHECK("ldexp to 2^-1074", ldexp(1.0, -1074) == 4.9406564584124654e-324);
	errno = 0;
	(void) ldexp(1.0, 1025);
	CHECK("ldexp overflow ERANGE", errno == ERANGE);

	/* pow: the reference is Python's math.pow.  The bound is analytic (see
	 * docs/R11C-LIBM-MAP.md): the x87 computes t = y*log2(x) in double, so
	 * the result error is about |t| ulp, and |t| = |log2(result)| lies in
	 * [|e|-1, |e|] where frexp(result) = m * 2^e.  The test allows |e| + 6 ulp. */
	for (ii = 0; ii < NPOW; ii++) {
		const struct pow_row *p = &pow_rows[ii];
		double x = make_d(p->xlo, p->xhi);
		double y = make_d(p->ylo, p->yhi);
		double want = make_d(p->rlo, p->rhi);
		double got2 = pow(x, y);
		double u = ulps(got2, want);
		int be;
		double allow;
		np++;
		(void) frexp(want, &be);
		allow = (be < 0 ? -be : be) + 6.0;
		if (u > worst_ulp)
			worst_ulp = u;
		if (u / allow > worst)
			worst = u / allow;
		if (u > allow) {
			badp++;
			printf("FAIL pow row %d x=%.17g y=%.17g got %.17g want %.17g (%.2f ulp, allow %.0f)\n",
			       ii, x, y, got2, want, u, allow);
		}
	}
	printf("pow: %d rows, %d over the bound, worst observed %.1f ulp, worst ratio observed/bound %.3f\n",
	       np, badp, worst_ulp, worst);
	CHECK("pow within the analytic bound", badp == 0);
	CHECK("pow 2^10", pow(2.0, 10.0) == 1024.0);
	CHECK("pow 2^-1", pow(2.0, -1.0) == 0.5);
	CHECK("pow (-2)^3", pow(-2.0, 3.0) == -8.0);
	CHECK("pow 0^0 = 1", pow(0.0, 0.0) == 1.0);

	return finish("FMATH");
}
