/* GFXA.AOU (R13A): the R13A display calls from a strict C89 program.
 *   GFXA matrix           the API matrix (fonts, views, transparent blit, copy, bad pointers, key events, the pixel
 *                         budget); "GFXA ok/FAIL ..." lines, "GFXA PASS checks=N"; also WIN1:/GFXA.LOG
 *   GFXA pattern T [crop] a 512x256 logical surface: a fixed pattern, a border, a moving bar and a frame counter,
 *                         T frames (0: until q); shown scaled to fit (or a 1:1 window with "crop"); at the end the
 *                         kernel's surface must equal the program's model: WIN1:/PATTERN.LOG
 *   GFXA card NAME PATH   a 320x200 card drawn with the font in PATH ("-": the system default) and its own palette;
 *                         keys until q; WIN1:/CARD<NAME>.LOG gets the font info and the surface checksum
 *   GFXA keys             native key events: shows held keys, logs every event to WIN1:/KEYS.LOG at q
 *   GFXA tsum             print "TSUM session=S sum=K" for every session: the kernel's checksum of its text as a
 *                         rendered console would draw it (qd_text_checksum)
 *   GFXA hold W H T       (spawned by the matrix) open a W x H surface, keep it T ticks; exit 100 - error if refused
 * Program title: "R13A graphics". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

char _prog_name[] = "R13A graphics";
int qd_con_channel(void);

static int checks, fails;
static char logbuf[8192];
static unsigned int loglen;

static void logs(const char *s)
{
	if (loglen + strlen(s) < sizeof logbuf) {
		strcpy(logbuf + loglen, s);
		loglen += (unsigned int) strlen(s);
	}
}

static void check(const char *what, int ok, long got)
{
	char line[200];

	checks++;
	if (!ok)
		fails++;
	sprintf(line, "GFXA %s %s (%ld)\n", ok ? "ok  " : "FAIL", what, got);
	logs(line);
	printf("%s", line);
}

static void save(const char *name, const char *text, unsigned int n)
{
	int ch = qd_open_ex(name, QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);

	if (ch >= 0) {
		qd_write_all(ch, text, n);
		qd_close(ch);
	}
}

static int spawn_hold(const char *w, const char *h, const char *t)
{
	char buf[64];
	int n = 0;

	n += sprintf(buf + n, "hold") + 1;
	n += sprintf(buf + n, "%s", w) + 1;
	n += sprintf(buf + n, "%s", h) + 1;
	n += sprintf(buf + n, "%s", t) + 1;
	return qd_spawn("GFXA.AOU", buf, (unsigned int) n, 0u, QD_SPAWN_CON_NEW);
}

/* ---- a minimal QFNT reader (the program's own check of what the kernel draws) ---- */
static unsigned char fimg[34000];
static unsigned int flen;

static unsigned int rd16(const unsigned char *p) { return (unsigned int) p[0] | ((unsigned int) p[1] << 8); }

static long read_file(const char *path, unsigned char *buf, unsigned int max)
{
	char name[96];
	int ch;
	long n, tot = 0;

	sprintf(name, "WIN1:%s", path);             /* the program's file calls take channel names */
	ch = qd_open_ex(name, QD_O_READ | QD_O_FILE);
	if (ch < 0)
		return ch;
	while (tot < (long) max && (n = qd_read(ch, buf + tot, max - (unsigned int) tot)) > 0)
		tot += n;
	qd_close(ch);
	return tot;
}

/* glyph row r (gw bits) of code c in the QFNT image */
static unsigned int fnt_row(const unsigned char *f, unsigned int c, unsigned int r)
{
	unsigned int gw = f[8], gh = f[9], rb = f[15], g = rd16(f + 64 + 2 * c), v;
	const unsigned char *p;

	if (g == 0xFFFFu)
		g = rd16(f + 18);
	p = f + 64 + 512 + (g * gh + r) * rb;
	v = rb == 1 ? p[0] : ((unsigned int) p[0] << 8) | p[1];
	return v >> (8 * rb - gw);
}

