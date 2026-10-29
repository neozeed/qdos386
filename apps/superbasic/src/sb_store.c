/*
 * sb_store.c - program lines and variables.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "sb_error.h"
#include "sb_lex.h"
#include "sb_state.h"
#include "sb_store.h"

static void def_free_all(SbState *st);

/* ---- program -------------------------------------------------------- */

SbLine *sb_prog_find(SbState *st, long number)
{
    SbLine *l;
    for (l = st->prog; l != NULL && l->number < number; l = l->next) {
    }
    if (l != NULL && l->number == number) {
        return l;
    }
    return NULL;
}

void sb_prog_delete(SbState *st, long number)
{
    SbLine *prev = NULL;
    SbLine *l;

    for (l = st->prog; l != NULL && l->number < number; l = l->next) {
        prev = l;
    }
    if (l == NULL || l->number != number) {
        return;
    }
    sb_def_drop(st, number);
    if (prev == NULL) {
        st->prog = l->next;
    } else {
        prev->next = l->next;
    }
    sb_mem_free(&st->mem, l->text);
    sb_mem_free(&st->mem, l);
}

int sb_prog_store(SbState *st, long number, const char *src, size_t len)
{
    SbLine *prev = NULL;
    SbLine *l;
    SbLine *nl;
    char *text;
    size_t tlen;
    int rc;

    /* an empty body deletes the line */
    {
        size_t i = 0;
        while (i < len && isspace((unsigned char)src[i])) {
            i++;
        }
        if (i == len) {
            sb_prog_delete(st, number);
            return SB_OK;
        }
    }

    rc = sb_lex_canonical(&st->mem, src, len, &text, &tlen);
    if (rc != SB_OK) {
        return rc;
    }

    nl = (SbLine *)sb_mem_alloc(&st->mem, sizeof(SbLine));
    if (nl == NULL) {
        sb_mem_free(&st->mem, text);
        return SB_E_OUT_OF_MEMORY;
    }
    nl->number = number;
    nl->text = text;
    nl->len = tlen;

    for (l = st->prog; l != NULL && l->number < number; l = l->next) {
        prev = l;
    }
    if (l != NULL && l->number == number) {
        /* replace the existing line: unlink it first */
        sb_def_drop(st, number);
        if (prev == NULL) {
            st->prog = l->next;
        } else {
            prev->next = l->next;
        }
        sb_mem_free(&st->mem, l->text);
        sb_mem_free(&st->mem, l);
    }
    /* insert before the first line with a greater number */
    for (prev = NULL, l = st->prog; l != NULL && l->number < number;
         prev = l, l = l->next) {
    }
    nl->next = l;
    if (prev == NULL) {
        st->prog = nl;
    } else {
        prev->next = nl;
    }
    return SB_OK;
}

void sb_prog_clear(SbState *st)
{
    SbLine *l = st->prog;
    while (l != NULL) {
        SbLine *next = l->next;
        sb_mem_free(&st->mem, l->text);
        sb_mem_free(&st->mem, l);
        l = next;
    }
    st->prog = NULL;
    def_free_all(st);
}

/* ---- variables ------------------------------------------------------ */

static int names_equal(const char *a, size_t alen, const char *b, size_t blen)
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

SbVar *sb_var_find(SbState *st, const char *name, size_t namelen)
{
    SbVar *v;
    for (v = st->vars; v != NULL; v = v->next) {
        if (names_equal(v->name, v->namelen, name, namelen)) {
            return v;
        }
    }
    return NULL;
}

/* A global variable of that name, made undefined if it does not exist yet.
 * An existing variable is left as it is. */
int sb_var_set_undef(SbState *st, const char *name, size_t namelen)
{
    SbVar *v = sb_var_find(st, name, namelen);
    SbValue u;
    int rc;

    if (v != NULL) {
        return SB_OK;
    }
    sb_val_init_undef(&u);
    rc = sb_var_set(st, name, namelen, &u);
    return rc;
}

