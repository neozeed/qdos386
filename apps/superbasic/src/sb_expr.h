/*
 * sb_expr.h - expression evaluation.
 *
 * Precedence, lowest first (each level verified against the JS ROM, see
 * docs/JS-SUPERBASIC-MAP.md, section "Expressions"):
 *
 *     OR  ||         logical / bitwise
 *     XOR ^^         logical / bitwise
 *     AND &&         logical / bitwise
 *     NOT            unary, logical
 *     = <> < > <= >= comparisons (numeric or string)
 *     + -            addition, subtraction
 *     * / DIV MOD    multiplication, division (floor-based DIV and MOD)
 *     &              string concatenation
 *     ^              power (left associative)
 *     -              unary minus, applies to a primary only
 *     primary        number, string, variable, (expr), LEN(expr), ERNUM
 */
#ifndef SB_EXPR_H
#define SB_EXPR_H

#include "sb_lex.h"
#include "sb_state.h"

/* Evaluate the expression beginning at the current token of @a lx.  On
 * success *out holds the value (caller must sb_val_free it) and the current
 * token is the first token after the expression.  On failure *out is
 * empty and an SB_E_ code is returned. */
int sb_expr_eval(SbState *st, SbLexer *lx, SbValue *out);

/* Subscripts after a name: "(" expr {"," expr} ")", the current token being
 * the "(".  Each subscript is rounded half up, floor(x + 0.5), as the ROM
 * does for DIM sizes and array subscripts.  When allow_to is set the single
 * subscript of a string may be a range "a TO b": has_to is then set and end
 * holds b.  On success the current token follows the ")". */
typedef struct SbSub {
    int count;
    long idx[SB_MAX_DIMS];
    int has_to;
    long end;
} SbSub;

int sb_expr_subscripts(SbState *st, SbLexer *lx, int allow_to, SbSub *sub);

#endif /* SB_EXPR_H */
