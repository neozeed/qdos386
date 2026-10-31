/* XFER: R14 serial TEST transfer probe, protocol "QXF1".  NOT XMODEM, NOT YMODEM and not a QLTerm feature: a
 * deliberately simple framed, checksummed, stop-and-wait protocol used to prove the serial path end to end
 * (tests/r14/serpeer.py implements the host side).
 *   XFER GET name path [SER1|SER2]     fetch host fixture <name> into path (created / replaced)
 *   XFER PUT path name [SER1|SER2]     send path to the host, which stores it as <name>
 *   XFER SUM path                      size and CRC-32 of a file
 * Frame (both directions): STX 'D' seq(2) len(2) data(len <= 1024) crc32(4) over seq..data, big-endian;
 * end: STX 'E' total(4) crc32-of-file(4) crc32(4) over total..filecrc.  The receiver answers each frame with the line
 * "ACK seq" or "NAK seq" ("ACK END" / "NAK END"); the sender repeats a frame on NAK or after 2 s, at most 10 times. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qdos386.h>

char _prog_name[] = "XFER";

static unsigned long crctab[256];
static void crc_init(void)
{
	unsigned long c;
	int n, k;
	for (n = 0; n < 256; n++) {
		c = (unsigned long) n;
		for (k = 0; k < 8; k++)
			c = (c & 1) ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
		crctab[n] = c;
	}
}
static unsigned long crc_upd(unsigned long c, const unsigned char *p, unsigned int n)
{
	c ^= 0xFFFFFFFFUL;
	while (n--)
		c = crctab[(c ^ *p++) & 0xff] ^ (c >> 8);
	return c ^ 0xFFFFFFFFUL;
}
static void put32(unsigned char *p, unsigned long v) { p[0] = (unsigned char) (v >> 24); p[1] = (unsigned char) (v >> 16); p[2] = (unsigned char) (v >> 8); p[3] = (unsigned char) v; }
static unsigned long get32(const unsigned char *p) { return ((unsigned long) p[0] << 24) | ((unsigned long) p[1] << 16) | ((unsigned long) p[2] << 8) | p[3]; }

static int ser;

static int rd(unsigned char *b, unsigned int n, unsigned long ticks)
{
	unsigned int got = 0;
	int r;
	while (got < n) {
		r = qd_ser_read_timeout(ser, b + got, n - got, ticks);
		if (r < 0)
			return r;
		got += (unsigned int) r;
	}
	return 0;
}

static int line(char *b, int max, unsigned long ticks)
{
	int n = 0;
	unsigned char c;
	for (;;) {
		if (rd(&c, 1, ticks) != 0)
			return -1;
		if (c == '\n') { b[n] = 0; return n; }
		if (c != '\r' && n < max - 1)
			b[n++] = (char) c;
	}
}

static int say(const char *s) { return qd_ser_write_all(ser, s, strlen(s), 300); }

static unsigned char frame[1040], data[1024];

/* receive frames into channel f; returns total bytes or negative */
static long receive(int f, unsigned long *crc_out, int *retries)
{
	unsigned long total = 0, crc = 0, want;
	unsigned int expect = 0, seq, len;
	unsigned char h[2];
	char msg[32];
	int r, tries = 0;

	for (;;) {
		if (tries > 10)
			return -2;
		/* hunt for STX */
		do {
			r = rd(h, 1, 300);
			if (r != 0)
				return -1;
		} while (h[0] != 2);
		if (rd(h + 1, 1, 200) != 0)
			return -1;
		if (h[1] == 'E') {
			if (rd(frame, 12, 200) != 0)
				return -1;
			if (crc_upd(0, frame, 8) != get32(frame + 8) || get32(frame) != total || get32(frame + 4) != crc) {
				say("NAK END\n"); tries++; ++*retries;
				continue;
			}
			say("ACK END\n");
			*crc_out = crc;
			return (long) total;
		}
		if (h[1] != 'D' || rd(frame, 4, 200) != 0) {
			tries++; ++*retries;
			continue;
		}
		seq = ((unsigned int) frame[0] << 8) | frame[1];
		len = ((unsigned int) frame[2] << 8) | frame[3];
		if (len > 1024 || rd(frame + 4, len + 4, 200) != 0) {
			sprintf(msg, "NAK %u\n", expect); say(msg); tries++; ++*retries;
			qd_ser_flush(ser, QD_SER_FLUSH_RX);
			continue;
		}
		want = get32(frame + 4 + len);
		if (crc_upd(0, frame, 4 + len) != want) {
			sprintf(msg, "NAK %u\n", seq); say(msg); tries++; ++*retries;
			continue;
		}
		if (seq == ((expect - 1) & 0xffff) && expect != 0) {   /* our ACK was lost: the repeat is acknowledged again */
			sprintf(msg, "ACK %u\n", seq); say(msg);
			continue;
		}
		if (seq != expect) {
			sprintf(msg, "NAK %u\n", expect); say(msg); tries++; ++*retries;
			continue;
		}
		if (len && qd_write_all(f, frame + 4, len) != (int) len)
			return -3;
		crc = crc_upd(crc, frame + 4, len);
		total += len;
		expect = (expect + 1) & 0xffff;
		tries = 0;
		sprintf(msg, "ACK %u\n", seq); say(msg);
	}
}

static int send_frame(unsigned int n, int *retries)
{
	char ans[32], want[32];
	int t;
	sprintf(want, (frame[1] == 'E') ? "ACK END" : "ACK %u", ((unsigned int) frame[2] << 8) | frame[3]);
	for (t = 0; t < 10; t++) {
		qd_ser_write_all(ser, frame, n, 500);
		if (line(ans, sizeof ans, 200) >= 0 && strcmp(ans, want) == 0)
			return 0;
		++*retries;
	}
	return -1;
}

