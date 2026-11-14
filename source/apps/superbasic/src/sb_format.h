/*
 * sb_format.h - conversion between the numeric and string forms of values.
 *
 * Output rules implemented here were measured against the JS ROM
 * (see docs/JS-SUPERBASIC-MAP.md, section "Numeric output"):
 *   - the value is rounded to 7 significant digits, ties rounding away
 *     from zero;
 *   - fixed notation is used when 0.1 <= |v| < 1E6 after rounding,
 *     otherwise exponent notation "d[.ddd]E[-]n";
 *   - no leading zero before the point ("  .5"), no trailing zeros,
 *     no "+" in exponents, no padding.
 */
#ifndef SB_FORMAT_H
#define SB_FORMAT_H

#include <stddef.h>

/* Format @a v into @a buf (at least 32 bytes).  Result is NUL-terminated. */
void sb_format_number(double v, char *buf, size_t size);

/* Round @a x to the precision of the QL 6-byte float: a 32-bit mantissa
 * (sign plus 31 bits) and a binary exponent.  Verified: the ROM's constant
 * for pi, exponent word 0x0802 and mantissa 0x6487ED51, is this rounding of
 * pi.  Used for numeric literals, string-to-number conversion and the
 * results of arithmetic. */
double sb_ql_quantise(double x);

/* Parse the whole of s[0..n-1] (leading/trailing blanks allowed) as a
 * number.  Returns 1 on success, 0 if the text is not numeric. */
int sb_parse_number(const char *s, size_t n, double *out);

/* Leading-number form used for strings in arithmetic: reads the longest
 * numeric prefix and ignores the rest.  Returns 0 if there is no number. */
int sb_parse_prefix(const char *s, size_t n, double *out);

#endif /* SB_FORMAT_H */
