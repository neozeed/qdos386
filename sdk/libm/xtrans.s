/ xtrans.s - x87 kernels for the R11D transcendental functions.  Historical
/ GAS 1.38 syntax, cdecl (arguments on the stack, result in ST0).  Each routine
/ sets the control word to PC=64, RC=nearest for its work, stores its single
/ result to a double (one rounding) and restores the caller's control word.
/ The x87 exception flags are cleared before return.
/
/ qd_x_exp(double x, int *st):      e^x.   st = 1 if x*log2e > 1025, -1 if < -1090.
/ qd_x_log(double x, int base):     base 0: ln x; base 1: log10 x.  x > 0 only.
/ qd_x_trig(double x, int op, int *st):  op 0 sin, 1 cos, 2 tan.  st = 1 when
/                                    |x| >= 2^63 (the x87 cannot reduce it).
/ qd_x_atan2(double y, double x):   atan(y/x) with the quadrant of (x, y).
/ (fmod is computed in C, trans.c: fprem is not exact on emulators without
/ true 80-bit remainder, so the exact subtraction method is used instead.)

	.globl _qd_x_exp
	.align 2
_qd_x_exp:
	pushl %ebp
	movl %esp,%ebp
	subl $16,%esp
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax
	orw $0x0300,%ax
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	fldl 8(%ebp)                    /* x */
	fldl2e                          /* log2e, x */
	fmulp %st,%st(1)                /* t = x*log2e */
	fld %st(0)                      /* t, t */
	fldl XLIMHI                     /* 1025, t, t */
	fcompp                          /* t */
	fnstsw %ax
	sahf
	jb Lx_over
	fld %st(0)
	fldl XLIMLO                     /* -1090, t, t */
	fxch %st(1)                     /* t, -1090, t */
	fcompp                          /* t */
	fnstsw %ax
	sahf
	jb Lx_under
	fld %st(0)                      /* t, t */
	frndint                         /* n, t */
	fxch %st(1)                     /* t, n */
	fsub %st(1),%st                 /* f, n */
	f2xm1                           /* 2^f - 1, n */
	fld1
	faddp %st,%st(1)                /* 2^f, n */
	fscale                          /* 2^t, n */
	fstp %st(1)
	fstpl -12(%ebp)
	fnclex
	movl 16(%ebp),%ecx
	movl $0,(%ecx)
	fldl -12(%ebp)
	jmp Lx_done
Lx_over:
	fstp %st(0)
	fnclex
	movl 16(%ebp),%ecx
	movl $1,(%ecx)
	fldz
	jmp Lx_done
Lx_under:
	fstp %st(0)
	fnclex
	movl 16(%ebp),%ecx
	movl $-1,(%ecx)
	fldz
Lx_done:
	fldcw -2(%ebp)
	leave
	ret

	.globl _qd_x_log
	.align 2
_qd_x_log:
	pushl %ebp
	movl %esp,%ebp
	subl $16,%esp
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax
	orw $0x0300,%ax
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	cmpl $0,16(%ebp)                /* base is the second argument (x is 8..15) */
	jne Llog10
	fldln2
	jmp Llogdo
Llog10:
	fldlg2
Llogdo:
	fldl 8(%ebp)                    /* x, c */
	fyl2x                           /* c * log2(x) */
	fstpl -12(%ebp)
	fnclex
	fldl -12(%ebp)
	fldcw -2(%ebp)
	leave
	ret

	.globl _qd_x_trig
	.align 2
_qd_x_trig:
	pushl %ebp
	movl %esp,%ebp
	subl $16,%esp
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax
	orw $0x0300,%ax
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	fldl 8(%ebp)                    /* x */
	movl 16(%ebp),%eax
	cmpl $1,%eax
	je Lcos
	cmpl $2,%eax
	je Ltan
	fsin
	jmp Ltrigchk
Lcos:
	fcos
	jmp Ltrigchk
Ltan:
	fptan                           /* 1, tan */
	fstp %st(0)                     /* tan */
Ltrigchk:
	fnstsw %ax
	movl 20(%ebp),%ecx
	testw $0x400,%ax               /* C2: argument not reduced */
	jne Ltrigbig
	movl $0,(%ecx)
	fstpl -12(%ebp)
	fnclex
	fldl -12(%ebp)
	jmp Ltrigdone
Ltrigbig:
	movl $1,(%ecx)
	fstp %st(0)
	fnclex
	fldz
Ltrigdone:
	fldcw -2(%ebp)
	leave
	ret

	.globl _qd_x_atan2
	.align 2
_qd_x_atan2:
	pushl %ebp
	movl %esp,%ebp
	subl $16,%esp
	fstcw -2(%ebp)
	movw -2(%ebp),%ax
	andw $0xf0ff,%ax
	orw $0x0300,%ax
	movw %ax,-4(%ebp)
	fldcw -4(%ebp)
	fldl 8(%ebp)                    /* y */
	fldl 16(%ebp)                   /* x, y */
	fpatan                          /* atan(y/x) with quadrant */
	fstpl -12(%ebp)
	fnclex
	fldl -12(%ebp)
	fldcw -2(%ebp)
	leave
	ret

	.align 2
XLIMHI:
	.double 0d1.025000000000000000000e+03
XLIMLO:
	.double 0d-1.090000000000000000000e+03
