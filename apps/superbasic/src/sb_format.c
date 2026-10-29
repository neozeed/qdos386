/*
 * sb_format.c - numeric output and numeric-string parsing.
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sb_format.h"

double sb_ql_quantise(double x)
{
    int ex;
    double m;
    double scaled;

    if (x == 0.0) {
        return 0.0;
    }
    m = frexp(x, &ex);                    /* x = m * 2^ex, 0.5 <= |m| < 1 */
    scaled = m * 2147483648.0;            /* m * 2^31: a 31-bit magnitude */
    if (scaled < 0.0) {
        scaled = -floor(-scaled + 0.5);
    } else {
        scaled = floor(scaled + 0.5);
    }
    return ldexp(scaled, ex - 31);
}

void sb_format_number(double v, char *buf, size_t size)
{
    char tmp[64];
    char digits[20];
    char *p;
    int neg;
    double a;
    int exp10;
    int i;
    int n;
    int carry;
    int intlen;
    size_t pos;

    if (size < 32) {
        if (size > 0) {
            buf[0] = '\0';
        }
        return;
    }
    if (v == 0.0) {
        strcpy(buf, "0");
        return;
    }

    neg = (v < 0.0);
    a = neg ? -v : v;

    /* 17 significant digits, then round to 7 by hand so that ties round
     * away from zero (the ROM rounds 1234567.5 to 1.234568E6). */
    sprintf(tmp, "%.16e", a);
    digits[0] = tmp[0];
    for (i = 0; i < 16; i++) {
        digits[1 + i] = tmp[2 + i];
    }
    digits[17] = '\0';
    p = strchr(tmp, 'e');
    exp10 = atoi(p + 1);

    if (digits[7] >= '5') {
        carry = 1;
        for (i = 6; i >= 0 && carry; i--) {
            if (digits[i] == '9') {
                digits[i] = '0';
            } else {
                digits[i]++;
                carry = 0;
            }
        }
        if (carry) {
            digits[0] = '1';
            for (i = 1; i < 7; i++) {
                digits[i] = '0';
            }
            exp10++;
        }
    }
    n = 7;
    while (n > 1 && digits[n - 1] == '0') {
        n--;
    }
    digits[n] = '\0';

    pos = 0;
    if (neg) {
        buf[pos++] = '-';
    }

    if (exp10 >= 6 || exp10 <= -2) {
        /* exponent notation: d[.ddd]E[-]n */
        buf[pos++] = digits[0];
        if (n > 1) {
            buf[pos++] = '.';
            for (i = 1; i < n; i++) {
                buf[pos++] = digits[i];
            }
        }
        sprintf(buf + pos, "E%d", exp10);
        return;
    }

    if (exp10 >= 0) {
        /* integer part (zero padded), then optional fraction */
        intlen = exp10 + 1;
        for (i = 0; i < intlen; i++) {
            buf[pos++] = (i < n) ? digits[i] : '0';
        }
        if (n > intlen) {
            buf[pos++] = '.';
            for (i = intlen; i < n; i++) {
                buf[pos++] = digits[i];
            }
        }
        buf[pos] = '\0';
        return;
    }

    /* exp10 == -1: 0.1 <= |v| < 1, written without the leading zero */
    buf[pos++] = '.';
    for (i = 0; i < n; i++) {
        buf[pos++] = digits[i];
    }
    buf[pos] = '\0';
}

int sb_parse_number(const char *s, size_t n, double *out)
{
    char buf[64];
    size_t i;
    size_t end;
    size_t k;
    size_t len;
    int nd;
    int ne;
    char *endp;

    i = 0;
    while (i < n && isspace((unsigned char)s[i])) {
        i++;
    }
    end = n;
    while (end > i && isspace((unsigned char)s[end - 1])) {
        end--;
    }
    if (end == i) {
        return 0;
    }
    len = end - i;
    if (len >= sizeof(buf)) {
        return 0;
    }

    /* grammar: [+|-] digits [. digits] [E [+|-] digits] */
    k = i;
    if (k < end && (s[k] == '+' || s[k] == '-')) {
        k++;
    }
    nd = 0;
    while (k < end && isdigit((unsigned char)s[k])) {
        k++;
        nd++;
    }
    if (k < end && s[k] == '.') {
        k++;
        while (k < end && isdigit((unsigned char)s[k])) {
            k++;
            nd++;
        }
    }
    if (nd == 0) {
        return 0;
    }
    if (k < end && (s[k] == 'E' || s[k] == 'e')) {
        k++;
        if (k < end && (s[k] == '+' || s[k] == '-')) {
            k++;
        }
        ne = 0;
        while (k < end && isdigit((unsigned char)s[k])) {
            k++;
            ne++;
        }
        if (ne == 0) {
            return 0;
        }
    }
    if (k != end) {
        return 0;
    }

    memcpy(buf, s + i, len);
    buf[len] = '\0';
    *out = sb_ql_quantise(strtod(buf, &endp));
    return 1;
}

/* Leading-number conversion used when a string takes part in arithmetic:
 * blanks are skipped, the longest numeric prefix is read and any text after
 * it is ignored ("3x" and "3-5" give 3, "  12" gives 12).  Fails when there
 * is no digit at all, or when an E has no exponent digits ("3E" fails).
 * Measured against the ROM (corpus 26). */
int sb_parse_prefix(const char *s, size_t n, double *out)
{
    char buf[64];
    size_t i = 0;
    size_t k;
    size_t len;
    int nd = 0;
    char *endp;

    while (i < n && isspace((unsigned char)s[i])) {
        i++;
    }
    k = i;
    if (k < n && (s[k] == '+' || s[k] == '-')) {
        k++;
    }
    while (k < n && isdigit((unsigned char)s[k])) {
        k++;
        nd++;
    }
    if (k < n && s[k] == '.') {
        k++;
        while (k < n && isdigit((unsigned char)s[k])) {
            k++;
            nd++;
        }
    }
    if (nd == 0) {
        return 0;
    }
    if (k < n && (s[k] == 'E' || s[k] == 'e')) {
        size_t q = k + 1;
        int ne = 0;
        if (q < n && (s[q] == '+' || s[q] == '-')) {
            q++;
        }
        while (q < n && isdigit((unsigned char)s[q])) {
            q++;
            ne++;
        }
        if (ne == 0) {
            return 0;
        }
        k = q;
    }
    len = k - i;
    if (len >= sizeof(buf)) {
        return 0;
    }
    memcpy(buf, s + i, len);
    buf[len] = '\0';
    *out = sb_ql_quantise(strtod(buf, &endp));
    return 1;
}
