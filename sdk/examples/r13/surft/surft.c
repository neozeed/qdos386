/* SURFT.AOU (R13): the display-surface kernel call from a C89 program.
 *   SURFT               the API matrix ("SURFT ok/FAIL ..."; ends "SURFT PASS checks=N"; also WIN1:/SURFT.LOG)
 *   SURFT text T MSG    move to a new private text session, print MSG and a ruler, keep it T ticks
 *   SURFT gfx T [w h]   open an INDEX8 surface (default 320x200) on a private session, draw a test card, keep it T
 *                       ticks (with "fg" as the 5th argument: also bring it to the front)
 *   SURFT panel K       (Gate 1) draw panel K (a fixed pattern of characters and attributes, through the console
 *                       calls and qd_text_cells) on a private text session, park the cursor at row 21, column 5*K,
 *                       then read keys until 'q' and log them to WIN1:/PANELK.LOG ("PANEL K session S keys=...")
 *   SURFT card K        (Gate 2) a 320x200 INDEX8 test card K on a private session: its own palette 0..47, colour
 *                       bars, every font code 0x20..0xFF, the four text attributes, a text-stream line with the
 *                       cursor shown; then keys until 'q'; logs "CARD K sum=S sum2=S2 ..." to WIN1:/CARDK.LOG
 *                       (sum: the surface checksum after drawing, sum2: at 'q' - switching must not change it)
 *   SURFT gfxfault      open a 320x200 surface, draw, then store through a NULL pointer: the kernel ends the job
 *                       (fault) and must free the surface and the session like any other ending
 *   SURFT caps          print the capability word: "SURFT caps version V text T index8 I show-text S show-gfx G"
 *   SURFT stats         print the kernel's surface counters: "SURFT stats open=O opened=N refused=R freed=F"
 *   SURFT child         (spawned by the matrix) open a 64x40 surface and end at once (status 0) / end 9
 * Program title: "Surface test". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

char _prog_name[] = "Surface test";

static int checks, fails;
static char logbuf[6144];
static unsigned int loglen;
static unsigned char pix[64 * 40], back[64 * 40];

static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	if (!ok)
		fails++;
	sprintf(line, "SURFT %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	if (loglen + strlen(line) < sizeof logbuf) {
		strcpy(logbuf + loglen, line);
		loglen += (unsigned int) strlen(line);
	}
	printf("%s", line);
}

static int spawn1(const char *a, const char *b, int con)
{
	char buf[64];
	int n = 0;

	n += sprintf(buf + n, "%s", a) + 1;
	if (b)
		n += sprintf(buf + n, "%s", b) + 1;
	return qd_spawn("SURFT.AOU", buf, (unsigned int) n, 0u, con);
}

static void matrix(void)
{
	struct qd_surface_info in, in2;
	long id, id2, r;
	int rc, i, caps;
	unsigned char rgb[6], rgb2[6];
	long s1, s2;
	unsigned long stv[8];

	caps = qd_surface_caps();
	check("caps: version 1, text and INDEX8 surfaces", (caps >> 16) == 1 && (caps & 3) == 3, caps);
	rc = qd_surface_info(0, &in);
	check("own session is a text session 80x25", rc == 0 && in.kind == QD_SURF_TEXT && in.w == 80 && in.h == 25, (long) in.session);
	id = qd_surface_new();
	rc = qd_surface_info(0, &in2);
	check("qd_surface_new: a private text session of its own", id > 0 && rc == 0 && in2.id == (unsigned long) id &&
	      (in2.flags & QD_SURF_PRIVATE) && in2.session >= 4 && in2.users == 1, (long) in2.session);
	printf("SURFT now on private session %lu\n", in2.session);
	check("stdout follows the job to the new session (this line is in it)", 1, 0);
	rc = qd_surface_info(in.id, &in);
	check("the old session still exists (the shell's)", rc == 0, rc);
	check("INDEX8 call on a text session refused -7", qd_gfx_fill(0, 0, 1, 1, 1) == QD_SURF_ERR_NOGFX, qd_gfx_fill(0, 0, 1, 1, 1));
	check("bad gfx sizes refused -4", qd_gfx_open(321, 10, 0) == QD_SURF_ERR_ARG && qd_gfx_open(10, 201, 0) == QD_SURF_ERR_ARG &&
	      qd_gfx_open(0, 5, 0) == QD_SURF_ERR_ARG && qd_gfx_open(5, 5, 0x80) == QD_SURF_ERR_ARG, 0);
	id2 = qd_gfx_open(64, 40, 0);
	rc = qd_surface_info(0, &in);
	check("gfx 64x40 on the private session (no second move)", id2 > 0 && rc == 0 && in.kind == QD_SURF_INDEX8 && in.w == 64 &&
	      in.h == 40 && in.stride == 64 && in.session == in2.session && (in.flags & QD_SURF_OWNER), id2);
	check("the text surface id is stale after the mode change", qd_surface_info(id, &in2) == QD_SURF_ERR_STALE, 0);
	check("second gfx open refused -6", qd_gfx_open(64, 40, 0) == QD_SURF_ERR_BUSY, 0);
	r = qd_gfx_fill(0, 0, 64, 40, 3);
	check("fill whole surface = 2560 pixels", r == 2560, r);
	r = qd_gfx_fill(60, 36, 10, 10, 4);
	check("fill clipped at the corner = 16", r == 16, r);
	check("negative size refused -4", qd_gfx_fill(0, 0, -1, 1, 1) == QD_SURF_ERR_ARG, 0);
	check("rectangle outside = 0", qd_gfx_fill(64, 0, 5, 5, 1) == 0 && qd_gfx_fill(-5, -5, 5, 5, 1) == 0, 0);
	check("huge rectangle cannot overflow", qd_gfx_fill(0x7fffffffL, 0, 10, 10, 1) == 0 &&
	      qd_gfx_fill(-0x7fffffffL, 0, 0x7fffffffL, 10, 1) == 0, 0);
	for (i = 0; i < 64 * 40; i++)
		pix[i] = (unsigned char) (i * 7 + i / 64);
	r = qd_gfx_blit(0, 0, 64, 40, pix, 64);
	memset(back, 0, sizeof back);
	check("blit 64x40 then read back: identical", r == 2560 && qd_gfx_read(0, 0, 64, 40, back, 64) == 2560 &&
	      memcmp(pix, back, sizeof pix) == 0, r);
	check("blit from kernel memory refused -14", qd_gfx_blit(0, 0, 8, 8, (const void *) 0x1000, 8) == QD_SURF_ERR_RANGE, 0);
	check("read into kernel memory refused -14", qd_gfx_read(0, 0, 8, 8, (void *) 0x1000, 8) == QD_SURF_ERR_RANGE, 0);
	check("blit whose last row runs past the program refused -14",
	      qd_gfx_blit(0, 0, 64, 40, (const void *) 0x1fffffc0ul, 64) == QD_SURF_ERR_RANGE, 0);
	check("stride smaller than the width refused -4", qd_gfx_blit(0, 0, 64, 4, pix, 32) == QD_SURF_ERR_ARG, 0);
	check("stride above 65536 refused -4", qd_gfx_blit(0, 0, 4, 4, pix, 70000ul) == QD_SURF_ERR_ARG, 0);
	check("bad rect pointer refused -14", (long) qtrap(0x3Bu, 5u, 0x1000u, 0u) == QD_SURF_ERR_RANGE, 0);
	s1 = qd_gfx_checksum();
	s2 = qd_gfx_checksum();
	check("checksum deterministic and non-negative", s1 == s2 && s1 >= 0, s1);
	qd_gfx_fill(10, 10, 1, 1, (unsigned int) (pix[10 * 64 + 10] ^ 1));
	check("checksum changes with one pixel", qd_gfx_checksum() != s1, qd_gfx_checksum());
	qd_gfx_blit(0, 0, 64, 40, pix, 64);
	check("and returns with it", qd_gfx_checksum() == s1, 0);
	rgb[0] = 10; rgb[1] = 20; rgb[2] = 30; rgb[3] = 40; rgb[4] = 50; rgb[5] = 60;
	rc = qd_gfx_palette(250, 2, rgb);
	check("palette 250..251 set and read back", rc == 2 && qd_gfx_get_palette(250, 2, rgb2) == 2 && memcmp(rgb, rgb2, 6) == 0, rc);
	check("palette past 255 / 0 entries refused -4", qd_gfx_palette(255, 2, rgb) == QD_SURF_ERR_ARG &&
	      qd_gfx_palette(0, 0, rgb) == QD_SURF_ERR_ARG, 0);
	check("palette from kernel memory refused -14", qd_gfx_palette(0, 4, (const unsigned char *) 0x1000) == QD_SURF_ERR_RANGE, 0);
	check("palette change changes the checksum", qd_gfx_checksum() != s1, 0);
	r = qd_gfx_scroll(0, 0, 64, 40, 10, 0);
	check("scroll up by 10", r == 2560 && qd_gfx_read(0, 0, 64, 1, back, 64) == 64 && memcmp(back, pix + 10 * 64, 64) == 0, r);
	r = qd_gfx_text(0, 0, "Hi!", 3, 15, 1, 0);
	check("text 'Hi!' drawn: 3 cells of 6x10", r == 3, r);
	check("text from kernel memory refused -14", qd_gfx_text(0, 0, (const char *) 0x1000, 3, 15, 1, 0) == QD_SURF_ERR_RANGE, 0);
	check("text longer than 256 refused -4", qd_gfx_text(0, 0, (const char *) pix, 300, 15, 1, 0) == QD_SURF_ERR_ARG, 0);
	rc = qd_gfx_cursor(2, 1, 1);
	check("cursor at (2,1) visible", rc == 0 && qd_gfx_cursor_get() == (2 | (1 << 8) | (1L << 16)), qd_gfx_cursor_get());
	check("cursor outside the 10x4 cell grid refused -4", qd_gfx_cursor(11, 0, 1) == QD_SURF_ERR_ARG &&
	      qd_gfx_cursor(0, 4, 1) == QD_SURF_ERR_ARG, 0);
	qd_gfx_colours(14, 0, 0);
	rc = qd_gfx_print("ab\ncd", 5);
	check("print: wraps and moves the text cursor", rc == 5 && qd_gfx_cursor_get() == (2 | (2 << 8) | (1L << 16)), qd_gfx_cursor_get());
	check("print longer than 256 refused -4", qd_gfx_print((const char *) pix, 257) == QD_SURF_ERR_ARG, 0);
	check("frame counter", qd_gfx_frame() == 1 && qd_gfx_frame() == 2, 0);
	check("foreground of a stale id refused -2", qd_surface_foreground(id) == QD_SURF_ERR_STALE, 0);
	rc = qd_surface_list(back, 16 + 32 * 20);
	check("session list includes this INDEX8 session", rc >= 2, rc);
	/* a child that opens a surface and ends: its surface and session go with it */
	{
		int k = spawn1("child", 0, QD_SPAWN_CON_NEW);
		long st;

		st = qd_wait(k, 3000);
		check("child on a new session opened 64x40 and ended 0", k > 0 && st == 0, st);
		qd_yield(10);
		qd_surface_stats(stv);
		check("its surface was freed (stats: open count back to 1)", stv[0] == 1, (long) stv[0]);
	}
	/* surface limit: SURF_GFX_MAX = 4; this job holds one, three children hold one each, a fifth is refused */
	{
		int k[4], n, ok = 1;
		long st;

		for (n = 0; n < 3; n++) {
			k[n] = spawn1("gfx", "400", QD_SPAWN_CON_NEW);
			if (k[n] <= 0)
				ok = 0;
		}
		qd_yield(40);
		qd_surface_stats(stv);
		check("4 INDEX8 surfaces open (this one + 3 children)", ok && stv[0] == 4, (long) stv[0]);
		k[3] = spawn1("child", 0, QD_SPAWN_CON_NEW);
		st = qd_wait(k[3], 3000);
		check("a fifth is refused: child ends 105 (SURF_ERR_FULL -5)", st == 105, st);
		for (n = 0; n < 3; n++)
			qd_kill(k[n]);
		for (n = 0; n < 3; n++)
			qd_wait(k[n], 3000);
		qd_yield(10);
		qd_surface_stats(stv);
		check("killed children: their surfaces freed (open 1)", stv[0] == 1, (long) stv[0]);
		check("refusal counted", stv[2] >= 1, (long) stv[2]);
	}
	rc = qd_gfx_close();
	rc = rc == 0 ? qd_surface_info(0, &in) : rc;
	check("close: back to the text session, same session index", rc == 0 && in.kind == QD_SURF_TEXT && in.session == in2.session, rc);
	check("INDEX8 call after close refused -7", qd_gfx_fill(0, 0, 1, 1, 1) == QD_SURF_ERR_NOGFX, 0);
	check("close twice refused -7", qd_gfx_close() == QD_SURF_ERR_NOGFX, 0);
	{
		unsigned short cells[3];

		cells[0] = (unsigned short) ('X' | 0x1f00);
		cells[1] = (unsigned short) ('Y' | 0x2f00);
		cells[2] = (unsigned short) ('Z' | 0x4f00);
		check("write 3 cells at (24, 77)", qd_text_cells(24, 77, cells, 3) == 3, 0);
		check("cells past the end refused", qd_text_cells(24, 78, cells, 3) == QD_SURF_ERR_ARG, 0);
		check("cells from kernel memory refused -14", qd_text_cells(0, 0, (const unsigned short *) 0x1000, 3) == QD_SURF_ERR_RANGE, 0);
	}
	check("bad op refused -4", (long) qtrap(0x3Bu, 99u, 0u, 0u) == QD_SURF_ERR_ARG, 0);
}

