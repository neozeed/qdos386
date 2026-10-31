/* QDOS/386 R14 QL compatibility layer: channels (io_*), files (fs_*), serial settings, error texts, the C68 start.
 *
 * Channel names: CON_... / SCR_... -> a window on the process's console session (qlc_win.c); SER1/SER2 + QL suffix
 * letters -> the R14 serial driver; WIN1_path -> the WIN1 volume (path separator '/', case-insensitive, long names);
 * a name starting with '/' -> the same, as a native path.  TCP_ and every other QL device: ERR_NF (not found).
 * Open modes: 0 old exclusive (read/write), 1 old shared (read), 2 new exclusive (ERR_EX if it exists), 3 new
 * overwrite, 4 directory (only fs_headr: the directory's name, for QLTerm's file-name handling).
 * Timeouts are QDOS frames (1/50 s; -1 forever, 0 no wait); a wait yields to the other co-jobs (qlc_job.c). */
#include <stdlib.h>
#include <string.h>
#include <qdos386.h>
#include "qlc.h"

struct qlc_chan qlc_ch[QLC_CHANS];
int qlc_con = -1;
int qlc_ser = -1;
static unsigned long def_baud = 9600;

int qlc_err(int e)
{
	switch (e) {
	case -6: case -12: case -13: return ERR_NO;
	case -7: case -18: case -44: case -87: case -88: return ERR_NF;
	case -9: case -84: return ERR_IU;
	case -14: return ERR_OR;
	case -15: return ERR_BP;
	case -19: case -51: return ERR_NC;
	case -46: case -90: case -91: return ERR_BN;
	case -80: return ERR_RO;
	case -81: case -82: return ERR_DF;
	case -83: return ERR_EX;
	case -40: case -85: return ERR_FE;
	}
	return e < 0 ? ERR_BP : e;
}

long qlc_ticks(timeout_t t) { return t < 0 ? -1L : 2L * t; }

struct qlc_chan *qlc_get(chanid_t id)
{
	long k = id - QLC_CH_BASE;
	if (k < 0 || k >= QLC_CHANS || qlc_ch[k].kind == QLC_K_FREE)
		return 0;
	return &qlc_ch[k];
}

static int up(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }

static int prefix(const char *name, const char *p)
{
	while (*p) {
		if (up((unsigned char) *name) != *p)
			return 0;
		name++, p++;
	}
	return 1;
}

static int con_open(void)
{
	if (qlc_con <= 0) {
		qlc_con = qd_open("CON");
		if (qlc_con > 0)
			qd_con_mode(qlc_con, 0);
	}
	return qlc_con;
}

/* deadline helper: 1 while the wait may go on */
static int may_wait(long ticks, unsigned long start)
{
	return ticks < 0 || (long) (qd_ticks() - start) < ticks;
}

chanid_t io_open(const char *name, int mode)
{
	char nat[80];
	int k, r, n;
	unsigned int fl;
	struct qlc_chan *c;
	const char *rest;

	for (k = 0; k < QLC_CHANS && qlc_ch[k].kind != QLC_K_FREE; k++)
		;
	if (k == QLC_CHANS)
		return ERR_NO;
	c = &qlc_ch[k];
	memset(c, 0, sizeof *c);
	if (prefix(name, "CON") || prefix(name, "SCR")) {
		if (con_open() <= 0)
			return ERR_NF;
		c->kind = QLC_K_WIN;
		qlc_win_init(&c->win, prefix(name, "CON"));
		return QLC_CH_BASE + k;
	}
	if (prefix(name, "SER")) {
		struct qd_ser_cfg cfg;
		if (strlen(name) > 12)
			return ERR_BN;
		strcpy(nat, "SER");
		strcat(nat, name + 3);
		r = qd_open(nat);
		if (r < 0)
			return qlc_err(r);
		if (qd_ser_getconfig(r, &cfg) == 0) {
			cfg.baud = def_baud;
			qd_ser_config(r, &cfg);
		}
		c->kind = QLC_K_NAT;
		c->nat = r;
		c->is_ser = 1;
		qlc_ser = r;
		con_open();
		return QLC_CH_BASE + k;
	}
	if (prefix(name, "WIN1_"))
		rest = name + 5;
	else if (name[0] == '/')
		rest = name + 1;
	else
		return ERR_NF;                  /* TCP_, RAM1_, FLP1_ ... and names without a device */
	n = (int) strlen(rest);
	if (n > 62)
		return ERR_BN;
	if (mode == 4) {                    /* directory: the part before the last '/' */
		char *s;
		struct qd_stat st;
		strcpy(c->dirname, rest);
		s = strrchr(c->dirname, '/');
		if (s)
			*s = 0;
		else
			c->dirname[0] = 0;
		strcpy(nat, "WIN1:/");                 /* qd_stat takes a channel name */
		strcat(nat, c->dirname);
		if (qd_stat(nat, &st) != 0 || !st.is_dir)
			return ERR_NF;
		c->kind = QLC_K_DIR;
		return QLC_CH_BASE + k;
	}
	switch (mode) {
	case 0: fl = QD_O_READ | QD_O_WRITE | QD_O_FILE; break;
	case 1: fl = QD_O_READ | QD_O_FILE; break;
	case 2: fl = QD_O_WRITE | QD_O_CREATE | QD_O_EXCL | QD_O_FILE; break;
	case 3: fl = QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE; break;
	default: return ERR_BP;
	}
	strcpy(nat, "WIN1:/");
	strcat(nat, rest);
	r = qd_open_ex(nat, fl);
	if (r < 0)
		return qlc_err(r);
	c->kind = QLC_K_NAT;
	c->nat = r;
	strcpy(c->fname, rest);
	return QLC_CH_BASE + k;
}