int sb_var_set(SbState *st, const char *name, size_t namelen, const SbValue *val)
{
    SbVar *v;
    size_t i;

    v = sb_var_find(st, name, namelen);
    if (v == NULL) {
        v = (SbVar *)sb_mem_alloc(&st->mem, sizeof(SbVar));
        if (v == NULL) {
            return SB_E_OUT_OF_MEMORY;
        }
        v->name = sb_mem_copy(&st->mem, name, namelen);
        if (v->name == NULL) {
            sb_mem_free(&st->mem, v);
            return SB_E_OUT_OF_MEMORY;
        }
        for (i = 0; i < namelen; i++) {
            v->name[i] = (char)toupper((unsigned char)name[i]);
        }
        v->namelen = namelen;
        sb_val_init_num(&v->val, 0.0);
        v->local = 0;
        v->frame_no = 0;
        v->ref = NULL;
        v->next = st->vars;
        st->vars = v;
    }
    if (val->type == SB_VAL_STR) {
        SbValue copy;
        int rc = sb_val_init_str(&st->mem, &copy, val->str, val->len);
        if (rc != SB_OK) {
            return rc;
        }
        sb_val_free(&st->mem, sb_var_slot(v));
        *sb_var_slot(v) = copy;
    } else {
        sb_val_free(&st->mem, sb_var_slot(v));
        *sb_var_slot(v) = *val;
    }
    return SB_OK;
}

SbValue *sb_var_slot(SbVar *v)
{
    return v->ref != NULL ? v->ref : &v->val;
}

void sb_var_clear(SbState *st)
{
    SbVar *v = st->vars;
    while (v != NULL) {
        SbVar *next = v->next;
        sb_val_free(&st->mem, &v->val);   /* a ref'd node owns no value */
        sb_mem_free(&st->mem, v->name);
        sb_mem_free(&st->mem, v);
        v = next;
    }
    st->vars = NULL;
}

/* ---- numeric arrays ------------------------------------------------- */

SbArray *sb_arr_find(SbState *st, const char *name, size_t namelen)
{
    SbArray *a;
    for (a = st->arrays; a != NULL; a = a->next) {
        if (names_equal(a->name, a->namelen, name, namelen)) {
            return a;
        }
    }
    return NULL;
}

static void arr_free_node(SbState *st, SbArray *a)
{
    sb_mem_free(&st->mem, a->vals);
    sb_mem_free(&st->mem, a->name);
    sb_mem_free(&st->mem, a);
}

/* Remove and free the array named @a name, if there is one. */
static void arr_unlink(SbState *st, const char *name, size_t namelen)
{
    SbArray **link = &st->arrays;
    while (*link != NULL) {
        SbArray *a = *link;
        if (names_equal(a->name, a->namelen, name, namelen)) {
            *link = a->next;
            arr_free_node(st, a);
            return;
        }
        link = &a->next;
    }
}

