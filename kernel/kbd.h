/*
 * QDOS/386 R7 - keyboard scancode decoder (pure, shared by all platforms).
 *
 * Input is scancode set 1, the form the i8042 delivers with its translation
 * bit set (command byte bit 6).  Only what the shell needs is decoded:
 * printable US ASCII, Shift, Enter, Backspace and Tab.  Releases, E0 extended
 * keys (except keypad Enter) and E1 Pause sequences produce no byte.
 */
#ifndef QDOS386_KBD_H
#define QDOS386_KBD_H

struct kbd_decoder {
	unsigned char shift;    /* a Shift key is down */
	unsigned char ext;      /* the previous byte was E0 */
	unsigned char skip;     /* bytes of an E1 (Pause) sequence still to skip */
#ifdef QDOS_VCON
	unsigned char alt;      /* R11M: an Alt key is down */
#endif
};

void kbd_reset(struct kbd_decoder *d);

/* Feed one scancode.  Returns 1 and sets *out when it yields a byte (Enter
 * gives '\r', Backspace gives 0x08, Tab gives '\t'), otherwise 0.
 * R11M (-DQDOS_VCON): Alt+F1..F4 and Alt+1..4 return 2 and set *out to the console number 0..3; the
 * key and its release produce no byte (the hotkey never reaches a program). */
int kbd_feed(struct kbd_decoder *d, unsigned char sc, unsigned char *out);

#endif
