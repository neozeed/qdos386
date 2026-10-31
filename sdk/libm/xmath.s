/ xmath.s - x87 kernels for libm (R11C).  Historical GAS 1.38 syntax, the
/ same conventions as sdk/lib/qtrap.s.  Each routine sets its own x87
/ control word and restores the caller's before it returns.
/ Arguments are on the stack (cdecl); a double result is left in ST(0).
/
/ qd_sqrt_raw(double x): correctly rounded square root for x >= 0.
/
/ qd_pow_core(double ax, double y, int *st): ax > 0.  Computes
/   ax^y = 2^(y*log2 ax) with every intermediate in the x87 extended format
/   (64-bit significand, control word PC=64, RC nearest), so the only rounding
/   of the result is the final store to double.  *st is 0 for a result inside
/   the range of the computation, +1 when y*log2(ax) > 1025 (certain overflow)
/   and -1 when y*log2(ax) < -1090 (certain underflow to zero).  The returned
/   double is 0 in the -1 case and a value the caller must check for overflow
/   (it may be +inf) in the +1 case.  The x87 exception flags are cleared.

	.globl _qd_sqrt_raw
	.align 2
_qd_sqrt_raw:
	pushl %ebp
	movl %esp,%ebp
	subl $8,%esp
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax                /* clear precision and rounding control */
	orw $0x0200,%ax                 /* 53-bit precision, round to nearest */
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	fldl 8(%ebp)
	fsqrt
	fldcw -2(%ebp)
	leave
	ret

	.globl _qd_pow_core
	.align 2
_qd_pow_core:
	pushl %ebp
	movl %esp,%ebp
	subl $16,%esp                   /* -2,-4: control words; -12: result */
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax
	orw $0x0300,%ax                 /* 64-bit significand, round to nearest */
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	fldl 16(%ebp)                   /* y */
	fldl 8(%ebp)                    /* ax, y */
	fyl2x                           /* t = y * log2(ax) */
	fld %st(0)                      /* t, t */
	fldl LIMHI                      /* 1025, t, t */
	fcompp                          /* t: compares 1025 with t */
	fnstsw %ax
	sahf
	jb Lover                        /* t > 1025 */
	fld %st(0)                      /* t, t */
	fldl LIMLO                      /* -1090, t, t */
	fxch %st(1)                     /* t, -1090, t */
	fcompp                          /* compares t with -1090; leaves t */
	fnstsw %ax
	sahf
	jb Lunder                       /* t < -1090 */
	fld %st(0)                      /* t, t */
	frndint                         /* n, t */
	fxch %st(1)                     /* t, n */
	fsub %st(1),%st                 /* f = t - n, n   (|f| <= 1/2) */
	f2xm1                           /* 2^f - 1, n */
	fld1                            /* 1, 2^f - 1, n */
	faddp %st,%st(1)                /* 2^f, n */
	fscale                          /* 2^f * 2^n, n */
	fstp %st(1)                     /* 2^t */
	fstpl -12(%ebp)                 /* the one rounding to double */
	fnclex
	movl 24(%ebp),%ecx
	movl $0,(%ecx)
	fldl -12(%ebp)
	jmp Ldone
Lover:
	fstp %st(0)
	fnclex
	movl 24(%ebp),%ecx
	movl $1,(%ecx)
	fldz
	jmp Ldone
Lunder:
	fstp %st(0)
	fnclex
	movl 24(%ebp),%ecx
	movl $-1,(%ecx)
	fldz
Ldone:
	fldcw -2(%ebp)
	leave
	ret

	.align 2
LIMHI:
	.double 0d1.025000000000000000000e+03
LIMLO:
	.double 0d-1.090000000000000000000e+03
