/*
 * sb_expr.c - recursive-descent expression evaluator.
 *
 * One C function per precedence level, so the grammar in sb_expr.h maps
 * directly onto the code.
 */
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "sb_error.h"
#include "sb_expr.h"
#include "sb_format.h"
#include "sb_interp.h"
#include "sb_store.h"
#include "sb_value.h"

/* binary operation kinds */
#define K_ADD   1
#define K_SUB   2
#define K_MUL   3
#define K_DIV   4
#define K_IDIV  5
#define K_MOD   6
#define K_POW   7
#define K_AND   8
#define K_OR    9
#define K_XOR  10
#define K_BAND 11
#define K_BOR  12
#define K_BXOR 13
#define K_CAT  14
#define K_EQ   20
#define K_NE   21
#define K_LT   22
#define K_GT   23
#define K_LE   24
#define K_GE   25

/* ---- arithmetic ----------------------------------------------------- */

static int check_finite(double r)
{
    if (r > DBL_MAX || r < -DBL_MAX) {
        return SB_E_OVERFLOW;
    }
    return SB_OK;
}

static int to_integer_bits(double x, long *out)
{
    /* the bitwise operators work on 32-bit integers */
    if (x >= 2147483648.0 || x <= -2147483649.0) {
        return SB_E_OVERFLOW;
    }
    *out = (long)x;
    return SB_OK;
}

static int div_operand(double x, double *out)
{
    double r = floor(x + 0.5);
    if (r < -32768.0 || r > 32767.0) {
        return SB_E_OVERFLOW;
    }
    *out = r;
    return SB_OK;
}

static int num_op(int kind, double a, double b, double *r)
{
    long la;
    long lb;
    double ia;
    double ib;
    int rc;

    switch (kind) {
    case K_ADD:
        *r = a + b;
        break;
    case K_SUB:
        *r = a - b;
        break;
    case K_MUL:
        *r = a * b;
        break;
    case K_DIV:
        if (b == 0.0) {
            return SB_E_OVERFLOW;
        }
        *r = a / b;
        break;
    case K_IDIV:
    case K_MOD:
        /* DIV and MOD take 16-bit integer operands: each is rounded half
         * up first, and an operand outside -32768..32767 is an overflow
         * (verified: 0.5 DIV 1 is 1, 32768 DIV 1 is -18) */
        rc = div_operand(a, &ia);
        if (rc != SB_OK) {
            return rc;
        }
        rc = div_operand(b, &ib);
        if (rc != SB_OK) {
            return rc;
        }
        if (ib == 0.0) {
            return SB_E_OVERFLOW;
        }
        if (kind == K_IDIV) {
            *r = floor(ia / ib);
        } else {
            *r = ia - ib * floor(ia / ib);
        }
        break;
    case K_POW:
        if (a == 0.0 && b < 0.0) {
            return SB_E_OVERFLOW;
        }
        if (a < 0.0 && floor(b) != b) {
            return SB_E_OVERFLOW;
        }
        *r = pow(a, b);
        break;
    case K_AND:
        *r = (a != 0.0 && b != 0.0) ? 1.0 : 0.0;
        break;
    case K_OR:
        *r = (a != 0.0 || b != 0.0) ? 1.0 : 0.0;
        break;
    case K_XOR:
        *r = ((a != 0.0) != (b != 0.0)) ? 1.0 : 0.0;
        break;
    case K_BAND:
    case K_BOR:
    case K_BXOR:
        rc = to_integer_bits(a, &la);
        if (rc != SB_OK) {
            return rc;
        }
        rc = to_integer_bits(b, &lb);
        if (rc != SB_OK) {
            return rc;
        }
        if (kind == K_BAND) {
            *r = (double)(la & lb);
        } else if (kind == K_BOR) {
            *r = (double)(la | lb);
        } else {
            *r = (double)(la ^ lb);
        }
        break;
    default:
        return SB_E_EXPRESSION;
    }
    *r = sb_ql_quantise(*r);
    return check_finite(*r);
}

/* Compare two values.  Numeric when both are numbers or numeric strings;
 * otherwise both must be strings.  Ordering ignores case, equality is exact
 * (both verified against the ROM). */
