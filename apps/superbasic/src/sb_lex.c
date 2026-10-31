/*
 * sb_lex.c - SuperBASIC tokenizer and canonical line form.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sb_error.h"
#include "sb_format.h"
#include "sb_lex.h"

typedef struct KwEntry {
    const char *name;
    int kw;
} KwEntry;

static const KwEntry kw_table[] = {
    { "PRINT", SB_KW_PRINT },   { "LET", SB_KW_LET },
    { "REM", SB_KW_REM },       { "REMARK", SB_KW_REM },
    { "GOTO", SB_KW_GOTO },     { "IF", SB_KW_IF },
    { "THEN", SB_KW_THEN },     { "FOR", SB_KW_FOR },
    { "TO", SB_KW_TO },         { "STEP", SB_KW_STEP },
    { "NEXT", SB_KW_NEXT },     { "STOP", SB_KW_STOP },
    { "CONTINUE", SB_KW_CONTINUE }, { "RUN", SB_KW_RUN },
    { "LIST", SB_KW_LIST },     { "NEW", SB_KW_NEW },
    { "CLEAR", SB_KW_CLEAR },   { "AND", SB_KW_AND },
    { "OR", SB_KW_OR },         { "XOR", SB_KW_XOR },
    { "NOT", SB_KW_NOT },       { "DIV", SB_KW_DIV },
    { "MOD", SB_KW_MOD },       { "LEN", SB_KW_LEN },
    { "ERNUM", SB_KW_ERNUM },   { "ABS", SB_KW_ABS },
    { "GOSUB", SB_KW_GOSUB },   { "RETURN", SB_KW_RETURN },
    { "INPUT", SB_KW_INPUT },   { "DIM", SB_KW_DIM },
    { "DATA", SB_KW_DATA },    { "READ", SB_KW_READ },
    { "RESTORE", SB_KW_RESTORE },
    { "DEFINE", SB_KW_DEFINE },  { "PROCEDURE", SB_KW_PROCEDURE },
    { "FUNCTION", SB_KW_FUNCTION }, { "END", SB_KW_END },
    { "LOCAL", SB_KW_LOCAL },
    { NULL, 0 }
};

static int ci_equal(const char *a, const char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
            return 0;
        }
    }
    return 1;
}

static int lookup_keyword(const char *s, size_t n)
{
    int i;
    for (i = 0; kw_table[i].name != NULL; i++) {
        if (strlen(kw_table[i].name) == n && ci_equal(s, kw_table[i].name, n)) {
            return kw_table[i].kw;
        }
    }
    return 0;
}

static int is_ident_start(int c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_char(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/* Look past blanks from @a p for the word "TO" followed by a non-identifier
 * character.  Returns the offset just after "TO", or 0 if not present. */
static size_t match_to_after_blanks(const char *src, size_t len, size_t p)
{
    while (p < len && (src[p] == ' ' || src[p] == '\t')) {
        p++;
    }
    if (p + 2 <= len && toupper((unsigned char)src[p]) == 'T'
        && toupper((unsigned char)src[p + 1]) == 'O'
        && (p + 2 == len || !is_ident_char((unsigned char)src[p + 2]))) {
        return p + 2;
    }
    return 0;
}

void sb_lex_start(SbLexer *lx, const char *src, size_t len, size_t pos)
{
    lx->src = src;
    lx->len = len;
    lx->pos = pos;
    lx->tstart = pos;
    lx->type = SB_TK_EOL;
    lx->kw = 0;
    lx->op = 0;
    lx->num = 0.0;
    lx->text = NULL;
    lx->tlen = 0;
    sb_lex_next(lx);
}

int sb_lex_is_kw(const SbLexer *lx, int kw)
{
    return lx->type == SB_TK_KW && lx->kw == kw;
}

