/*
 * SDK example: stream.  Reads WIN1_README.TXT through a file channel in
 * 64-byte pieces, counts the bytes, seeks back to the start, and checks that
 * the first 16 bytes read again are the same.  Exit 0 on success.
 * RUN SDKSTM.AOU runs it; the file is read only, nothing is written.
 * Build:  qdos386-gcc -o SDKSTM.aout stream.c
 */
#include "qdos386.h"

static unsigned char first[16];

int main(int argc, char **argv)
{
	unsigned char buf[64];
	unsigned long total = 0;
	unsigned int i;
	int ch, got, k;
	int con;

	(void) argc;
	(void) argv;
	con = qd_open("CON");
	ch = qd_open("WIN1_README.TXT");
	if (ch <= 0) {
		qd_write(con, "stream: cannot open WIN1_README.TXT\n", 36);
		return 2;
	}
	for (;;) {
		got = qd_read(ch, buf, sizeof buf);
		if (got < 0)
			return 3;
		if (got == 0)
			break;
		if (total < sizeof first)
			for (k = 0; k < got && total + (unsigned long) k < sizeof first; k++)
				first[total + k] = buf[k];
		total += (unsigned long) got;
	}
	if (qd_seek(ch, 0) != 0)
		return 4;
	got = qd_read(ch, buf, sizeof first);
	for (i = 0; i < sizeof first; i++)
		if (buf[i] != first[i])
			return 5;
	qd_close(ch);
	if (total == 0)
		return 6;
	qd_write(con, "QDOS/386 SDK STREAM PASS\n", 25);
	qd_close(con);
	return 0;
}
