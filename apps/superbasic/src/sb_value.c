/*
 * sb_value.c - value construction, release and conversion.
 */
#include <string.h>

#include "sb_error.h"
#include "sb_format.h"
#include "sb_value.h"

void sb_val_init_num(SbValue *v, double d)
{
    v->type = SB_VAL_NUM;
    v->num = d;
    v->str = NULL;
    v->len = 0;
}

void sb_val_init_undef(SbValue *v)
{
    v->type = SB_VAL_UNDEF;
    v->num = 0.0;
    v->str = NULL;
    v->len = 0;
}

int sb_val_init_str(SbAlloc *a, SbValue *v, const char *s, size_t n)
{
    char *p;

    p = sb_mem_copy(a, s, n);
    if (p == NULL) {
        sb_val_init_num(v, 0.0);
        return SB_E_OUT_OF_MEMORY;
    }
    v->type = SB_VAL_STR;
    v->num = 0.0;
    v->str = p;
    v->len = n;
    return SB_OK;
}

void sb_val_free(SbAlloc *a, SbValue *v)
{
    if (v->type == SB_VAL_STR) {
        sb_mem_free(a, v->str);
    }
    sb_val_init_num(v, 0.0);
}

int sb_val_to_number(const SbValue *v, double *out)
{
    switch (v->type) {
    case SB_VAL_NUM:
        *out = v->num;
        return SB_OK;
    case SB_VAL_STR:
        if (sb_parse_prefix(v->str, v->len, out)) {
            return SB_OK;
        }
        return SB_E_EXPRESSION;
    default:
        return SB_E_EXPRESSION;
    }
}

int sb_val_to_string(SbAlloc *a, const SbValue *v, SbValue *out)
{
    char buf[32];

    switch (v->type) {
    case SB_VAL_STR:
        return sb_val_init_str(a, out, v->str, v->len);
    case SB_VAL_NUM:
        sb_format_number(v->num, buf, sizeof(buf));
        return sb_val_init_str(a, out, buf, strlen(buf));
    default:
        sb_val_init_undef(out);
        return SB_E_EXPRESSION;
    }
}
