/* strtod.c - C89 strtod and atof (R11C), C locale only.
 *
 * Grammar: optional white space, optional sign, digits with an optional
 * point, optional exponent (e or E, optional sign, digits).  The exponent
 * is consumed only if it has digits.  No hexadecimal, no infinity, no NaN.
 *
 * Conversion: the significant digits are kept exactly (up to 801 of them;
 * a non-zero digit beyond that is recorded as a sticky digit, so rounding
 * still sees it).  The value is then rounded to the nearest double with
 * exact integer arithmetic in fpbig.c (round-half-even).  Errors: ERANGE
 * for overflow (the result is HUGE_VAL, which is DBL_MAX here), and for a
 * result that is zero or subnormal from non-zero input. */
#include <stdlib.h>
#include <errno.h>
#include "fpbig.h"

#define MAXSIG 801

static int sd_space(int c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

static int sd_digit(int c)
{
	return c >= '0' && c <= '9';
}

struct sdig {
	char dig[MAXSIG + 2];
	int nd;          /* significant digits kept */
	int sticky;      /* a non-zero digit was dropped beyond MAXSIG */
	long intdrop;    /* integer-part digits dropped beyond MAXSIG */
};

/* Returns 1 when the digit was dropped (beyond MAXSIG), 0 when it was kept
 * or is a leading zero.  A dropped digit does not move the decimal point of
 * the kept digits, so only dropped integer digits are counted (intdrop). */
static int sd_add(struct sdig *st, int c, int isint)
{
	if (st->nd == 0 && c == '0')
		return 0;                  /* leading zeros are not significant */
	if (st->nd < MAXSIG) {
		st->dig[st->nd++] = (char) c;
		return 0;
	}
	if (c != '0')
		st->sticky = 1;
	if (isint)
		st->intdrop++;
	return 1;
}

double strtod(const char *s, char **end)
{
	const char *p = s;
	struct sdig st;
	int neg = 0, any = 0, edig = 0;
	long fracd = 0, expv = 0, exp10;
	double r;
	int err;

	st.nd = 0;
	st.sticky = 0;
	st.intdrop = 0;
	while (sd_space((unsigned char) *p))
		p++;
	if (*p == '+')
		p++;
	else if (*p == '-') {
		neg = 1;
		p++;
	}
	while (sd_digit((unsigned char) *p)) {
		any = 1;
		(void) sd_add(&st, *p, 1);
		p++;
	}
	if (*p == '.') {
		p++;
		while (sd_digit((unsigned char) *p)) {
			any = 1;
			if (!sd_add(&st, *p, 0))
				fracd++;       /* kept or leading zero: counted in the exponent */
			p++;
		}
	}
	if (!any) {
		if (end != NULL)
			*end = (char *) s;
		return 0.0;
	}
	if (*p == 'e' || *p == 'E') {
		const char *q = p + 1;
		int eneg = 0;
		long ev = 0;
		if (*q == '+')
			q++;
		else if (*q == '-') {
			eneg = 1;
			q++;
		}
		while (sd_digit((unsigned char) *q)) {
			if (ev < 100000000L)
				ev = ev * 10 + (*q - '0');
			edig = 1;
			q++;
		}
		if (edig) {
			expv = eneg ? -ev : ev;
			p = q;
		}
	}
	if (end != NULL)
		*end = (char *) p;

	if (st.nd == 0)
		return neg ? -0.0 : 0.0;   /* all digits zero */
	if (st.sticky) {
		st.dig[st.nd++] = '1';     /* sits below every kept digit */
		fracd++;
	}
	exp10 = expv - fracd + st.intdrop;
	/* The clamp must not cut the digit count out of the magnitude: qd_fp_from_decimal
	 * decides overflow and underflow from ndig + exp10 and does its bignum work
	 * only inside that range, so a wide clamp is enough. */
	if (exp10 > 1000000L)
		exp10 = 1000000L;
	if (exp10 < -1000000L)
		exp10 = -1000000L;
	err = qd_fp_from_decimal(st.dig, st.nd, (int) exp10, &r);
	if (err)
		errno = ERANGE;
	return neg ? -r : r;
}

double atof(const char *s)
{
	return strtod(s, NULL);
}