static int compare_values(const SbValue *a, const SbValue *b, int kind, double *r)
{
    double x;
    double y;
    int c;
    size_t i;
    size_t n;

    if (a->type == SB_VAL_STR && b->type == SB_VAL_STR) {
        if (sb_parse_number(a->str, a->len, &x)
            && sb_parse_number(b->str, b->len, &y)) {
            c = (x < y) ? -1 : (x > y ? 1 : 0);
        } else if (kind == K_EQ || kind == K_NE) {
            int same = (a->len == b->len && memcmp(a->str, b->str, a->len) == 0);
            c = same ? 0 : 1;
        } else {
            n = (a->len < b->len) ? a->len : b->len;
            c = 0;
            for (i = 0; i < n && c == 0; i++) {
                int ca = toupper((unsigned char)a->str[i]);
                int cb = toupper((unsigned char)b->str[i]);
                c = (ca < cb) ? -1 : (ca > cb ? 1 : 0);
            }
            if (c == 0) {
                /* equal ignoring case: raw byte order breaks the tie, so
                 * "A"<"a" is true (verified against the ROM) */
                for (i = 0; i < n && c == 0; i++) {
                    unsigned char ra = (unsigned char)a->str[i];
                    unsigned char rb = (unsigned char)b->str[i];
                    c = (ra < rb) ? -1 : (ra > rb ? 1 : 0);
                }
            }
            if (c == 0) {
                c = (a->len < b->len) ? -1 : (a->len > b->len ? 1 : 0);
            }
        }
    } else {
        int rc;
        rc = sb_val_to_number(a, &x);
        if (rc != SB_OK) {
            return rc;
        }
        rc = sb_val_to_number(b, &y);
        if (rc != SB_OK) {
            return rc;
        }
        c = (x < y) ? -1 : (x > y ? 1 : 0);
    }

    switch (kind) {
    case K_EQ: *r = (c == 0); break;
    case K_NE: *r = (c != 0); break;
    case K_LT: *r = (c < 0); break;
    case K_GT: *r = (c > 0); break;
    case K_LE: *r = (c <= 0); break;
    case K_GE: *r = (c >= 0); break;
    default:   return SB_E_EXPRESSION;
    }
    return SB_OK;
}

/* lhs := lhs <kind> rhs.  On error lhs is left unchanged. */
static int apply_binary(SbState *st, int kind, SbValue *lhs, const SbValue *rhs)
{
    double a;
    double b;
    double r;
    int rc;

    if (kind == K_CAT) {
        SbValue ls;
        SbValue rs;
        SbValue res;
        char *p;

        rc = sb_val_to_string(&st->mem, lhs, &ls);
        if (rc != SB_OK) {
            return rc;
        }
        rc = sb_val_to_string(&st->mem, rhs, &rs);
        if (rc != SB_OK) {
            sb_val_free(&st->mem, &ls);
            return rc;
        }
        p = (char *)sb_mem_alloc(&st->mem, ls.len + rs.len + 1);
        if (p == NULL) {
            sb_val_free(&st->mem, &ls);
            sb_val_free(&st->mem, &rs);
            return SB_E_OUT_OF_MEMORY;
        }
        if (ls.len > 0) {
            memcpy(p, ls.str, ls.len);
        }
        if (rs.len > 0) {
            memcpy(p + ls.len, rs.str, rs.len);
        }
        p[ls.len + rs.len] = '\0';
        res.type = SB_VAL_STR;
        res.num = 0.0;
        res.str = p;
        res.len = ls.len + rs.len;
        sb_val_free(&st->mem, &ls);
        sb_val_free(&st->mem, &rs);
        sb_val_free(&st->mem, lhs);
        *lhs = res;
        return SB_OK;
    }

    if (kind >= K_EQ && kind <= K_GE) {
        rc = compare_values(lhs, rhs, kind, &r);
        if (rc != SB_OK) {
            return rc;
        }
        sb_val_free(&st->mem, lhs);
        sb_val_init_num(lhs, r);
        return SB_OK;
    }

    rc = sb_val_to_number(lhs, &a);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_val_to_number(rhs, &b);
    if (rc != SB_OK) {
        return rc;
    }
    rc = num_op(kind, a, b, &r);
    if (rc != SB_OK) {
        return rc;
    }
    sb_val_free(&st->mem, lhs);
    sb_val_init_num(lhs, r);
    return SB_OK;
}