void sb_lex_next(SbLexer *lx)
{
    const char *s = lx->src;
    size_t n = lx->len;
    size_t p = lx->pos;
    int c;

    while (p < n && (s[p] == ' ' || s[p] == '\t')) {
        p++;
    }
    lx->tstart = p;
    lx->kw = 0;
    lx->op = 0;
    lx->text = NULL;
    lx->tlen = 0;

    if (p >= n) {
        lx->type = SB_TK_EOL;
        lx->pos = p;
        return;
    }

    c = (unsigned char)s[p];

    /* identifiers and keywords */
    if (is_ident_start(c)) {
        size_t start = p;
        int kw;
        while (p < n && is_ident_char((unsigned char)s[p])) {
            p++;
        }
        if (p < n && s[p] == '$') {
            p++;
            lx->type = SB_TK_IDENT;
            lx->text = s + start;
            lx->tlen = p - start;
            lx->pos = p;
            return;
        }
        kw = lookup_keyword(s + start, p - start);
        if (kw == 0 && p - start == 2 && toupper((unsigned char)s[start]) == 'G'
            && toupper((unsigned char)s[start + 1]) == 'O') {
            size_t after = match_to_after_blanks(s, n, p);
            if (after != 0) {
                lx->type = SB_TK_KW;
                lx->kw = SB_KW_GOTO;
                lx->pos = after;
                return;
            }
        }
        if (kw != 0) {
            lx->type = SB_TK_KW;
            lx->kw = kw;
        } else {
            lx->type = SB_TK_IDENT;
            lx->text = s + start;
            lx->tlen = p - start;
        }
        lx->pos = p;
        return;
    }

    /* numbers: digits [. digits] [E [+-] digits], or . digits */
    if (isdigit(c) || (c == '.' && p + 1 < n && isdigit((unsigned char)s[p + 1]))) {
        size_t start = p;
        char buf[64];
        size_t len;
        char *endp;
        int ok;

        while (p < n && isdigit((unsigned char)s[p])) {
            p++;
        }
        if (p < n && s[p] == '.') {
            p++;
            while (p < n && isdigit((unsigned char)s[p])) {
                p++;
            }
        }
        if (p < n && (s[p] == 'E' || s[p] == 'e')) {
            size_t q = p + 1;
            if (q < n && (s[q] == '+' || s[q] == '-')) {
                q++;
            }
            if (q < n && isdigit((unsigned char)s[q])) {
                while (q < n && isdigit((unsigned char)s[q])) {
                    q++;
                }
                p = q;
            }
        }
        len = p - start;
        ok = 0;
        if (len < sizeof(buf)) {
            memcpy(buf, s + start, len);
            buf[len] = '\0';
            lx->num = sb_ql_quantise(strtod(buf, &endp));
            ok = (endp == buf + len);
        }
        lx->pos = p;
        if (ok) {
            lx->type = SB_TK_NUM;
        } else {
            lx->type = SB_TK_ERR;
        }
        return;
    }

    /* string literal: no escapes, ends at the next quote */
    if (c == '"') {
        size_t start = p + 1;
        p = start;
        while (p < n && s[p] != '"') {
            p++;
        }
        if (p >= n) {
            lx->type = SB_TK_ERR;
            lx->pos = n;
            return;
        }
        lx->type = SB_TK_STR;
        lx->text = s + start;
        lx->tlen = p - start;
        lx->pos = p + 1;
        return;
    }

    /* operators */
    lx->type = SB_TK_OP;
    p++;
    switch (c) {
    case '+': lx->op = SB_OP_PLUS; break;
    case '-': lx->op = SB_OP_MINUS; break;
    case '*': lx->op = SB_OP_STAR; break;
    case '/': lx->op = SB_OP_SLASH; break;
    case '(': lx->op = SB_OP_LPAR; break;
    case ')': lx->op = SB_OP_RPAR; break;
    case ',': lx->op = SB_OP_COMMA; break;
    case ';': lx->op = SB_OP_SEMI; break;
    case ':': lx->op = SB_OP_COLON; break;
    case '#': lx->op = SB_OP_HASH; break;
    case '^':
        if (p < n && s[p] == '^') {
            p++;
            lx->op = SB_OP_BITXOR;
        } else {
            lx->op = SB_OP_CARET;
        }
        break;
    case '&':
        if (p < n && s[p] == '&') {
            p++;
            lx->op = SB_OP_BITAND;
        } else {
            lx->op = SB_OP_AMP;
        }
        break;
    case '|':
        if (p < n && s[p] == '|') {
            p++;
            lx->op = SB_OP_BITOR;
        } else {
            lx->type = SB_TK_ERR;
        }
        break;
    case '=':
        if (p < n && s[p] == '=') {
            p++;
        }
        lx->op = SB_OP_EQ;
        break;
    case '<':
        if (p < n && s[p] == '>') {
            p++;
            lx->op = SB_OP_NE;
        } else if (p < n && s[p] == '=') {
            p++;
            lx->op = SB_OP_LE;
        } else {
            lx->op = SB_OP_LT;
        }
        break;
    case '>':
        if (p < n && s[p] == '=') {
            p++;
            lx->op = SB_OP_GE;
        } else {
            lx->op = SB_OP_GT;
        }
        break;
    default:
        lx->type = SB_TK_ERR;
        break;
    }
    lx->pos = p;
}

