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

int app_slot_owner(int slot);                  /* job id, or 0 if free */
void app_release_owner(unsigned int job_id);   /* called from job reclaim */

#endif
