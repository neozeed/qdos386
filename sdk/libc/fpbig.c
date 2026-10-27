/* fpbig.c - exact conversions between binary doubles and decimal digits (R11C).
 *
 * Integers are arrays of 16-bit limbs (least significant first) held in
 * unsigned long, so that every product fits in 32 bits and no 64-bit
 * arithmetic is needed (GCC 1.40 would call helpers for it).  All storage
 * is static; the library is not reentrant, which the QDOS/386 job model
 * does not require (each job has its own copy of libc).
 *
 * qd_fp_exact turns a double into its exact decimal expansion, from which
 * printf rounds %f, %e and %g with the exact value.  qd_fp_from_decimal
 * turns a decimal string into the nearest double, by exact integer
 * arithmetic and a restoring division: no rounding error from the x87.
 * Both are verified against Python's exact conversions by the R11C tests. */
#include <float.h>
#include <errno.h>
#include "fpbig.h"

#define BIG_LIMBS 260
#define MANT_BITS 53

typedef struct {
	int n;                          /* limbs in use; 0 means zero */
	unsigned long d[BIG_LIMBS];
} big;

static big bn;                      /* numerator */
static big bm;                      /* denominator */
static big bt;                      /* shifted denominator */
static big bd;                      /* decimal conversion scratch */
static int big_overflow;

static void big_norm(big *a)
{
	while (a->n > 0 && a->d[a->n - 1] == 0)
		a->n--;
}

static void big_set(big *a, unsigned long v)
{
	a->n = 0;
	while (v != 0) {
		a->d[a->n++] = v & 0xffffUL;
		v >>= 16;
	}
}

/* a *= m, for 0 < m < 65536 */
static void big_mul_small(big *a, unsigned long m)
{
	unsigned long carry = 0;
	int i;

	for (i = 0; i < a->n; i++) {
		unsigned long t = a->d[i] * m + carry;
		a->d[i] = t & 0xffffUL;
		carry = t >> 16;
	}
	while (carry != 0) {
		if (a->n >= BIG_LIMBS) {
			big_overflow = 1;
			return;
		}
		a->d[a->n++] = carry & 0xffffUL;
		carry >>= 16;
	}
}

static void big_mul_pow2(big *a, int k)
{
	while (k >= 15) {
		big_mul_small(a, 32768UL);
		k -= 15;
	}
	if (k > 0)
		big_mul_small(a, 1UL << k);
}

static void big_mul_pow5(big *a, int k)
{
	while (k >= 6) {
		big_mul_small(a, 15625UL);
		k -= 6;
	}
	if (k > 0) {
		unsigned long p = 1;
		while (k-- > 0)
			p *= 5;
		big_mul_small(a, p);
	}
}

static void big_mul_pow10(big *a, int k)
{
	while (k >= 4) {
		big_mul_small(a, 10000UL);
		k -= 4;
	}
	if (k > 0) {
		unsigned long p = 1;
		while (k-- > 0)
			p *= 10;
		big_mul_small(a, p);
	}
}

/* a += v, for 0 <= v < 2^32 */
static void big_add_ulong(big *a, unsigned long v)
{
	unsigned long carry = v;
	int i = 0;

	while (carry != 0) {
		unsigned long t;
		if (i == a->n) {
			if (i >= BIG_LIMBS) {
				big_overflow = 1;
				return;
			}
			a->d[a->n++] = 0;
		}
		t = a->d[i] + (carry & 0xffffUL);
		carry >>= 16;
		a->d[i] = t & 0xffffUL;
		carry += t >> 16;
		i++;
	}
}

static int big_cmp(const big *a, const big *b)
{
	int i;

	if (a->n != b->n)
		return a->n < b->n ? -1 : 1;
	for (i = a->n - 1; i >= 0; i--)
		if (a->d[i] != b->d[i])
			return a->d[i] < b->d[i] ? -1 : 1;
	return 0;
}