/* the cell of code c at (x, y) on the surface equals the font image's glyph in ink / paper */
static int cell_ok(const unsigned char *f, unsigned int c, long x, long y, unsigned int ink, unsigned int paper)
{
	static unsigned char px[32 * 32];
	unsigned int gw = f[8], gh = f[9], cw = f[10], ch = f[11], xo = f[12], yo = f[13], r, k;

	if (qd_gfx_read(x, y, (long) cw, (long) ch, px, cw) != (long) (cw * ch))
		return 0;
	for (r = 0; r < ch; r++)
		for (k = 0; k < cw; k++) {
			int on = r >= yo && r < yo + gh && k >= xo && k < xo + gw && ((fnt_row(f, c, r - yo) >> (gw - 1 - (k - xo))) & 1u);
			if (px[r * cw + k] != (on ? ink : paper))
				return 0;
		}
	return 1;
}

static void corrupt_checks(void)
{
	static const char *const bad[] = { "BADMAGIC", "BADHCRC", "BADPCRC", "TRUNC", "BIG", "DUPMAP", "WIDEBITS", "BADCELL",
	                                    "EMPTY", "BADMAP" };
	char path[64], what[96];
	long ids[8];
	int before = qd_font_list(ids, 8), i;

	for (i = 0; i < (int) (sizeof bad / sizeof bad[0]); i++) {
		long r;
		sprintf(path, "/SYSTEM/FONTS/%s.FNT", bad[i]);
		r = qd_font_open(path);
		sprintf(what, "broken font %s.FNT refused -4", bad[i]);
		check(what, r == QD_SURF_ERR_ARG, r);
	}
	check("the font table is unchanged by the refusals", qd_font_list(ids, 8) == before, (long) before);
}