int main(int argc, char **argv)
{
	char path[100], cmd[120], ans[80];
	const char *port;
	unsigned long crc = 0, t;
	long total;
	int f, r, n, retries = 0;
	unsigned int seq = 0;

	crc_init();
	if (argc < 3 || (strcmp(argv[1], "SUM") && argc < 4)) {
		printf("usage: XFER GET name path [port] | PUT path name [port] | SUM path   (QXF1 test probe, not XMODEM)\n");
		return 2;
	}
	sprintf(path, "WIN1:%s", strcmp(argv[1], "PUT") == 0 || strcmp(argv[1], "SUM") == 0 ? argv[2] : argv[3]);
	if (strcmp(argv[1], "SUM") == 0) {
		f = qd_open_ex(path, QD_O_READ);
		if (f < 0) { printf("XFER SUM %s: cannot open (%d)\n", argv[2], f); return 1; }
		total = 0;
		while ((n = qd_read(f, data, sizeof data > 256 ? 256 : sizeof data)) > 0) {
			crc = crc_upd(crc, data, (unsigned int) n);
			total += n;
		}
		qd_close(f);
		printf("XFER SUM %s size=%ld crc32=%08lx\n", argv[2], total, crc);
		return 0;
	}
	port = argc > 4 ? argv[4] : "SER1";
	ser = qd_open(port);
	if (ser < 0) { printf("XFER: cannot open %s (%d)\n", port, ser); return 1; }
	{
		struct qd_ser_cfg c;
		qd_ser_getconfig(ser, &c);
		c.baud = 115200;
		qd_ser_config(ser, &c);
	}
	qd_ser_flush(ser, QD_SER_FLUSH_RX);
	t = qd_ticks();
	if (strcmp(argv[1], "GET") == 0) {
		sprintf(cmd, "XGET %s\n", argv[2]);
		say(cmd);
		if (line(ans, sizeof ans, 500) < 0 || strncmp(ans, "XSIZE ", 6) != 0) {
			printf("XFER GET %s: no answer from the host\n", argv[2]);
			qd_close(ser); return 1;
		}
		if (atol(ans + 6) < 0) {
			printf("XFER GET %s: the host has no such file (nothing written)\n", argv[2]);
			qd_close(ser); return 1;
		}
		f = qd_open_ex(path, QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC);
		if (f < 0) { printf("XFER GET: cannot create %s (%d)\n", argv[3], f); qd_close(ser); return 1; }
		total = receive(f, &crc, &retries);
		r = qd_close(f);
		printf("XFER GET %s -> %s: %ld bytes crc32=%08lx retries=%d close=%d ticks=%lu host='%s' %s\n", argv[2], argv[3],
		       total, crc, retries, r, qd_ticks() - t, ans + 6, total >= 0 && r == 0 ? "OK" : "FAILED");
		qd_close(ser);
		return total >= 0 && r == 0 ? 0 : 1;
	}
	if (strcmp(argv[1], "PUT") == 0) {
		long size;
		if ((f = qd_open_ex(path, QD_O_READ)) < 0 || (size = qd_fsize(f)) < 0) {
			printf("XFER PUT: cannot open %s (%d)\n", argv[2], f); qd_close(ser); return 1;
		}
		sprintf(cmd, "XPUT %s %ld\n", argv[3], size);
		say(cmd);
		if (line(ans, sizeof ans, 500) < 0 || strcmp(ans, "READY") != 0) {
			printf("XFER PUT: host not ready ('%s')\n", ans); qd_close(f); qd_close(ser); return 1;
		}
		total = 0;
		for (;;) {
			n = 0;
			while (n < 1024 && (r = qd_read(f, data + n, 1024 - n > 256 ? 256 : 1024 - n)) > 0)
				n += r;
			if (n == 0)
				break;
			frame[0] = 2; frame[1] = 'D';
			frame[2] = (unsigned char) (seq >> 8); frame[3] = (unsigned char) seq;
			frame[4] = (unsigned char) (n >> 8); frame[5] = (unsigned char) n;
			memcpy(frame + 6, data, (unsigned int) n);
			put32(frame + 6 + n, crc_upd(0, frame + 2, (unsigned int) n + 4));
			if (send_frame((unsigned int) n + 10, &retries) != 0) { total = -1; break; }
			crc = crc_upd(crc, data, (unsigned int) n);
			total += n;
			seq = (seq + 1) & 0xffff;
		}
		qd_close(f);
		if (total >= 0) {
			frame[0] = 2; frame[1] = 'E'; frame[2] = 0; frame[3] = 0;     /* frame[2..3] unused by send_frame for E */
			put32(frame + 2, (unsigned long) total); put32(frame + 6, crc); put32(frame + 10, crc_upd(0, frame + 2, 8));
			if (send_frame(14, &retries) != 0)
				total = -1;
		}
		r = total >= 0 ? line(ans, sizeof ans, 500) : -1;
		printf("XFER PUT %s -> %s: %ld bytes crc32=%08lx retries=%d ticks=%lu host='%s' %s\n", argv[2], argv[3], total,
		       crc, retries, qd_ticks() - t, r >= 0 ? ans : "", total >= 0 && r >= 0 && strncmp(ans, "STORED", 6) == 0 ? "OK" : "FAILED");
		qd_close(ser);
		return total >= 0 ? 0 : 1;
	}
	printf("XFER: unknown command %s\n", argv[1]);
	qd_close(ser);
	return 2;
}
