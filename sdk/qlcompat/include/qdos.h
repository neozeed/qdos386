/*
 * QDOS/386 R14 - narrow QL (QDOS / C68) source-compatibility layer, docs/R14-QL-COMPAT-ABI.md.
 *
 * Declares ONLY the QDOS calls, types and C68 runtime names that QLTerm C R1 uses, with the argument types it passes.
 * They are implemented natively (sdk/qlcompat/qlc_*.c) on QDOS/386 services: channels, the R14 serial driver, the
 * caller's console session (QL windows are emulated on its 80x25 cells), files on WIN1, and cooperative co-jobs for
 * mt_cjob children.  This is NOT the QL binary ABI: no 68000 code, trap numbers, QDOS memory map or system variables
 * exist; structures have the native i386 layout.  Calls that cannot be provided return a QDOS error code, never a
 * pretended success (ERR_NI for not implemented).
 */
#ifndef QDOS386_QLCOMPAT_QDOS_H
#define QDOS386_QLCOMPAT_QDOS_H

typedef long chanid_t;
typedef long jobid_t;
typedef short timeout_t;
typedef unsigned char colour_t;

typedef struct {                /* sd_wdef / sd_chenq rectangle: width, height, x, y */
	short q_width, q_height, q_x, q_y;
} QLRECT_t;

typedef struct WINDOWDEF {      /* a window definition (pixels); R14A: the C68 tag, for Hack's _condetails */
	unsigned char border_colour, border_width, paper, ink;
	short width, height, x_origin, y_origin;
} WINDOWDEF_t;

/* QDOS error codes */
#define ERR_NC  (-1)            /* not complete (timeout) */
#define ERR_NJ  (-2)            /* invalid job */
#define ERR_OM  (-3)            /* out of memory */
#define ERR_OR  (-4)            /* out of range */
#define ERR_BO  (-5)            /* buffer overflow */
#define ERR_NO  (-6)            /* channel not open */
#define ERR_NF  (-7)            /* not found */
#define ERR_EX  (-8)            /* already exists */
#define ERR_IU  (-9)            /* in use */
#define ERR_EF  (-10)           /* end of file */
#define ERR_DF  (-11)           /* drive full */
#define ERR_BN  (-12)           /* bad name */
#define ERR_TE  (-13)           /* transmission error */
#define ERR_FF  (-14)           /* format failed */
#define ERR_BP  (-15)           /* bad parameter */
#define ERR_FE  (-16)           /* bad or changed medium */
#define ERR_XP  (-17)           /* error in expression */
#define ERR_OV  (-18)           /* overflow */
#define ERR_NI  (-19)           /* not implemented */
#define ERR_RO  (-20)           /* read only */
#define ERR_BL  (-21)           /* bad line */

/* C68 runtime: the inherited channel count and the converted command tail (built before main) */
extern char *_SPorig;

/* channels */
chanid_t io_open(const char *name, int mode);          /* 0 old exclusive, 1 old shared (read), 2 new, 3 overwrite, 4 dir */
int io_close(chanid_t ch);
int io_fbyte(chanid_t ch, timeout_t t, char *c);        /* 0, ERR_NC, ERR_EF, ... */
int io_sbyte(chanid_t ch, timeout_t t, int c);
int io_fstrg(chanid_t ch, timeout_t t, void *buf, short n);        /* bytes (> 0), or an error */
int io_sstrg(chanid_t ch, timeout_t t, void *buf, short n);        /* bytes (>= 0), or an error */
int io_edlin(chanid_t ch, timeout_t t, char **p, short max, short cur, short *len);
int fs_headr(chanid_t ch, timeout_t t, void *buf, short n);       /* 64-byte QDOS header (emulated) */
int fs_heads(chanid_t ch, timeout_t t, void *buf, short n);       /* ERR_NI */
int fs_save(chanid_t ch, void *buf, unsigned long n);
int fs_date(chanid_t ch, timeout_t t, int which, long *date);     /* ERR_NI */

/* screen (the window must be a CON_ or SCR_ channel) */
int sd_curs(chanid_t ch, timeout_t t);
int sd_cure(chanid_t ch, timeout_t t);
int sd_tab(chanid_t ch, timeout_t t, short col);
int sd_pos(chanid_t ch, timeout_t t, short col, short row);
int sd_ncol(chanid_t ch, timeout_t t);
int sd_pcol(chanid_t ch, timeout_t t);
int sd_nrow(chanid_t ch, timeout_t t);
int sd_prow(chanid_t ch, timeout_t t);
int sd_scrol(chanid_t ch, timeout_t t, short pixels);
int sd_pan(chanid_t ch, timeout_t t, short pixels);
int sd_clear(chanid_t ch, timeout_t t);
int sd_clrtp(chanid_t ch, timeout_t t);
int sd_clrbt(chanid_t ch, timeout_t t);
int sd_clrln(chanid_t ch, timeout_t t);
int sd_clrrt(chanid_t ch, timeout_t t);
int sd_donl(chanid_t ch, timeout_t t);
int sd_setul(chanid_t ch, timeout_t t, int on);
int sd_chenq(chanid_t ch, timeout_t t, QLRECT_t *r);
int sd_fount(chanid_t ch, timeout_t t, char *f1, char *f2);
/* the colour calls take a word argument (QLTerm's qlterm.h calls the _sd_ names with short arguments) */
int _sd_setin(chanid_t ch, timeout_t t, short colour);
int _sd_setst(chanid_t ch, timeout_t t, short colour);
int _sd_setpa(chanid_t ch, timeout_t t, short colour);
int _sd_wdef(chanid_t ch, timeout_t t, short border_colour, short border_width, QLRECT_t *r);
#define sd_setin(ch, t, c) _sd_setin((ch), (t), (short) (c))
#define sd_setst(ch, t, c) _sd_setst((ch), (t), (short) (c))
#define sd_setpa(ch, t, c) _sd_setpa((ch), (t), (short) (c))
#define sd_wdef(ch, t, c, w, r) _sd_wdef((ch), (t), (short) (c), (short) (w), (r))

/* jobs and system */
jobid_t mt_cjob(long code, long data, char *start, jobid_t owner, char **base);
int mt_activ(jobid_t id, unsigned char prio, timeout_t t);
int mt_prior(jobid_t id, int prio);
int mt_susjb(jobid_t id, timeout_t frames, char *flag);
int mt_frjob(jobid_t id, int err);
long mt_rclck(void);
int mt_inf(char **sysvars, long *version);
int mt_dmode(short *mode, short *display);
int mt_baud(long rate);
int mt_ipcom(void *cmd);                                /* ERR_NI: no QL IPC */
int mt_trans(void *table, void *msg);                   /* 0: the serial driver never translates */
int ut_err(long err, chanid_t ch);

#endif
