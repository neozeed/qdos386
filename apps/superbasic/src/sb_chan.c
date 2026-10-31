/*
 * sb_chan.c - the channel handle.  Pure C89: no stdio, no platform calls.
 */
#include <stddef.h>

#include "sb_chan.h"
#include "sb_error.h"
#include "sb_mem.h"

struct SbChan {
    SbAlloc mem;
    const SbChanOps *ops;
    void *obj;
    unsigned flags;
};

SbChan *sb_chan_new(const SbAlloc *mem, const SbChanOps *ops, void *obj,
                    unsigned flags)
{
    SbAlloc a = *mem;
    SbChan *ch = (SbChan *)sb_mem_alloc(&a, sizeof(SbChan));

    if (ch == NULL) {
        return NULL;
    }
    ch->mem = a;
    ch->ops = ops;
    ch->obj = obj;
    ch->flags = flags;
    return ch;
}

void sb_chan_free(SbChan *ch)
{
    SbAlloc a;

    if (ch == NULL) {
        return;
    }
    if (ch->ops->close != NULL) {
        ch->ops->close(ch->obj);
    }
    a = ch->mem;
    sb_mem_free(&a, ch);
}

unsigned sb_chan_flags(const SbChan *ch)
{
    return (ch == NULL) ? 0u : ch->flags;
}

int sb_chan_write(SbChan *ch, const char *s, size_t n)
{
    size_t off = 0;

    if (ch == NULL) {
        return SB_E_NOT_OPEN;
    }
    if ((ch->flags & SB_CH_WRITE) == 0 || ch->ops->write == NULL) {
        return SB_E_NOT_IMPLEMENTED;
    }
    while (off < n) {
        size_t done = 0;
        int rc = ch->ops->write(ch->obj, s + off, n - off, &done);
        if (rc != SB_OK) {
            return rc;
        }
        if (done == 0 || done > n - off) {
            return SB_E_IO;
        }
        off += done;
    }
    return SB_OK;
}

int sb_chan_read(SbChan *ch, char *buf, size_t n, size_t *got)
{
    int rc;

    *got = 0;
    if (ch == NULL) {
        return SB_E_NOT_OPEN;
    }
    if ((ch->flags & SB_CH_READ) == 0 || ch->ops->read == NULL) {
        return SB_E_NOT_IMPLEMENTED;
    }
    rc = ch->ops->read(ch->obj, buf, n, got);
    if (rc != SB_OK) {
        return rc;
    }
    if (*got > n) {
        *got = 0;
        return SB_E_IO;
    }
    return SB_OK;
}

int sb_chan_flush(SbChan *ch)
{
    if (ch == NULL) {
        return SB_E_NOT_OPEN;
    }
    if (ch->ops->flush == NULL) {
        return SB_OK;
    }
    return ch->ops->flush(ch->obj);
}
