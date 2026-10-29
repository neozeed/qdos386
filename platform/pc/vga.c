/*
 * QDOS/386 R10-VGA - 80x25 colour text console on the PC platform.
 *
 * Writes character and attribute pairs directly to physical 0xB8000 (VGA
 * colour text mode 3, which the BIOS sets at boot).  No BIOS video call is
 * made, so this is safe from protected mode.  The CRTC cursor registers
 * (0x3D4/0x3D5, indices 0x0E/0x0F) are updated when present; the text does
 * not depend on them.
 *
 * Control bytes: CR (col 0), LF (CR+LF with scrolling), BS (cursor left, no
 * erase; the shell's "\b \b" sequence then erases), TAB (next multiple of 8,
 * wrapping to the next line).  Other control bytes are ignored.  Output past
 * the last column wraps; output past the last row scrolls the screen up.
 *
 * The file is empty unless QDOS_CON_VGA is defined, so the default image
 * is unchanged.
 */
#include "vga.h"

#ifdef QDOS_CON_VGA

#define VGA_TEXT      ((volatile unsigned short *) 0xB8000u)
#define VGA_COLS      80
#define VGA_ROWS      25
#define VGA_CRTC_ADDR 0x3d4
#define VGA_CRTC_DATA 0x3d5

extern void outb(unsigned int port, unsigned int val);

static unsigned int vga_x = 0;
static unsigned int vga_y = 0;
static unsigned int vga_attr = VGA_DEFAULT_ATTR;

static unsigned short vga_cell(unsigned int ch)
{
	return (unsigned short) ((ch & 0xffu) | (vga_attr << 8));
}

/* CRTC cursor.  Port writes are best effort: a device without a CRTC simply
 * ignores them, and the text on screen is written regardless. */
static void vga_cursor(void)
{
	unsigned int x = vga_x >= VGA_COLS ? VGA_COLS - 1 : vga_x;
	unsigned int pos = vga_y * VGA_COLS + x;

	outb(VGA_CRTC_ADDR, 0x0e);
	outb(VGA_CRTC_DATA, (pos >> 8) & 0xffu);
	outb(VGA_CRTC_ADDR, 0x0f);
	outb(VGA_CRTC_DATA, pos & 0xffu);
}

void vga_clear(void)
{
	unsigned int i;

	for (i = 0; i < VGA_COLS * VGA_ROWS; i++)
		VGA_TEXT[i] = vga_cell(' ');
	vga_x = 0;
	vga_y = 0;
	vga_cursor();
}

static void vga_scroll(void)
{
	unsigned int i;

	for (i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++)
		VGA_TEXT[i] = VGA_TEXT[i + VGA_COLS];
	for (i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++)
		VGA_TEXT[i] = vga_cell(' ');
}

static void vga_newline(void)
{
	vga_x = 0;
	vga_y++;
	if (vga_y >= VGA_ROWS) {
		vga_scroll();
		vga_y = VGA_ROWS - 1;
	}
}

void vga_init(void)
{
	vga_attr = VGA_DEFAULT_ATTR;
	vga_clear();
}

void vga_putc(unsigned char c)
{
	switch (c) {
	case '\r':
		vga_x = 0;
		break;
	case '\n':
		vga_newline();
		break;
	case '\b':
		if (vga_x > 0)
			vga_x--;
		break;
	case '\t':
		vga_x = (vga_x / 8 + 1) * 8;
		if (vga_x >= VGA_COLS)
			vga_newline();
		break;
	default:
		if (c < 32 || c == 127)
			break;
		if (vga_x >= VGA_COLS)
			vga_newline();
		VGA_TEXT[vga_y * VGA_COLS + vga_x] = vga_cell(c);
		vga_x++;
		break;
	}
	vga_cursor();
}

#ifdef QDOS_VCON
/* R11M: the virtual console layer drives the screen through these three calls (kernel/vcon.h).  The
 * foreground console is drawn cell by cell, or reloaded whole when it scrolls or the focus changes. */
#ifdef QDOS_SURFACES
#include "present.h"
/* R13: while the presenter has the card in graphics mode (or is switching it), text output only updates the cells;
 * the presenter redraws them when text mode is back. */
#define TEXT_GUARD() do { if (!pres_text_ok()) return; } while (0)
#else
#define TEXT_GUARD() do { } while (0)
#endif

void vdisp_put(unsigned int row, unsigned int col, unsigned short cell)
{
	TEXT_GUARD();
	VGA_TEXT[row * VGA_COLS + col] = cell;
}

void vdisp_load(const unsigned short *cells)
{
	unsigned int i;

	TEXT_GUARD();
	for (i = 0; i < VGA_COLS * VGA_ROWS; i++)
		VGA_TEXT[i] = cells[i];
}

static unsigned int vd_pos = 0xffffffffu;
static int vd_vis = -1;

#ifdef QDOS_SURFACES
void vdisp_reset_cache(void)
{
	vd_pos = 0xffffffffu;
	vd_vis = -1;
}
#endif

void vdisp_cursor(unsigned int row, unsigned int col, int visible)
{
	unsigned int pos = row * VGA_COLS + col;

	TEXT_GUARD();
	if (visible != vd_vis) {                  /* cursor start register: bit 5 disables the cursor */
		outb(VGA_CRTC_ADDR, 0x0a);
		outb(VGA_CRTC_DATA, visible ? 0x0d : 0x20);
		outb(VGA_CRTC_ADDR, 0x0b);
		outb(VGA_CRTC_DATA, 0x0e);
		vd_vis = visible;
	}
	if (pos != vd_pos) {
		outb(VGA_CRTC_ADDR, 0x0e);
		outb(VGA_CRTC_DATA, (pos >> 8) & 0xffu);
		outb(VGA_CRTC_ADDR, 0x0f);
		outb(VGA_CRTC_DATA, pos & 0xffu);
		vd_pos = pos;
	}
}
#endif /* QDOS_VCON */

#endif /* QDOS_CON_VGA */