static void matrix(void)
{
	struct qd_font_info fi;
	struct qd_surface_info si;
	long id, r, mid, rid, k[5];
	int caps = qd_surface_caps(), i, st;
	unsigned long ms[8], ms0[8];
	static unsigned char pix[64 * 32], back[64 * 32], model[64 * 32], spr[16];

	check("caps: R13A calls present", caps > 0 && (caps & (int) QD_SURF_CAP_R13A) != 0, caps);
	check("a private session of its own (font selections stay off the shell's console)", qd_surface_new() > 0, 0);
	qd_gfx_memstats(ms0);
	check("built-in fonts listed: recovery 6x9 in 6x10, console 8x16", qd_font_info(QD_FONT_RECOVERY, &fi) == 0 && fi.glyph_w == 6 &&
	      fi.glyph_h == 9 && fi.cell_w == 6 && fi.cell_h == 10 && qd_font_info(QD_FONT_CONSOLE, &fi) == 0 && fi.cell_w == 8 &&
	      fi.cell_h == 16 && (fi.flags & 1u), (long) fi.cell_h);
	check("a missing font file is refused (negative), nothing loaded", qd_font_open("/SYSTEM/FONTS/NOSUCH.FNT") < 0, 0);
	corrupt_checks();
	/* Minerva from the disk */
	mid = qd_font_open("/SYSTEM/FONTS/MINERVA.FNT");
	check("MINERVA.FNT opens (font id)", mid > 2, mid);
	check("its header: 5x9 in 6x10, 225 glyphs, QL encoding, name", qd_font_info(mid, &fi) == 0 && fi.glyph_w == 5 &&
	      fi.glyph_h == 9 && fi.cell_w == 6 && fi.cell_h == 10 && fi.glyphs == 225 && fi.encoding == 2 &&
	      strncmp(fi.name, "Minerva 1.98 QL", 15) == 0 && (fi.flags & 2u), (long) fi.glyphs);
	/* the recovery font loaded from a buffer: the kernel copies it */
	r = read_file("/SYSTEM/FONTS/RECOVERY.FNT", fimg, sizeof fimg);
	flen = r > 0 ? (unsigned int) r : 0u;
	rid = qd_font_load(fimg, flen);
	check("a font image loads from a buffer", rid > 2, rid);
	if (flen > 600)
		fimg[600] ^= 0xFF;
	check("the kernel kept its own copy (changing the buffer changes nothing)", qd_font_info(rid, &fi) == 0 && fi.glyphs == 96, (long) fi.glyphs);
	if (flen > 600)
		fimg[600] ^= 0xFF;
	check("a font image from kernel memory refused -14", qd_font_load((const void *) 0x1000, 600) == QD_SURF_ERR_RANGE, 0);
	check("a font path from kernel memory refused -14", qd_font_open((const char *) 0x1000) == QD_SURF_ERR_RANGE, 0);
	check("a zero-length / oversized image refused -4", qd_font_load(fimg, 0) == QD_SURF_ERR_ARG &&
	      qd_font_load(fimg, 40000u) == QD_SURF_ERR_ARG, 0);
	/* select Minerva for this session, draw, compare every pixel with the file's glyphs */
	check("select Minerva for this session", qd_font_select(mid) == 0, 0);
	id = qd_gfx_open(320, 200, 0);
	check("a 320x200 surface opens with the session's font", id > 0 && qd_font_info(0, &fi) == 0 && fi.id == (unsigned long) mid, id);
	r = read_file("/SYSTEM/FONTS/MINERVA.FNT", fimg, sizeof fimg);
	qd_gfx_fill(0, 0, 320, 200, 0);
	qd_gfx_text(12, 20, "A`\x7f\x80g", 5, 14, 1, 0);
	check("Minerva glyphs drawn exactly as in the file: A, pound ($60), copyright ($7F), a-umlaut ($80), g",
	      r > 0 && cell_ok(fimg, 'A', 12, 20, 14, 1) && cell_ok(fimg, 0x60, 18, 20, 14, 1) && cell_ok(fimg, 0x7f, 24, 20, 14, 1) &&
	      cell_ok(fimg, 0x80, 30, 20, 14, 1) && cell_ok(fimg, 'g', 36, 20, 14, 1), r);
	check("text grid follows the font cell: 53 x 20", qd_surface_info(0, &si) == 0 && si.cols == 53 && si.rows == 20, (long) si.cols);
	check("release while selected: the font stays alive (the session holds it)", qd_font_release(mid) == 0 &&
	      qd_font_info(mid, &fi) == 0 && !(fi.flags & 2u), (long) fi.refs);
	check("select the recovery font loaded from the buffer: the surface switches font", qd_font_select(rid) == 0 &&
	      qd_font_info(0, &fi) == 0 && fi.id == (unsigned long) rid, (long) fi.id);
	check("Minerva is now unreferenced: its id is stale", qd_font_info(mid, &fi) == QD_SURF_ERR_STALE, 0);
	qd_gfx_fill(0, 0, 320, 200, 0);
	qd_gfx_text(0, 0, "Az", 2, 15, 0, 0);
	check("recovery glyphs drawn exactly as in the file (A, z)", read_file("/SYSTEM/FONTS/RECOVERY.FNT", fimg, sizeof fimg) > 0 &&
	      cell_ok(fimg, 'A', 0, 0, 15, 0) && cell_ok(fimg, 'z', 6, 0, 15, 0), 0);
	check("a stale font id cannot be selected -2", qd_font_select(mid) == QD_SURF_ERR_STALE, 0);
	/* transparent blit */
	qd_gfx_fill(0, 0, 64, 32, 7);
	for (i = 0; i < 16; i++)
		spr[i] = (unsigned char) ((i % 5) == 0 ? 0 : 20 + i);
	r = qd_gfx_blit_key(10, 10, 4, 4, spr, 4, 0);
	qd_gfx_read(0, 0, 64, 32, back, 64);
	st = 1;
	for (i = 0; i < 16; i++) {
		unsigned char want = (i % 5) == 0 ? 7 : (unsigned char) (20 + i);
		if (back[(10 + i / 4) * 64 + 10 + i % 4] != want)
			st = 0;
	}
	check("transparent blit: key-colour pixels keep the surface, the rest are copied", r == 12 && st, r);
	r = qd_gfx_blit_key(-2, -2, 4, 4, spr, 4, 0);
	check("transparent blit clipped at the top-left: only the visible part (2x2)", r >= 0 && r <= 4, r);
	check("transparent blit from kernel memory refused -14", qd_gfx_blit_key(0, 0, 4, 4, (const void *) 0x1000, 4, 0) == QD_SURF_ERR_RANGE, 0);
	check("transparent blit with stride < width refused -4", qd_gfx_blit_key(0, 0, 4, 4, spr, 2, 0) == QD_SURF_ERR_ARG, 0);
	/* overlap-safe copy */
	for (i = 0; i < 64 * 32; i++)
		pix[i] = (unsigned char) (i * 7 + i / 64);
	qd_gfx_blit(0, 0, 64, 32, pix, 64);
	memcpy(model, pix, sizeof model);
	{
		int y;
		for (y = 20; y >= 0; y--)                /* the model: rows 0..20 of x 0..39 moved by (+3, +2), as memmove */
			memmove(model + (y + 2) * 64 + 3, model + y * 64, 40);
	}
	r = qd_gfx_copy(0, 0, 40, 21, 3, 2);
	qd_gfx_read(0, 0, 64, 32, back, 64);
	check("copy within the surface, overlapping (+3, +2): equals memmove", r == 40 * 21 && memcmp(back, model, sizeof back) == 0, r);
	memcpy(model, back, sizeof model);
	{
		int y;
		for (y = 0; y < 20; y++)
			memmove(model + y * 64, model + (y + 5) * 64 + 4, 30);
	}
	r = qd_gfx_copy(4, 5, 30, 20, 0, 0);
	qd_gfx_read(0, 0, 64, 32, back, 64);
	check("copy overlapping the other way (-4, -5): equals memmove", r == 600 && memcmp(back, model, sizeof back) == 0, r);
	check("copy with a negative width refused -4", qd_gfx_copy(0, 0, -1, 5, 0, 0) == QD_SURF_ERR_ARG, 0);
	r = qd_gfx_copy(300, 190, 50, 50, 0, 0);
	check("copy of a rectangle half outside: clipped (20 x 10)", r == 200, r);
	r = qd_gfx_copy(0, 0, 50, 50, 300, 190);
	check("copy to a destination half outside: clipped (20 x 10)", r == 200, r);
	check("key events: enable, nothing queued, disable", qd_key_events(1) == 0 && qd_key_read(ms, 8) == 0 && qd_key_events(0) == 0, 0);
	check("key event buffer in kernel memory refused -14", qd_key_events(1) == 0 && qd_key_read((unsigned long *) 0x1000, 8) == QD_SURF_ERR_RANGE &&
	      qd_key_events(0) == 0, 0);
	check("view of a 320x200 surface: crop at (0,0) accepted, outside refused -4", qd_gfx_view(QD_VIEW_CROP, 0, 0) == 0 &&
	      qd_gfx_view(QD_VIEW_CROP, 320, 0) == QD_SURF_ERR_ARG && qd_gfx_view(7, 0, 0) == QD_SURF_ERR_ARG && qd_gfx_view(QD_VIEW_FIT, 0, 0) == 0, 0);
	qd_gfx_close();
	check("select the default again; release the buffer font", qd_font_select(0) == 0 && qd_font_release(rid) == 0, 0);
	check("loaded fonts are all freed (stale ids)", qd_font_info(rid, &fi) == QD_SURF_ERR_STALE, 0);
	/* logical sizes */
	id = qd_gfx_open(512, 256, 0);
	check("a 512x256 logical surface opens", id > 0, id);
	check("crop view inside it accepted; vx past the width refused", qd_gfx_view(QD_VIEW_CROP, 192, 56) == 0 && qd_gfx_view(QD_VIEW_CROP, 512, 0) == QD_SURF_ERR_ARG, 0);
	r = qd_gfx_fill(500, 250, 50, 50, 3);
	check("fill clipped at the 512x256 edges (12 x 6)", r == 72, r);
	qd_gfx_close();
	check("513 x 256 and 512 x 257 refused -4", qd_gfx_open(513, 256, 0) == QD_SURF_ERR_ARG && qd_gfx_open(512, 257, 0) == QD_SURF_ERR_ARG, 0);
	/* the pixel budget: three 512x256 surfaces fit, a fourth is refused although only 3 of 4 surfaces are open */
	for (i = 0; i < 3; i++)
		k[i] = spawn_hold("512", "256", "300");
	qd_yield(100);
	qd_gfx_memstats(ms);
	check("three 512x256 surfaces held by children: about 3 x 129 KiB in use", ms[0] >= 3ul * 131072ul && ms[0] < 3ul * 140000ul, (long) ms[0]);
	k[3] = spawn_hold("512", "256", "10");
	st = k[3] > 0 ? qd_wait((int) k[3], 3000) : -1;
	qd_gfx_memstats(ms);
	check("a fourth is refused by the pixel budget (FULL -5, child status 105)", st == 105 && ms[2] == ms0[2] + 1, (long) st);
	for (i = 0; i < 3; i++)
		st = qd_wait((int) k[i], 3000);
	qd_yield(20);
	qd_gfx_memstats(ms);
	check("all freed: pixel bytes and loaded fonts back to the start", ms[0] == ms0[0] && ms[3] == ms0[3], (long) ms[0]);
}

