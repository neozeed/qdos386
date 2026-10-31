/* BOUNCE.AOU (R13): moving shapes on a private 320x200 INDEX8 surface - four sprites (blitted from program memory)
 * bounce inside the play area, erased and redrawn every frame, with a binary frame counter.  It never touches the
 * VGA and keeps moving while its session is hidden.
 *   BOUNCE [frames [ticks]]   stop after that many frames (default 0: until 'q'), one frame per `ticks` (default 2)
 * Before animating it checks the error rules of the drawing calls (bad pointer, negative and off-surface
 * rectangles).  At the end the kernel's surface must equal the program's own model; WIN1:/BOUNCE.LOG gets
 * "BOUNCE frames=N shown=S hidden=H errs=E sum=K model=M verify=PASS|FAIL".  Program title: "Bouncing shapes". */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"
#include "../gfxmodel.h"

char _prog_name[] = "Bouncing shapes";
int qd_con_channel(void);

#define SW 16
#define SH 12
#define TOP 20

struct spr { long x, y, dx, dy; unsigned int c; unsigned char img[SW * SH]; };

int main(int argc, char **argv)
{
	static struct spr s[4];
	static const long ix[4] = { 10, 100, 200, 290 }, iy[4] = { 30, 80, 140, 60 }, vx[4] = { 3, -2, 4, -5 },
	                  vy[4] = { 2, 3, -3, 1 };
	static const unsigned int col[4] = { 10, 12, 14, 13 };
	unsigned long frames = 0, max = argc > 1 ? (unsigned long) atol(argv[1]) : 0UL, shown = 0, hidden = 0, msum;
	unsigned int ticks = argc > 2 ? (unsigned int) atoi(argv[2]) : 2u, i;
	int ch = qd_con_channel(), k, x, y, ok, lf, errs = 0;
	long id, ksum;
	char l[180];
	unsigned char pal[768];

	id = qd_gfx_open(0, 0, 0);
	if (id < 0) {
		printf("BOUNCE: no graphics surface (%ld)\n", id);
		return 2;
	}
	for (i = 0; i < 256; i++) {                               /* EGA-like 0..15, a grey ramp above */
		static const unsigned char e[16][3] = {
			{ 0, 0, 0 }, { 0, 0, 170 }, { 0, 170, 0 }, { 0, 170, 170 }, { 170, 0, 0 }, { 170, 0, 170 },
			{ 170, 85, 0 }, { 170, 170, 170 }, { 85, 85, 85 }, { 85, 85, 255 }, { 85, 255, 85 },
			{ 85, 255, 255 }, { 255, 85, 85 }, { 255, 85, 255 }, { 255, 255, 85 }, { 255, 255, 255 } };
		if (i < 16)
			memcpy(pal + 3 * i, e[i], 3);
		else
			pal[3 * i] = pal[3 * i + 1] = pal[3 * i + 2] = (unsigned char) i;
	}
	gm_palette(0, 256, pal);
	/* the error rules: nothing may change the surface */
	if (qd_gfx_blit(0, 0, 8, 8, (const void *) 0x1000, 8) != QD_SURF_ERR_RANGE) errs++;       /* kernel memory */
	if (qd_gfx_blit(0, 0, 8, 8, (const void *) 0x7FFFFF00UL, 8) != QD_SURF_ERR_RANGE) errs++; /* not mapped */
	if (qd_gfx_fill(0, 0, -1, 5, 3) != QD_SURF_ERR_ARG) errs++;                                 /* negative */
	if (qd_gfx_fill(400, 300, 10, 10, 3) != 0) errs++;                                           /* outside */
	if (qd_gfx_fill(-50, -50, 10, 10, 3) != 0) errs++;
	if (qd_gfx_blit(0, 0, 4, 4, pal, 2) != QD_SURF_ERR_ARG) errs++;                              /* stride < width */
	if (qd_gfx_palette(250, 10, pal) != QD_SURF_ERR_ARG) errs++;                                 /* past 255 */
	gm_fill(0, 0, GM_W, GM_H, 1);
	gm_text_static(2, 0, "BOUNCE - moving shapes", 15, 1);
	gm_fill(0, TOP - 1, GM_W, 1, 7);
	for (k = 0; k < 4; k++) {
		s[k].x = ix[k];
		s[k].y = iy[k];
		s[k].dx = vx[k];
		s[k].dy = vy[k];
		s[k].c = col[k];
		for (y = 0; y < SH; y++)                               /* a framed box with a diagonal */
			for (x = 0; x < SW; x++)
				s[k].img[y * SW + x] = (unsigned char) ((x == 0 || y == 0 || x == SW - 1 || y == SH - 1 || x == y + 2)
				                                        ? 15u : s[k].c);
		gm_blit(s[k].x, s[k].y, SW, SH, s[k].img, SW);
	}
	for (;;) {
		if (max && frames >= max)
			break;
		if (qd_con_queued(ch) > 0 && qd_con_getkey(ch) == 'q')
			break;
		frames++;
		for (k = 0; k < 4; k++) {
			gm_fill(s[k].x, s[k].y, SW, SH, 1);
			s[k].x += s[k].dx;
			s[k].y += s[k].dy;
			if (s[k].x < 0) { s[k].x = -s[k].x; s[k].dx = -s[k].dx; }
			if (s[k].x > GM_W - SW) { s[k].x = 2 * (GM_W - SW) - s[k].x; s[k].dx = -s[k].dx; }
			if (s[k].y < TOP) { s[k].y = 2 * TOP - s[k].y; s[k].dy = -s[k].dy; }
			if (s[k].y > GM_H - SH) { s[k].y = 2 * (GM_H - SH) - s[k].y; s[k].dy = -s[k].dy; }
		}
		for (k = 0; k < 4; k++)
			gm_blit(s[k].x, s[k].y, SW, SH, s[k].img, SW);
		gm_counter(150, 12, frames);
		if (qd_surface_focus())
			shown++;
		else
			hidden++;
		qd_gfx_frame();
		qd_yield(ticks);
	}
	ok = gm_verify(&ksum, &msum) && errs == 0;
	lf = sprintf(l, "BOUNCE frames=%lu shown=%lu hidden=%lu errs=%d sum=%ld model=%lu verify=%s\n", frames, shown, hidden,
	             errs, ksum, msum, ok ? "PASS" : "FAIL");
	printf("%s", l);
	y = qd_open_ex("WIN1:/BOUNCE.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	if (y >= 0) {
		qd_write_all(y, l, (unsigned int) lf);
		qd_close(y);
	}
	return ok ? 0 : 3;
}
