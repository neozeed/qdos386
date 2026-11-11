/* fpbig.h - internal interface of the exact binary/decimal conversions
 * (fpbig.c).  Not part of the public headers. */
#ifndef QD_FPBIG_H
#define QD_FPBIG_H

/* Exact decimal expansion of x (finite, >= 0, not zero).  The digits of x
 * are written to dig (no leading zeros; room for 800 characters), with
 * *ndig digits and decimal point position *dp: x = 0.d1d2... * 10^dp.
 * The expansion is exact: no rounding is done here. */
void qd_fp_exact(double x, char *dig, int *ndig, int *dp);

/* Correctly rounded (round-half-even) value of 0.DIG * 10^(ndig+exp10) with
 * ndig significant decimal digits in dig (no leading zero, 1 <= ndig <= 801).
 * Returns 0, or ERANGE when the result overflows (*out = DBL_MAX) or is zero
 * or subnormal (*out = the rounded value). */
int qd_fp_from_decimal(const char *dig, int ndig, int exp10, double *out);

#endif
