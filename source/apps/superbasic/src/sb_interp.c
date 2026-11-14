/*
 * sb_interp.c - statements, control flow and the run loop.
 *
 * Control flow is expressed with small status codes rather than with
 * non-local jumps:
 *   SB_OK           statement finished; a separator must follow
 *   SB_ST_THEN      an IF was true; statements follow on the same line
 *   SB_ST_SKIP      an IF was false; the rest of the line is skipped
 *   SB_ST_JUMP      continue at st->jmp_line / st->jmp_pos
 *   SB_ST_STOP      STOP executed in a program; st->stop_* saved
 *   SB_ST_END       execution ends quietly (also a missing line target)
 *   negative        an SB_E_ error code
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sb_error.h"
#include "sb_expr.h"
#include "sb_format.h"
#include "sb_check.h"
#include "sb_interp.h"
#include "sb_lex.h"
#include "sb_state.h"
#include "sb_store.h"
#include "sb_value.h"

#define SB_ST_THEN 1
#define SB_ST_SKIP 2
#define SB_ST_JUMP 3
#define SB_ST_STOP 4
#define SB_ST_END  5   /* quiet end of the run                     */
#define SB_ST_EOL  6   /* the current line is finished             */
#define SB_ST_RETP 7   /* RETurn or END DEFine ends a DEFine call (SB4) */

#define SB_MAX_LINE 32767

/* ---- output --------------------------------------------------------- */

/* Write through a channel handle and track the output column (PRINT's comma
 * tabs).  The column counts the bytes the handle accepted. */
static int out_to(SbState *st, SbChan *ch, const char *s, size_t n)
{
    size_t i;
    int rc = sb_chan_write(ch, s, n);

    if (rc != SB_OK) {
        return rc;
    }
    for (i = 0; i < n; i++) {
        st->col = (s[i] == '\n') ? 0 : st->col + 1;
    }
    return SB_OK;
}

/* Default output is channel #1, the screen (verified: PRINT with no channel
 * and PRINT #1 are the same place on the ROM). */

/* Error reports go to the diagnostic handle.  A failing handle is ignored:
 * there is nowhere else to report the failure. */
static void report(SbState *st, int code)
{
    char buf[128];
    const char *msg = sb_error_text(code);

    if (st->diag == NULL) {
        return;
    }
    if (st->cur_line >= 0) {
        sprintf(buf, "%s at line %ld\n", msg, st->cur_line);
    } else {
        sprintf(buf, "%s\n", msg);
    }
    (void)sb_chan_write(st->diag, buf, strlen(buf));
}

static void gosub_clear_all(SbState *st);

/* ---- setup and teardown --------------------------------------------- */

void sb_init(SbState *st, const SbAlloc *mem)
{
    int i;
    st->mem = *mem;
    for (i = 0; i < SB_NCHAN; i++) {
        st->chan[i] = NULL;
    }
    st->diag = NULL;
    st->prog = NULL;
    st->vars = NULL;
    st->fors = NULL;
    st->arrays = NULL;
    st->strarrays = NULL;
    st->data_line = -1;
    st->data_pos = SB_DATA_POS_DONE;
    st->data_set = 0;
    st->gosubs = NULL;
    st->gosub_depth = 0;
    st->col = 0;
    st->err = 0;
    st->cur_line = -1;
    st->in_direct = 0;
    st->stop_line = -1;
    st->stop_pos = 0;
    st->jmp_line = 0;
    st->jmp_pos = 0;
    st->steps = 0;
    st->step_limit = SB_DEFAULT_STEP_LIMIT;
    st->frames = NULL;
    st->call_depth = 0;
    sb_val_init_undef(&st->ret_val);
    st->defs = NULL;
    st->jmp_gosub = 0;
    st->ext = NULL;
    st->ext_ctx = NULL;
}

static void for_clear(SbState *st)
{
    SbFor *f = st->fors;
    while (f != NULL) {
        SbFor *next = f->next;
        sb_mem_free(&st->mem, f->name);
        sb_mem_free(&st->mem, f);
        f = next;
    }
    st->fors = NULL;
}

void sb_set_channel(SbState *st, int n, SbChan *ch)
{
    if (n >= 0 && n < SB_NCHAN) {
        st->chan[n] = ch;
    }
}

void sb_set_diag(SbState *st, SbChan *ch)
{
    st->diag = ch;
}

void sb_destroy(SbState *st)
{
    sb_prog_clear(st);
    sb_var_clear(st);
    sb_arr_clear(st);
    sb_sarr_clear(st);
    for_clear(st);
    gosub_clear_all(st);
}

/* Remove FOR records created by typed (direct) lines; they cannot outlive
 * the line that created them. */
static void for_clear_direct(SbState *st)
{
    while (st->fors != NULL && st->fors->line_no < 0) {
        SbFor *f = st->fors;
        st->fors = f->next;
        sb_mem_free(&st->mem, f->name);
        sb_mem_free(&st->mem, f);
    }
}

/* Drop every pending GOSUB return. */
static void gosub_clear_all(SbState *st)
{
    while (st->gosubs != NULL) {
        SbGosub *g = st->gosubs;
        st->gosubs = g->next;
        sb_mem_free(&st->mem, g);
    }
    st->gosub_depth = 0;
}

/* Drop GOSUB returns made by typed lines; a RETURN can only resume a typed
 * line while that line is still running.  Frames made by program lines
 * stay, so a program can be stopped inside a subroutine and continued. */
static void gosub_clear_direct(SbState *st)
{
    SbGosub **link = &st->gosubs;
    while (*link != NULL) {
        SbGosub *g = *link;
        if (g->line_no < 0) {
            *link = g->next;
            sb_mem_free(&st->mem, g);
            st->gosub_depth--;
        } else {
            link = &g->next;
        }
    }
}

/* ---- helpers -------------------------------------------------------- */

static int is_op(const SbLexer *lx, int op)
{
    return lx->type == SB_TK_OP && lx->op == op;
}

static int at_separator(const SbLexer *lx)
{
    return lx->type == SB_TK_EOL || is_op(lx, SB_OP_COLON);
}

static int same_name(const char *a, size_t alen, const char *b, size_t blen)
{
    size_t i;
    if (alen != blen) {
        return 0;
    }
    for (i = 0; i < alen; i++) {
        if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) {
            return 0;
        }
    }
    return 1;
}

