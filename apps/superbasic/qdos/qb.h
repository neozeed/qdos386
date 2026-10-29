/*
 * qb.h - QDOS/386 host layer for the SuperBASIC/C89 core (R12 Stage 4).
 *
 * The core (apps/superbasic/src, unchanged apart from one #ifndef guard in
 * sb_state.h) reaches the world only through SbChan handles.  This layer
 * supplies those handles over the job's CON channel: output goes out with
 * qd_write, input comes from a line editor on raw console keys.  A SuperBASIC
 * channel handle is NOT a QDOS channel number and not a FILE*: the adapter
 * object decides what it talks to.
 *
 * Strict C89.
 */
#ifndef QB_H
#define QB_H

#include "sb_chan.h"
#include "sb_state.h"

#define QB_LINE_MAX  255      /* longest typed line (without the newline)   */
#define QB_HIST_N    16       /* command history entries (typed lines)      */

/* The interpreter's private stack.  It MUST be a block of the shared kernel heap, not an array in the program's own window: the
 * scheduler loads the next job's page directory while still running on the interrupted job's stack, and a stack inside a paged
 * job's private window is not mapped in the other directories (the guest crashed with a triple fault: docs/R12-SUPERBASIC-PORT.md).
 * The heap is about 0.9 MB for the whole system, so the default is modest. */
#define QB_STACK_DEFAULT_KIB 256u
#define QB_STACK_MIN_KIB     128u
#define QB_STACK_MAX_KIB     384u

/* ---- console I/O (qb_io.c) ---- */
int  qb_io_init(void);                 /* 0 ok; <0 no console              */
void qb_io_done(void);                 /* restore the console mode         */
int  qb_io_editor(void);               /* 1 = raw-key line editor, 0 = cooked fallback */
void qb_out(const char *s, unsigned int n);   /* straight to the console   */
void qb_puts(const char *s);
/* Next typed line into dst (cap >= QB_LINE_MAX + 2), newline included.
 * Returns the length (> 0), or 0 at end of input (Ctrl+D on an empty line). */
int  qb_getline(char *dst, int cap);
int  qb_attach(SbState *st);           /* install #0..#3 and the diag handle */
void qb_detach(SbState *st);
int  qb_break_seen(void);              /* Ctrl+C was typed while a program ran (cleared on read) */

/* ---- private stack (qb_stack.c) ---- */
/* Run fn(arg) on a heap block of kib KiB (the largest of kib, kib/2, .. QB_STACK_MIN_KIB that the heap can give), painted first.
 * The core's call-depth limit (qb_depth_limit) is set from the size actually obtained.  Returns fn's result, or -1 if no block
 * could be obtained (fn not called).  The block stays allocated until qb_stack_free() so that the figures below can be read. */
int  qb_stack_call(unsigned int kib, int (*fn)(void *), void *arg);
void qb_stack_free(void);
unsigned int qb_stack_used(void);      /* high-water mark in bytes (valid after the call) */
unsigned int qb_stack_size(void);      /* usable bytes */
int  qb_stack_intact(void);            /* 1 if the guard words at the low end were untouched */
extern int qb_depth_limit;             /* SB_MAX_CALL_DEPTH of the core (see sb_state.h) */

#endif /* QB_H */
