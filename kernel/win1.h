/*
 * QDOS/386 R6 - the WIN1_ volume: one FAT16 volume mounted from the platform
 * disk, reached through channels named "WIN1_<FILE>" (for example
 * "WIN1_HELLO.AOU").  Only the root directory is visible; the channel API
 * does not change if directories are added later.
 */
#ifndef QDOS386_WIN1_H
#define QDOS386_WIN1_H

#include "fat16.h"

/* Mount the platform disk as WIN1_.  Idempotent once successful. */
int win1_mount(void);

/* The mounted volume, or 0 if win1_mount has not succeeded. */
struct fat16_vol *win1_volume(void);

#ifdef QDOS_FS_WRITE
/* R11M: a new job inherits its creator's current directory (called by job_alloc_sized). */
void win1_cwd_inherit(unsigned int child_id, unsigned int parent_id);
/* R11M: open a program image by path (relative to the calling job's directory, case-folded). */
int win1_open_img(const char *name, struct fat16_file *f);
#endif

#endif
