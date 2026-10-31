/*
 * QDOS/386 R13A - loadable bitmap fonts (QFNT v1, docs/R13A-FONT-FORMAT.md) and the font registry.  Opt-in with
 * -DQDOS_R13A (R13 builds keep kernel/font.h).  A font is immutable once registered; surfaces, sessions, the system
 * defaults and the job that loaded it hold references; the block is freed by the SYSTEM job (kfont_poll) when the
 * last reference goes, never under the presenter.  Two fonts are built in (never freed): the recovery font (id 1,
 * public-domain X11 6x9 in a 6x10 cell) and the console font (id 2, X11 8x16, "distributed and modified without
 * restrictions"); see docs/R13A-FONT-PROVENANCE.md.  Freestanding C89; also built on the host by tests/r13a.
 */
#ifndef QDOS386_FONTREG_H
#define QDOS386_FONTREG_H

#define QFNT_HDR        64u
#define QFNT_VERSION    1u
#define QFNT_MAX_FILE   (64u + 512u + 512u * 32u * 2u)   /* 33,344 bytes: 512 glyphs of 16 x 32 */
#define QFNT_NONE       0xFFFFu

#define KFONT_SLOTS     8          /* registered fonts at once (2 built in) */
#define KFONT_RECOVERY  1u         /* ids of the built-in fonts (slot | generation 0 << 8) */
#define KFONT_CONSOLE   2u

#define KFONT_ERR_FORMAT (-4)      /* SURF_ERR_ARG: not a valid QFNT v1 file (any rule) */
#define KFONT_ERR_FULL   (-5)
#define KFONT_ERR_MEM    (-3)
#define KFONT_ERR_STALE  (-2)

#define FONT_A_UNDER   0x01u       /* the R13 text attributes */
#define FONT_A_INVERSE 0x02u
#define FONT_A_BOLD    0x04u
#define FONT_A_NOPAPER 0x08u

struct kfont {
	unsigned int magic;            /* KFONT_MAGIC while registered */
	unsigned int id;               /* (generation << 8) | (slot + 1) */
	int refs;
	int builtin;
	unsigned int owner;            /* the job that loaded it (holds one reference), 0 for built-ins */
	unsigned int gw, gh, cw, ch, xo, yo, rb, count, def, enc;
	unsigned short map[256];       /* code -> glyph, QFNT_NONE: the default glyph */
	char name[20];
	unsigned int crc;              /* the file's payload checksum (identifies the font) */
	const unsigned char *bits;     /* count * gh * rb bytes */
};

/* Validate a QFNT image (every rule of docs/R13A-FONT-FORMAT.md) and fill f (bits point into data).  0 or
 * KFONT_ERR_FORMAT; *why gets a short reason. */
int kfont_parse(const unsigned char *data, unsigned int len, struct kfont *f, const char **why);
unsigned int kfont_crc32(const unsigned char *p, unsigned int n);

void kfont_init(void);
/* Register a copy of a QFNT image (from a caller's buffer, already range-checked) for job `owner` -> id (> 0) or
 * KFONT_ERR_*; the loader holds one reference. */
int kfont_load(const unsigned char *data, unsigned int len, unsigned int owner, const char **why);
struct kfont *kfont_get(unsigned int id);          /* a registered font, or 0 (stale / never) */
void kfont_ref(struct kfont *f);
void kfont_unref(struct kfont *f);                  /* the last reference: queued for kfont_poll */
int kfont_release(unsigned int id, unsigned int owner);   /* drop the loader's reference */
void kfont_job_ended(unsigned int job_id);         /* drop every loader reference held by that job */
void kfont_poll(void);                              /* the SYSTEM job: free fonts without references */
int kfont_list(unsigned int *ids, int max);
unsigned int kfont_live(void);                      /* loaded (non-built-in) fonts registered now */
unsigned int kfont_bytes(void);                     /* heap bytes they use */

/* System defaults (each holds a reference): the font for INDEX8 text and the font of graphical text consoles. */
struct kfont *kfont_default(int console);
int kfont_set_default(int console, unsigned int id);

/* Glyph row r (0..gh-1) of code c, gw bits, leftmost pixel = most significant. */
unsigned int kfont_row(const struct kfont *f, unsigned int c, unsigned int r);
/* Cell row r (0..ch-1) of code c with attributes (bold, underline), cw bits, leftmost = most significant. */
unsigned int kfont_cell_row(const struct kfont *f, unsigned int c, unsigned int r, unsigned int attr);
/* Draw code c with its full cell into an 8-bit buffer (w x h, stride); clipped.  -> 1 if any pixel was inside. */
int kfont_draw_cell(const struct kfont *f, unsigned char *buf, unsigned int w, unsigned int h, unsigned int stride,
                    int x, int y, unsigned int c, unsigned int ink, unsigned int paper, unsigned int attr);

extern const unsigned char font_recovery[];
extern const unsigned int font_recovery_len;
extern const unsigned char font_console[];
extern const unsigned int font_console_len;

#endif
