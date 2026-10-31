/* float.h - characteristics of floating types (R11C).
 * The historical GCC 1.40 target has float = IEEE single (4 bytes), double =
 * IEEE double (8 bytes), and long double = double (8 bytes, no 80-bit type).
 * The x87 has more precision internally; the C types do not.  These values
 * describe the C types, and are checked by the R11C tests. */
#ifndef QD_FLOAT_H
#define QD_FLOAT_H

#define FLT_RADIX          2
/* 1 = rounds to nearest.  The R11C runtime keeps the x87 control word at
 * round-to-nearest and 53-bit precision during library calls. */
#define FLT_ROUNDS         1

#define FLT_MANT_DIG       24
#define FLT_DIG            6
#define FLT_MIN_EXP        (-125)
#define FLT_MIN_10_EXP     (-37)
#define FLT_MAX_EXP        128
#define FLT_MAX_10_EXP     38
#define FLT_MAX            3.40282346638528860e+38F
#define FLT_MIN            1.17549435082228751e-38F
#define FLT_EPSILON        1.19209289550781250e-07F

#define DBL_MANT_DIG       53
#define DBL_DIG            15
#define DBL_MIN_EXP        (-1021)
#define DBL_MIN_10_EXP     (-307)
#define DBL_MAX_EXP        1024
#define DBL_MAX_10_EXP     308
#define DBL_MAX            1.79769313486231570815e+308
#define DBL_MIN            2.22507385850720138309e-308
#define DBL_EPSILON        2.22044604925031308085e-16

/* long double is the same type as double on this compiler (R11C finding). */
#define LDBL_MANT_DIG      DBL_MANT_DIG
#define LDBL_DIG           DBL_DIG
#define LDBL_MIN_EXP       DBL_MIN_EXP
#define LDBL_MIN_10_EXP    DBL_MIN_10_EXP
#define LDBL_MAX_EXP       DBL_MAX_EXP
#define LDBL_MAX_10_EXP    DBL_MAX_10_EXP
#define LDBL_MAX           DBL_MAX
#define LDBL_MIN           DBL_MIN
#define LDBL_EPSILON       DBL_EPSILON

#endif
