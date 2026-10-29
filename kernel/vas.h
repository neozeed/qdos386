/*
 * QDOS/386 R12A - per-process address spaces (QDOS_DYNPROC kernels).  See docs/R12A-MEMORY-MAP.md.
 *
 * An address space is a page directory built from the kernel template: identity-mapped kernel memory is shared, every
 * frame-pool page is either one of the space's own frames or not present, and private pages (image, heap, stack) are
 * frames from the pool, mapped one by one (no contiguity).  Frames the space owns carry VAS_OWN (a PTE "available" bit),
 * so teardown frees exactly them and never a template frame.
 *
 * RULE: directories, tables and pool frames are reached through VAS_PTR(physical address).  In the kernel that is the
 * identity mapping of the TEMPLATE directory, so every vas_* function that touches memory must run while the template is
 * the active directory (proc.c: vas_enter_template / vas_leave_template), on a kernel stack.
 *
 * Pure C89; tests/host/r12a builds it with -DVAS_HOST against a simulated physical memory.
 */
#ifndef QDOS386_VAS_H
#define QDOS386_VAS_H

#include "frames.h"

#define VAS_PAGE        4096u
#define VAS_SPAN        0x00400000u          /* bytes per directory entry */
#define VAS_P           0x001u
#define VAS_RW          0x002u
#define VAS_US          0x004u
#define VAS_OWN         0x200u               /* AVL bit 9: frame owned by this space (freed with it) */
#define VAS_MAX_TABLES  16                   /* private page tables per space, directory not counted */

#define VAS_OK            0
#define VAS_ERR_NOFRAMES (-60)               /* same value as the R11J/R11K PG_ERR_NOFRAMES */
#define VAS_ERR_TABLES   (-66)               /* more private tables than VAS_MAX_TABLES */
#define VAS_ERR_ARG      (-67)               /* unaligned address, mapping over a present page */

#ifdef VAS_HOST
extern unsigned char *vas_host_mem;          /* the host test's simulated physical memory */
#define VAS_PTR(pa) ((unsigned int *) (vas_host_mem + (pa)))
#else
#define VAS_PTR(pa) ((unsigned int *) (pa))
#endif

struct vas {
	unsigned int dir;                        /* physical address of the directory, 0 = none */
	int ntab;
	unsigned int tidx[VAS_MAX_TABLES];       /* directory index of each private table */
	unsigned int tpa[VAS_MAX_TABLES];        /* its frame */
	unsigned int owned;                      /* data frames mapped with VAS_OWN */
};

/* Build the template: directory at tpl_dir, tables at tpl_tab[0..ndirs-1] (all page aligned), identity-mapping physical
 * 0 .. ndirs * 4 MiB with supervisor pages. */
void vas_template_build(unsigned int tpl_dir, const unsigned int *tpl_tab, unsigned int ndirs);

/* New directory from the template (directory entries 0 .. ndirs-1 are the identity spans).  On failure everything taken
 * is returned and *v is empty.  Returns VAS_OK or VAS_ERR_NOFRAMES / VAS_ERR_TABLES. */
int vas_create(struct vas *v, struct fr_pool *fp, unsigned int tpl_dir, unsigned int ndirs);

/* Map a new zero-filled frame, owned by the space, at va (page aligned, not present). */
int vas_map_new(struct vas *v, struct fr_pool *fp, unsigned int va, unsigned int flags);

/* Make a template (shared) page not present in this space; only possible where the space has a private copy of the
 * table (spans that contain pool frames).  Used for page 0, so a NULL dereference by a process faults. */
int vas_hide(struct vas *v, unsigned int va);

/* The PTE that maps va (0 if not present). */
unsigned int vas_lookup(const struct vas *v, unsigned int va);

/* Copy n bytes to va in the space; every page must be mapped.  Returns VAS_OK or VAS_ERR_ARG. */
int vas_copy_to(const struct vas *v, unsigned int va, const unsigned char *src, unsigned int n);

/* Release every owned frame, every private table and the directory.  Returns the number of frames returned to the pool
 * (data frames + tables + directory).  Safe on a partially built or empty space. */
unsigned int vas_destroy(struct vas *v, struct fr_pool *fp);

/* Frames held by the space: data frames + private tables + the directory. */
unsigned int vas_frames(const struct vas *v);

#endif
