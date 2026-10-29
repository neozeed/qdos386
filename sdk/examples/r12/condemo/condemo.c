/* CONDEMO: R12 interactive console demo (C89), the "native program that positions and clears text on its own console".
 * Draws a title and a frame with qd_con_putat, then reports every key as "KEY 0xNN name" inside the frame.  Keys:
 *   c  clear the inside of the frame     x  clear the whole console and redraw     q  quit (the console is cleared)
 * Runs on any console (START CONDEMO.AOU /CON:1).  Exits with status 0.  The last line written is "CONDEMO DONE". */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

int qd_con_channel(void);

static int ch;

static const char *keyname(int k, char *buf)
{
	if (k == QD_KEY_UP) return "UP";
	if (k == QD_KEY_DOWN) return "DOWN";
	if (k == QD_KEY_LEFT) return "LEFT";
	if (k == QD_KEY_RIGHT) return "RIGHT";
	if (k == QD_KEY_HOME) return "HOME";
	if (k == QD_KEY_END) return "END";
	if (k == QD_KEY_PGUP) return "PGUP";
	if (k == QD_KEY_PGDN) return "PGDN";
	if (k == QD_KEY_DEL) return "DEL";
	if (k == QD_KEY_INS) return "INS";
	if (k >= QD_KEY_F1 && k < QD_KEY_F1 + 10) {
		sprintf(buf, "F%d", k - QD_KEY_F1 + 1);
		return buf;
	}
	if (k == QD_KEY_ESC) return "ESC";
	if (k == '\r') return "ENTER";
	if (k == 8) return "BS";
	if (k == '\t') return "TAB";
	if (k > 0 && k < 27) {
		sprintf(buf, "CTRL-%c", 'A' + k - 1);
		return buf;
	}
	if (k >= 32 && k < 127) {
		sprintf(buf, "'%c'", k);
		return buf;
	}
	return "?";
}

static void frame(void)
{
	int r;
	static const char top[] = "+------------------------------------------------------------------------+";
	static const char mid[] = "|                                                                        |";

	qd_con_attr(ch, QD_ATTR(14, 1));
	qd_con_putat(ch, 0, 0, "CONDEMO - console API 2: keys are reported below; c clears the frame, x clears the screen, q quits", 80);
	qd_con_attr(ch, QD_ATTR(7, 0));
	qd_con_putat(ch, 2, 3, top, 74);
	for (r = 3; r < 21; r++)
		qd_con_putat(ch, r, 3, mid, 74);
	qd_con_putat(ch, 21, 3, top, 74);
}

static void clear_inside(void)
{
	int r;
	for (r = 3; r < 21; r++) {
		qd_con_goto(ch, r, 4);
		qd_write(ch, "                                                                        ", 72);
	}
}

int main(void)
{
	int k, line = 0, ver, caps, con;
	char nb[16], msg[80];

	ch = qd_con_channel();
	con = qd_con_number(ch);
	ver = qd_con_version(ch);
	caps = qd_con_caps(ch);
	qd_con_clear(ch, QD_CLR_ALL);
	frame();
	sprintf(msg, "console %d, API version %d, caps 0x%X", con, ver, caps < 0 ? 0 : caps);
	qd_con_putat(ch, 23, 3, msg, (int) strlen(msg));
	for (;;) {
		k = qd_con_getkey(ch);
		if (k < 0)
			break;
		if (k == 'q')
			break;
		if (k == 'c') {
			clear_inside();
			line = 0;
			continue;
		}
		if (k == 'x') {
			qd_con_clear(ch, QD_CLR_ALL);
			frame();
			line = 0;
			continue;
		}
		sprintf(msg, "KEY 0x%02X %s", k, keyname(k, nb));
		qd_con_putat(ch, 3 + line, 5, msg, (int) strlen(msg));
		line = (line + 1) % 18;
	}
	qd_con_clear(ch, QD_CLR_ALL);
	qd_puts("CONDEMO DONE\n");
	return 0;
}