int io_close(chanid_t id)
{
	struct qlc_chan *c = qlc_get(id);
	int r = 0;
	if (c == 0)
		return ERR_NO;
	if (c->kind == QLC_K_NAT) {
		r = qd_close(c->nat);
		if (c->nat == qlc_ser)
			qlc_ser = -1;
	}
	c->kind = QLC_K_FREE;
	return r < 0 ? qlc_err(r) : 0;
}

static int key_read(timeout_t t, char *out)
{
	long ticks = qlc_ticks(t);
	unsigned long start = qd_ticks();
	int k, q;
	for (;;) {
		while (qd_con_queued(qlc_con) > 0) {
			k = qd_con_getkey(qlc_con);
			q = qlc_key(k);
			if (q >= 0) {
				*out = (char) q;
				return 0;
			}
		}
		if (ticks == 0 || !may_wait(ticks, start))
			return ERR_NC;
		qlc_sleep(1);
	}
}

int io_fbyte(chanid_t id, timeout_t t, char *b)
{
	struct qlc_chan *c = qlc_get(id);
	long ticks = qlc_ticks(t);
	unsigned long start = qd_ticks();
	int r;
	if (c == 0 || c->kind == QLC_K_DIR)
		return c ? ERR_BP : ERR_NO;
	if (c->kind == QLC_K_WIN) {
		qlc_win_cursor_sync();
		return key_read(t, b);
	}
	for (;;) {
		r = qd_read(c->nat, b, 1);
		if (r == 1)
			return 0;
		if (r == 0)
			return ERR_EF;
		if (r != QD_SER_AGAIN)
			return qlc_err(r);
		if (ticks == 0 || !may_wait(ticks, start))
			return ERR_NC;
		qlc_sleep(1);
	}
}

int io_fstrg(chanid_t id, timeout_t t, void *buf, short n)
{
	struct qlc_chan *c = qlc_get(id);
	long ticks = qlc_ticks(t);
	unsigned long start = qd_ticks();
	unsigned char *p = (unsigned char *) buf;
	int got = 0, r;
	if (c == 0 || c->kind == QLC_K_DIR)
		return c ? ERR_BP : ERR_NO;
	if (n <= 0)
		return 0;
	if (c->kind == QLC_K_WIN) {
		while (got < n) {
			r = key_read(got ? 0 : t, (char *) p + got);
			if (r != 0)
				break;
			got++;
		}
		return got ? got : ERR_NC;
	}
	while (got < n) {
		r = qd_read(c->nat, p + got, (unsigned int) (n - got > 256 ? 256 : n - got));
		if (r > 0) {
			got += r;
			continue;
		}
		if (r == 0)
			return got ? got : ERR_EF;
		if (r != QD_SER_AGAIN)
			return got ? got : qlc_err(r);
		if (ticks == 0 || !may_wait(ticks, start))
			return got ? got : ERR_NC;
		qlc_sleep(1);
	}
	return got;
}

static int nat_write(struct qlc_chan *c, timeout_t t, const unsigned char *p, int n)
{
	long ticks = qlc_ticks(t);
	unsigned long start = qd_ticks();
	int done = 0, r;
	while (done < n) {
		r = qd_write(c->nat, p + done, (unsigned int) (n - done > 256 ? 256 : n - done));
		if (r > 0) {
			done += r;
			continue;
		}
		if (r != QD_SER_AGAIN && r != 0)
			return done ? done : qlc_err(r);
		if (ticks == 0 || !may_wait(ticks, start))
			return done ? done : ERR_NC;
		qlc_sleep(1);
	}
	return done;
}

int io_sbyte(chanid_t id, timeout_t t, int b)
{
	struct qlc_chan *c = qlc_get(id);
	unsigned char x = (unsigned char) b;
	int r;
	if (c == 0 || c->kind == QLC_K_DIR)
		return c ? ERR_BP : ERR_NO;
	if (c->kind == QLC_K_WIN) {
		qlc_win_putc(&c->win, x);
		qlc_win_cursor_sync();
		return 0;
	}
	r = nat_write(c, t, &x, 1);
	return r == 1 ? 0 : r;
}