/* ---- canonical form ------------------------------------------------- */

typedef struct Buf {
    SbAlloc *a;
    char *data;
    size_t len;
    size_t cap;
    int failed;
} Buf;

static void buf_put(Buf *b, const char *s, size_t n)
{
    char *nd;
    size_t ncap;

    if (b->failed) {
        return;
    }
    if (b->len + n + 1 > b->cap) {
        ncap = (b->cap == 0) ? 64 : b->cap;
        while (ncap < b->len + n + 1) {
            ncap *= 2;
        }
        nd = (char *)sb_mem_alloc(b->a, ncap);
        if (nd == NULL) {
            b->failed = 1;
            return;
        }
        if (b->data != NULL) {
            memcpy(nd, b->data, b->len);
            sb_mem_free(b->a, b->data);
        }
        b->data = nd;
        b->cap = ncap;
    }
    if (n > 0) {
        memcpy(b->data + b->len, s, n);
    }
    b->len += n;
    b->data[b->len] = '\0';
}

static const char *canonical_keyword(int kw)
{
    int i;
    for (i = 0; kw_table[i].name != NULL; i++) {
        if (kw_table[i].kw == kw) {
            if (kw == SB_KW_REM) {
                return "REMark";
            }
            if (kw == SB_KW_GOTO) {
                return "GO TO";
            }
            if (kw == SB_KW_DEFINE) {
                return "DEFine";
            }
            if (kw == SB_KW_PROCEDURE) {
                return "PROCedure";
            }
            if (kw == SB_KW_FUNCTION) {
                return "FuNction";
            }
            if (kw == SB_KW_LOCAL) {
                return "LOCal";
            }
            if (kw == SB_KW_RETURN) {
                return "RETurn";
            }
            return kw_table[i].name;
        }
    }
    return "";
}

int sb_lex_canonical(SbAlloc *a, const char *src, size_t len,
                     char **out, size_t *outlen)
{
    SbLexer lx;
    Buf b;
    size_t copied = 0;
    int prev_end = 0;      /* the last token was the keyword END */

    b.a = a;
    b.data = NULL;
    b.len = 0;
    b.cap = 0;
    b.failed = 0;

    sb_lex_start(&lx, src, len, 0);
    while (lx.type != SB_TK_EOL) {
        if (lx.type == SB_TK_ERR) {
            if (b.data != NULL) {
                sb_mem_free(a, b.data);
            }
            return SB_E_EXPRESSION;
        }
        /* blanks and other unmodified text before this token */
        buf_put(&b, src + copied, lx.tstart - copied);
        if (lx.type == SB_TK_KW) {
            const char *name = canonical_keyword(lx.kw);
            buf_put(&b, name, strlen(name));
            if (lx.kw == SB_KW_DEFINE && prev_end) {
                /* the ROM lists "END DEFine " with a trailing blank (verified:
                 * probe list_defines) */
                buf_put(&b, " ", 1);
            }
            prev_end = (lx.kw == SB_KW_END);
            if (lx.kw == SB_KW_REM) {
                /* the rest of the line is a comment, copied verbatim */
                buf_put(&b, src + lx.pos, len - lx.pos);
                copied = len;
                break;
            }
        } else {
            buf_put(&b, src + lx.tstart, lx.pos - lx.tstart);
            prev_end = 0;
        }
        copied = lx.pos;
        sb_lex_next(&lx);
        if (b.failed) {
            break;
        }
    }
    if (!b.failed && copied < len) {
        buf_put(&b, src + copied, len - copied);
    }
    if (b.failed) {
        if (b.data != NULL) {
            sb_mem_free(a, b.data);
        }
        return SB_E_OUT_OF_MEMORY;
    }
    if (b.data == NULL) {
        b.data = sb_mem_copy(a, "", 0);
        if (b.data == NULL) {
            return SB_E_OUT_OF_MEMORY;
        }
    }
    *out = b.data;
    *outlen = b.len;
    return SB_OK;
}