/* a -= b, requires a >= b */
static void big_sub(big *a, const big *b)
{
	long borrow = 0;
	int i;

	for (i = 0; i < a->n; i++) {
		long t = (long) a->d[i] - borrow - (i < b->n ? (long) b->d[i] : 0L);
		if (t < 0) {
			t += 65536L;
			borrow = 1;
		} else {
			borrow = 0;
		}
		a->d[i] = (unsigned long) t;
	}
	big_norm(a);
}

/* a >>= 1 */
static void big_shr1(big *a)
{
	int i;

	for (i = 0; i < a->n; i++) {
		unsigned long up = (i + 1 < a->n) ? (a->d[i + 1] & 1UL) : 0UL;
		a->d[i] = (a->d[i] >> 1) | (up << 15);
	}
	big_norm(a);
}

static int big_bits(const big *a)
{
	unsigned long top;
	int b;

	if (a->n == 0)
		return 0;
	top = a->d[a->n - 1];
	b = (a->n - 1) * 16;
	while (top != 0) {
		b++;
		top >>= 1;
	}
	return b;
}

/* a /= dv (0 < dv < 65536); returns the remainder */
static unsigned long big_divmod(big *a, unsigned long dv)
{
	unsigned long rem = 0;
	int i;

	for (i = a->n - 1; i >= 0; i--) {
		unsigned long cur = (rem << 16) + a->d[i];
		a->d[i] = cur / dv;
		rem = cur % dv;
	}
	big_norm(a);
	return rem;
}

/* Decimal digits of a (a is not changed) into out; returns the count.
 * out needs room for 800 characters. */
static int big_to_dec(const big *a, char *out)
{
	unsigned long chunks[256];
	int nc = 0, len = 0, i;

	if (a->n == 0) {
		out[0] = '0';
		return 1;
	}
	bd = *a;
	while (bd.n > 0)
		chunks[nc++] = big_divmod(&bd, 10000UL);
	/* the top chunk without leading zeros, the others in four digits */
	{
		unsigned long v = chunks[nc - 1];
		char tmp[8];
		int k = 0;
		do {
			tmp[k++] = (char) ('0' + v % 10);
			v /= 10;
		} while (v != 0);
		while (k > 0)
			out[len++] = tmp[--k];
	}
	for (i = nc - 2; i >= 0; i--) {
		unsigned long v = chunks[i];
		out[len + 3] = (char) ('0' + v % 10); v /= 10;
		out[len + 2] = (char) ('0' + v % 10); v /= 10;
		out[len + 1] = (char) ('0' + v % 10); v /= 10;
		out[len + 0] = (char) ('0' + v % 10);
		len += 4;
	}
	return len;
}

void qd_fp_exact(double x, char *dig, int *ndig, int *dp)
{
	union {
		double d;
		unsigned long w[2];        /* w[0] low word, w[1] high word */
	} u;
	unsigned long hi, lo, mhi;
	int E, e2, len;

	u.d = x;
	hi = u.w[1];
	lo = u.w[0];
	E = (int) ((hi >> 20) & 0x7ffUL);
	mhi = hi & 0xfffffUL;
	if (E == 0) {
		e2 = -1074;                /* subnormal: no hidden bit */
	} else {
		mhi |= 0x100000UL;
		e2 = E - 1075;
	}
	/* m = mhi * 2^32 + lo, the 53-bit significand */
	bn.d[0] = lo & 0xffffUL;
	bn.d[1] = lo >> 16;
	bn.d[2] = mhi & 0xffffUL;
	bn.d[3] = mhi >> 16;
	bn.n = 4;
	big_norm(&bn);
	big_overflow = 0;
	if (e2 >= 0) {
		big_mul_pow2(&bn, e2);
		len = big_to_dec(&bn, dig);
		*dp = len;
	} else {
		big_mul_pow5(&bn, -e2);
		len = big_to_dec(&bn, dig);
		*dp = len + e2;
	}
	*ndig = len;
}

