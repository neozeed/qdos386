/*
 * QDOS/386 R12B - per-process environment (QDOS_ENV kernels).  See docs/R12B-ENV-PROCESS-ABI.md.
 *
 * An environment is a block of entries "NAME=value\0", sorted by NAME, owned by one job and kept in the kernel heap
 * (owner = the job id, so the ordinary reclaim frees it).  Names: 1..31 characters, first a letter or '_', then
 * letters, digits or '_'; compared without regard to case and stored in upper case.  Values: 0..255 bytes, any byte
 * except NUL (an empty value is a set variable, distinct from an absent one).  At most 32 variables and 2,048 bytes of
 * entries per job.  A new process gets a copy (snapshot) of its creator's block before it can run.
 *
 * The block functions are pure C89 (tests/host/r12b builds them with -DENV_HOST); the trap glue is kernel only.
 */
#ifndef QDOS386_ENV_H
#define QDOS386_ENV_H

#define ENV_MAX_VARS   32
#define ENV_NAME_MAX   31
#define ENV_VALUE_MAX  255
#define ENV_BYTES_MAX  2048

/* QDOS-style codes (negative) */
#define ENV_OK            0
#define ENV_ERR_NOMEM   (-3)     /* kernel heap exhausted */
#define ENV_ERR_RANGE   (-4)     /* value longer than ENV_VALUE_MAX */
#define ENV_ERR_FULL    (-5)     /* too many variables or too many bytes */
#define ENV_ERR_NOTFOUND (-7)    /* no such variable (also: list index past the end) */
#define ENV_ERR_NAME   (-15)     /* name malformed */

/* Pure block operations. */
int env_name_ok(const char *name, unsigned int nlen);                  /* ENV_OK or ENV_ERR_NAME */
int env_find(const char *blk, unsigned int used, const char *name, unsigned int nlen,
             unsigned int *voff, unsigned int *vlen);                  /* ENV_OK or ENV_ERR_NOTFOUND */
unsigned int env_count(const char *blk, unsigned int used);
/* Entry number i: offset and length of "NAME=value" (without the NUL).  ENV_ERR_NOTFOUND past the end. */
int env_entry(const char *blk, unsigned int used, unsigned int i, unsigned int *off, unsigned int *len);
/* Build into dst (capacity ENV_BYTES_MAX) the block src with NAME set to val (unset != 0: removed).  The name is
 * validated and upper-cased; *dused gets the new size.  src is never changed. */
int env_edit(const char *src, unsigned int used, char *dst, unsigned int *dused, const char *name,
             unsigned int nlen, const char *val, unsigned int vlen, int unset);

#ifndef ENV_HOST
/* Kernel glue (jobs.c traps and proc.c spawn). */
int env_get_trap(unsigned int name_addr, unsigned int buf_addr, unsigned int size);
int env_set_trap(unsigned int name_addr, unsigned int val_addr);       /* val_addr 0: unset */
/* index 0xFFFFFFFF: (count << 16) | bytes.  index 0xFFFFFFFE (diagnostic): the next buf_addr (<= 16) copies at process
 * creation fail with ENV_ERR_NOMEM, so the spawn rollback can be tested; returns the number of copies made so far. */
int env_list_trap(unsigned int index, unsigned int buf_addr, unsigned int size);
int env_inherit(int child_slot, unsigned int child_id, int parent_slot);   /* ENV_OK or ENV_ERR_NOMEM */
void env_job_gone(int slot);
/* For kernel users (the program search path): the value of name for the job in slot, or 0; *vlen its length. */
const char *env_value_of(int slot, const char *name, unsigned int *vlen);
#endif

#endif
