/*
 * sb_value.h - run-time values of SuperBASIC expressions.
 *
 * A value is either a number (stored as a C double in this SB1 prototype;
 * see docs/KNOWN-LIMITATIONS.md for how this differs from the QL 6-byte
 * float), a string (heap-owned), or UNDEF, the value of a variable that has
 * never been assigned.  PRINT shows UNDEF as "*"; using it in an operation
 * raises "error in expression" (both verified against the ROM).
 */
#ifndef SB_VALUE_H
#define SB_VALUE_H

#include <stddef.h>

#include "sb_mem.h"

#define SB_VAL_NUM   0
#define SB_VAL_STR   1
#define SB_VAL_UNDEF 2

typedef struct SbValue {
    int type;
    double num;
    char *str;      /* owned when type == SB_VAL_STR */
    size_t len;
} SbValue;

void sb_val_init_num(SbValue *v, double d);
void sb_val_init_undef(SbValue *v);

/* Set @a v to a copy of the @a n bytes at @a s.  @a v must not own a string.
 * Returns SB_OK or SB_E_OUT_OF_MEMORY. */
int sb_val_init_str(SbAlloc *a, SbValue *v, const char *s, size_t n);

/* Release any string owned by @a v and reset it to number zero. */
void sb_val_free(SbAlloc *a, SbValue *v);

/* Numeric view of a value: strings must be numeric (else SB_E_EXPRESSION),
 * UNDEF is an error. */
int sb_val_to_number(const SbValue *v, double *out);

/* String view of a value: numbers are formatted as PRINT would show them.
 * The result is written into @a out (which must not own a string). */
int sb_val_to_string(SbAlloc *a, const SbValue *v, SbValue *out);

#endif /* SB_VALUE_H */
