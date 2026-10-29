/*
 * sb_check.c - grammar-only check of a typed line.
 *
 * The check mirrors the shape of the statement and expression grammar in
 * sb_expr.c and sb_interp.c but computes nothing.  Errors are syntax errors
 * only; runtime errors are left to execution.
 */
#include "sb_check.h"
#include "sb_error.h"
#include "sb_lex.h"

static int at_op(const SbLexer *lx, int op)
{
    return lx->type == SB_TK_OP && lx->op == op;
}

static int at_end(const SbLexer *lx)
{
    return lx->type == SB_TK_EOL || at_op(lx, SB_OP_COLON);
}

static int expect_op(SbLexer *lx, int op)
{
    if (!at_op(lx, op)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    return SB_OK;
}

static int is_binop(const SbLexer *lx)
{
    if (lx->type == SB_TK_KW) {
        return lx->kw == SB_KW_AND || lx->kw == SB_KW_OR
            || lx->kw == SB_KW_XOR || lx->kw == SB_KW_DIV
            || lx->kw == SB_KW_MOD;
    }
    if (lx->type == SB_TK_OP) {
        switch (lx->op) {
        case SB_OP_PLUS: case SB_OP_MINUS: case SB_OP_STAR: case SB_OP_SLASH:
        case SB_OP_CARET: case SB_OP_AMP: case SB_OP_EQ: case SB_OP_NE:
        case SB_OP_LT: case SB_OP_GT: case SB_OP_LE: case SB_OP_GE:
        case SB_OP_BITOR: case SB_OP_BITAND: case SB_OP_BITXOR:
            return 1;
        default:
            return 0;
        }
    }
    return 0;
}

static int ck_expr(SbLexer *lx);

/* ( expr {, expr} ) after a name: subscripts.  Only the single subscript of
 * a string may be a range "a TO b" (allow_to), and a range takes no comma,
 * as in sb_expr_subscripts. */
static int ck_arglist(SbLexer *lx, int allow_to)
{
    int rc;
    int ranged = 0;
    rc = expect_op(lx, SB_OP_LPAR);
    if (rc != SB_OK) {
        return rc;
    }
    for (;;) {
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (allow_to && !ranged && lx->type == SB_TK_KW && lx->kw == SB_KW_TO) {
            sb_lex_next(lx);
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            ranged = 1;
        }
        if (at_op(lx, SB_OP_COMMA) && !ranged) {
            sb_lex_next(lx);
            continue;
        }
        return expect_op(lx, SB_OP_RPAR);
    }
}

static int ck_primary(SbLexer *lx)
{
    int rc;

    switch (lx->type) {
    case SB_TK_NUM:
    case SB_TK_STR:
        sb_lex_next(lx);
        return SB_OK;
    case SB_TK_IDENT:
        sb_lex_next(lx);
        if (at_op(lx, SB_OP_LPAR)) {
            return ck_arglist(lx, 1);
        }
        return SB_OK;
    case SB_TK_KW:
        if (lx->kw == SB_KW_ERNUM) {
            sb_lex_next(lx);
            return SB_OK;
        }
        if (lx->kw == SB_KW_LEN || lx->kw == SB_KW_ABS) {
            sb_lex_next(lx);
            rc = expect_op(lx, SB_OP_LPAR);
            if (rc != SB_OK) {
                return rc;
            }
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            return expect_op(lx, SB_OP_RPAR);
        }
        return SB_E_EXPRESSION;
    case SB_TK_OP:
        if (lx->op == SB_OP_LPAR) {
            sb_lex_next(lx);
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            return expect_op(lx, SB_OP_RPAR);
        }
        return SB_E_EXPRESSION;
    default:
        return SB_E_EXPRESSION;
    }
}

/* prefix NOT and unary sign, then a primary */
static int ck_unary(SbLexer *lx)
{
    for (;;) {
        if (lx->type == SB_TK_KW && lx->kw == SB_KW_NOT) {
            sb_lex_next(lx);
            continue;
        }
        if (lx->type == SB_TK_OP
            && (lx->op == SB_OP_PLUS || lx->op == SB_OP_MINUS)) {
            sb_lex_next(lx);
            continue;
        }
        break;
    }
    return ck_primary(lx);
}

static int ck_expr(SbLexer *lx)
{
    int rc;
    rc = ck_unary(lx);
    if (rc != SB_OK) {
        return rc;
    }
    while (is_binop(lx)) {
        sb_lex_next(lx);
        rc = ck_unary(lx);
        if (rc != SB_OK) {
            return rc;
        }
    }
    return SB_OK;
}

/* PRINT items: expressions separated by ; or ,, optional #channel, */
static int ck_print(SbLexer *lx)
{
    int rc;

    if (at_op(lx, SB_OP_HASH)) {
        sb_lex_next(lx);
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (!at_end(lx)) {          /* PRINT #n alone prints a blank line */
            rc = expect_op(lx, SB_OP_COMMA);
            if (rc != SB_OK) {
                return rc;
            }
        }
    }
    for (;;) {
        if (at_end(lx)) {
            return SB_OK;
        }
        if (at_op(lx, SB_OP_SEMI) || at_op(lx, SB_OP_COMMA)) {
            sb_lex_next(lx);
            continue;
        }
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (!(at_end(lx) || at_op(lx, SB_OP_SEMI) || at_op(lx, SB_OP_COMMA))) {
            return SB_E_EXPRESSION;
        }
    }
}

/* One statement.  Returns SB_OK when the statement must be followed by a
 * separator, 1 when the statement (IF ... THEN) is followed directly by the
 * statements of its body, or an error. */
static int ck_statement(SbLexer *lx)
{
    int rc;

    if (lx->type == SB_TK_IDENT) {
        int paren = 0;
        sb_lex_next(lx);
        if (at_op(lx, SB_OP_LPAR)) {
            rc = ck_arglist(lx, 1);
            if (rc != SB_OK) {
                return rc;
            }
            paren = 1;
        }
        if (at_op(lx, SB_OP_EQ)) {
            sb_lex_next(lx);
            return ck_expr(lx);
        }
        if (paren) {
            return SB_OK;             /* p(a, b): a procedure call */
        }
        /* p a, b: a procedure call with its arguments to the end of the
         * statement (the same list rule the interpreter uses) */
        if (at_end(lx)) {
            return SB_OK;
        }
        for (;;) {
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            if (!at_op(lx, SB_OP_COMMA)) {
                return SB_OK;
            }
            sb_lex_next(lx);
        }
    }
    if (lx->type != SB_TK_KW) {
        return SB_E_EXPRESSION;
    }

    switch (lx->kw) {
    case SB_KW_PRINT:
        sb_lex_next(lx);
        return ck_print(lx);
    case SB_KW_LET:
        sb_lex_next(lx);
        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        return ck_statement(lx);
    case SB_KW_REM:
        lx->pos = lx->len;      /* the rest of the line is a comment */
        sb_lex_next(lx);
        return SB_OK;
    case SB_KW_GOTO:
    case SB_KW_GOSUB:
        sb_lex_next(lx);
        return ck_expr(lx);
    case SB_KW_RETURN:
        sb_lex_next(lx);
        if (at_end(lx)) {
            return SB_OK;
        }
        return ck_expr(lx);       /* RETurn value, inside a DEFine */
    case SB_KW_DEFINE:
        sb_lex_next(lx);
        if (lx->type != SB_TK_KW
            || (lx->kw != SB_KW_PROCEDURE && lx->kw != SB_KW_FUNCTION)) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        if (at_op(lx, SB_OP_LPAR)) {
            sb_lex_next(lx);
            for (;;) {
                if (lx->type != SB_TK_IDENT) {
                    return SB_E_EXPRESSION;
                }
                sb_lex_next(lx);
                if (!at_op(lx, SB_OP_COMMA)) {
                    break;
                }
                sb_lex_next(lx);
            }
            rc = expect_op(lx, SB_OP_RPAR);
            if (rc != SB_OK) {
                return rc;
            }
        }
        return at_end(lx) ? SB_OK : SB_E_EXPRESSION;
    case SB_KW_END:
        sb_lex_next(lx);
        if (lx->type != SB_TK_KW || lx->kw != SB_KW_DEFINE) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        return SB_OK;
    case SB_KW_LOCAL:
        sb_lex_next(lx);
        for (;;) {
            if (lx->type != SB_TK_IDENT) {
                return SB_E_EXPRESSION;
            }
            sb_lex_next(lx);
            if (at_op(lx, SB_OP_LPAR)) {
                rc = ck_arglist(lx, 0);
                if (rc != SB_OK) {
                    return rc;
                }
            }
            if (!at_op(lx, SB_OP_COMMA)) {
                return SB_OK;
            }
            sb_lex_next(lx);
        }
    case SB_KW_DIM:
        sb_lex_next(lx);
        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        return ck_arglist(lx, 0);
    case SB_KW_DATA:
        sb_lex_next(lx);
        if (at_end(lx)) {
            return SB_E_EXPRESSION;    /* DATA needs at least one item */
        }
        for (;;) {
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            if (!at_op(lx, SB_OP_COMMA)) {
                return SB_OK;
            }
            sb_lex_next(lx);
        }
    case SB_KW_RESTORE:
        sb_lex_next(lx);
        if (at_end(lx)) {
            return SB_OK;              /* bare RESTORE: verified */
        }
        return ck_expr(lx);
    case SB_KW_READ:
        sb_lex_next(lx);
        if (at_end(lx)) {
            return SB_OK;              /* bare READ: verified no-op */
        }
        for (;;) {
            if (lx->type != SB_TK_IDENT) {
                return SB_E_EXPRESSION;
            }
            sb_lex_next(lx);
            if (!at_op(lx, SB_OP_COMMA)) {
                return SB_OK;
            }
            sb_lex_next(lx);
        }
    case SB_KW_INPUT:
        sb_lex_next(lx);
        if (at_end(lx)) {
            return SB_OK;              /* INPUT alone: verified no-op */
        }
        if (at_op(lx, SB_OP_HASH)) {
            sb_lex_next(lx);
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            rc = expect_op(lx, SB_OP_COMMA);
            if (rc != SB_OK) {
                return rc;
            }
        }
        for (;;) {
            if (lx->type != SB_TK_IDENT) {
                return SB_E_EXPRESSION;
            }
            sb_lex_next(lx);
            if (at_op(lx, SB_OP_LPAR)) {
                rc = ck_arglist(lx, 1);
                if (rc != SB_OK) {
                    return rc;
                }
            }
            if (at_op(lx, SB_OP_COMMA) || at_op(lx, SB_OP_SEMI)) {
                sb_lex_next(lx);
                continue;
            }
            return SB_OK;
        }
    case SB_KW_IF:
        sb_lex_next(lx);
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (!(lx->type == SB_TK_KW && lx->kw == SB_KW_THEN)) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        return 1;
    case SB_KW_FOR:
        sb_lex_next(lx);
        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        rc = expect_op(lx, SB_OP_EQ);
        if (rc != SB_OK) {
            return rc;
        }
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (!(lx->type == SB_TK_KW && lx->kw == SB_KW_TO)) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        rc = ck_expr(lx);
        if (rc != SB_OK) {
            return rc;
        }
        if (lx->type == SB_TK_KW && lx->kw == SB_KW_STEP) {
            sb_lex_next(lx);
            return ck_expr(lx);
        }
        return SB_OK;
    case SB_KW_NEXT:
        sb_lex_next(lx);
        if (lx->type == SB_TK_IDENT) {
            sb_lex_next(lx);
        }
        return SB_OK;
    case SB_KW_STOP:
    case SB_KW_CONTINUE:
    case SB_KW_NEW:
    case SB_KW_CLEAR:
        sb_lex_next(lx);
        return SB_OK;
    case SB_KW_RUN:
        sb_lex_next(lx);
        if (lx->type == SB_TK_NUM) {
            sb_lex_next(lx);
        }
        return SB_OK;
    case SB_KW_LIST:
        sb_lex_next(lx);
        if (at_op(lx, SB_OP_HASH)) {       /* LIST #n[,] [line] */
            sb_lex_next(lx);
            rc = ck_expr(lx);
            if (rc != SB_OK) {
                return rc;
            }
            if (!at_end(lx)) {
                rc = expect_op(lx, SB_OP_COMMA);
                if (rc != SB_OK) {
                    return rc;
                }
            }
        }
        if (lx->type == SB_TK_NUM) {
            sb_lex_next(lx);
        }
        return SB_OK;
    default:
        return SB_E_EXPRESSION;
    }
}

int sb_check_line(const char *src, size_t len)
{
    SbLexer lx;
    int rc;

    sb_lex_start(&lx, src, len, 0);
    for (;;) {
        while (at_op(&lx, SB_OP_COLON)) {
            sb_lex_next(&lx);
        }
        if (lx.type == SB_TK_EOL) {
            return SB_OK;
        }
        rc = ck_statement(&lx);
        if (rc < 0) {
            return SB_E_EXPRESSION;
        }
        if (rc == 1) {
            continue;           /* IF ... THEN: the body follows directly */
        }
        if (!at_end(&lx)) {
            return SB_E_EXPRESSION;
        }
    }
}
