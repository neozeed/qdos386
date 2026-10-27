/* malloc.c - QDOS/386 libc (R9): malloc family over the QDOS kernel heap.
 *
 * Each block is one kernel heap block (QT_MEM_ALLOC) of size + HDR bytes.  The
 * kernel owns the block for the job: a job's blocks are reclaimed when it
 * exits, so a program that forgets to free does not leak into the next run.
 * The first HDR bytes hold a magic word and the requested size.  The magic is
 * cleared before the block is returned, so a second free of the same pointer
 * is detected here and ignored; a wild pointer is caught by the kernel's own
 * checks (QT_MEM_FREE refuses it).  Alignment is the kernel's 16 bytes, and
 * the user pointer is 16 bytes past the kernel block, so it is 16-aligned too.
 *
 * Not C89 portable: the heap is shared with the kernel's job stacks and
 * channels, so the limit is the kernel heap, not the process. */
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

#define HDR        16u
#define MAGIC_LIVE 0x4c4c414du        /* "MALL" */
#define MAGIC_DEAD 0x44414544u        /* "DEAD" */
#define MAX_BLOCK  0x7ffffff0u        /* request plus header must fit */

struct qd_hdr {
	unsigned long magic;
	unsigned long size;
	unsigned long pad[2];
};

/* Count of frees ignored because the block was already free (or not ours). */
unsigned long qd_bad_frees = 0;

void *malloc(size_t n)
{
	struct qd_hdr *h;
	if (n == 0 || n > MAX_BLOCK - HDR)
		return NULL;
	h = (struct qd_hdr *) qd_alloc((unsigned int) n + HDR);
	if (h == NULL)
		return NULL;
	h->magic = MAGIC_LIVE;
	h->size = (unsigned long) n;
	return (void *) ((char *) h + HDR);
}

void *calloc(size_t nmemb, size_t size)
{
	size_t total;
	void *p;
	if (nmemb == 0 || size == 0)
		return NULL;
	if (nmemb > (MAX_BLOCK - HDR) / size)
		return NULL;
	total = nmemb * size;
	p = malloc(total);
	if (p != NULL)
		memset(p, 0, total);
	return p;
}

void free(void *p)
{
	struct qd_hdr *h;
	if (p == NULL)
		return;
	h = (struct qd_hdr *) ((char *) p - HDR);
	if (h->magic != MAGIC_LIVE) {
		qd_bad_frees++;
		return;
	}
	h->magic = MAGIC_DEAD;
	qd_free((void *) h);
}

void *realloc(void *p, size_t n)
{
	struct qd_hdr *h;
	void *q;
	size_t old;
	if (p == NULL)
		return malloc(n);
	if (n == 0) {
		free(p);
		return NULL;
	}
	h = (struct qd_hdr *) ((char *) p - HDR);
	if (h->magic != MAGIC_LIVE) {
		qd_bad_frees++;
		return NULL;
	}
	old = (size_t) h->size;
	q = malloc(n);
	if (q == NULL)
		return NULL;      /* the old block is still valid */
	memcpy(q, p, old < n ? old : n);
	free(p);
	return q;
}
