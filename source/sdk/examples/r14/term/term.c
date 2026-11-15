/* TERM: R14 minimal serial terminal (docs/R14-SERIAL-ABI.md).
 *   TERM [SER1|SER2[suffix]] [baud]        default SER1 9600
 * Rows 0..23 show what arrives (printable bytes and code page 437 bytes 0x80..0xFF as they are; CR, LF, BS and TAB are
 * obeyed; other control bytes are not shown).  Row 24 is the status line: port, settings, byte counts, drops, line
 * errors.  Keys typed on TERM's console are sent: printable keys and Ctrl+letter as their codes, Enter as CR,
 * Backspace as BS, cursor keys as ESC [ A/B/C/D.  F10 ends TERM.  There is no terminal emulation (no escape sequence
 * interpreter): received ESC bytes are not shown.
 * TERM never polls: it sleeps in the kernel until serial data or a key arrives (qd_ser_wait QD_SER_RX | QD_SER_KEY),
 * at most half a second, to refresh the status line.  Keyboard input reaches it only while its console is in the
 * foreground; received data keeps being drawn into its own console when it is not. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qdos386.h>

char _prog_name[] = "TERM";

static int con, ser;
static struct qd_ser_cfg cfg;
static char port[16];
static int row, col;

static void status(void)
{
	struct qd_ser_status st;
	char line[96];
	int n;

	if (qd_ser_status(ser, &st) != 0)
		return;
	sprintf(line, " TERM %s %lu %u%c%u | rx %lu tx %lu | drop %lu err %lu brk %lu | F10 ends", port, cfg.baud,
		cfg.data_bits, cfg.parity, cfg.stop_bits, st.rx_bytes, st.tx_bytes, st.rx_dropped,
		st.overrun + st.parity + st.framing, st.breaks);
	n = (int) strlen(line);
	while (n < 79)
		line[n++] = ' ';
	line[79] = 0;
	qd_con_cursor(con, &row, &col);
	qd_con_goto(con, 24, 0);
	qd_con_attr(con, QD_ATTR(0, 7));
	qd_write(con, line, 79);
	qd_con_attr(con, QD_ATTR(7, 0));
	qd_con_goto(con, row, col);
}

static void show(const unsigned char *b, int n)
{
	char out[300];
	int k = 0, i;

	for (i = 0; i < n; i++) {
		unsigned char c = b[i];
		if (c >= 32 && c != 127)
			out[k++] = (char) c;
		else if (c == '\r' || c == '\n' || c == '\b' || c == '\t')
			out[k++] = (char) c;
	}
	if (k)
		qd_write(con, out, (unsigned int) k);
}

static void key(int k)
{
	unsigned char b[4];
	int n = 0;

	switch (k) {
	case QD_KEY_UP:    b[0] = 27; b[1] = '['; b[2] = 'A'; n = 3; break;
	case QD_KEY_DOWN:  b[0] = 27; b[1] = '['; b[2] = 'B'; n = 3; break;
	case QD_KEY_RIGHT: b[0] = 27; b[1] = '['; b[2] = 'C'; n = 3; break;
	case QD_KEY_LEFT:  b[0] = 27; b[1] = '['; b[2] = 'D'; n = 3; break;
	default:
		if (k >= 0 && k < 0x80) { b[0] = (unsigned char) k; n = 1; }
	}
	if (n)
		qd_ser_write_all(ser, b, (unsigned int) n, 100);
}

int main(int argc, char **argv)
{
	unsigned char buf[256];
	int r, k, quit = 0;
	unsigned long last = 0;

	strcpy(port, argc > 1 ? argv[1] : "SER1");
	con = qd_open("CON");
	ser = qd_open(port);
	if (ser < 0) {
		printf("TERM: cannot open %s (%d%s)\n", port, ser,
		       ser == QD_SER_NOTFOUND ? ": no such port" : ser == QD_SER_INUSE ? ": in use" : "");
		return 1;
	}
	qd_ser_getconfig(ser, &cfg);
	cfg.baud = argc > 2 ? (unsigned long) atol(argv[2]) : 9600UL;
	if ((r = qd_ser_config(ser, &cfg)) != 0) {
		printf("TERM: %s: unsupported setting (%d)\n", port, r);
		qd_close(ser);
		return 1;
	}
	qd_con_mode(con, 0);                   /* raw keys, no echo */
	qd_con_clear(con, QD_CLR_ALL);
	qd_con_region(con, 0, 23);
	qd_con_goto(con, 0, 0);
	status();
	while (!quit) {
		r = qd_ser_wait(ser, QD_SER_RX | QD_SER_KEY, 50);
		while ((r = qd_read(ser, buf, sizeof buf)) > 0)
			show(buf, r);
		while (qd_con_queued(con) > 0) {
			k = qd_con_getkey(con);
			if (k == QD_KEY_F1 + 9) {
				quit = 1;
				break;
			}
			key(k);
		}
		if (qd_ticks() - last >= 50) {
			status();
			last = qd_ticks();
		}
	}
	status();
	qd_ser_drain(ser, 100);
	qd_close(ser);
	qd_con_region(con, 0, 24);
	qd_con_goto(con, 24, 0);
	printf("\nTERM: %s closed\n", port);
	return 0;
}
