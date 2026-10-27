/*
 * QDOS/386 R7 - QT_IMG_LOAD: the RUN service.
 *
 * img_load_request(name) reads the named file from the WIN1_ root directory,
 * refuses anything over IMG_MAX bytes before reading it, validates it with the
 * R5 loader, requires slot 1, and loads it as a new job.  The staging buffer is
 * a kernel heap block freed within the same trap.  Returns the job id, or a
 * negative error (FAT errors -40..-47, APP_ERR_* from aout.h).
 */
#ifndef QDOS386_IMGLOAD_H
#define QDOS386_IMGLOAD_H

#define IMG_MAX 8192u              /* executable size cap for RUN (and boot) */

/* RUN service: load the named file into slot 1. */
int img_load_request(unsigned int name_addr);

/* Boot path (kernel): load the named file from the WIN1_ root into the given
 * slot (0 or 1).  The name is a kernel string.  Returns the job id or a
 * negative error; the same checks as RUN, with the slot chosen by the caller. */
int img_load_kernel(const char *name, unsigned int want_slot);

#endif
