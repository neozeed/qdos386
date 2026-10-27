/*
 * SDK example: args.  Prints argc and each argv string, and exits with argc.
 * RUN SDKARG.AOU one "two words" prints three arguments and exits with 3.
 * Build:  qdos386-gcc -o SDKARG.aout args.c
 */
#include "qdos386.h"

static void put_num(int con, unsigned int v)
{
	char buf[12];
	int i = 0;

	do {
		buf[i++] = (char) ('0' + (v % 10));
		v /= 10;
	} while (v != 0);
	while (i > 0) {
		i--;
		qd_putc(con, buf[i]);
	}
}

int main(int argc, char **argv)
{
	int i;
	int con = qd_open("CON");

	qd_write(con, "argc = ", 7);
	put_num(con, (unsigned int) argc);
	qd_write(con, "\n", 1);
	for (i = 0; i < argc; i++) {
		unsigned int n = 0;
		while (argv[i][n] != 0)
			n++;
		qd_write(con, "argv[", 5);
		put_num(con, (unsigned int) i);
		qd_write(con, "] = \"", 5);
		qd_write(con, argv[i], n);
		qd_write(con, "\"\n", 2);
	}
	qd_close(con);
	return argc;
}
