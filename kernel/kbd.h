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
	unsigned char ctrl;     /* R12: a Ctrl key is down */
#endif
};

#ifdef QDOS_VCON
/* R12: key codes delivered to programs as single bytes (raw console mode).  Bytes 0x80.. never occur in typed text. */
#define KBD_KEY_UP    0x80
#define KBD_KEY_DOWN  0x81
#define KBD_KEY_LEFT  0x82
#define KBD_KEY_RIGHT 0x83
#define KBD_KEY_HOME  0x84
#define KBD_KEY_END   0x85
#define KBD_KEY_PGUP  0x86
#define KBD_KEY_PGDN  0x87
#define KBD_KEY_DEL   0x88
#define KBD_KEY_INS   0x89
#define KBD_KEY_F1    0x90          /* F1..F10 = 0x90..0x99 */
#endif

void kbd_reset(struct kbd_decoder *d);

/* Feed one scancode.  Returns 1 and sets *out when it yields a byte (Enter
 * gives '\r', Backspace gives 0x08, Tab gives '\t'), otherwise 0.
 * R11M (-DQDOS_VCON): Alt+F1..F4 and Alt+1..4 return 2 and set *out to the console number 0..3; the
 * key and its release produce no byte (the hotkey never reaches a program). */
int kbd_feed(struct kbd_decoder *d, unsigned char sc, unsigned char *out);
/* R12C (-DQDOS_TASKMGR): Ctrl+Esc returns 3 (the task selector hotkey); no byte is produced. */

#endif