int sb_arr_dim(SbState *st, const char *name, size_t namelen, int ndims,
               const long *sizes)
{
    SbArray *a;
    size_t count = 1;
    size_t i;
    size_t k;

    if (ndims < 1 || ndims > SB_MAX_DIMS) {
        return SB_E_BAD_PARAMETER;
    }
    for (i = 0; i < (size_t)ndims; i++) {
        size_t extent = (size_t)sizes[i] + 1;
        if (sizes[i] < 0 || extent > (size_t)SB_MAX_ARRAY_ELEMENTS
            || count > (size_t)SB_MAX_ARRAY_ELEMENTS / extent) {
            return SB_E_OUT_OF_MEMORY;
        }
        count *= extent;
    }

    a = (SbArray *)sb_mem_alloc(&st->mem, sizeof(SbArray));
    if (a == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    a->name = sb_mem_copy(&st->mem, name, namelen);
    a->vals = (SbValue *)sb_mem_alloc(&st->mem, count * sizeof(SbValue));
    if (a->name == NULL || a->vals == NULL) {
        arr_free_node(st, a);
        return SB_E_OUT_OF_MEMORY;
    }
    for (k = 0; k < namelen; k++) {
        a->name[k] = (char)toupper((unsigned char)name[k]);
    }
    a->namelen = namelen;
    a->ndims = ndims;
    for (i = 0; i < (size_t)ndims; i++) {
        a->sizes[i] = sizes[i];
    }
    a->count = count;
    for (k = 0; k < count; k++) {
        sb_val_init_num(&a->vals[k], 0.0);
    }
    a->local = 0;
    a->frame_no = 0;

    arr_unlink(st, name, namelen);
    a->next = st->arrays;
    st->arrays = a;
    return SB_OK;
}

int sb_arr_offset(const SbArray *a, const long *idx, int n, size_t *off)
{
    size_t o = 0;
    int i;

    if (n != a->ndims) {
        return SB_E_EXPRESSION;
    }
    for (i = 0; i < n; i++) {
        if (idx[i] < 0 || idx[i] > a->sizes[i]) {
            return SB_E_OUT_OF_RANGE;
        }
        o = o * ((size_t)a->sizes[i] + 1) + (size_t)idx[i];
    }
    *off = o;
    return SB_OK;
}

void sb_arr_clear(SbState *st)
{
    SbArray *a = st->arrays;
    while (a != NULL) {
        SbArray *next = a->next;
        arr_free_node(st, a);
        a = next;
    }
    st->arrays = NULL;
}


SbStrArray *sb_sarr_find(SbState *st, const char *name, size_t namelen)
{
    SbStrArray *s;
    for (s = st->strarrays; s != NULL; s = s->next) {
        if (names_equal(s->name, s->namelen, name, namelen)) {
            return s;
        }
    }
    return NULL;
}

static void sarr_free_node(SbState *st, SbStrArray *s)
{
    sb_mem_free(&st->mem, s->str);
    sb_mem_free(&st->mem, s->name);
    sb_mem_free(&st->mem, s);
}

int sb_sarr_dim(SbState *st, const char *name, size_t namelen, long len)
{
    SbStrArray *s;
    SbStrArray **link;
    size_t k;

    if (len < 1) {
        return SB_E_OUT_OF_RANGE;
    }
    if (len > SB_MAX_ARRAY_ELEMENTS) {
        return SB_E_OUT_OF_MEMORY;
    }
    s = (SbStrArray *)sb_mem_alloc(&st->mem, sizeof(SbStrArray));
    if (s == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    s->name = sb_mem_copy(&st->mem, name, namelen);
    s->str = (char *)sb_mem_alloc(&st->mem, (size_t)len);
    if (s->name == NULL || s->str == NULL) {
        sarr_free_node(st, s);
        return SB_E_OUT_OF_MEMORY;
    }
    for (k = 0; k < namelen; k++) {
        s->name[k] = (char)toupper((unsigned char)name[k]);
    }
    s->namelen = namelen;
    s->len = (size_t)len;
    memset(s->str, 0, (size_t)len);   /* ROM fill: NUL bytes, verified */
    s->local = 0;
    s->frame_no = 0;

    link = &st->strarrays;
    while (*link != NULL) {
        SbStrArray *old = *link;
        if (names_equal(old->name, old->namelen, name, namelen)) {
            *link = old->next;
            sarr_free_node(st, old);
            break;
        }
        link = &old->next;
    }
    s->next = st->strarrays;
    st->strarrays = s;
    return SB_OK;
}

void sb_sarr_clear(SbState *st)
{
    SbStrArray *s = st->strarrays;
    while (s != NULL) {
        SbStrArray *next = s->next;
        sarr_free_node(st, s);
        s = next;
    }
    st->strarrays = NULL;
}

/* ---- SB4: call locals ----------------------------------------------- */

static void var_node_free(SbState *st, SbVar *v)
{
    sb_val_free(&st->mem, &v->val);
    sb_mem_free(&st->mem, v->name);
    sb_mem_free(&st->mem, v);
}

static int new_var_node(SbState *st, const char *name, size_t namelen,
                        int depth, SbVar **out)
{
    SbVar *v;
    size_t i;

    v = (SbVar *)sb_mem_alloc(&st->mem, sizeof(SbVar));
    if (v == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    v->name = sb_mem_copy(&st->mem, name, namelen);
    if (v->name == NULL) {
        sb_mem_free(&st->mem, v);
        return SB_E_OUT_OF_MEMORY;
    }
    for (i = 0; i < namelen; i++) {
        v->name[i] = (char)toupper((unsigned char)name[i]);
    }
    v->namelen = namelen;
    sb_val_init_undef(&v->val);
    v->local = 1;
    v->frame_no = depth;
    v->ref = NULL;
    v->next = st->vars;
    st->vars = v;
    *out = v;
    return SB_OK;
}

int sb_local_var(SbState *st, const char *name, size_t namelen, int depth,
                 SbValue **slot)
{
    SbVar *v;
    int rc = new_var_node(st, name, namelen, depth, &v);
    if (rc != SB_OK) {
        return rc;
    }
    /* a LOCal scalar, and a parameter with no argument, starts undefined:
     * it prints as "*" (verified: probes local_uninit, proc_params_noargs_count) */
    if (slot != NULL) {
        *slot = &v->val;
    }
    return SB_OK;
}

int sb_local_ref(SbState *st, const char *name, size_t namelen, int depth,
                 SbValue *ref)
{
    SbVar *v;
    int rc = new_var_node(st, name, namelen, depth, &v);
    if (rc != SB_OK) {
        return rc;
    }
    v->ref = ref;
    return SB_OK;
}

int sb_local_arr(SbState *st, const char *name, size_t namelen, int depth,
                 int ndims, const long *sizes)
{
    SbArray *a;
    size_t count = 1;
    size_t i;
    size_t k;

    if (ndims < 1 || ndims > SB_MAX_DIMS) {
        return SB_E_BAD_PARAMETER;
    }
    for (i = 0; i < (size_t)ndims; i++) {
        size_t extent = (size_t)sizes[i] + 1;
        if (sizes[i] < 0 || extent > (size_t)SB_MAX_ARRAY_ELEMENTS
            || count > (size_t)SB_MAX_ARRAY_ELEMENTS / extent) {
            return SB_E_OUT_OF_MEMORY;
        }
        count *= extent;
    }
    a = (SbArray *)sb_mem_alloc(&st->mem, sizeof(SbArray));
    if (a == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    a->name = sb_mem_copy(&st->mem, name, namelen);
    a->vals = (SbValue *)sb_mem_alloc(&st->mem, count * sizeof(SbValue));
    if (a->name == NULL || a->vals == NULL) {
        if (a->name != NULL) {
            sb_mem_free(&st->mem, a->name);
        }
        if (a->vals != NULL) {
            sb_mem_free(&st->mem, a->vals);
        }
        sb_mem_free(&st->mem, a);
        return SB_E_OUT_OF_MEMORY;
    }
    for (k = 0; k < namelen; k++) {
        a->name[k] = (char)toupper((unsigned char)name[k]);
    }
    a->namelen = namelen;
    a->ndims = ndims;
    for (i = 0; i < (size_t)ndims; i++) {
        a->sizes[i] = sizes[i];
    }
    a->count = count;
    for (k = 0; k < count; k++) {
        sb_val_init_num(&a->vals[k], 0.0);   /* LOCal arrays start at zero */
    }
    a->local = 1;
    a->frame_no = depth;
    a->next = st->arrays;
    st->arrays = a;
    return SB_OK;
}

int sb_local_sarr(SbState *st, const char *name, size_t namelen, int depth,
                  long len)
{
    SbStrArray *s;
    size_t k;

    if (len < 1) {
        return SB_E_OUT_OF_RANGE;
    }
    if (len > SB_MAX_ARRAY_ELEMENTS) {
        return SB_E_OUT_OF_MEMORY;
    }
    s = (SbStrArray *)sb_mem_alloc(&st->mem, sizeof(SbStrArray));
    if (s == NULL) {
        return SB_E_OUT_OF_MEMORY;
    }
    s->name = sb_mem_copy(&st->mem, name, namelen);
    s->str = (char *)sb_mem_alloc(&st->mem, (size_t)len);
    if (s->name == NULL || s->str == NULL) {
        if (s->name != NULL) {
            sb_mem_free(&st->mem, s->name);
        }
        if (s->str != NULL) {
            sb_mem_free(&st->mem, s->str);
        }
        sb_mem_free(&st->mem, s);
        return SB_E_OUT_OF_MEMORY;
    }
    for (k = 0; k < namelen; k++) {
        s->name[k] = (char)toupper((unsigned char)name[k]);
    }
    s->namelen = namelen;
    s->len = (size_t)len;
    memset(s->str, 0, (size_t)len);
    s->local = 1;
    s->frame_no = depth;
    s->next = st->strarrays;
    st->strarrays = s;
    return SB_OK;
}

/* Remove every local made at call depth @a depth.  Frames end innermost
 * first, so the locals of inner calls are already gone. */
void sb_frame_unbind(SbState *st, int depth)
{
    SbVar **pv = &st->vars;
    SbArray **pa = &st->arrays;
    SbStrArray **ps = &st->strarrays;

    while (*pv != NULL) {
        SbVar *v = *pv;
        if (v->local && v->frame_no == depth) {
            *pv = v->next;
            var_node_free(st, v);
        } else {
            pv = &v->next;
        }
    }
    while (*pa != NULL) {
        SbArray *a = *pa;
        if (a->local && a->frame_no == depth) {
            *pa = a->next;
            sb_mem_free(&st->mem, a->name);
            sb_mem_free(&st->mem, a->vals);
            sb_mem_free(&st->mem, a);
        } else {
            pa = &a->next;
        }
    }
    while (*ps != NULL) {
        SbStrArray *s = *ps;
        if (s->local && s->frame_no == depth) {
            *ps = s->next;
            sb_mem_free(&st->mem, s->name);
            sb_mem_free(&st->mem, s->str);
            sb_mem_free(&st->mem, s);
        } else {
            ps = &s->next;
        }
    }
}

/* ---- SB4: DEFine names ---------------------------------------------- */

static SbDefEntry *def_find(SbState *st, const char *name, size_t namelen)
{
    SbDefEntry *d;
    for (d = st->defs; d != NULL; d = d->next) {
        if (names_equal(d->name, d->namelen, name, namelen)) {
            return d;
        }
    }
    return NULL;
}

void sb_def_set(SbState *st, const char *name, size_t nlen, int func,
                long number, int valid)
{
    SbDefEntry *d;

    d = def_find(st, name, nlen);
    if (d == NULL) {
        d = (SbDefEntry *)sb_mem_alloc(&st->mem, sizeof(SbDefEntry));
        if (d == NULL) {
            return;     /* out of memory: the name is not remembered */
        }
        d->name = sb_mem_copy(&st->mem, name, nlen);
        if (d->name == NULL) {
            sb_mem_free(&st->mem, d);
            return;
        }
        {
            size_t k;
            for (k = 0; k < nlen; k++) {
                d->name[k] = (char)toupper((unsigned char)name[k]);
            }
        }
        d->namelen = nlen;
        d->next = st->defs;
        st->defs = d;
    }
    d->number = number;
    d->func = func;
    d->valid = valid;
}

/* A DEFine statement has been executed for the stored line @a number.  The
 * ROM registers a DEFine when it runs, not when the line is typed in
 * (probe p16 deleted_proc_direct: -12 before any RUN). */
void sb_def_note(SbState *st, long number, const char *text, size_t len)
{
    SbLexer lx;
    const char *name;
    size_t nlen;
    int func;

    sb_lex_start(&lx, text, len, 0);
    if (lx.type != SB_TK_KW || lx.kw != SB_KW_DEFINE) {
        return;
    }
    sb_lex_next(&lx);
    if (lx.type != SB_TK_KW
        || (lx.kw != SB_KW_PROCEDURE && lx.kw != SB_KW_FUNCTION)) {
        return;
    }
    func = (lx.kw == SB_KW_FUNCTION);
    sb_lex_next(&lx);
    if (lx.type != SB_TK_IDENT) {
        return;
    }
    name = lx.text;
    nlen = lx.tlen;
    sb_def_set(st, name, nlen, func, number, 1);
}

/* The program line @a number is being replaced or deleted. */
void sb_def_drop(SbState *st, long number)
{
    SbDefEntry *d;
    for (d = st->defs; d != NULL; d = d->next) {
        if (d->valid && d->number == number) {
            d->valid = 0;
        }
    }
}

int sb_def_lookup(SbState *st, const char *name, size_t namelen,
                  long *number, int *func)
{
    SbDefEntry *d = def_find(st, name, namelen);
    if (d == NULL) {
        return SB_E_BAD_NAME;       /* never defined: -12 (verified) */
    }
    if (!d->valid) {
        return SB_E_NOT_FOUND;      /* defined, line removed: -7 (verified) */
    }
    *number = d->number;
    *func = d->func;
    return SB_OK;
}

int sb_def_seen(SbState *st, const char *name, size_t namelen, int *func)
{
    SbDefEntry *d = def_find(st, name, namelen);
    if (d == NULL) {
        return 0;
    }
    *func = d->func;
    return 1;
}

static void def_free_all(SbState *st)
{
    SbDefEntry *d = st->defs;
    while (d != NULL) {
        SbDefEntry *next = d->next;
        sb_mem_free(&st->mem, d->name);
        sb_mem_free(&st->mem, d);
        d = next;
    }
    st->defs = NULL;
}

