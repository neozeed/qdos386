/*
 * QDOS/386 R13 - the display presenter: the one piece of code that drives the display hardware for sessions.
 * VGA builds: platform/pc/vgapres.c (text mode 3 and mode 13h by register programming); other builds: stubs in
 * platform/pc/platform.c (no display: graphics surfaces exist only in memory).  See docs/R13-DISPLAY-ABI.md.
 */
#ifndef QDOS386_PRESENT_H
#define QDOS386_PRESENT_H

#define PRES_CAP_TEXT    0x01u    /* the physical 80x25 text display is driven */
#define PRES_CAP_MODE13  0x02u    /* 320x200x256 (mode 13h) is driven */

void pres_init(void);             /* boot, before the first session is shown: save the BIOS text-mode state */
unsigned int pres_caps(void);
void pres_focus_changed(void);    /* the foreground session, its mode or the selector changed (interrupts off) */
int pres_text_ok(void);           /* 1: the display is in text mode now and text may be written to it directly */
void pres_poll(void);             /* the SYSTEM job, every tick: mode switches and frame copies (interrupts on) */
/* Counters for tests (all since boot): mode switches to text, to mode 13h, full frames, dirty-row copies, palette
 * loads. */
void pres_stats(unsigned int *out5);
#ifdef QDOS_R13A
#define PRES_CAP_MODE12  0x04u    /* R13A: 640x480x16 (mode 12h) for rendered text consoles */
/* R13A: the presenter's copy of what it shows, for test harnesses that cannot read pixels as text (a fixed kernel
 * object; its address is printed at boot).  mode: 0 VGA text, 2 mode 13h, 3 mode 12h rendered text, 1 switching. */
struct pres_shadow {
	unsigned int magic;           /* 0x44485351 "QSHD" */
	unsigned int version;         /* 1 */
	unsigned int mode, session, cursor, font_id, cell_w, cell_h, ox, oy, frames, pad;
	unsigned short cell[2000];    /* the cells last rendered (mode 3 only) */
};
const struct pres_shadow *pres_shadow_ptr(void);
#endif

#endif