/* ---- pattern: the 512x256 test, with the program's own model ---- */
#define PW 512
#define PH 256
static unsigned char pm[PW * PH], pb[PW * PH], ppal[768], pbp[768];

static void p_fill(long x, long y, long w, long h, unsigned int c)
{
	long i, j;

	qd_gfx_fill(x, y, w, h, c);
	for (j = y; j < y + h; j++)
		for (i = x; i < x + w; i++)
			if (i >= 0 && i < PW && j >= 0 && j < PH)
				pm[j * PW + i] = (unsigned char) c;
}

static unsigned char p_base(long x, long y)
{
	if (x == 0 || y == 0 || x == PW - 1 || y == PH - 1)
		return 15;
	return (unsigned char) (16 + (((x >> 4) ^ (y >> 4)) & 7) + 8 * ((x >> 6) & 7) + 64 * ((y >> 6) & 3));
}

static void p_counter(unsigned long v)
{
	int b;

	for (b = 0; b < 32; b++)
		p_fill(8 + (31 - b) * 10, 8, 8, 8, (v >> b) & 1u ? 255u : 8u);
}

static int pattern(unsigned long max, int crop)
{
	static unsigned char row[PW], strip[16 * PH];
	unsigned long frames = 0, shown = 0, hidden = 0, h1, h2;
	long pbx = -1;
	long id = qd_gfx_open(PW, PH, 0), ks;
	int ch = qd_con_channel(), x, y, ok;
	unsigned int i;
	char l[160];

	if (id < 0) {
		printf("GFXA: no 512x256 surface (%ld)\n", id);
		return 2;
	}
	for (i = 0; i < 256; i++) {
		ppal[3 * i] = (unsigned char) (i < 16 ? i * 16 : (i * 37) & 255);
		ppal[3 * i + 1] = (unsigned char) (i < 16 ? i * 16 : (i * 91) & 255);
		ppal[3 * i + 2] = (unsigned char) (i < 16 ? i * 16 : 255 - i);
	}
	ppal[3 * 255] = ppal[3 * 255 + 1] = ppal[3 * 255 + 2] = 255;
	ppal[3 * 8] = ppal[3 * 8 + 1] = ppal[3 * 8 + 2] = 40;
	qd_gfx_palette(0, 256, ppal);
	for (y = 0; y < PH; y++) {
		for (x = 0; x < PW; x++)
			row[x] = pm[y * PW + x] = p_base(x, y);
		qd_gfx_blit(0, y, PW, 1, row, PW);
	}
	if (crop)
		qd_gfx_view(QD_VIEW_CROP, 96, 0);
	for (;;) {
		long bx = (long) ((frames * 4u) % (PW - 16)), yy, xx;

		if (max && frames >= max)
			break;
		if (qd_con_queued(ch) > 0 && qd_con_getkey(ch) == 'q')
			break;
		if (pbx >= 0) {                           /* restore the previous bar, then draw the new one */
			for (yy = 24; yy < PH - 1; yy++)
				for (xx = 0; xx < 16; xx++)
					strip[(yy - 24) * 16 + xx] = pm[yy * PW + pbx + xx] = p_base(pbx + xx, yy);
			qd_gfx_blit(pbx, 24, 16, PH - 25, strip, 16);
		}
		p_fill(bx, 24, 16, PH - 25, 254);
		pbx = bx;
		frames++;
		p_counter(frames);
		if (qd_surface_focus())
			shown++;
		else
			hidden++;
		qd_gfx_frame();
		qd_yield(2);
	}
	qd_gfx_read(0, 0, PW, PH, pb, PW);
	qd_gfx_get_palette(0, 256, pbp);
	h1 = 2166136261UL;
	for (i = 0; i < PW * PH; i++)
		h1 = ((h1 ^ pm[i]) * 16777619UL) & 0xffffffffUL;
	for (i = 0; i < 768; i++)
		h1 = ((h1 ^ ppal[i]) * 16777619UL) & 0xffffffffUL;
	h2 = h1 & 0x7fffffffUL;
	ks = qd_gfx_checksum();
	ok = memcmp(pb, pm, sizeof pm) == 0 && memcmp(pbp, ppal, 768) == 0 && (unsigned long) ks == h2;
	sprintf(l, "PATTERN frames=%lu shown=%lu hidden=%lu sum=%ld model=%lu verify=%s\n", frames, shown, hidden, ks, h2,
	        ok ? "PASS" : "FAIL");
	printf("%s", l);
	save("WIN1:/PATTERN.LOG", l, (unsigned int) strlen(l));
	return ok ? 0 : 3;
}

