/*
 * sb_mem.c - hosted default allocator for the SuperBASIC/C89 core.
 */
#include <stdlib.h>
#include <string.h>

#include "sb_mem.h"

static void *default_alloc(void *ctx, size_t size)
{
    void *p;
    p = malloc(size > 0 ? size : 1);
    if (p != NULL && ctx != NULL) {
        ++(*(long *)ctx);
    }
    return p;
}

static void default_release(void *ctx, void *ptr)
{
    if (ptr != NULL) {
        free(ptr);
        if (ctx != NULL) {
            --(*(long *)ctx);
        }
    }
}

void sb_alloc_default_init(SbAlloc *a, long *live_counter)
{
    a->ctx = (void *)live_counter;
    a->alloc = default_alloc;
    a->release = default_release;
}

void *sb_mem_alloc(SbAlloc *a, size_t size)
{
    return a->alloc(a->ctx, size);
}

void sb_mem_free(SbAlloc *a, void *ptr)
{
    if (ptr != NULL) {
        a->release(a->ctx, ptr);
    }
}

char *sb_mem_copy(SbAlloc *a, const char *src, size_t n)
{
    char *p;
    p = (char *)sb_mem_alloc(a, n + 1);
    if (p != NULL) {
        if (n > 0) {
            memcpy(p, src, n);
        }
        p[n] = '\0';
    }
    return p;
}
