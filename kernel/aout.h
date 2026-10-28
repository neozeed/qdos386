/*
 * QDOS/386 R5 - guest-side a.out loader and transfer-area reader.
 *
 * The transfer area (XFER_BASE) holds a directory and exact a.out payloads.
 * Both targets present the same bytes at the same address.  aout_check is
 * pure: it reads the image and changes nothing.  app_load validates first and
 * only then creates the job and copies text and data into the slot.
 */
#ifndef QDOS386_AOUT_H
#define QDOS386_AOUT_H

#include "appmap.h"

/* Transfer directory (see tools/make-xfer.py). */
#define XFER_MAGIC       0x52465851u   /* 'Q','X','F','R' little endian */
#define XFER_VERSION     1u
#define XFER_DIR_BYTES   128u
#define XFER_MAX_IMAGES  4u

/* a.out (OMAGIC, see docs/R5-AOUT-FORMAT.md). */
#define AOUT_MAGIC       0x107u
#define AOUT_HDR_BYTES   32u
#define AOUT_SYM_BYTES   12u

/* Loader results.  Negative; the job-related codes are used by app_load only. */
#define APP_OK           0
#define APP_ERR_SIZE     (-20)  /* truncated, trailing bytes, or a table past the end */
#define APP_ERR_MAGIC    (-21)  /* not OMAGIC 0407 */
#define APP_ERR_RELOC    (-22)  /* relocation records present */
#define APP_ERR_FORMAT   (-23)  /* symbol or string table layout invalid */
#define APP_ERR_LAYOUT   (-24)  /* alignment, or image larger than a slot */
#define APP_ERR_ENTRY    (-25)  /* entry is not a slot base, or _start disagrees */
#define APP_ERR_XFER     (-26)  /* transfer directory invalid */
#define APP_ERR_NOIMAGE  (-27)  /* no such image index */
#define APP_ERR_SLOTBUSY (-28)  /* the slot for this image is in use */
#define APP_ERR_NOJOB    (-29)  /* no free job slot */
#define APP_ERR_NOMEM    (-30)  /* no heap memory for the job stack */
#define APP_ERR_TOOBIG    (-31)  /* RUN: file larger than IMG_MAX (imgload.h) */
#define APP_ERR_NOTSLOT1  (-32)  /* RUN: image is not linked for slot 1 */
#define APP_ERR_NOTSLOT0  (-33)  /* boot: image is not linked for slot 0 */
#define APP_ERR_ARGS     (-34)  /* argument block: too many, too long, or not NUL-terminated */
#define APP_ERR_NOPROFILE (-35)  /* R11H: profile not available on this memory map */
#define APP_ERR_ADDROCC  (-36)  /* R11H: the profile's address range is occupied */

/* R8 limits.  Text + data + BSS of one image may not exceed APP_IMAGE_LIMIT;
 * the slot's top APP_ARG_BYTES hold the argument block, so the limit is below
 * the slot size minus the block.  See docs/R8-LOADER-DESIGN.md. */
#ifdef QDOS_LAYOUT_R9
/* R9: slot 0 keeps the 224 KiB shell limit; slot 1 takes a 1 MiB image less
 * the argument block at the top of the slot. */
#define APP_IMAGE_LIMIT_OF(n) ((n) == 0 ? 0x00038000u : 0x000FFE00u)   /* profiles 1..3 */
#else
#define APP_IMAGE_LIMIT_OF(n) 0x00038000u     /* 224 KiB, both slots */
#endif
#define APP_ARG_BYTES    512u            /* argument block at the top of the slot */
#define APP_ARG_MAX      16u             /* argc, including argv[0] */
#define APP_ARG_STR_OFF  80u             /* strings start here inside the block */
#define APP_ARG_STR_MAX  (APP_ARG_BYTES - APP_ARG_STR_OFF)
#ifndef QDOS_APP_STACK
#define QDOS_APP_STACK   16384u          /* default: 16 KiB (R11L self-hosting kernels build with -DQDOS_APP_STACK=65536u) */
#endif
#define APP_STACK_BYTES  QDOS_APP_STACK  /* job stack for applications (heap block) */
#define APP_RESERVED     (-1)            /* slot reserved while its image is copied */

/* Source-neutral reader: fill dst with n bytes at offset off of the image.
 * Returns APP_OK, or a negative code (APP_ERR_SIZE, or a FAT error). */
typedef int (*app_rd_fn)(void *ctx, unsigned int off, unsigned char *dst, unsigned int n);

struct app_image {
	const unsigned char *img;   /* exact file bytes (in the transfer area) */
	unsigned int len;           /* exact file length from the directory */
	unsigned int slot;          /* 0 or 1 */
	unsigned int text;          /* bytes */
	unsigned int data;          /* bytes, includes alignment padding */
	unsigned int bss;           /* bytes, not in the file */
	unsigned int entry;         /* == slot base */
};

struct app_xfer_entry {
	unsigned int offset;        /* from XFER_BASE */
	unsigned int length;        /* exact file length */
	char name[5];               /* 4 characters, NUL terminated */
};

/* Parse and validate the directory at base (size bytes available).  Fills
 * entries and returns the count, or APP_ERR_XFER.  Pure. */
int xfer_parse(const unsigned char *base, unsigned int size,
               struct app_xfer_entry *entries, unsigned int max);

int app_xfer_init(void);                       /* parse XFER_BASE at boot */
int app_image_count(void);
int app_image_bytes(int index, const unsigned char **img, unsigned int *len);

/* Validate an image.  Pure: no memory is written and no job is created. */
int aout_check(const unsigned char *img, unsigned int len, struct app_image *out);

/* Validate, claim the slot, create the job, then copy text and data and zero
 * BSS.  Returns the job id (positive) or a negative APP_ERR_* code. */
int app_load(int index);

/* The same load from a byte buffer (R6: a file read through a channel).  The
 * buffer is checked exactly as a transfer image is; see aout_check. */
int app_load_bytes(const unsigned char *img, unsigned int len);

/* R8: validate and load an image from any reader, with its argument block.
 * want_slot: 0 or 1 to require that slot, -1 for either.  packed holds plen
 * bytes of NUL-terminated strings (argv[0] first); plen 0 means argc 0.
 * Validation reads only; the slot is reserved only after it passes. */
int app_load_src(app_rd_fn rd, void *ctx, unsigned int len, int want_slot,
                 const unsigned char *packed, unsigned int plen);

/* Argument block address for a job (0 if none). */
unsigned int app_args_of(int job_slot);

int app_slot_owner(int slot);                  /* job id, or 0 if free */
void app_release_owner(unsigned int job_id);   /* called from job reclaim */

#endif
