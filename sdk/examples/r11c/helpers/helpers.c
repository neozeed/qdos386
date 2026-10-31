/* HELPERS: the compiler support routines for double <-> integer conversion
 * (libgcc-qd.a), tested through ordinary C conversions.  Inputs are volatile
 * so that GCC 1.40 does not fold the conversion at compile time.  The
 * unsigned long long to double conversion (___floatundidf) is NOT in
 * libgcc-qd.a; it is not tested here and its three LGTEST discrepancies are
 * documented elsewhere. */
#include <stdio.h>
#include "../check.h"

int main(void)
{
	volatile double d;
	volatile long long ll;
	volatile unsigned int ui;
	volatile int si;
	double e;

	/* ___floatdidf: exact for |x| <= 2^53, single rounding beyond */
	ll = 0LL;
	d = (double) ll;
	CHECK("floatdidf 0", d == 0.0);
	ll = 1LL;
	CHECK("floatdidf 1", (double) ll == 1.0);
	ll = -1LL;
	CHECK("floatdidf -1", (double) ll == -1.0);
	ll = 1LL << 53;
	CHECK("floatdidf 2^53 exact", (double) ll == 9007199254740992.0);
	ll = (1LL << 53) + 1LL;
	CHECK("floatdidf 2^53+1 rounds to even", (double) ll == 9007199254740992.0);
	ll = (1LL << 53) + 3LL;
	CHECK("floatdidf 2^53+3 rounds to 2^53+4", (double) ll == 9007199254740996.0);
	ll = 1LL << 62;
	CHECK("floatdidf 2^62", (double) ll == 4611686018427387904.0);
	ll = -(1LL << 62);
	CHECK("floatdidf -2^62", (double) ll == -4611686018427387904.0);
	ll = 1000000000000000000LL;
	CHECK("floatdidf 1e18", (double) ll == 1e18);
	ll = 0x7fffffffffffffffLL;
	CHECK("floatdidf LLONG_MAX rounds to 2^63", (double) ll == 9223372036854775808.0);
	ll = -0x7fffffffffffffffLL - 1LL;
	CHECK("floatdidf LLONG_MIN is -2^63", (double) ll == -9223372036854775808.0);
	ll = 4294967296LL;
	CHECK("floatdidf 2^32", (double) ll == 4294967296.0);
	ll = 123456789012345LL;
	CHECK("floatdidf 123456789012345", (double) ll == 123456789012345.0);

	/* ___fixdfdi: truncation toward zero */
	d = 3.9;
	CHECK("fixdfdi 3.9 -> 3", (long long) d == 3LL);
	d = -3.9;
	CHECK("fixdfdi -3.9 -> -3", (long long) d == -3LL);
	d = 0.5;
	CHECK("fixdfdi 0.5 -> 0", (long long) d == 0LL);
	d = -0.5;
	CHECK("fixdfdi -0.5 -> 0", (long long) d == 0LL);
	d = 1e18;
	CHECK("fixdfdi 1e18", (long long) d == 1000000000000000000LL);
	d = 4294967296.0 * 3.0;
	CHECK("fixdfdi 3*2^32", (long long) d == 12884901888LL);
	d = -4294967296.0 * 3.0;
	CHECK("fixdfdi -3*2^32", (long long) d == -12884901888LL);
	d = -9223372036854775808.0;
	CHECK("fixdfdi -2^63 exact", (long long) d == (-0x7fffffffffffffffLL - 1LL));
	d = 9007199254740991.0;
	CHECK("fixdfdi 2^53-1", (long long) d == 9007199254740991LL);

	/* ___fixunsdfsi: values in [0, 2^32) */
	d = 3.9;
	CHECK("fixunsdfsi 3.9 -> 3", (unsigned int) d == 3U);
	d = 2147483648.0;
	CHECK("fixunsdfsi 2^31", (unsigned int) d == 2147483648U);
	d = 4294967295.0;
	CHECK("fixunsdfsi 2^32-1", (unsigned int) d == 4294967295U);
	d = -0.5;
	CHECK("fixunsdfsi -0.5 -> 0", (unsigned int) d == 0U);

	/* ___fixdfsi: truncation toward zero, signed */
	d = -7.9;
	CHECK("fixdfsi -7.9 -> -7", (int) d == -7);
	d = 2000000000.0;
	CHECK("fixdfsi 2e9", (int) d == 2000000000);
	d = 2.0;
	si = (int) d;
	CHECK("fixdfsi 2", si == 2);

	/* round trip through the helpers */
	ll = 1234567890123LL;
	e = (double) ll;
	CHECK("round trip floatdidf/fixdfdi", (long long) e == 1234567890123LL);

	return finish("HELPERS");
}
