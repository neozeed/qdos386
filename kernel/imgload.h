/*
 * QDOS/386 R8 - QT_IMG_LOAD: the RUN service, and the kernel boot load.
 *
 * img_load_request(name, args, args_len) loads the named WIN1_ root file into
 * slot 1 by streaming it: the header and symbols are checked by reading, then
 * text and data are copied into the slot in chunks.  Nothing is staged in a
 * whole-file buffer.  args points to user bytes: NUL-terminated arguments after
 * the program name, which the kernel makes argv[0] itself.  args may be 0 with
 * args_len 0.  Returns the job id, or a negative FAT_ERR_*, CHAN_ERR_*, or
 * APP_ERR_* code.
 *
 * IMG_FILE_MAX is the largest file accepted (symbols and strings included).
 * The text + data + BSS limit is APP_IMAGE_LIMIT in aout.h.
 */
#ifndef QDOS386_IMGLOAD_H
#define QDOS386_IMGLOAD_H

#ifdef QDOS_LAYOUT_R9
#define IMG_FILE_MAX (2048u * 1024u)   /* R9: 1 MiB image plus symbols */
#else
#define IMG_FILE_MAX (512u * 1024u)
#endif

int img_load_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len);

/* Boot path (kernel): load the named file into the given slot (0 or 1), with
 * argv = { name }.  The name is a kernel string. */
int img_load_kernel(const char *name, unsigned int want_slot);

#endif
