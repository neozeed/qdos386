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

#endif
