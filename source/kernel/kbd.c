/*
 * QDOS/386 R7 - scancode set 1 decoder.  See kbd.h.
 *
 * The tables are indexed by make code (0x00..0x39).  Unused codes are NUL.
 * Each string literal below is a separate piece so that \0 escapes never run
 * into a following digit.
 */
#include "kbd.h"

#define KBD_TABLE_LEN 0x3a

static const unsigned char plain[] =
	"\0\0" "1234567890-=" "\0\0" "qwertyuiop[]" "\0\0" "asdfghjkl;'`"
	"\0" "\\" "zxcvbnm,./" "\0\0\0" " ";

static const unsigned char upper[] =
	"\0\0" "!@#$%^&*()_+" "\0\0" "QWERTYUIOP{}" "\0\0" "ASDFGHJKL:\"~"
	"\0" "|" "ZXCVBNM<>?" "\0\0\0" " ";

void kbd_reset(struct kbd_decoder *d)
{
	d->shift = 0;
	d->ext = 0;
	d->skip = 0;
#ifdef QDOS_VCON
	d->alt = 0;
	d->ctrl = 0;
#endif
}

int kbd_feed(struct kbd_decoder *d, unsigned char sc, unsigned char *out)
{
	unsigned int code;
	int make;

	if (d->skip != 0) {                 /* inside E1 (Pause): drop */
		d->skip--;
		return 0;
	}
	if (sc == 0xe0) {
		d->ext = 1;
		return 0;
	}
	if (sc == 0xe1) {
		d->skip = 5;                /* 1D 45 E1 9D C5 follow */
		return 0;
	}
	if (d->ext) {                       /* extended key: keypad Enter (and, R12, the cursor/edit keys) */
		d->ext = 0;
#ifdef QDOS_VCON
		if ((sc & 0x7f) == 0x38) {      /* right Alt (AltGr) */
			d->alt = (sc & 0x80) == 0;
			return 0;
		}
		if ((sc & 0x7f) == 0x1d) {      /* right Ctrl */
			d->ctrl = (sc & 0x80) == 0;
			return 0;
		}
		if ((sc & 0x80) == 0 && !d->alt) {   /* R12: Up Down Left Right Home End PgUp PgDn Insert Delete */
			unsigned char k = 0;

			switch (sc) {
			case 0x48: k = KBD_KEY_UP; break;
			case 0x50: k = KBD_KEY_DOWN; break;
			case 0x4b: k = KBD_KEY_LEFT; break;
			case 0x4d: k = KBD_KEY_RIGHT; break;
			case 0x47: k = KBD_KEY_HOME; break;
			case 0x4f: k = KBD_KEY_END; break;
			case 0x49: k = KBD_KEY_PGUP; break;
			case 0x51: k = KBD_KEY_PGDN; break;
			case 0x52: k = KBD_KEY_INS; break;
			case 0x53: k = KBD_KEY_DEL; break;
			default: break;
			}
			if (k != 0) {
				*out = k;
				return 1;
			}
		}
#endif
		if ((sc & 0x7f) == 0x1c && (sc & 0x80) == 0) {
			*out = '\r';
			return 1;
		}
		return 0;
	}

	make = (sc & 0x80) == 0;
	code = (unsigned int) (sc & 0x7f);

	if (code == 0x2a || code == 0x36) {  /* left or right Shift */
		d->shift = make ? 1 : 0;
		return 0;
	}
#ifdef QDOS_VCON
	if (code == 0x38) {                  /* Alt */
		d->alt = make ? 1 : 0;
		return 0;
	}
	if (code == 0x1d) {                  /* left Ctrl (R12) */
		d->ctrl = make ? 1 : 0;
		return 0;
	}
	if (make && d->alt) {                /* console hotkeys: Alt+F1..F4, Alt+1..4 */
		if (code >= 0x3b && code <= 0x3e) {
			*out = (unsigned char) (code - 0x3b);
			return 2;
		}
		if (code >= 0x02 && code <= 0x05) {
			*out = (unsigned char) (code - 0x02);
			return 2;
		}
	}
#endif
	if (!make)
		return 0;

#ifdef QDOS_TASKMGR
	if (code == 0x01 && d->ctrl)         /* R12C: Ctrl+Esc opens the task selector; never reaches a program */
		return 3;
#endif
#ifdef QDOS_VCON
	if (code == 0x01) {                  /* R12: Esc */
		*out = 0x1b;
		return 1;
	}
	if (code >= 0x3b && code <= 0x44) {  /* R12: F1..F10 (Alt+F1..F4 were taken above) */
		*out = (unsigned char) (KBD_KEY_F1 + (code - 0x3b));
		return 1;
	}
#endif
	if (code == 0x1c) {
		*out = '\r';
		return 1;
	}
	if (code == 0x0e) {
		*out = 0x08;
		return 1;
	}
	if (code == 0x0f) {
		*out = '\t';
		return 1;
	}
	if (code >= KBD_TABLE_LEN)
		return 0;
	*out = d->shift ? upper[code] : plain[code];
#ifdef QDOS_VCON
	if (d->ctrl && *out != 0) {          /* R12: Ctrl+letter is 0x01..0x1A; Ctrl+[ \ ] are 0x1B..0x1D; others are dropped */
		unsigned char c = *out;

		if (c >= 'a' && c <= 'z')
			*out = (unsigned char) (c - 'a' + 1);
		else if (c >= 'A' && c <= 'Z')
			*out = (unsigned char) (c - 'A' + 1);
		else if (c == '[' || c == '{')
			*out = 0x1b;
		else if (c == '\\' || c == '|')
			*out = 0x1c;
		else if (c == ']' || c == '}')
			*out = 0x1d;
		else
			return 0;
	}
#endif
	return *out != 0;
}
