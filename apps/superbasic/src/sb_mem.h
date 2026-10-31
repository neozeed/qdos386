/*
 * sb_mem.h - memory allocation abstraction for the SuperBASIC/C89 core.
 *
 * The interpreter never calls malloc/free directly.  All heap use goes
 * through an SbAlloc record so that a host (or a test) can substitute its
 * own allocator and count live blocks.
 *
 * Strict ISO C89.
 */
#ifndef SB_MEM_H
#define SB_MEM_H

#include <stddef.h>

typedef struct SbAlloc {
    void *ctx;
    void *(*alloc)(void *ctx, size_t size);
    void (*release)(void *ctx, void *ptr);
} SbAlloc;

/* Initialise @a with the hosted default allocator (malloc/free).  If
 * live_counter is non-NULL it is incremented per live block so tests can
 * detect leaks. */
void sb_alloc_default_init(SbAlloc *a, long *live_counter);

void *sb_mem_alloc(SbAlloc *a, size_t size);
void sb_mem_free(SbAlloc *a, void *ptr);

/* Copy @a n bytes of @a src into a freshly allocated NUL-terminated block.
 * Returns NULL on allocation failure. */
char *sb_mem_copy(SbAlloc *a, const char *src, size_t n);

#endif /* SB_MEM_H */
