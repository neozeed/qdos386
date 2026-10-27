/* math.h - the R11C libm subset (libm.a).  Only the functions listed here
 * exist.  There is no infinity or NaN: HUGE_VAL is DBL_MAX (C89 allows an
 * implementation to choose it) and domain or range errors set errno to EDOM
 * or ERANGE and return 0 or HUGE_VAL as documented in docs/R11C-LIBM-MAP.md.
 * The functions are not all correctly rounded; each one's accuracy is stated
 * in that document. */
#ifndef QD_MATH_H
#define QD_MATH_H

#define HUGE_VAL 1.79769313486231570815e+308

double floor(double x);
double ceil(double x);
double fabs(double x);
double frexp(double x, int *exp);
double ldexp(double x, int exp);
double modf(double x, double *ip);
double sqrt(double x);
double pow(double x, double y);

#endif
