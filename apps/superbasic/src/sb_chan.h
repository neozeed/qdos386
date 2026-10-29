/*
 * sb_chan.h - channel handles: the only way the core reaches a device.
 *
 * The core sees an SbChan as an opaque pointer.  A host (or a test) builds
 * one from a table of operations and an object of its own; the core never
 * looks inside the object and never calls stdio, a system service or a
 * platform trap.  See docs/SB3-CHANNEL-ABI.md.
 *
 * Status: every function returns SB_OK or an SB_E_ code (sb_error.h).  An
 * end of input is not an error: sb_chan_read returns SB_OK with *got == 0.
 */
#ifndef SB_CHAN_H
#define SB_CHAN_H

#include <stddef.h>

#include "sb_mem.h"

/* Capabilities a handle is created with.  An operation the handle does not
 * have fails with SB_E_NOT_IMPLEMENTED, as the ROM does for INPUT #3. */
#define SB_CH_READ   1u
#define SB_CH_WRITE  2u

/* Operations supplied by an adapter.  write may accept fewer bytes than
 * asked (partial write): it sets *done to the number taken, which must be at
 * least 1 unless it returns an error.  read sets *got to at most n; *got == 0
 * with SB_OK means end of input.  flush and close may be NULL. */
typedef struct SbChanOps {
    int (*write)(void *obj, const char *s, size_t n, size_t *done);
    int (*read)(void *obj, char *buf, size_t n, size_t *got);
    int (*flush)(void *obj);
    void (*close)(void *obj);
} SbChanOps;

/* The opaque handle.  Its fields are private to sb_chan.c. */
typedef struct SbChan SbChan;

/* Make a handle around @a obj.  @a ops is kept by pointer and must outlive
 * the handle.  Returns NULL when out of memory. */
SbChan *sb_chan_new(const SbAlloc *mem, const SbChanOps *ops, void *obj,
                    unsigned flags);

/* Close the handle (calls the adapter's close) and free it.  NULL is a
 * no-op. */
void sb_chan_free(SbChan *ch);

unsigned sb_chan_flags(const SbChan *ch);

/* Write all n bytes, looping over partial writes.  SB_E_NOT_OPEN for a NULL
 * handle; SB_E_NOT_IMPLEMENTED without SB_CH_WRITE; SB_E_IO when the adapter
 * makes no progress or reports more bytes than it was given; otherwise the
 * adapter's own error. */
int sb_chan_write(SbChan *ch, const char *s, size_t n);

/* Read up to n bytes into buf.  *got is set before any error is returned. */
int sb_chan_read(SbChan *ch, char *buf, size_t n, size_t *got);

/* Ask the adapter to push out buffered bytes; SB_OK when it has none. */
int sb_chan_flush(SbChan *ch);

#endif /* SB_CHAN_H */
