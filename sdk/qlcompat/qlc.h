/* QDOS/386 R14 QL compatibility layer - internal interface (not for applications). */
#ifndef QDOS386_QLC_H
#define QDOS386_QLC_H
#include "qdos.h"

#define QLC_CHANS     24
#define QLC_CH_BASE   0x00010000L     /* QL channel ids are QLC_CH_BASE + slot (positive, never 0) */
#define QLC_K_FREE    0
#define QLC_K_WIN     1               /* CON_ / SCR_ window */
#define QLC_K_NAT     2               /* a native channel: file or serial */
#define QLC_K_DIR     3               /* io_open mode 4: a directory (fs_headr only) */

struct qlc_win {
	int x, y, w, h;                   /* cells on the 80 x 25 console */
	int cx, cy, pend;                 /* cursor, pending newline */
	int ink, strip, paper, ul;        /* QL colours 0..7 */
	int cursor_on;
	int is_con;                       /* CON_ (keyboard) or SCR_ */
};

struct qlc_chan {
	int kind;
	int nat;                          /* native channel (QLC_K_NAT) */
	int is_ser;
	int eof;
	struct qlc_win win;
	char dirname[64];                 /* QLC_K_DIR: the directory path after the device ("HOME", "" = root) */
	char fname[64];                   /* QLC_K_NAT file: its name after the device, for fs_headr */
};

extern struct qlc_chan qlc_ch[QLC_CHANS];
extern int qlc_con;                   /* the native CON channel of the process */
extern int qlc_ser;                   /* the last serial channel opened (for waits), or -1 */

struct qlc_chan *qlc_get(chanid_t id);
int qlc_err(int native);              /* native QDOS/386 error -> QDOS error */
long qlc_ticks(timeout_t t);          /* QDOS timeout (frames, -1 forever) -> ticks, -1 forever */

/* co-jobs */
void qlc_sleep(long ticks);           /* the calling co-job yields for at least this many ticks (0: just yield) */
int qlc_cojob_current(void);

/* windows */
void qlc_win_init(struct qlc_win *w, int is_con);
int qlc_win_putc(struct qlc_win *w, int c);
void qlc_win_cursor_sync(void);
int qlc_key(int native);              /* native key code -> QL key code, or -1 to ignore */
#endif