static int expect_op(SbLexer *lx, int op)
{
    if (!is_op(lx, op)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    return SB_OK;
}

/* Evaluate an expression and convert it to a number. */
static int eval_number(SbState *st, SbLexer *lx, double *out)
{
    SbValue v;
    int rc;

    rc = sb_expr_eval(st, lx, &v);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_val_to_number(&v, out);
    sb_val_free(&st->mem, &v);
    return rc;
}

/* ---- statements ----------------------------------------------------- */

/* PRINT: items separated by ';' (no space) or ',' (tab to next multiple of
 * 8 columns).  A trailing separator suppresses the newline. */
/* ---- channels ------------------------------------------------------- */

/* A channel number is an expression rounded half up.  Values far outside
 * any channel are mapped to -1, which is not open. */
static int channel_number(SbState *st, SbLexer *lx, long *chan)
{
    double d;
    int rc;

    rc = eval_number(st, lx, &d);
    if (rc != SB_OK) {
        return rc;
    }
    if (d > 1.0e6 || d < -1.0e6) {
        *chan = -1;
    } else {
        *chan = (long)floor(d + 0.5);
    }
    return SB_OK;
}

/* Verified against the ROM: #0, #1 and #2 are the keyboard (input) and the
 * screen (output); #3 is the output window (INPUT from it gives -19); any
 * other number, or a channel the host has not installed, is not open (-6). */
static SbChan *output_channel(SbState *st, long chan, int *rc)
{
    if (chan < 0 || chan > 3 || st->chan[chan] == NULL) {
        *rc = SB_E_NOT_OPEN;
        return NULL;
    }
    *rc = SB_OK;
    return st->chan[chan];
}

static SbChan *input_channel(SbState *st, long chan, int *rc)
{
    if (chan == 3) {
        *rc = SB_E_NOT_IMPLEMENTED;
        return NULL;
    }
    if (chan < 0 || chan > 2 || st->chan[chan] == NULL) {
        *rc = SB_E_NOT_OPEN;
        return NULL;
    }
    *rc = SB_OK;
    return st->chan[chan];
}

typedef struct TextBuf {
    char *p;
    size_t n;
    size_t cap;
    int col;            /* column, for comma tabs, as the line would show */
} TextBuf;

static int tb_add(SbState *st, TextBuf *tb, const char *s, size_t k)
{
    size_t i;
    if (tb->n + k + 1 > tb->cap) {
        size_t ncap = (tb->cap > 0) ? tb->cap : 64;
        char *np;
        while (ncap < tb->n + k + 1) {
            ncap *= 2;
        }
        np = (char *)sb_mem_alloc(&st->mem, ncap);
        if (np == NULL) {
            return SB_E_OUT_OF_MEMORY;
        }
        if (tb->n > 0) {
            memcpy(np, tb->p, tb->n);
        }
        sb_mem_free(&st->mem, tb->p);
        tb->p = np;
        tb->cap = ncap;
    }
    if (k > 0) {
        memcpy(tb->p + tb->n, s, k);
    }
    tb->n += k;
    for (i = 0; i < k; i++) {
        tb->col = (s[i] == '\n') ? 0 : tb->col + 1;
    }
    return SB_OK;
}

static int stmt_print(SbState *st, SbLexer *lx)
{
    TextBuf tb;
    SbChan *ch = st->chan[1];
    int trailing = 0;
    int need_sep = 0;
    int rc;

    tb.p = NULL;
    tb.n = 0;
    tb.cap = 0;
    tb.col = st->col;

    if (is_op(lx, SB_OP_HASH)) {
        long chan;
        sb_lex_next(lx);
        rc = channel_number(st, lx, &chan);
        if (rc != SB_OK) {
            return rc;
        }
        ch = output_channel(st, chan, &rc);
        if (rc != SB_OK) {
            return rc;
        }
        if (!at_separator(lx)) {
            rc = expect_op(lx, SB_OP_COMMA);
            if (rc != SB_OK) {
                return rc;
            }
        }
    }

    for (;;) {
        SbValue v;

        if (at_separator(lx)) {
            break;
        }
        if (is_op(lx, SB_OP_SEMI) || is_op(lx, SB_OP_COMMA)) {
            if (is_op(lx, SB_OP_COMMA)) {
                int pad = 8 - (tb.col % 8);
                while (pad-- > 0) {
                    rc = tb_add(st, &tb, " ", 1);
                    if (rc != SB_OK) {
                        goto fail;
                    }
                }
            }
            trailing = 1;
            need_sep = 0;
            sb_lex_next(lx);
            continue;
        }
        if (need_sep) {
            rc = SB_E_EXPRESSION;
            goto fail;
        }
        if (is_op(lx, SB_OP_HASH)) {
            rc = SB_E_EXPRESSION;   /* the channel comes first */
            goto fail;
        }
        rc = sb_expr_eval(st, lx, &v);
        if (rc != SB_OK) {
            goto fail;
        }
        if (v.type == SB_VAL_STR) {
            rc = tb_add(st, &tb, v.str, v.len);
        } else if (v.type == SB_VAL_NUM) {
            char buf[32];
            sb_format_number(v.num, buf, sizeof(buf));
            rc = tb_add(st, &tb, buf, strlen(buf));
        } else {
            rc = tb_add(st, &tb, "*", 1);
        }
        sb_val_free(&st->mem, &v);
        if (rc != SB_OK) {
            goto fail;
        }
        trailing = 0;
        need_sep = 1;
    }
    if (!trailing) {
        rc = tb_add(st, &tb, "\n", 1);
        if (rc != SB_OK) {
            goto fail;
        }
    }
    if (tb.n > 0) {
        rc = out_to(st, ch, tb.p, tb.n);
        if (rc != SB_OK) {
            goto fail;
        }
    }
    sb_mem_free(&st->mem, tb.p);
    return SB_OK;

fail:
    sb_mem_free(&st->mem, tb.p);
    return rc;
}

/* name(subscripts) = expression: a string slice or a numeric array element.
 * Assigning to a name that was never made is "bad name" (-12), verified.
 * A slice keeps the string's length: the value is truncated, or padded with
 * spaces, to the width of the slice, and an end past the string is clamped
 * (verified against the ROM). */
static int sarr_slice_assign(SbState *st, SbStrArray *sa, const SbSub *sub,
                            const SbValue *v)
{
    SbValue s;
    long start = sub->idx[0];
    long end = sub->has_to ? sub->end : start;
    long len = (long)sa->len;
    long k;
    int rc;

    rc = sb_val_to_string(&st->mem, v, &s);
    if (rc != SB_OK) {
        return rc;
    }
    if (start < 1 || start > len) {
        sb_val_free(&st->mem, &s);
        return SB_E_OUT_OF_RANGE;
    }
    if (end > len) {
        end = len;
    }
    if (end < start - 1) {
        sb_val_free(&st->mem, &s);
        return SB_E_OUT_OF_RANGE;
    }
    for (k = 0; k < end - start + 1; k++) {
        sa->str[start - 1 + k] = (k < (long)s.len) ? s.str[k] : ' ';
    }
    sb_val_free(&st->mem, &s);
    return SB_OK;
}

static int assign_designator(SbState *st, SbLexer *lx, const char *name,
                             size_t nlen)
{
    SbSub sub;
    SbValue v;
    int is_string = (name[nlen - 1] == '$');
    int rc;

    rc = sb_expr_subscripts(st, lx, 1, &sub);
    if (rc != SB_OK) {
        return rc;
    }
    rc = expect_op(lx, SB_OP_EQ);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_expr_eval(st, lx, &v);
    if (rc != SB_OK) {
        return rc;
    }

    if (is_string && sub.count == 1 && sb_sarr_find(st, name, nlen) != NULL) {
        rc = sarr_slice_assign(st, sb_sarr_find(st, name, nlen), &sub, &v);
        sb_val_free(&st->mem, &v);
        return rc;
    }
    if (is_string) {
        SbVar *var;
        SbValue s;
        long start;
        long end;
        long len;
        long k;

        var = sb_var_find(st, name, nlen);
        if (var == NULL) {
            sb_val_free(&st->mem, &v);
            return SB_E_BAD_NAME;
        }
        if (sb_var_slot(var)->type != SB_VAL_STR || sub.count != 1) {
            sb_val_free(&st->mem, &v);
            return SB_E_EXPRESSION;
        }
        rc = sb_val_to_string(&st->mem, &v, &s);
        sb_val_free(&st->mem, &v);
        if (rc != SB_OK) {
            return rc;
        }
        start = sub.idx[0];
        end = sub.has_to ? sub.end : start;
        len = (long)sb_var_slot(var)->len;
        if (start < 1 || start > len) {
            sb_val_free(&st->mem, &s);
            return SB_E_OUT_OF_RANGE;
        }
        if (end > len) {
            end = len;
        }
        if (end < start - 1) {
            sb_val_free(&st->mem, &s);
            return SB_E_OUT_OF_RANGE;
        }
        for (k = 0; k < end - start + 1; k++) {
            sb_var_slot(var)->str[start - 1 + k] =
                (k < (long)s.len) ? s.str[k] : ' ';
        }
        sb_val_free(&st->mem, &s);
        return SB_OK;
    }

    if (sub.has_to) {
        sb_val_free(&st->mem, &v);
        return SB_E_NOT_IMPLEMENTED;   /* a(1 TO 2) = ...: verified -19 */
    }
    {
        SbArray *arr = sb_arr_find(st, name, nlen);
        size_t off;
        double d;

        if (arr == NULL) {
            sb_val_free(&st->mem, &v);
            return SB_E_BAD_NAME;
        }
        rc = sb_arr_offset(arr, sub.idx, sub.count, &off);
        if (rc != SB_OK) {
            sb_val_free(&st->mem, &v);
            return rc;
        }
        rc = sb_val_to_number(&v, &d);
        sb_val_free(&st->mem, &v);
        if (rc != SB_OK) {
            return rc;
        }
        sb_val_init_num(&arr->vals[off], d);
    }
    return SB_OK;
}

/* name = expression, with the current token at the name. */
static int stmt_assign(SbState *st, SbLexer *lx)
{
    const char *name;
    size_t nlen;
    int is_string;
    SbValue v;
    SbValue res;
    int rc;

    if (lx->type != SB_TK_IDENT) {
        return SB_E_EXPRESSION;
    }
    name = lx->text;
    nlen = lx->tlen;
    is_string = (name[nlen - 1] == '$');
    sb_lex_next(lx);
    if (is_op(lx, SB_OP_LPAR)) {
        return assign_designator(st, lx, name, nlen);
    }
    rc = expect_op(lx, SB_OP_EQ);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_expr_eval(st, lx, &v);
    if (rc != SB_OK) {
        return rc;
    }
    if (is_string) {
        rc = sb_val_to_string(&st->mem, &v, &res);
    } else {
        double d;
        rc = sb_val_to_number(&v, &d);
        if (rc == SB_OK) {
            sb_val_init_num(&res, d);
        }
    }
    sb_val_free(&st->mem, &v);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_var_set(st, name, nlen, &res);
    sb_val_free(&st->mem, &res);
    return rc;
}

static int goto_target(double n, long *out)
{
    if (n < 0.0 || n > (double)SB_MAX_LINE || n != (double)(long)n) {
        return SB_E_BAD_LINE;
    }
    *out = (long)n;
    return SB_OK;
}

static int stmt_goto(SbState *st, SbLexer *lx)
{
    double n;
    long target;
    int rc;

    if (lx->type != SB_TK_NUM) {
        return SB_E_EXPRESSION;
    }
    n = lx->num;
    sb_lex_next(lx);
    rc = goto_target(n, &target);
    if (rc != SB_OK) {
        return rc;
    }
    st->jmp_line = target;
    st->jmp_pos = 0;
    return SB_ST_JUMP;
}

/* IF expression THEN: when false the rest of the line is skipped; when true
 * the statements after THEN run as part of this line. */
static int stmt_if(SbState *st, SbLexer *lx)
{
    double cond;
    int rc;

    rc = eval_number(st, lx, &cond);
    if (rc != SB_OK) {
        return rc;
    }
    if (!sb_lex_is_kw(lx, SB_KW_THEN)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    return (cond != 0.0) ? SB_ST_THEN : SB_ST_SKIP;
}

/* Skip forward to the NEXT that closes the loop named by the FOR clause
 * that has just been rejected.  The search covers the rest of the current
 * line only: the JS ROM ends the program quietly when it does not find the
 * matching NEXT on that line (observed; see the map). */
static int skip_to_next(SbState *st, SbLexer *lx, const char *name, size_t nlen)
{
    SbLexer scan;
    int depth = 0;

    (void)st;
    scan = *lx;
    for (;;) {
        if (scan.type == SB_TK_EOL) {
            return SB_ST_END;
        }
        if (sb_lex_is_kw(&scan, SB_KW_FOR)) {
            depth++;
        } else if (sb_lex_is_kw(&scan, SB_KW_NEXT)) {
            sb_lex_next(&scan);
            if (depth > 0) {
                depth--;
                continue;
            }
            if (scan.type == SB_TK_IDENT && same_name(scan.text, scan.tlen, name, nlen)) {
                sb_lex_next(&scan);
                *lx = scan;
                return SB_OK;
            }
            continue;
        }
        sb_lex_next(&scan);
    }
}

/* Remove the FOR record of a name, if there is one.  Verified (probe p14):
 * a FOR replaces an earlier record of the same name. */
static void for_remove_named(SbState *st, const char *name, size_t nlen)
{
    SbFor **link = &st->fors;
    while (*link != NULL) {
        SbFor *f = *link;
        if (same_name(f->name, f->namelen, name, nlen)) {
            *link = f->next;
            sb_mem_free(&st->mem, f->name);
            sb_mem_free(&st->mem, f);
        } else {
            link = &f->next;
        }
    }
}

static int push_for(SbState *st, const char *name, size_t nlen, double limit,
                    double step, long line_no, size_t pos)
{
    SbFor *f;

    for_remove_named(st, name, nlen);
    f = (SbFor *)sb_mem_alloc(&st->mem, sizeof(SbFor));
    if (f == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    f->name = sb_mem_copy(&st->mem, name, nlen);
    if (f->name == NULL) {
        sb_mem_free(&st->mem, f);
        return SB_E_OUT_OF_MEMORY;
    }
    f->namelen = nlen;
    f->limit = limit;
    f->step = step;
    f->line_no = line_no;
    f->pos = pos;
    f->done = 0;
    f->depth = st->call_depth;
    f->next = st->fors;
    st->fors = f;
    return SB_OK;
}

/* FOR name = start TO limit [STEP step].  The loop variable keeps its last
 * in-range value after NEXT (observed: FOR i=1 TO 3 : NEXT i leaves i=3). */
static int stmt_for(SbState *st, SbLexer *lx)
{
    const char *name;
    size_t nlen;
    double start;
    double limit;
    double step = 1.0;
    SbValue v;
    int rc;

    if (lx->type != SB_TK_IDENT || lx->text[lx->tlen - 1] == '$') {
        return SB_E_EXPRESSION;
    }
    name = lx->text;
    nlen = lx->tlen;
    sb_lex_next(lx);
    rc = expect_op(lx, SB_OP_EQ);
    if (rc != SB_OK) {
        return rc;
    }
    rc = eval_number(st, lx, &start);
    if (rc != SB_OK) {
        return rc;
    }
    if (!sb_lex_is_kw(lx, SB_KW_TO)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    rc = eval_number(st, lx, &limit);
    if (rc != SB_OK) {
        return rc;
    }
    if (sb_lex_is_kw(lx, SB_KW_STEP)) {
        sb_lex_next(lx);
        rc = eval_number(st, lx, &step);
        if (rc != SB_OK) {
            return rc;
        }
    }
    if (step == 0.0) {
        return SB_E_BAD_PARAMETER;
    }

    sb_val_init_num(&v, start);
    rc = sb_var_set(st, name, nlen, &v);
    if (rc != SB_OK) {
        return rc;
    }
    if (!((step > 0.0) ? (start <= limit) : (start >= limit))) {
        return skip_to_next(st, lx, name, nlen);
    }
    return push_for(st, name, nlen, limit, step,
                    st->in_direct ? -1 : st->cur_line, lx->tstart);
}

/* NEXT [name]: go round again, or leave the loop (the variable is not
 * advanced past its limit). */
static int stmt_next(SbState *st, SbLexer *lx)
{
    SbFor *f;
    SbVar *var;
    double cur;
    double nxt;

    if (lx->type == SB_TK_IDENT) {
        for (f = st->fors; f != NULL; f = f->next) {
            if (same_name(f->name, f->namelen, lx->text, lx->tlen)) {
                break;
            }
        }
        sb_lex_next(lx);
    } else {
        f = st->fors;
    }
    if (f == NULL) {
        return SB_E_NOT_FOUND;      /* verified: NEXT without FOR is -7 */
    }
    if (f->line_no < 0 && !st->in_direct) {
        return SB_E_NOT_IMPLEMENTED;
    }
    var = sb_var_find(st, f->name, f->namelen);
    if (var == NULL || sb_var_slot(var)->type != SB_VAL_NUM) {
        return SB_E_EXPRESSION;
    }
    cur = sb_var_slot(var)->num;
    nxt = cur + f->step;
    if ((f->step > 0.0) ? (nxt <= f->limit) : (nxt >= f->limit)) {
        sb_var_slot(var)->num = nxt;
        f->done = 0;
        st->jmp_line = f->line_no;
        st->jmp_pos = f->pos;
        return SB_ST_JUMP;
    }
    /* The loop has finished.  Its record stays (verified: a second NEXT of
     * the same name after the loop ends does not give -7 in a program;
     * probes p14 and p15); only loops begun after it are dropped. */
    while (st->fors != NULL && st->fors != f) {
        SbFor *top = st->fors;
        st->fors = top->next;
        sb_mem_free(&st->mem, top->name);
        sb_mem_free(&st->mem, top);
    }
    f->done = 1;
    return SB_OK;
}

static int stmt_stop(SbState *st, SbLexer *lx)
{
    (void)lx;
    if (st->in_direct) {
        return SB_OK;
    }
    st->stop_line = st->cur_line;
    st->stop_pos = lx->tstart;
    return SB_ST_STOP;
}

static int stmt_continue(SbState *st)
{
    if (st->stop_line < 0) {
        return SB_OK;
    }
    st->jmp_line = st->stop_line;
    st->jmp_pos = st->stop_pos;
    return SB_ST_JUMP;
}

static void defs_scan(SbState *st);

static int stmt_run(SbState *st, SbLexer *lx)
{
    long target;
    double n;

    if (lx->type == SB_TK_NUM) {
        n = lx->num;
        sb_lex_next(lx);
        if (goto_target(n, &target) != SB_OK) {
            return SB_E_BAD_LINE;
        }
    } else {
        if (st->prog == NULL) {
            return SB_ST_END;
        }
        target = st->prog->number;
    }
    /* RUN keeps the variables and the arrays (verified: x=5 then RUN still
     * shows 5; a(1)=7 then RUN still shows 7); only the FOR and GOSUB
     * frames are reset */
    for_clear(st);
    gosub_clear_all(st);
    defs_scan(st);
    st->stop_line = -1;
    st->jmp_line = target;
    st->jmp_pos = 0;
    return SB_ST_JUMP;
}

static int stmt_list(SbState *st, SbLexer *lx)
{
    long from = 0;
    long chan = 1;
    SbChan *ch = st->chan[1];
    SbLine *l;
    char buf[32];
    int rc;

    if (is_op(lx, SB_OP_HASH)) {
        sb_lex_next(lx);
        rc = channel_number(st, lx, &chan);
        if (rc != SB_OK) {
            return rc;
        }
        ch = output_channel(st, chan, &rc);
        if (rc != SB_OK) {
            return rc;
        }
        if (!at_separator(lx)) {
            rc = expect_op(lx, SB_OP_COMMA);
            if (rc != SB_OK) {
                return rc;
            }
        }
    }
    if (lx->type == SB_TK_NUM) {
        from = (long)lx->num;
        sb_lex_next(lx);
    }
    for (l = st->prog; l != NULL; l = l->next) {
        if (l->number < from) {
            continue;
        }
        sprintf(buf, "%ld ", l->number);
        rc = out_to(st, ch, buf, strlen(buf));
        if (rc == SB_OK) {
            rc = out_to(st, ch, l->text, l->len);
        }
        if (rc == SB_OK) {
            rc = out_to(st, ch, "\n", 1);
        }
        if (rc != SB_OK) {
            return rc;
        }
    }
    return SB_OK;
}

static void mark_frames_dead(SbState *st);

static int stmt_new(SbState *st)
{
    mark_frames_dead(st);
    sb_prog_clear(st);
    sb_var_clear(st);
    sb_arr_clear(st);
    sb_sarr_clear(st);
    st->data_set = 0;
    st->data_line = -1;
    st->data_pos = SB_DATA_POS_DONE;
    for_clear(st);
    gosub_clear_all(st);
    st->stop_line = -1;
    return SB_ST_END;
}

/* ---- INPUT ----------------------------------------------------------- */

/* Read one line from the keyboard into a fresh buffer.  The end of the input
 * is reported as -19 in hosted use: the ROM would wait, which a finite
 * input stream cannot reproduce (see KNOWN-LIMITATIONS). */
static int read_line(SbState *st, SbChan *ch, char **out, size_t *outlen)
{
    size_t cap = 64;
    size_t n = 0;
    int got = 0;
    char *buf;
    int c;

    if (ch == NULL) {
        return SB_E_NOT_OPEN;
    }
    buf = (char *)sb_mem_alloc(&st->mem, cap);
    if (buf == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    for (;;) {
        {
            char byte = 0;
            size_t k = 0;
            int brc = sb_chan_read(ch, &byte, 1, &k);
            if (brc != SB_OK) {
                sb_mem_free(&st->mem, buf);
                return brc;
            }
            c = (k == 0) ? -1 : (unsigned char)byte;
        }
        if (c < 0) {
            if (!got) {
                sb_mem_free(&st->mem, buf);
                return SB_E_NOT_IMPLEMENTED;
            }
            break;
        }
        got = 1;
        if (c == '\n') {
            break;
        }
        if (c == '\r') {
            continue;
        }
        if (n == cap) {
            char *nb;
            nb = (char *)sb_mem_alloc(&st->mem, cap * 2);
            if (nb == NULL) {
                sb_mem_free(&st->mem, buf);
                return SB_E_OUT_OF_MEMORY;
            }
            memcpy(nb, buf, n);
            sb_mem_free(&st->mem, buf);
            buf = nb;
            cap *= 2;
        }
        buf[n++] = (char)c;
    }
    *out = buf;
    *outlen = n;
    return SB_OK;
}

/* One INPUT target: a numeric or string variable, or a numeric array
 * element.  The target is checked before a line is read; a numeric target
 * takes the leading number of the line ("3x" is 3, empty is -17). */
static int input_item(SbState *st, SbLexer *lx, SbChan *ch)
{
    const char *name;
    size_t nlen;
    int is_string;
    SbSub sub;
    SbArray *arr = NULL;
    size_t off = 0;
    char *line;
    size_t n;
    SbValue sv;
    int rc;

    if (lx->type != SB_TK_IDENT) {
        return SB_E_EXPRESSION;
    }
    name = lx->text;
    nlen = lx->tlen;
    is_string = (name[nlen - 1] == '$');
    sb_lex_next(lx);
    if (is_op(lx, SB_OP_LPAR)) {
        if (is_string) {
            return SB_E_NOT_IMPLEMENTED;   /* slices and string arrays */
        }
        rc = sb_expr_subscripts(st, lx, 0, &sub);
        if (rc != SB_OK) {
            return rc;
        }
        arr = sb_arr_find(st, name, nlen);
        if (arr == NULL) {
            return SB_E_BAD_NAME;
        }
        rc = sb_arr_offset(arr, sub.idx, sub.count, &off);
        if (rc != SB_OK) {
            return rc;
        }
    }

    rc = read_line(st, ch, &line, &n);
    if (rc != SB_OK) {
        return rc;
    }
    rc = sb_val_init_str(&st->mem, &sv, line, n);
    sb_mem_free(&st->mem, line);
    if (rc != SB_OK) {
        return rc;
    }
    if (is_string) {
        rc = sb_var_set(st, name, nlen, &sv);
    } else {
        double d;
        rc = sb_val_to_number(&sv, &d);
        if (rc == SB_OK) {
            if (arr != NULL) {
                sb_val_init_num(&arr->vals[off], d);
            } else {
                SbValue nv;
                sb_val_init_num(&nv, d);
                rc = sb_var_set(st, name, nlen, &nv);
            }
        }
    }
    sb_val_free(&st->mem, &sv);
    return rc;
}

/* INPUT [#n,] item {, item | ; item}: one line of input per item. */
static int stmt_input(SbState *st, SbLexer *lx)
{
    long chan = 0;
    SbChan *ch = st->chan[0];
    int rc;

    if (at_separator(lx)) {
        return SB_OK;                  /* INPUT alone: verified no-op */
    }
    if (is_op(lx, SB_OP_HASH)) {
        sb_lex_next(lx);
        rc = channel_number(st, lx, &chan);
        if (rc != SB_OK) {
            return rc;
        }
        rc = expect_op(lx, SB_OP_COMMA);
        if (rc != SB_OK) {
            return rc;
        }
        ch = input_channel(st, chan, &rc);
        if (rc != SB_OK) {
            return rc;
        }
    }
    for (;;) {
        rc = input_item(st, lx, ch);
        if (rc != SB_OK) {
            return rc;
        }
        if (is_op(lx, SB_OP_COMMA) || is_op(lx, SB_OP_SEMI)) {
            sb_lex_next(lx);
            continue;
        }
        return SB_OK;
    }
}

/* ---- DIM ------------------------------------------------------------- */

/* DIM name(size {, size}).  Sizes are rounded half up; each dimension takes
 * subscripts 0..size.  Re-dimensioning makes a new array of zeros. */
static int stmt_dim(SbState *st, SbLexer *lx)
{
    const char *name;
    size_t nlen;
    long sizes[SB_MAX_DIMS];
    int ndims = 0;
    int rc;

    if (lx->type != SB_TK_IDENT) {
        return SB_E_EXPRESSION;
    }
    name = lx->text;
    nlen = lx->tlen;
    sb_lex_next(lx);
    if (!is_op(lx, SB_OP_LPAR)) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    for (;;) {
        double d;
        if (ndims >= SB_MAX_DIMS) {
            return SB_E_BAD_PARAMETER;
        }
        rc = eval_number(st, lx, &d);
        if (rc != SB_OK) {
            return rc;
        }
        if (d > 1.0e9) {
            sizes[ndims] = 1000000000L;
        } else if (d < -1.0e9) {
            sizes[ndims] = -1000000000L;
        } else {
            sizes[ndims] = (long)floor(d + 0.5);
        }
        if (sizes[ndims] < 0) {
            return SB_E_OUT_OF_RANGE;
        }
        ndims++;
        if (is_op(lx, SB_OP_COMMA)) {
            sb_lex_next(lx);
            continue;
        }
        if (!is_op(lx, SB_OP_RPAR)) {
            return SB_E_EXPRESSION;
        }
        sb_lex_next(lx);
        break;
    }
    if (name[nlen - 1] == '$') {
        if (ndims != 1) {
            return SB_E_NOT_IMPLEMENTED;   /* 2+ subscripts: see KNOWN-LIMITATIONS */
        }
        if (sizes[0] < 1) {
            return SB_E_OUT_OF_RANGE;
        }
        rc = sb_sarr_dim(st, name, nlen, sizes[0]);
        if (rc != SB_OK) {
            return rc;
        }
        /* verified: DIM name$(n) also sets the scalar name$ to "" */
        {
            SbValue empty;
            rc = sb_val_init_str(&st->mem, &empty, "", 0);
            if (rc != SB_OK) {
                return rc;
            }
            rc = sb_var_set(st, name, nlen, &empty);
            sb_val_free(&st->mem, &empty);
        }
        return rc;
    }
    return sb_arr_dim(st, name, nlen, ndims, sizes);
}

/* ---- GOSUB and RETURN ------------------------------------------------ */

/* GOSUB n: the return point is the next token of the current line, so
 * "10 GOSUB 30 : PRINT x" resumes at the PRINT (verified).  A target that
 * does not exist ends a program run quietly, as GOTO does, but in a typed
 * line it is -7 (verified).  A non-integer target is treated as missing. */
static int stmt_gosub(SbState *st, SbLexer *lx)
{
    double n;
    long target;
    SbGosub *g;
    int rc;

    rc = eval_number(st, lx, &n);
    if (rc != SB_OK) {
        return rc;
    }
    if (n < 0.0 || n > (double)SB_MAX_LINE || n != floor(n)) {
        return SB_E_NOT_FOUND;
    }
    target = (long)n;
    if (sb_prog_find(st, target) == NULL) {
        return st->in_direct ? SB_E_NOT_FOUND : SB_ST_END;
    }
    if (st->gosub_depth >= SB_MAX_GOSUB) {
        return SB_E_OUT_OF_MEMORY;
    }
    g = (SbGosub *)sb_mem_alloc(&st->mem, sizeof(SbGosub));
    if (g == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    g->line_no = st->cur_line;
    g->pos = lx->tstart;
    g->next = st->gosubs;
    st->gosubs = g;
    st->gosub_depth++;
    st->jmp_line = target;
    st->jmp_pos = 0;
    st->jmp_gosub = 1;
    return SB_ST_JUMP;
}

/* ---- SB4: DEFine PROCedure, DEFine FuNction, LOCal, RETurn, calls ----- */

/* CLEAR and NEW free the variables, so every active call's storage goes.
 * Each call then ends with -7 (verified for CLEAR: probe
 * clear_then_return_big). */
static void mark_frames_dead(SbState *st)
{
    SbFrame *f;
    for (f = st->frames; f != NULL; f = f->next) {
        f->dead = 1;
    }
}

static int exec_text(SbState *st, const char *text, size_t len, size_t pos);

/* Lexer helper: is the token after @a lx the operator op? */
static int next_is_op(const SbLexer *lx, int op)
{
    SbLexer pk = *lx;
    sb_lex_next(&pk);
    return is_op(&pk, op);
}

static int next_is_separator(const SbLexer *lx)
{
    SbLexer pk = *lx;
    sb_lex_next(&pk);
    return at_separator(&pk) || is_op(&pk, SB_OP_COMMA) || is_op(&pk, SB_OP_RPAR);
}

int sb_def_kind(SbState *st, const char *name, size_t nlen)
{
    int func = 0;
    if (!sb_def_seen(st, name, nlen, &func)) {
        return SB_DEF_NONE;
    }
    return func ? SB_DEF_FUNC : SB_DEF_PROC;
}

/* Is @a l the line END DEFine? */
static int is_end_define(const SbLine *l)
{
    SbLexer lx;
    sb_lex_start(&lx, l->text, l->len, 0);
    if (lx.type != SB_TK_KW || lx.kw != SB_KW_END) {
        return 0;
    }
    sb_lex_next(&lx);
    return lx.type == SB_TK_KW && lx.kw == SB_KW_DEFINE;
}

/* Walk the parameter list of a DEFine line.  Returns the number of
 * parameters; when index is 0 or more, *name and *nlen describe that one
 * (they point into the line text). */
static int scan_params(const SbLine *defline, int index, const char **name,
                       size_t *nlen)
{
    SbLexer lx;
    int n = 0;

    sb_lex_start(&lx, defline->text, defline->len, 0);
    sb_lex_next(&lx);      /* PROCedure or FuNction */
    sb_lex_next(&lx);      /* the name */
    sb_lex_next(&lx);
    if (!is_op(&lx, SB_OP_LPAR)) {
        return 0;
    }
    sb_lex_next(&lx);
    while (lx.type == SB_TK_IDENT) {
        if (n == index) {
            *name = lx.text;
            *nlen = lx.tlen;
        }
        n++;
        sb_lex_next(&lx);
        if (!is_op(&lx, SB_OP_COMMA)) {
            break;
        }
        sb_lex_next(&lx);
    }
    return n;
}

/* The line that ends the body of defline: its END DEFine, or the largest
 * line number when the program has none. */
static long body_end_number(const SbLine *defline)
{
    const SbLine *l;
    long last = defline->number;
    for (l = defline->next; l != NULL; l = l->next) {
        if (is_end_define(l)) {
            return l->number;
        }
        last = l->number;
    }
    return last;
}

/* DEFine in program flow: skip to the matching END DEFine line.  A DEFine
 * inside a running body is not modelled (see KNOWN-LIMITATIONS, SB4). */
static int stmt_define(SbState *st, SbLexer *lx)
{
    SbLine *cur;
    SbLine *l;

    if (lx->type != SB_TK_KW
        || (lx->kw != SB_KW_PROCEDURE && lx->kw != SB_KW_FUNCTION)) {
        return SB_E_EXPRESSION;
    }
    if (st->in_direct) {
        /* A typed DEFine runs its typed header and body lines as direct
         * lines.  The ROM then knows the name, but it has no stored header
         * line, so a call gives -7 (verified: probe p17 typed_defined_then_call). */
        int tfunc = (lx->kw == SB_KW_FUNCTION);
        sb_lex_next(lx);
        if (lx->type == SB_TK_IDENT) {
            sb_def_set(st, lx->text, lx->tlen, tfunc, -1, 0);
        }
        lx->pos = lx->len;
        sb_lex_next(lx);
        return SB_OK;
    }
    if (st->frames != NULL) {
        return SB_E_NOT_IMPLEMENTED;
    }
    cur = sb_prog_find(st, st->cur_line);
    if (cur == NULL) {
        return SB_E_NOT_FOUND;
    }
    for (l = cur->next; l != NULL; l = l->next) {
        if (is_end_define(l)) {
            break;
        }
    }
    if (l == NULL) {
        return SB_E_EXPRESSION;   /* no END DEFine: not verified */
    }
    st->jmp_line = l->number;
    st->jmp_pos = 0;
    st->jmp_gosub = 0;
    return SB_ST_JUMP;
}

/* END DEFine: the end of a call's body (a procedure returns; a function
 * that ends without RETurn gives -17, verified), or a no-op in program
 * flow. */
static int stmt_end(SbState *st, SbLexer *lx)
{
    if (at_separator(lx)) {
        return SB_ST_END;   /* a bare END ends the run (see KNOWN-LIMITATIONS) */
    }
    if (lx->type != SB_TK_KW || lx->kw != SB_KW_DEFINE) {
        return SB_E_EXPRESSION;
    }
    sb_lex_next(lx);
    if (st->frames == NULL) {
        return SB_OK;
    }
    if (st->frames->is_func) {
        sb_val_free(&st->mem, &st->ret_val);
        sb_val_init_undef(&st->ret_val);
        return SB_E_EXPRESSION;
    }
    sb_val_free(&st->mem, &st->ret_val);
    sb_val_init_undef(&st->ret_val);
    return SB_ST_RETP;
}

/* LOCal name [(n)] [, ...]: must come before any other statement of the
 * body (-21 otherwise, verified). */
static int stmt_local(SbState *st, SbLexer *lx)
{
    int depth;
    if (st->frames == NULL) {
        return SB_E_BAD_LINE;     /* outside a DEFine: -21 (verified: probe p20 local_in_direct_typed) */
    }
    if (st->frames->body_started) {
        return SB_E_BAD_LINE;
    }
    depth = st->frames->depth;
    for (;;) {
        const char *name;
        size_t nlen;
        SbSub sub;
        int rc;

        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        name = lx->text;
        nlen = lx->tlen;
        sb_lex_next(lx);
        if (is_op(lx, SB_OP_LPAR)) {
            rc = sb_expr_subscripts(st, lx, 0, &sub);
            if (rc != SB_OK) {
                return rc;
            }
            if (name[nlen - 1] == '$') {
                if (sub.count != 1) {
                    return SB_E_EXPRESSION;
                }
                rc = sb_local_sarr(st, name, nlen, depth, sub.idx[0]);
            } else {
                long sizes[SB_MAX_DIMS];
                int k;
                if (sub.count > SB_MAX_DIMS) {
                    return SB_E_EXPRESSION;
                }
                for (k = 0; k < sub.count; k++) {
                    sizes[k] = sub.idx[k];
                }
                rc = sb_local_arr(st, name, nlen, depth, sub.count, sizes);
            }
        } else {
            rc = sb_local_var(st, name, nlen, depth, NULL);
        }
        if (rc != SB_OK) {
            return rc;
        }
        if (!is_op(lx, SB_OP_COMMA)) {
            break;
        }
        sb_lex_next(lx);
    }
    return SB_OK;
}

/* One actual argument of a call.  A by-reference argument is the caller's
 * storage; a by-value one owns its value until it is bound. */
typedef struct CallArg {
    int by_ref;
    SbValue *ref;
    SbValue val;
} CallArg;

static void args_free(SbState *st, CallArg *args, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (!args[i].by_ref) {
            sb_val_free(&st->mem, &args[i].val);
        }
    }
}

/* One argument.  a is NULL for an extra argument: it is evaluated and
 * dropped (verified: probe proc_too_many).  Verified rules:
 *   a plain variable, or an element of a numeric array, is passed by
 *   reference; anything else is an expression, passed by value. */
static int one_arg(SbState *st, SbLexer *lx, CallArg *a)
{
    SbValue v;
    int rc;

    if (lx->type == SB_TK_IDENT) {
        const char *name = lx->text;
        size_t nlen = lx->tlen;

        if (next_is_separator(lx)) {
            SbVar *var = sb_var_find(st, name, nlen);
            if (var == NULL) {
                /* an undefined name passed by reference is created, as an
                 * undefined variable (verified: probe p12 undef_var_arg) */
                rc = sb_var_set_undef(st, name, nlen);
                if (rc != SB_OK) {
                    return rc;
                }
                var = sb_var_find(st, name, nlen);
            }
            sb_lex_next(lx);
            if (a != NULL) {
                a->by_ref = 1;
                a->ref = sb_var_slot(var);
            }
            return SB_OK;
        } else if (next_is_op(lx, SB_OP_LPAR) && name[nlen - 1] != '$'
                   && sb_arr_find(st, name, nlen) != NULL) {
            SbLexer t = *lx;
            SbSub sub;
            SbArray *arr = sb_arr_find(st, name, nlen);
            size_t off;

            sb_lex_next(&t);
            rc = sb_expr_subscripts(st, &t, 0, &sub);
            if (rc != SB_OK) {
                return rc;
            }
            if (!sub.has_to && (is_op(&t, SB_OP_COMMA) || is_op(&t, SB_OP_RPAR)
                                || at_separator(&t))) {
                rc = sb_arr_offset(arr, sub.idx, sub.count, &off);
                if (rc != SB_OK) {
                    return rc;
                }
                *lx = t;
                if (a != NULL) {
                    a->by_ref = 1;
                    a->ref = &arr->vals[off];
                }
                return SB_OK;
            }
        }
    }
    rc = sb_expr_eval(st, lx, &v);
    if (rc != SB_OK) {
        return rc;
    }
    if (a != NULL) {
        a->by_ref = 0;
        a->val = v;
    } else {
        sb_val_free(&st->mem, &v);
    }
    return SB_OK;
}

/* The argument list of a call.  For a function (paren) the list is in
 * parentheses; for a procedure it may be empty and runs to the end of the
 * statement.  args has room for nparams; the rest are dropped. */
static int collect_args(SbState *st, SbLexer *lx, int paren, int nparams,
                        CallArg *args, int *nargs)
{
    int n = 0;
    int rc;

    *nargs = 0;
    if (paren) {
        sb_lex_next(lx);
        if (is_op(lx, SB_OP_RPAR)) {
            sb_lex_next(lx);
            return SB_OK;
        }
    } else if (at_separator(lx)) {
        return SB_OK;
    }
    for (;;) {
        CallArg *a = (n < nparams) ? &args[n] : NULL;
        if (a != NULL) {
            a->by_ref = 0;
            sb_val_init_undef(&a->val);
            a->ref = NULL;
        }
        rc = one_arg(st, lx, a);
        if (rc != SB_OK) {
            args_free(st, args, n < nparams ? n : nparams);
            return rc;
        }
        n++;
        if (!is_op(lx, SB_OP_COMMA)) {
            break;
        }
        sb_lex_next(lx);
    }
    if (paren) {
        rc = expect_op(lx, SB_OP_RPAR);
        if (rc != SB_OK) {
            args_free(st, args, n < nparams ? n : nparams);
            return rc;
        }
    }
    *nargs = n;
    return SB_OK;
}

/* Bind the parameters of defline at call depth depth.  Missing arguments
 * are left as locals with their initial value.  Extra arguments were
 * already dropped by collect_args. */
static int bind_params(SbState *st, const SbLine *defline, int depth,
                       CallArg *args, int nargs)
{
    int nparams = scan_params(defline, -1, NULL, NULL);
    int i;

    for (i = 0; i < nparams; i++) {
        const char *pname = NULL;
        size_t plen = 0;
        int rc;

        scan_params(defline, i, &pname, &plen);
        if (i < nargs && args[i].by_ref) {
            rc = sb_local_ref(st, pname, plen, depth, args[i].ref);
        } else {
            SbValue *slot = NULL;
            rc = sb_local_var(st, pname, plen, depth, &slot);
            if (rc == SB_OK && i < nargs) {
                sb_val_free(&st->mem, slot);
                *slot = args[i].val;
                sb_val_init_undef(&args[i].val);
            }
        }
        if (rc != SB_OK) {
            return rc;
        }
    }
    return SB_OK;
}

/* Run the statements of a body.  The body is the lines after defline up to
 * end_number.  A jump inside the body (or a GOSUB, or a RETurn from one)
 * continues here; any other jump leaves the call and goes to the caller,
 * which is how GOTO out of a function works (verified: probe
 * goto_out_of_function).  Returns SB_ST_RETP, SB_ST_JUMP (to be carried
 * out by the caller), SB_ST_STOP/SB_ST_END, or an error. */
static int run_body(SbState *st, const SbLine *defline, long end_number,
                    SbLine *line, size_t pos)
{
    for (;;) {
        int rc;

        if (line == NULL) {
            return SB_ST_END;
        }
        if (st->frames != NULL && st->frames->dead) {
            return SB_E_NOT_FOUND;
        }
        if (++st->steps > st->step_limit) {
            return SB_E_OUT_OF_RANGE;
        }
        st->cur_line = line->number;
        st->in_direct = 0;
        rc = exec_text(st, line->text, line->len, pos);
        pos = 0;
        if (rc == SB_ST_EOL) {
            line = line->next;
            continue;
        }
        if (rc == SB_ST_JUMP) {
            long n = st->jmp_line;
            int local = st->jmp_gosub || (n > defline->number && n <= end_number);
            st->jmp_gosub = 0;
            if (local) {
                line = sb_prog_find(st, n);
                pos = st->jmp_pos;
                if (line == NULL) {
                    return SB_ST_END;
                }
                continue;
            }
            return SB_ST_JUMP;
        }
        return rc;
    }
}

/* Run one call of defline.  The frame is made, the parameters bound, the
 * body run, and the frame and its locals removed again.  On RETP the
 * value (for a function) is moved to *result.  name decides the result
 * type: a $ name gives a string (verified: probe string_fn_returns_number). */
/* A call that ends with a loop still running (RETurn from inside a FOR)
 * loses that loop's record (verified: probe p14 proc_return_in_loop); the
 * records of loops that had finished stay. */
static void for_drop_active_of_depth(SbState *st, int depth)
{
    SbFor **link = &st->fors;
    while (*link != NULL) {
        SbFor *f = *link;
        if (!f->done && f->depth == depth) {
            *link = f->next;
            sb_mem_free(&st->mem, f->name);
            sb_mem_free(&st->mem, f);
        } else {
            link = &f->next;
        }
    }
}

static int run_def(SbState *st, SbLine *defline, const char *name, size_t nlen,
                   int func, CallArg *args, int nargs, SbValue *result)
{
    SbFrame *f;
    long saved_line;
    int saved_direct;
    int dead;
    int rc;

    if (st->call_depth >= SB_MAX_CALL_DEPTH) {
        return SB_E_OUT_OF_MEMORY;
    }
    f = (SbFrame *)sb_mem_alloc(&st->mem, sizeof(SbFrame));
    if (f == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    f->is_func = func;
    f->dead = 0;
    f->gosub_base = st->gosub_depth;
    f->depth = st->call_depth + 1;
    f->body_started = 0;
    f->next = st->frames;
    st->frames = f;
    st->call_depth++;

    rc = bind_params(st, defline, f->depth, args, nargs);
    if (rc == SB_OK) {
        long end_number = body_end_number(defline);
        saved_line = st->cur_line;
        saved_direct = st->in_direct;
        rc = run_body(st, defline, end_number, defline->next, 0);
        st->cur_line = saved_line;
        st->in_direct = saved_direct;
    }

    dead = f->dead;
    if (!dead) {
        sb_frame_unbind(st, f->depth);
        for_drop_active_of_depth(st, f->depth);
    }
    while (st->gosub_depth > f->gosub_base && st->gosubs != NULL) {
        SbGosub *g = st->gosubs;
        st->gosubs = g->next;
        st->gosub_depth--;
        sb_mem_free(&st->mem, g);
    }
    st->frames = f->next;
    st->call_depth--;
    sb_mem_free(&st->mem, f);

    if (rc == SB_ST_RETP) {
        rc = dead ? SB_E_NOT_FOUND : SB_OK;
    }
    if (rc == SB_OK && func) {
        SbValue v = st->ret_val;
        sb_val_init_undef(&st->ret_val);
        if (v.type == SB_VAL_UNDEF) {
            return SB_E_EXPRESSION;   /* a function must return a value */
        }
        if (name[nlen - 1] == '$') {
            rc = sb_val_to_string(&st->mem, &v, result);
            sb_val_free(&st->mem, &v);
        } else {
            double d;
            rc = sb_val_to_number(&v, &d);
            sb_val_free(&st->mem, &v);
            if (rc == SB_OK) {
                sb_val_init_num(result, d);
            }
        }
    } else {
        sb_val_free(&st->mem, &st->ret_val);
        sb_val_init_undef(&st->ret_val);
    }
    return rc;
}

/* A call by name: a procedure statement (result NULL) or a function in an
 * expression (result set).  lx is at the argument list. */
static int call_named(SbState *st, SbLexer *lx, const char *name, size_t nlen,
                      int want_func, SbValue *result)
{
    long number;
    int func;
    SbLine *defline;
    CallArg *args;
    int nparams;
    int nargs = 0;
    int paren;
    int rc;

    rc = sb_def_lookup(st, name, nlen, &number, &func);
    if (rc != SB_OK) {
        return rc;                 /* -12 never defined, -7 line gone */
    }
    if (func != want_func) {
        return SB_E_BAD_NAME;      /* a function as a statement: -12 (verified) */
    }
    defline = sb_prog_find(st, number);
    if (defline == NULL) {
        return SB_E_BAD_NAME;
    }
    nparams = scan_params(defline, -1, NULL, NULL);
    args = (CallArg *)sb_mem_alloc(&st->mem,
                                   (size_t)(nparams > 0 ? nparams : 1) * sizeof(CallArg));
    if (args == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    paren = is_op(lx, SB_OP_LPAR);
    rc = collect_args(st, lx, paren, nparams, args, &nargs);
    if (rc == SB_OK) {
        if (want_func) {
            /* the caller's expression was at the function's name; a
             * function call always has its list in parentheses */
            if (!paren) {
                rc = SB_E_EXPRESSION;
            }
        }
    }
    if (rc == SB_OK) {
        rc = run_def(st, defline, name, nlen, func, args, nargs, result);
    }
    args_free(st, args, nargs < nparams ? nargs : nparams);
    sb_mem_free(&st->mem, args);
    return rc;
}

/* A function call inside an expression: the current token is the name's
 * left parenthesis (the expression parser has the name). */
int sb_call_function(SbState *st, SbLexer *lx, const char *name, size_t nlen,
                     SbValue *out)
{
    return call_named(st, lx, name, nlen, 1, out);
}

/* RUN registers every DEFine in the program before the first line runs: a
 * call that comes before its DEFine line still works (verified: probe p19
 * call_before_define prints "in q" first), and a DEFine that was never
 * RUN is unknown (-12, verified: probe p16 deleted_proc_direct). */
static void defs_scan(SbState *st)
{
    SbLine *l;
    for (l = st->prog; l != NULL; l = l->next) {
        sb_def_note(st, l->number, l->text, l->len);
    }
}

/* A procedure call statement: name has been consumed, lx is after it. */
static int stmt_call(SbState *st, SbLexer *lx, const char *name, size_t nlen)
{
    return call_named(st, lx, name, nlen, 0, NULL);
}

/* An identifier that starts a statement: an assignment, a procedure call,
 * or a name that is not defined (-7, verified: probe typed_defined_then_call). */
static int stmt_ident(SbState *st, SbLexer *lx)
{
    const char *name = lx->text;
    size_t nlen = lx->tlen;
    long number;
    int func;
    int rc;

    if (next_is_op(lx, SB_OP_EQ)) {
        return stmt_assign(st, lx);
    }
    if (sb_def_seen(st, name, nlen, &func) && func) {
        return SB_E_BAD_NAME;      /* a function as a statement: -12, even when its line is gone (verified: probe p16 fn_as_stmt) */
    }
    rc = sb_def_lookup(st, name, nlen, &number, &func);
    if (rc == SB_OK) {
        if (func) {
            return SB_E_BAD_NAME;
        }
        if (next_is_op(lx, SB_OP_LPAR) && sb_arr_find(st, name, nlen) != NULL) {
            return stmt_assign(st, lx);
        }
        sb_lex_next(lx);
        return stmt_call(st, lx, name, nlen);
    }
    if (rc == SB_E_NOT_FOUND) {
        return SB_E_NOT_FOUND;     /* defined, then removed: -7 (verified) */
    }
    /* never defined: an array assignment, or a bare name, which is -12 */
    if (next_is_op(lx, SB_OP_LPAR) && sb_arr_find(st, name, nlen) != NULL) {
        return stmt_assign(st, lx);
    }
    {
        /* QDOS/386 R12B: a host extension procedure (SbExtName) */
        const SbExtName *e = sb_ext_find(st, name, nlen, 0);
        if (e != NULL) {
            sb_lex_next(lx);
            return e->proc(st, lx);
        }
    }
    if (next_is_op(lx, SB_OP_LPAR)) {
        return stmt_assign(st, lx);
    }
    return SB_E_BAD_NAME;
}

const SbExtName *sb_ext_find(const SbState *st, const char *name, size_t nlen,
                             int func)
{
    const SbExtName *e;
    if (st->ext == NULL) {
        return NULL;
    }
    for (e = st->ext; e->name != NULL; e++) {
        if (same_name(e->name, strlen(e->name), name, nlen)
            && (func ? e->func != NULL : e->proc != NULL)) {
            return e;
        }
    }
    return NULL;
}

void sb_new(SbState *st)
{
    (void)stmt_new(st);
}

int sb_in_direct_mode(const SbState *st)
{
    return st->in_direct && st->cur_line == -1 && st->frames == NULL;
}

/* RETurn [value].  Inside a GOSUB subroutine it returns from the GOSUB;
 * otherwise, inside a DEFine call, it ends the call (the value is kept for a
 * function and ignored for a procedure, verified: probe return_value_in_proc). */
static int stmt_return(SbState *st, SbLexer *lx)
{
    SbGosub *g;
    SbValue v;
    int rc;

    if (st->frames == NULL || st->gosub_depth > st->frames->gosub_base) {
        g = st->gosubs;
        if (g == NULL) {
            return SB_E_NOT_FOUND;   /* RETURN with no GOSUB: -7, verified */
        }
        st->gosubs = g->next;
        st->gosub_depth--;
        st->jmp_line = g->line_no;
        st->jmp_pos = g->pos;
        st->jmp_gosub = 1;
        sb_mem_free(&st->mem, g);
        return SB_ST_JUMP;
    }
    if (st->frames->dead) {
        return SB_E_NOT_FOUND;
    }
    if (!at_separator(lx)) {
        rc = sb_expr_eval(st, lx, &v);
        if (rc != SB_OK) {
            return rc;
        }
    } else {
        sb_val_init_undef(&v);
    }
    sb_val_free(&st->mem, &st->ret_val);
    st->ret_val = v;
    return SB_ST_RETP;
}

/* ---- statement dispatch and the line executor ----------------------- */

/* Execute statements from @a pos in @a text until the line ends or control
 * leaves it.  Returns SB_ST_* codes or an SB_E_ error. */
/* ---- DATA, READ, RESTORE -------------------------------------------- */

/* Verified against the JS ROM (docs/JS-SB3-MAP.md): DATA items are
 * expressions, evaluated when READ takes them; RESTORE positions a cursor
 * that is separate from the program counter and the GOSUB stack; RUN keeps
 * the cursor.  Limitation: READ sees only a DATA statement that is the first
 * statement of its numbered line. */
#define DATA_KW_LEN 4

static int is_data_line(const SbLine *ln)
{
    return ln->len >= DATA_KW_LEN
        && memcmp(ln->text, "DATA", DATA_KW_LEN) == 0
        && (ln->len == DATA_KW_LEN || ln->text[DATA_KW_LEN] == ' ');
}

/* The first DATA line with a number greater than @a number, or NULL. */
static SbLine *data_line_after(SbState *st, long number)
{
    SbLine *ln;
    for (ln = st->prog; ln != NULL; ln = ln->next) {
        if (ln->number > number && is_data_line(ln)) {
            return ln;
        }
    }
    return NULL;
}

/* Take the next DATA item as a value.  Moves the cursor past the item and
 * its comma; moves to the next DATA line when the current one is used up. */
static int read_item(SbState *st, SbValue *out)
{
    SbLexer lx;
    SbLine *cur;
    SbLine *nxt;
    int rc;

    for (;;) {
        if (!st->data_set) {
            return SB_E_END_OF_DATA;
        }
        cur = (st->data_line >= 0) ? sb_prog_find(st, st->data_line) : NULL;
        if (cur != NULL && st->data_pos != SB_DATA_POS_DONE
            && st->data_pos < cur->len) {
            sb_lex_start(&lx, cur->text, cur->len, st->data_pos);
            if (at_separator(&lx)) {
                st->data_pos = cur->len;
                continue;
            }
            rc = sb_expr_eval(st, &lx, out);
            if (rc != SB_OK) {
                return rc;
            }
            st->data_pos = is_op(&lx, SB_OP_COMMA) ? lx.pos : cur->len;
            return SB_OK;
        }
        nxt = data_line_after(st, st->data_line);
        if (nxt == NULL) {
            return SB_E_END_OF_DATA;
        }
        st->data_line = nxt->number;
        st->data_pos = DATA_KW_LEN;
    }
}

/* DATA as a statement does nothing; its items are read by READ.  The rest
 * of the statement is skipped so the next statement (after a colon) runs. */
static int stmt_data(SbState *st, SbLexer *lx)
{
    (void)st;
    while (!at_separator(lx)) {
        sb_lex_next(lx);
    }
    return SB_OK;
}

/* RESTORE with no argument: the cursor goes before the first DATA line.
 * RESTORE n: the cursor goes to the first DATA line numbered n or more. */
static int stmt_restore(SbState *st, SbLexer *lx)
{
    double d;
    int rc;

    if (at_separator(lx)) {
        st->data_set = 1;
        st->data_line = -1;
        st->data_pos = SB_DATA_POS_DONE;
        return SB_OK;
    }
    rc = eval_number(st, lx, &d);
    if (rc != SB_OK) {
        return rc;
    }
    if (d > 1.0e9) {
        d = 1.0e9;
    }
    if (d < 0.0) {
        d = 0.0;
    }
    st->data_set = 1;
    st->data_line = (long)floor(d + 0.5) - 1;
    st->data_pos = SB_DATA_POS_DONE;
    return SB_OK;
}

/* READ a, b$, ...: each name takes the next DATA item.  A string item into
 * a numeric name is -17; a number into a string name gives its text.  The
 * names before a failure keep their new values, as in the ROM. */
static int stmt_read(SbState *st, SbLexer *lx)
{
    SbValue v;
    SbValue s;
    int rc;

    if (at_separator(lx)) {
        return SB_OK;
    }
    for (;;) {
        const char *name;
        size_t nlen;

        if (lx->type != SB_TK_IDENT) {
            return SB_E_EXPRESSION;
        }
        name = lx->text;
        nlen = lx->tlen;
        sb_lex_next(lx);
        rc = read_item(st, &v);
        if (rc != SB_OK) {
            return rc;
        }
        if (name[nlen - 1] == '$') {
            rc = sb_val_to_string(&st->mem, &v, &s);
            sb_val_free(&st->mem, &v);
            if (rc != SB_OK) {
                return rc;
            }
            rc = sb_var_set(st, name, nlen, &s);
            sb_val_free(&st->mem, &s);
        } else {
            if (v.type == SB_VAL_STR) {
                sb_val_free(&st->mem, &v);
                return SB_E_EXPRESSION;
            }
            rc = sb_var_set(st, name, nlen, &v);
            sb_val_free(&st->mem, &v);
        }
        if (rc != SB_OK) {
            return rc;
        }
        if (!is_op(lx, SB_OP_COMMA)) {
            return SB_OK;
        }
        sb_lex_next(lx);
    }
}

static int exec_text(SbState *st, const char *text, size_t len, size_t pos)
{
    SbLexer lx;
    int rc;

    sb_lex_start(&lx, text, len, pos);
    for (;;) {
        /* CLEAR or NEW ran inside a call: its storage is gone, so nothing
         * more of the call runs (-7 comes from run_body) */
        if (st->frames != NULL && st->frames->dead) {
            return SB_E_NOT_FOUND;
        }
        while (is_op(&lx, SB_OP_COLON)) {
            sb_lex_next(&lx);
        }
        if (lx.type == SB_TK_EOL) {
            return SB_ST_EOL;   /* end of line: caller moves on */
        }
        if (st->frames != NULL
            && !(lx.type == SB_TK_KW && lx.kw == SB_KW_LOCAL)) {
            st->frames->body_started = 1;   /* LOCal must now come first */
        }

        if (lx.type == SB_TK_KW) {
            int kw = lx.kw;
            sb_lex_next(&lx);
            switch (kw) {
            case SB_KW_PRINT:    rc = stmt_print(st, &lx); break;
            case SB_KW_LET:
                if (lx.type != SB_TK_IDENT) {
                    return SB_E_EXPRESSION;
                }
                rc = stmt_assign(st, &lx);
                break;
            case SB_KW_REM:
                lx.pos = len;
                sb_lex_next(&lx);
                rc = SB_OK;
                break;
            case SB_KW_GOTO:     rc = stmt_goto(st, &lx); break;
            case SB_KW_IF:       rc = stmt_if(st, &lx); break;
            case SB_KW_FOR:      rc = stmt_for(st, &lx); break;
            case SB_KW_NEXT:     rc = stmt_next(st, &lx); break;
            case SB_KW_STOP:     rc = stmt_stop(st, &lx); break;
            case SB_KW_CONTINUE: rc = stmt_continue(st); break;
            case SB_KW_RUN:      rc = stmt_run(st, &lx); break;
            case SB_KW_LIST:     rc = stmt_list(st, &lx); break;
            case SB_KW_NEW:      rc = stmt_new(st); break;
            case SB_KW_CLEAR:
                mark_frames_dead(st);
                sb_var_clear(st);
                sb_arr_clear(st);
                sb_sarr_clear(st);
                rc = SB_OK;
                break;
            case SB_KW_INPUT:    rc = stmt_input(st, &lx); break;
            case SB_KW_DIM:      rc = stmt_dim(st, &lx); break;
            case SB_KW_GOSUB:    rc = stmt_gosub(st, &lx); break;
            case SB_KW_RETURN:   rc = stmt_return(st, &lx); break;
            case SB_KW_DEFINE:   rc = stmt_define(st, &lx); break;
            case SB_KW_END:      rc = stmt_end(st, &lx); break;
            case SB_KW_LOCAL:    rc = stmt_local(st, &lx); break;
            case SB_KW_DATA:     rc = stmt_data(st, &lx); break;
            case SB_KW_READ:     rc = stmt_read(st, &lx); break;
            case SB_KW_RESTORE:  rc = stmt_restore(st, &lx); break;
            default:             rc = SB_E_EXPRESSION; break;
            }
        } else if (lx.type == SB_TK_IDENT) {
            rc = stmt_ident(st, &lx);
        } else {
            return SB_E_EXPRESSION;
        }

        if (rc == SB_ST_SKIP) {
            return SB_ST_EOL;
        }
        if (rc == SB_ST_THEN) {
            continue;
        }
        if (rc != SB_OK) {
            return rc;
        }
        if (!at_separator(&lx)) {
            return SB_E_EXPRESSION;
        }
    }
}

/* Run from a program line (line != NULL) or from a typed line (line ==
 * NULL, text in dtext).  Returns SB_OK or an error code. */
static int run_loop(SbState *st, SbLine *line, const char *dtext, size_t dlen,
                    size_t pos)
{
    for (;;) {
        const char *txt;
        size_t len;
        int rc;

        if (++st->steps > st->step_limit) {
            return SB_E_OUT_OF_RANGE;
        }
        if (line != NULL) {
            txt = line->text;
            len = line->len;
            st->cur_line = line->number;
            st->in_direct = 0;
        } else {
            txt = dtext;
            len = dlen;
            st->cur_line = -1;
            st->in_direct = 1;
        }

        rc = exec_text(st, txt, len, pos);
        if (rc == SB_ST_END || rc == SB_ST_STOP) {
            return SB_OK;       /* quiet end, or STOP: the run is over */
        }
        if (rc == SB_ST_EOL) {
            if (line == NULL) {
                return SB_OK;   /* typed line finished */
            }
            line = line->next;
            pos = 0;
            if (line == NULL) {
                return SB_OK;   /* last program line finished */
            }
            continue;
        }
        if (rc == SB_ST_JUMP) {
            st->jmp_gosub = 0;
            if (st->jmp_line < 0) {
                /* a NEXT that returns within the typed line */
                pos = st->jmp_pos;
                line = NULL;
                continue;
            }
            line = sb_prog_find(st, st->jmp_line);
            pos = st->jmp_pos;
            if (line == NULL) {
                return SB_OK;   /* missing target ends the run quietly */
            }
            continue;
        }
        return rc;   /* SB_OK cannot occur here; anything else is an error */
    }
}

/* ---- public entry points -------------------------------------------- */

static int parse_line_number(const char *text, size_t len, long *number,
                             size_t *rest)
{
    size_t i = 0;
    long n = 0;
    int digits = 0;

    while (i < len && isdigit((unsigned char)text[i])) {
        n = n * 10 + (text[i] - '0');
        if (n > 100000L) {
            return 0;
        }
        i++;
        digits++;
    }
    if (digits == 0) {
        return 0;
    }
    *number = n;
    *rest = i;
    return 1;
}

int sb_execute(SbState *st, const char *text, size_t len)
{
    size_t i = 0;
    int rc;

    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        len--;
    }
    while (i < len && (text[i] == ' ' || text[i] == '\t')) {
        i++;
    }
    if (i == len) {
        return SB_OK;
    }

    if (isdigit((unsigned char)text[i])) {
        long number;
        size_t after;
        if (!parse_line_number(text + i, len - i, &number, &after)
            || number > SB_MAX_LINE) {
            st->err = SB_E_BAD_LINE;
            report(st, SB_E_BAD_LINE);
            return SB_E_BAD_LINE;
        }
        /* the ROM lists "n stmt" with exactly one blank after the number,
         * whatever the blanks in the source (verified, corpus 14) */
        {
            size_t b = i + after;
            while (b < len && (text[b] == ' ' || text[b] == '\t')) {
                b++;
            }
            rc = sb_prog_store(st, number, text + b, len - b);
        }
        if (rc != SB_OK) {
            st->err = rc;
            report(st, rc);
        }
        return rc;
    }

    /* the ROM checks a typed line's syntax before running any of it */
    rc = sb_check_line(text + i, len - i);
    if (rc != SB_OK) {
        st->err = rc;
        report(st, rc);
        return rc;
    }
    st->steps = 0;
    for_clear_direct(st);
    gosub_clear_direct(st);
    st->cur_line = -1;
    st->in_direct = 1;
    rc = run_loop(st, NULL, text + i, len - i, 0);
    st->in_direct = 0;
    if (rc != SB_OK) {
        st->err = rc;
        report(st, rc);
    }
    return rc;
}

int sb_run(SbState *st)
{
    int rc;
    if (st->prog == NULL) {
        return SB_OK;
    }
    sb_var_clear(st);
    sb_arr_clear(st);
    sb_sarr_clear(st);
    for_clear(st);
    gosub_clear_all(st);
    defs_scan(st);
    st->stop_line = -1;
    rc = run_loop(st, st->prog, NULL, 0, 0);
    if (rc != SB_OK) {
        st->err = rc;
        report(st, rc);
    }
    return rc;
}