/* 2^k * x, exact when the result is representable (the callers ensure it):
 * each step moves monotonically toward the result, so every intermediate
 * value is representable too. */
static double scale2(double x, int k)
{
	while (k > 0) {
		x = x * 2.0;
		k--;
	}
	while (k < 0) {
		x = x * 0.5;
		k++;
	}
	return x;
}

int qd_fp_from_decimal(const char *dig, int ndig, int exp10, double *out)
{
	int i, t, K, B, x, E2, top;
	unsigned char qb[64];
	int sticky, rbit, j;
	double dm;
	unsigned long mlo, mhi;

	if (ndig == 0) {
		*out = 0.0;
		return 0;
	}
	if (ndig + exp10 > 311) {
		*out = DBL_MAX;
		return ERANGE;
	}
	if (ndig + exp10 < -352) {
		*out = 0.0;
		return ERANGE;
	}

	/* N = dig * 10^exp10 (if exp10 >= 0), M = 10^-exp10 otherwise */
	big_overflow = 0;
	bn.n = 0;
	i = 0;
	while (i < ndig) {
		unsigned long chunk = 0, mul = 1;
		int c = 0;
		while (c < 4 && i < ndig) {
			chunk = chunk * 10 + (unsigned long) (dig[i] - '0');
			mul *= 10;
			i++;
			c++;
		}
		big_mul_small(&bn, mul);
		big_add_ulong(&bn, chunk);
	}
	bm.n = 0;
	big_set(&bm, 1UL);
	if (exp10 >= 0)
		big_mul_pow10(&bn, exp10);
	else
		big_mul_pow10(&bm, -exp10);

	/* shift so that bits(N') - bits(M') = MANT_BITS + 2 */
	t = (MANT_BITS + 2) - (big_bits(&bn) - big_bits(&bm));
	if (t >= 0)
		big_mul_pow2(&bn, t);
	else
		big_mul_pow2(&bm, -t);

	/* restoring division: Q = floor(N'/M'), bits 0..K */
	K = MANT_BITS + 3;
	bt = bm;
	big_mul_pow2(&bt, K);
	for (j = 0; j <= K; j++)
		qb[j] = 0;
	for (j = K; j >= 0; j--) {
		if (big_cmp(&bn, &bt) >= 0) {
			big_sub(&bn, &bt);
			qb[j] = 1;
		}
		big_shr1(&bt);
	}
	if (big_overflow) {
		*out = DBL_MAX;
		return ERANGE;
	}
	sticky = bn.n != 0;            /* remainder: the value is above Q */
	B = 0;
	for (j = K; j >= 0; j--)
		if (qb[j]) {
			B = j + 1;
			break;
		}

	/* keep the top MANT_BITS bits, or fewer when the result is subnormal:
	 * the quantum is 2^-1074 */
	x = B - MANT_BITS;
	if (x < t - 1074)
		x = t - 1074;
	for (j = 0; j < x - 1; j++)
		if (qb[j])
			sticky = 1;
	rbit = (x - 1 < B) ? qb[x - 1] : 0;

	mlo = 0;
	mhi = 0;
	for (j = x; j < B; j++)
		if (qb[j]) {
			if (j - x < 32)
				mlo |= 1UL << (j - x);
			else
				mhi |= 1UL << (j - x - 32);
		}
	dm = (double) mhi * 4294967296.0 + (double) mlo;
	if (rbit && (sticky || (mlo & 1UL)))
		dm = dm + 1.0;

	E2 = x - t;
	top = 0;
	{
		double v = dm;
		while (v >= 2.0) {
			v = v * 0.5;
			top++;
		}
	}
	/* dm * 2^E2 has its leading bit at 2^(top + E2) */
	if (dm == 0.0) {
		*out = 0.0;
		return ERANGE;
	}
	if (top + E2 > 1023) {
		*out = DBL_MAX;
		return ERANGE;
	}
	*out = scale2(dm, E2);
	if (top + E2 < -1022)
		return ERANGE;
	return 0;
}
