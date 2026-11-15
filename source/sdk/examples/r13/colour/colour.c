/* COLOUR.AOU (R13): palette animation on a private 320x200 INDEX8 surface.  The pixels are drawn once (a diagonal
 * ramp of colours 16..255); every frame rotates palette entries 16..255 by one and updates a binary frame counter.
 * It never touches the VGA: the kernel shows the surface when its session is in front, and the program keeps
 * animating when it is not.
 *   COLOUR [frames [ticks]]   stop after that many frames (default 0: until 'q'), one frame per `ticks` (default 4)
 * Keys (only while shown): q quit.  At the end the kernel's surface must equal the program's own model (pixels,
 * palette, checksum); WIN1:/COLOUR.LOG gets "COLOUR frames=N shown=S hidden=H sum=K model=M verify=PASS|FAIL".
 * Program title: "Palette cycle". */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"
#include "../gfxmodel.h"

char _prog_name[] = "Palette cycle";
int qd_con_channel(void);

static unsigned char base[240 * 3], rot[240 * 3];

static void hue(unsigned int i, unsigned char *p)            /* 240 steps around the colour wheel */
{
	unsigned int s = i / 40u, t = (i % 40u) * 255u / 40u, u = 255u - t;
	unsigned char r = 0, g = 0, b = 0;

	switch (s) {
	case 0: r = 255; g = (unsigned char) t; break;
	case 1: r = (unsigned char) u; g = 255; break;
	case 2: g = 255; b = (unsigned char) t; break;
	case 3: g = (unsigned char) u; b = 255; break;
	case 4: b = 255; r = (unsigned char) t; break;
	default: b = (unsigned char) u; r = 255; break;
	}
	p[0] = r;
	p[1] = g;
	p[2] = b;
}

int main(int argc, char **argv)
{
	static unsigned char row[GM_W];
	unsigned long frames = 0, max = argc > 1 ? (unsigned long) atol(argv[1]) : 0UL, shown = 0, hidden = 0, msum;
	unsigned int ticks = argc > 2 ? (unsigned int) atoi(argv[2]) : 4u, i;
	int ch = qd_con_channel(), y, x, ok, lf;
	long id, ksum;
	char l[160];

	id = qd_gfx_open(0, 0, 0);
	if (id < 0) {
		printf("COLOUR: no graphics surface (%ld)\n", id);
		return 2;
	}
	for (i = 0; i < 256; i++) {                               /* 0..15 dark greys, then the wheel */
		unsigned char p[3];
		if (i < 16)
			p[0] = p[1] = p[2] = (unsigned char) (i * 8);
		else
			hue(i - 16, p);
		gm_palette(i, 1, p);
		if (i >= 16)
			memcpy(base + 3 * (i - 16), p, 3);
	}
	gm_fill(0, 0, GM_W, 20, 0);
	gm_text_static(2, 0, "COLOUR - palette cycling", 15, 0);
	for (y = 20; y < GM_H; y++) {
		for (x = 0; x < GM_W; x++)
			row[x] = (unsigned char) (16 + ((x / 2 + y) % 240));
		gm_blit(0, y, GM_W, 1, row, GM_W);
	}
	for (;;) {
		if (max && frames >= max)
			break;
		if (qd_con_queued(ch) > 0 && qd_con_getkey(ch) == 'q')
			break;
		frames++;
		for (i = 0; i < 240; i++)
			memcpy(rot + 3 * i, base + 3 * ((i + frames) % 240), 3);
		gm_palette(16, 240, rot);
		gm_counter(150, 12, frames);
		if (qd_surface_focus())
			shown++;
		else
			hidden++;
		qd_gfx_frame();
		qd_yield(ticks);
	}
	ok = gm_verify(&ksum, &msum);
	lf = sprintf(l, "COLOUR frames=%lu shown=%lu hidden=%lu sum=%ld model=%lu verify=%s\n", frames, shown, hidden, ksum,
	             msum, ok ? "PASS" : "FAIL");
	printf("%s", l);
	y = qd_open_ex("WIN1:/COLOUR.LOG", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC | QD_O_FILE);
	if (y >= 0) {
		qd_write_all(y, l, (unsigned int) lf);
		qd_close(y);
	}
	return ok ? 0 : 3;
}