/* ---- card: a font sample on its own surface ---- */
static int card(const char *name, const char *path)
{
	static unsigned char rgb[48];
	struct qd_font_info fi;
	long id, fid = 0, sum, sum2;
	int ch = qd_con_channel(), r, i, key;
	char l[200];
	unsigned char codes[40];

	if (strcmp(path, "-") != 0) {
		fid = qd_font_open(path);
		if (fid < 0) {
			printf("GFXA card: font %s refused (%ld)\n", path, fid);
			return 2;
		}
		qd_font_select(fid);
	}
	id = qd_gfx_open(320, 200, 0);
	if (id < 0)
		return (int) (100 - id);
	for (i = 0; i < 16; i++) {
		rgb[3 * i] = (unsigned char) (i & 1 ? 200 : 20);
		rgb[3 * i + 1] = (unsigned char) (i & 2 ? 200 : 20 + 10 * (name[0] & 7));
		rgb[3 * i + 2] = (unsigned char) (i & 4 ? 200 : 60);
	}
	qd_gfx_palette(0, 16, rgb);
	qd_gfx_fill(0, 0, 320, 200, 1);
	qd_font_info(0, &fi);
	sprintf(l, "CARD %s: %.19s %lux%lu", name, fi.name, fi.glyph_w, fi.glyph_h);
	qd_gfx_text(0, 0, l, (unsigned int) strlen(l), 15, 1, 0);
	for (r = 0; r < 6; r++) {
		int n = 0;
		for (i = 0; i < 40 && 0x20 + r * 40 + i < 256; i++)
			codes[n++] = (unsigned char) (0x20 + r * 40 + i);
		qd_gfx_text(0, (long) (fi.cell_h * (unsigned long) (2 + r)), (const char *) codes, (unsigned int) n, 14, 0, 0);
	}
	qd_gfx_colours(10, 1, 0);
	qd_gfx_cursor(0, 12, 1);
	qd_gfx_print("stream> ", 8);
	sum = qd_gfx_checksum();
	for (;;) {
		key = qd_con_getkey(ch);
		if (key < 0 || key == 'q')
			break;
	}
	sum2 = qd_gfx_checksum();
	sprintf(l, "CARD %s font=%lu name=%.19s cell=%lux%lu sum=%ld sum2=%ld\n", name, fi.id, fi.name, fi.cell_w, fi.cell_h, sum, sum2);
	sprintf((char *) codes, "WIN1:/CARD%s.LOG", name);
	save((const char *) codes, l, (unsigned int) strlen(l));
	return 0;
}

