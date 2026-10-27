/* math.h - the R11C libm subset (libm.a).  Only the functions listed here
 * exist.  There is no infinity or NaN: HUGE_VAL is DBL_MAX (C89 allows an
 * implementation to choose it) and domain or range errors set errno to EDOM
 * or ERANGE and return 0 or HUGE_VAL as documented in docs/R11C-LIBM-MAP.md.
 * The functions are not all correctly rounded; each one's accuracy is stated
 * in docs/R11D-LIBM-MAP.md (the R11D functions) and docs/R11C-LIBM-MAP.md. */
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
double exp(double x);
double log(double x);
double log10(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double fmod(double x, double y);

#endif