int io_sstrg(chanid_t id, timeout_t t, void *buf, short n)
{
	struct qlc_chan *c = qlc_get(id);
	const unsigned char *p = (const unsigned char *) buf;
	int i;
	if (c == 0 || c->kind == QLC_K_DIR)
		return c ? ERR_BP : ERR_NO;
	if (n <= 0)
		return 0;
	if (c->kind == QLC_K_WIN) {
		for (i = 0; i < n; i++)
			qlc_win_putc(&c->win, p[i]);
		qlc_win_cursor_sync();
		return n;
	}
	return nat_write(c, t, p, n);
}

/* ---- files ---- */

static void put32(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char) (v >> 24); p[1] = (unsigned char) (v >> 16); p[2] = (unsigned char) (v >> 8); p[3] = (unsigned char) v;
}

/* a QDOS file header as far as QDOS/386 has the information: length, type 0 (data), no dataspace, the name; the
 * dates are 0 (unknown) */
int fs_headr(chanid_t id, timeout_t t, void *buf, short n)
{
	struct qlc_chan *c = qlc_get(id);
	unsigned char h[64];
	const char *name;
	long size = 0;
	int k;
	(void) t;
	if (c == 0)
		return ERR_NO;
	if (c->kind == QLC_K_WIN || c->is_ser)
		return ERR_BP;
	memset(h, 0, sizeof h);
	if (c->kind == QLC_K_NAT) {
		size = qd_fsize(c->nat);
		if (size < 0)
			return qlc_err((int) size);
		name = c->fname;
	} else {
		name = c->dirname;
		h[5] = 0xFF;                     /* QDOS: type 255 = directory */
	}
	put32(h, (unsigned long) size);
	k = (int) strlen(name);
	if (k > 36)
		k = 36;
	h[14] = 0;
	h[15] = (unsigned char) k;
	memcpy(h + 16, name, (unsigned int) k);
	if (n > 64)
		n = 64;
	if (n < 0)
		return ERR_BP;
	memcpy(buf, h, (unsigned int) n);
	return n;
}

int fs_save(chanid_t id, void *buf, unsigned long n)
{
	struct qlc_chan *c = qlc_get(id);
	if (c == 0)
		return ERR_NO;
	if (c->kind != QLC_K_NAT || c->is_ser)
		return ERR_BP;
	return nat_write(c, -1, (const unsigned char *) buf, (int) n);
}

int fs_heads(chanid_t id, timeout_t t, void *buf, short n)
{
	(void) id; (void) t; (void) buf; (void) n;
	return ERR_NI;                       /* FAT files carry no QDOS header (type, dataspace) */
}

int fs_date(chanid_t id, timeout_t t, int which, long *date)
{
	(void) id; (void) t; (void) which; (void) date;
	return ERR_NI;                       /* no kernel call sets a file's time stamp */
}

/* ---- serial settings, IPC, translation ---- */

int mt_baud(long rate)
{
	int k;
	struct qd_ser_cfg cfg;
	if (rate <= 0 || rate > 115200L || 115200L % rate != 0)
		return ERR_BP;
	def_baud = (unsigned long) rate;
	qd_ser_default_baud(1, def_baud);
	qd_ser_default_baud(2, def_baud);
	for (k = 0; k < QLC_CHANS; k++)
		if (qlc_ch[k].kind == QLC_K_NAT && qlc_ch[k].is_ser && qd_ser_getconfig(qlc_ch[k].nat, &cfg) == 0) {
			cfg.baud = def_baud;
			qd_ser_config(qlc_ch[k].nat, &cfg);
		}
	return 0;
}

int mt_ipcom(void *cmd)
{
	(void) cmd;
	return ERR_NI;                       /* no QL IPC processor: no beep, no Hermes */
}

int mt_trans(void *table, void *msg)
{
	(void) table; (void) msg;
	return 0;                            /* the serial driver never translates: nothing to switch */
}

int ut_err(long err, chanid_t ch)
{
	static const char *const m[] = { "", "not complete", "invalid job", "out of memory", "out of range",
		"buffer full", "channel not open", "not found", "already exists", "in use", "end of file", "drive full",
		"bad name", "Xmit error", "format failed", "bad parameter", "bad or changed medium", "error in expression",
		"overflow", "not implemented", "read only", "bad line" };
	const char *s = (err < 0 && err >= -21) ? m[-err] : "unknown error";
	io_sstrg(ch, -1, (void *) s, (short) strlen(s));
	return 0;
}

/* ---- the C68 start: _SPorig = inherited channel count 0 (QDOS/386 passes none) + the command tail ---- */
#ifndef QLC_HOST_TEST
#undef main
char *_SPorig;
static char sp_block[2 + 130];
int qlc_program_main(void);         /* the program's own main, compiled with -Dmain=qlc_program_main */

int main(int argc, char **argv)
{
	int i, n = 0, k;
	sp_block[0] = sp_block[1] = 0;
	for (i = 1; i < argc; i++) {
		k = (int) strlen(argv[i]);
		if (n + k + 2 > 128)
			break;
		if (n)
			sp_block[2 + n++] = ' ';
		memcpy(sp_block + 2 + n, argv[i], (unsigned int) k);
		n += k;
	}
	sp_block[2 + n] = 0;
	_SPorig = sp_block;
	con_open();
	return qlc_program_main();
}
#endif