/* ---- token helpers -------------------------------------------------- */

static int is_op(const SbLexer *lx, int op)
{
    return lx->type == SB_TK_OP && lx->op == op;
}

static int expect_op(SbLexer *lx, int op)
{
    if (!is_op(lx, op)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    return SB_OK;
}

/* ---- precedence levels ---------------------------------------------- */

static int parse_or(SbState *st, SbLexer *lx, SbValue *out);

/* Round a subscript or size to the nearest integer, halves upwards, and
 * clamp huge values so they fail the range check rather than overflow. */
static long sub_round(double d)
{
    if (d > 1.0e9) {
        return 1000000000L;
    }
    if (d < -1.0e9) {
        return -1000000000L;
    }
    return (long)floor(d + 0.5);
}

/* One numeric expression, converted to a number. */
static int eval_subscript(SbState *st, SbLexer *lx, double *d)
{
    SbValue v;
    int rc;

    rc = parse_or(st, lx, &v);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_val_to_number(&v, d);
    sb_val_free(&st->mem, &v);
    return rc;
}

int sb_expr_subscripts(SbState *st, SbLexer *lx, int allow_to, SbSub *sub)
{
    double d;
    int rc;

    sub->count = 0;
    sub->has_to = 0;
    sub->end = 0;
    if (!is_op(lx, SB_OP_LPAR)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    for (;;) {
        if (sub->count >= SB_MAX_DIMS) {
            return SB_E_BAD_PARAMETER;
        }
        rc = eval_subscript(st, lx, &d);
        if (rc != SB_OK) {
            return rc;
        }
        sub->idx[sub->count] = sub_round(d);
        sub->count++;
        if (allow_to && sub->count == 1 && lx->type == SB_TK_KW
            && lx->kw == SB_KW_TO) {
            sb_lex_next(lx);
            rc = eval_subscript(st, lx, &d);
            if (rc != SB_OK) {
                return rc;
            }
            sub->has_to = 1;
            sub->end = sub_round(d);
        }
        if (is_op(lx, SB_OP_COMMA) && !sub->has_to) {
            sb_lex_next(lx);
            continue;
        }
        if (!is_op(lx, SB_OP_RPAR)) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        return SB_OK;
    }
}

/* name(subscripts) as a value: a string slice when the name ends in '$',
 * otherwise an element of a numeric array.  Reading an undefined name is
 * "error in expression" (verified against the ROM). */
/* a$(i) and a$(i TO j) on a string made by DIM: same 1-origin, clamp and
 * error rules as the scalar slice below (verified). */
static int sarr_slice_value(SbState *st, const SbStrArray *sa,
                            const SbSub *sub, SbValue *out)
{
    long start = sub->idx[0];
    long end = sub->has_to ? sub->end : start;
    long len = (long)sa->len;

    if (start < 1 || start > len) {
        return SB_E_OUT_OF_RANGE;
    }
    if (end > len) {
        end = len;
    }
    if (end < start - 1) {
        return SB_E_OUT_OF_RANGE;
    }
    return sb_val_init_str(&st->mem, out, sa->str + (start - 1),
                           (size_t)(end - start + 1));
}

static int designator_value(SbState *st, const char *name, size_t nlen,
                            const SbSub *sub, SbValue *out)
{
    SbVar *v;
    SbArray *a;
    size_t off;
    int rc;

    if (name[nlen - 1] == '$' && sub->count == 1
        && sb_sarr_find(st, name, nlen) != NULL) {
        return sarr_slice_value(st, sb_sarr_find(st, name, nlen), sub, out);
    }
    if (name[nlen - 1] == '$') {
        long start;
        long end;
        long len;

        if (sub->count != 1) {
            return SB_E_EXPRESSION;
        }
        v = sb_var_find(st, name, nlen);
        if (v == NULL || sb_var_slot(v)->type != SB_VAL_STR) {
            return SB_E_EXPRESSION;
        }
        /* 1-origin; the end is clamped to the string, and the slice is
         * empty when the end is just before the start (verified) */
        start = sub->idx[0];
        end = sub->has_to ? sub->end : start;
        len = (long)sb_var_slot(v)->len;
        if (start < 1 || start > len) {
            return SB_E_OUT_OF_RANGE;
        }
        if (end > len) {
            end = len;
        }
        if (end < start - 1) {
            return SB_E_OUT_OF_RANGE;
        }
        return sb_val_init_str(&st->mem, out, sb_var_slot(v)->str + (start - 1),
                               (size_t)(end - start + 1));
    }
    if (sub->has_to) {
        return SB_E_NOT_IMPLEMENTED;   /* a(1 TO 2): verified -19 */
    }
    a = sb_arr_find(st, name, nlen);
    if (a == NULL) {
        return SB_E_EXPRESSION;
    }
    rc = sb_arr_offset(a, sub->idx, sub->count, &off);
    if (rc != SB_OK) {
        return rc;
    }
    sb_val_init_num(out, a->vals[off].num);
    return SB_OK;
}

static int parse_primary(SbState *st, SbLexer *lx, SbValue *out)
{
    int rc;

    switch (lx->type) {
    case SB_TK_NUM:
        sb_val_init_num(out, lx->num);
        sb_lex_next(lx);
        return SB_OK;

    case SB_TK_STR:
        rc = sb_val_init_str(&st->mem, out, lx->text, lx->tlen);
        sb_lex_next(lx);
        return rc;

    case SB_TK_IDENT: {
        const char *name = lx->text;
        size_t nlen = lx->tlen;
        SbVar *v;

        sb_lex_next(lx);
        if (is_op(lx, SB_OP_LPAR)
            && sb_def_kind(st, name, nlen) == SB_DEF_FUNC) {
            return sb_call_function(st, lx, name, nlen, out);
        }
        if (is_op(lx, SB_OP_LPAR)) {
            SbSub sub;
            rc = sb_expr_subscripts(st, lx, 1, &sub);
            if (rc != SB_OK) {
                return rc;
            }
            return designator_value(st, name, nlen, &sub, out);
        }
        v = sb_var_find(st, name, nlen);
        if (v == NULL) {
            if (sb_def_kind(st, name, nlen) == SB_DEF_PROC) {
                return SB_E_EXPRESSION;   /* a procedure has no value (-17, verified) */
            }
            sb_val_init_undef(out);
            return SB_OK;
        }
        if (sb_var_slot(v)->type == SB_VAL_UNDEF) {
            sb_val_init_undef(out);      /* a LOCal or argument not yet set */
            return SB_OK;
        }
        if (sb_var_slot(v)->type == SB_VAL_STR) {
            return sb_val_init_str(&st->mem, out, sb_var_slot(v)->str, sb_var_slot(v)->len);
        }
        sb_val_init_num(out, sb_var_slot(v)->num);
        return SB_OK;
    }

    case SB_TK_KW:
        if (lx->kw == SB_KW_ERNUM) {
            sb_val_init_num(out, (double)st->err);
            sb_lex_next(lx);
            return SB_OK;
        }
        if (lx->kw == SB_KW_ABS) {
            double n;
            sb_lex_next(lx);
            rc = expect_op(lx, SB_OP_LPAR);
            if (rc != SB_OK) {
                return rc;
            }
            rc = parse_or(st, lx, out);
            if (rc != SB_OK) {
                return rc;
            }
            rc = expect_op(lx, SB_OP_RPAR);
            if (rc != SB_OK) {
                sb_val_free(&st->mem, out);
                return rc;
            }
            rc = sb_val_to_number(out, &n);
            sb_val_free(&st->mem, out);
            if (rc != SB_OK) {
                return rc;
            }
            sb_val_init_num(out, n < 0.0 ? -n : n);
            return SB_OK;
        }
        if (lx->kw == SB_KW_LEN) {
            double n;
            sb_lex_next(lx);
            rc = expect_op(lx, SB_OP_LPAR);
            if (rc != SB_OK) {
                return rc;
            }
            rc = parse_or(st, lx, out);
            if (rc != SB_OK) {
                return rc;
            }
            rc = expect_op(lx, SB_OP_RPAR);
            if (rc != SB_OK) {
                sb_val_free(&st->mem, out);
                return rc;
            }
            /* a number counts the characters PRINT would show for it,
             * so LEN(5) is 1 (verified against the ROM) */
            if (out->type == SB_VAL_NUM) {
                char nb[32];
                sb_format_number(out->num, nb, sizeof(nb));
                n = (double)strlen(nb);
            } else if (out->type == SB_VAL_STR) {
                n = (double)out->len;
            } else {
                sb_val_free(&st->mem, out);
                return SB_E_EXPRESSION;
            }
            sb_val_free(&st->mem, out);
            sb_val_init_num(out, n);
            return SB_OK;
        }
        return SB_E_EXPRESSION;

    case SB_TK_OP:
        if (lx->op == SB_OP_LPAR) {
            sb_lex_next(lx);
            rc = parse_or(st, lx, out);
            if (rc != SB_OK) {
                return rc;
            }
            rc = expect_op(lx, SB_OP_RPAR);
            if (rc != SB_OK) {
                sb_val_free(&st->mem, out);
            }
            return rc;
        }
        if (lx->op == SB_OP_HASH) {
            /* channel numbers: no channel support in SB1 */
            return SB_E_NOT_IMPLEMENTED;
        }
        return SB_E_EXPRESSION;

    default:
        return SB_E_EXPRESSION;
    }
}

/* Unary + and -, which may be chained ("- -2" is 2).  They bind tighter
 * than ^, so -2^2 is 4 (verified against the ROM). */
static int parse_unary(SbState *st, SbLexer *lx, SbValue *out)
{
    int rc;
    if (is_op(lx, SB_OP_MINUS) || is_op(lx, SB_OP_PLUS)) {
        int negate = is_op(lx, SB_OP_MINUS);
        double n;
        sb_lex_next(lx);
        rc = parse_unary(st, lx, out);
        if (rc != SB_OK) {
            return rc;
        }
        rc = sb_val_to_number(out, &n);
        if (rc != SB_OK) {
            sb_val_free(&st->mem, out);
            return rc;
        }
        sb_val_free(&st->mem, out);
        sb_val_init_num(out, negate ? -n : n);
        return SB_OK;
    }
    return parse_primary(st, lx, out);
}

/* Left-associative binary level: operand (op operand)*. */
#define DEFINE_LEVEL(name, next, MATCH, KIND_OF)                          \
static int name(SbState *st, SbLexer *lx, SbValue *out)                   \
{                                                                         \
    SbValue rhs;                                                          \
    int rc;                                                               \
    int kind;                                                             \
    rc = next(st, lx, out);                                               \
    if (rc != SB_OK) {                                                    \
        return rc;                                                        \
    }                                                                     \
    for (;;) {                                                            \
        kind = KIND_OF(lx);                                               \
        if (kind == 0) {                                                  \
            break;                                                        \
        }                                                                 \
        sb_lex_next(lx);                                                  \
        sb_val_init_num(&rhs, 0.0);                                       \
        rc = next(st, lx, &rhs);                                          \
        if (rc == SB_OK) {                                                \
            rc = apply_binary(st, kind, out, &rhs);                       \
        }                                                                 \
        sb_val_free(&st->mem, &rhs);                                      \
        if (rc != SB_OK) {                                                \
            sb_val_free(&st->mem, out);                                   \
            return rc;                                                    \
        }                                                                 \
    }                                                                     \
    (void)MATCH;                                                          \
    return SB_OK;                                                         \
}

static int kind_power(const SbLexer *lx)
{
    return is_op(lx, SB_OP_CARET) ? K_POW : 0;
}

static int kind_concat(const SbLexer *lx)
{
    return is_op(lx, SB_OP_AMP) ? K_CAT : 0;
}

static int kind_mul(const SbLexer *lx)
{
    if (is_op(lx, SB_OP_STAR)) {
        return K_MUL;
    }
    if (is_op(lx, SB_OP_SLASH)) {
        return K_DIV;
    }
    if (sb_lex_is_kw(lx, SB_KW_DIV)) {
        return K_IDIV;
    }
    if (sb_lex_is_kw(lx, SB_KW_MOD)) {
        return K_MOD;
    }
    return 0;
}

static int kind_add(const SbLexer *lx)
{
    if (is_op(lx, SB_OP_PLUS)) {
        return K_ADD;
    }
    if (is_op(lx, SB_OP_MINUS)) {
        return K_SUB;
    }
    return 0;
}

static int kind_cmp(const SbLexer *lx)
{
    if (is_op(lx, SB_OP_EQ)) {
        return K_EQ;
    }
    if (is_op(lx, SB_OP_NE)) {
        return K_NE;
    }
    if (is_op(lx, SB_OP_LT)) {
        return K_LT;
    }
    if (is_op(lx, SB_OP_GT)) {
        return K_GT;
    }
    if (is_op(lx, SB_OP_LE)) {
        return K_LE;
    }
    if (is_op(lx, SB_OP_GE)) {
        return K_GE;
    }
    return 0;
}

static int kind_and(const SbLexer *lx)
{
    if (sb_lex_is_kw(lx, SB_KW_AND)) {
        return K_AND;
    }
    if (is_op(lx, SB_OP_BITAND)) {
        return K_BAND;
    }
    return 0;
}

static int kind_xor(const SbLexer *lx)
{
    if (sb_lex_is_kw(lx, SB_KW_XOR)) {
        return K_XOR;
    }
    if (is_op(lx, SB_OP_BITXOR)) {
        return K_BXOR;
    }
    return 0;
}

static int kind_or(const SbLexer *lx);

/* OR and XOR share one level and associate left ("1 XOR 1 OR 1" is 1, and
 * "123456 OR X XOR -5" is 0 on the ROM); AND binds tighter. */
static int kind_orxor(const SbLexer *lx)
{
    int k = kind_or(lx);
    if (k == 0) {
        k = kind_xor(lx);
    }
    return k;
}

static int kind_or(const SbLexer *lx)
{
    if (sb_lex_is_kw(lx, SB_KW_OR)) {
        return K_OR;
    }
    if (is_op(lx, SB_OP_BITOR)) {
        return K_BOR;
    }
    return 0;
}

/* Declared above with the same shape as the generated levels. */
static int parse_pow(SbState *st, SbLexer *lx, SbValue *out);
static int parse_concat(SbState *st, SbLexer *lx, SbValue *out);
static int parse_mul(SbState *st, SbLexer *lx, SbValue *out);
static int parse_add(SbState *st, SbLexer *lx, SbValue *out);
static int parse_cmp(SbState *st, SbLexer *lx, SbValue *out);
static int parse_not(SbState *st, SbLexer *lx, SbValue *out);
static int parse_and(SbState *st, SbLexer *lx, SbValue *out);

DEFINE_LEVEL(parse_concat, parse_unary, 0, kind_concat)
DEFINE_LEVEL(parse_pow, parse_concat, 0, kind_power)
DEFINE_LEVEL(parse_mul, parse_pow, 0, kind_mul)
DEFINE_LEVEL(parse_add, parse_mul, 0, kind_add)
DEFINE_LEVEL(parse_cmp, parse_add, 0, kind_cmp)

static int parse_not(SbState *st, SbLexer *lx, SbValue *out)
{
    int rc;
    if (sb_lex_is_kw(lx, SB_KW_NOT)) {
        double n;
        sb_lex_next(lx);
        rc = parse_not(st, lx, out);
        if (rc != SB_OK) {
            return rc;
        }
        rc = sb_val_to_number(out, &n);
        sb_val_free(&st->mem, out);
        if (rc != SB_OK) {
            return rc;
        }
        sb_val_init_num(out, (n == 0.0) ? 1.0 : 0.0);
        return SB_OK;
    }
    return parse_cmp(st, lx, out);
}

DEFINE_LEVEL(parse_and, parse_not, 0, kind_and)
DEFINE_LEVEL(parse_or, parse_and, 0, kind_orxor)

int sb_expr_eval(SbState *st, SbLexer *lx, SbValue *out)
{
    sb_val_init_num(out, 0.0);
    return parse_or(st, lx, out);
}