int qd_con_channel(void);

/* A private session of our own: START ... /NEW gives one already; otherwise make one. */
static long own_private(void)
{
	struct qd_surface_info in;

	if (qd_surface_info(0, &in) == 0 && (in.flags & QD_SURF_PRIVATE) && in.users == 1)
		return (long) in.id;
	return qd_surface_new();
}

static int panel(int k)
{
	int ch = qd_con_channel(), r, c, key, nk = 0, lc;
	char l[96], keys[64];
	unsigned short cells[80];
	struct qd_surface_info in;

	own_private();
	qd_con_show_cursor(ch, 0);
	qd_con_attr(ch, QD_ATTR(7, 0));
	qd_con_clear(ch, QD_CLR_ALL);
	qd_con_attr(ch, QD_ATTR(14, 1));
	sprintf(l, "PANEL %d session %d", k, qd_con_number(ch));
	qd_con_putat(ch, 0, 0, l, (int) strlen(l));
	for (r = 0; r < 16; r++) {
		for (c = 0; c < 40; c++)
			l[c] = (char) ('A' + (r * 3 + k * 7 + c) % 26);
		qd_con_attr(ch, (int) ((unsigned int) r | ((unsigned int) ((r + k) & 7) << 4)));
		qd_con_putat(ch, 2 + r, 2, l, 40);
	}
	for (c = 0; c < 80; c++)
		cells[c] = (unsigned short) (('0' + (c + k) % 10) | ((((c + k) & 15) | (((c / 10 + k) & 7) << 4)) << 8));
	qd_text_cells(19u, 0u, cells, 80u);
	qd_con_attr(ch, QD_ATTR(7, 0));
	qd_con_goto(ch, 21, 5 * k);
	qd_con_show_cursor(ch, 1);
	for (;;) {
		key = qd_con_getkey(ch);
		if (key < 0 || key == 'q')
			break;
		if (nk < (int) sizeof keys - 4)
			keys[nk++] = (char) key;
	}
	keys[nk] = 0;
	qd_surface_info(0, &in);
	lc = sprintf(l, "PANEL %d session %lu keys=%s end=%d\n", k, in.session, keys, key);
	sprintf(keys, "WIN1:/PANEL%d.LOG", k);
	r = qd_open_ex(keys, QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	if (r >= 0) {
		qd_write_all(r, l, (unsigned int) lc);
		qd_close(r);
	}
	return 0;
}

/* Palette entries 0..15 of card K (R, G, B 0..255), 16..47 a ramp: the judge recomputes the picture from these. */
static const unsigned char ega[16][3] = {
	{ 0, 0, 0 }, { 0, 0, 170 }, { 0, 170, 0 }, { 0, 170, 170 }, { 170, 0, 0 }, { 170, 0, 170 }, { 170, 85, 0 },
	{ 170, 170, 170 }, { 85, 85, 85 }, { 85, 85, 255 }, { 85, 255, 85 }, { 85, 255, 255 }, { 255, 85, 85 },
	{ 255, 85, 255 }, { 255, 255, 85 }, { 255, 255, 255 }
};

static int card(int k)
{
	static unsigned char rgb[48 * 3];
	int ch = qd_con_channel(), i, key, nk = 0, lc, r;
	long sum, sum2, id;
	char l[200], keys[40];
	unsigned char codes[40];
	unsigned long st[8];

	id = qd_gfx_open(320u, 200u, 0u);
	if (id < 0)
		return (int) (100 - id);
	for (i = 0; i < 16; i++) {
		rgb[3 * i] = ega[i][0];
		rgb[3 * i + 1] = ega[i][1];
		rgb[3 * i + 2] = ega[i][2];
	}
	for (i = 0; i < 32; i++) {
		rgb[48 + 3 * i] = (unsigned char) (k * 60);
		rgb[48 + 3 * i + 1] = (unsigned char) (i * 8);
		rgb[48 + 3 * i + 2] = (unsigned char) (255 - i * 8);
	}
	qd_gfx_palette(0u, 48u, rgb);
	qd_gfx_fill(0, 0, 320, 200, (unsigned int) k);                 /* background: blue (1) / green (2) */
	sprintf(l, "CARD %d - Minerva font, QL character set", k);
	qd_gfx_text(0, 0, l, (unsigned int) strlen(l), 15, 0, 0);
	for (r = 0; r < 6; r++) {                                        /* codes 0x20..0xFF, 40 per row */
		int n = 0;
		for (i = 0; i < 40 && 0x20 + r * 40 + i < 256; i++)
			codes[n++] = (unsigned char) (0x20 + r * 40 + i);
		qd_gfx_text(0, 10 + r * 10, (const char *) codes, (unsigned int) n, 14, 0, 0);
	}
	qd_gfx_fill(0, 70, 320, 10, 7);
	qd_gfx_text(0, 70, "UNDER", 5, 0, 7, 1);
	qd_gfx_text(36, 70, "INVERSE", 7, 0, 7, 2);
	qd_gfx_text(84, 70, "BOLD", 4, 0, 7, 4);
	qd_gfx_text(114, 70, "NOPAPER", 7, 4, 3, 8);
	for (i = 0; i < 32; i++)
		qd_gfx_fill(i * 10, 100, 10, 60, (unsigned int) (16 + i));
	qd_gfx_colours(10, 0, 0);
	qd_gfx_cursor(0, 17, 1);
	sprintf(l, "stream %d>", k);
	qd_gfx_print(l, (unsigned int) strlen(l));
	sum = qd_gfx_checksum();
	for (;;) {
		key = qd_con_getkey(ch);
		if (key < 0 || key == 'q')
			break;
		if (nk < (int) sizeof keys - 2)
			keys[nk++] = (char) key;
	}
	keys[nk] = 0;
	sum2 = qd_gfx_checksum();
	qd_surface_stats(st);
	lc = sprintf(l, "CARD %d sum=%ld sum2=%ld keys=%s end=%d pres text=%lu gfx=%lu full=%lu rows=%lu\n", k, sum, sum2,
	             keys, key, st[4], st[5], st[6], st[7]);
	sprintf(keys, "WIN1:/CARD%d.LOG", k);
	r = qd_open_ex(keys, QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	if (r >= 0) {
		qd_write_all(r, l, (unsigned int) lc);
		qd_close(r);
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "gfxfault") == 0) {
		volatile int *np = (volatile int *) 0;

		if (qd_gfx_open(0, 0, 0) < 0)
			return 2;
		qd_gfx_fill(0, 0, 320, 200, 4);
		qd_yield(50u);
		*np = 1;                                   /* the fault */
		return 9;
	}
	if (argc > 1 && strcmp(argv[1], "caps") == 0) {
		int c = qd_surface_caps();

		printf("SURFT caps version %d text %d index8 %d show-text %d show-gfx %d\n", c < 0 ? -1 : c >> 16,
		       c > 0 && (c & 1), c > 0 && (c & 2) != 0, c > 0 && (c & 4) != 0, c > 0 && (c & 8) != 0);
		return 0;
	}
	if (argc > 1 && strcmp(argv[1], "stats") == 0) {
		unsigned long st[8];

		qd_surface_stats(st);
		printf("SURFT stats open=%lu opened=%lu refused=%lu freed=%lu\n", st[0], st[1], st[2], st[3]);
		return 0;
	}
	if (argc > 2 && strcmp(argv[1], "card") == 0)
		return card(atoi(argv[2]));
	if (argc > 2 && strcmp(argv[1], "panel") == 0)
		return panel(atoi(argv[2]));
	if (argc > 1 && strcmp(argv[1], "child") == 0) {
		long r = qd_gfx_open(64, 40, 0);
		if (r < 0)
			return (int) (100 - r);
		qd_gfx_fill(0, 0, 64, 40, 5);
		return 0;
	}
	if (argc > 2 && strcmp(argv[1], "text") == 0) {
		long r = own_private();
		int k;

		printf("SURFT text session %ld: %s\n", r & 0xff, argc > 3 ? argv[3] : "");
		for (k = 0; k < 8; k++)
			printf("%d----+----%d----+----%d----+----%d----+----%d----+----%d----+----%d----+----\n", k, k, k, k, k, k, k);
		qd_yield((unsigned int) atol(argv[2]));
		return 0;
	}
	if (argc > 2 && strcmp(argv[1], "gfx") == 0) {
		unsigned int w = argc > 4 ? (unsigned int) atoi(argv[3]) : 320u, h = argc > 4 ? (unsigned int) atoi(argv[4]) : 200u;
		long r = qd_gfx_open(w, h, argc > 5 && strcmp(argv[5], "fg") == 0 ? QD_GFX_FOREGROUND : 0u);
		int y;

		if (r < 0)
			return (int) (100 - r);
		for (y = 0; y < (int) h; y += 10)
			qd_gfx_fill(0, y, (long) w, 10, (unsigned int) (16 + y / 10));
		qd_gfx_text(4, 4, "SURFT test card", 15, 15, 0, 0);
		qd_yield((unsigned int) atol(argv[2]));
		return 0;
	}
	if (qd_surface_caps() < 0) {
		printf("SURFT: this kernel has no display surfaces (QT_SURFACE answers %d)\n", qd_surface_caps());
		return 2;
	}
	matrix();
	{
		char l[64];
		int ch;

		sprintf(l, "SURFT %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		printf("%s", l);
		ch = qd_open_ex("WIN1:/SURFT.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
		if (ch >= 0) {
			qd_write_all(ch, logbuf, loglen);
			qd_write_all(ch, l, (unsigned int) strlen(l));
			qd_close(ch);
		}
	}
	return fails == 0 ? 0 : 3;
}