/* ---- keys: native key events ---- */
static int keys(void)
{
	static unsigned long ev[64];
	static char out[6000];
	unsigned int on = 0, n, i;
	long id = qd_gfx_open(320, 200, 0);
	int focus = -1, f, done = 0;
	char l[64];
	static unsigned char held[256];

	if (id < 0)
		return 2;
	qd_key_events(1);
	qd_gfx_fill(0, 0, 320, 200, 1);
	qd_gfx_text(2, 2, "GFXA keys: q ends", 17, 15, 1, 0);
	while (!done) {
		f = qd_surface_focus();
		if (f != focus) {
			focus = f;
			sprintf(l, "F %d\n", f);
			if (on + strlen(l) < sizeof out) {
				strcpy(out + on, l);
				on += (unsigned int) strlen(l);
			}
		}
		n = (unsigned int) qd_key_read(ev, 64);
		for (i = 0; i < n; i++) {
			unsigned long e = ev[i];
			unsigned int code = QD_KEV_CODE(e) | ((e & QD_KEV_EXT) ? 0x80u : 0u);
			sprintf(l, "%c %02x %02x %lx\n", (e & QD_KEV_SYNTH) ? 'S' : (e & QD_KEV_DOWN) ? 'D' : 'U', code, QD_KEV_CHAR(e),
			        (e >> 12) & 7ul);
			if (on + strlen(l) < sizeof out) {
				strcpy(out + on, l);
				on += (unsigned int) strlen(l);
			}
			held[code] = (unsigned char) ((e & QD_KEV_DOWN) ? 1 : 0);
			qd_gfx_fill((long) (code % 32) * 10, 20 + (long) (code / 32) * 12, 8, 10, held[code] ? 10u : 8u);
			if ((e & QD_KEV_DOWN) && QD_KEV_CHAR(e) == 'q')
				done = 1;
		}
		qd_yield(2);
	}
	qd_key_events(0);
	save("WIN1:/KEYS.LOG", out, on);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 4 && strcmp(argv[1], "hold") == 0) {
		long r = qd_gfx_open((unsigned int) atoi(argv[2]), (unsigned int) atoi(argv[3]), 0);
		if (r < 0)
			return (int) (100 - r);
		qd_gfx_fill(0, 0, 512, 256, 9);
		qd_yield((unsigned int) atol(argv[4]));
		return 0;
	}
	if (argc > 2 && strcmp(argv[1], "pattern") == 0)
		return pattern((unsigned long) atol(argv[2]), argc > 3 && strcmp(argv[3], "crop") == 0);
	if (argc > 3 && strcmp(argv[1], "card") == 0)
		return card(argv[2], argv[3]);
	if (argc > 1 && strcmp(argv[1], "keys") == 0)
		return keys();
	if (argc > 1 && strcmp(argv[1], "tsum") == 0) {
		static unsigned long lb[4 + 8 * 20];
		int n = qd_surface_list(lb, sizeof lb), i;

		for (i = 0; i < n; i++)
			printf("TSUM session=%lu sum=%ld\n", lb[4 + i * 8 + 1], qd_text_checksum((long) lb[4 + i * 8]));
		return 0;
	}
	matrix();
	{
		char l[64];

		sprintf(l, "GFXA %s checks=%d fails=%d\n", fails == 0 ? "PASS" : "FAIL", checks, fails);
		printf("%s", l);
		logs(l);
		save("WIN1:/GFXA.LOG", logbuf, loglen);
	}
	return fails == 0 ? 0 : 3;
}
