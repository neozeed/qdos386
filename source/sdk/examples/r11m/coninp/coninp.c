/* CONINP: R11M guest test of per-console input.  Reads lines from its own console (blocking, with the kernel's
 * console wait) and echoes them back as "GOT <console>: <line>" until it reads "quit".  Used with two copies on two
 * consoles to prove input goes only to the focused console.  Optional argument "cooked" selects the cooked line discipline
 * with kernel echo.  Completion marker: "CONINP DONE <console>". */
#include <stdio.h>
#include <string.h>
#include "qdos386.h"

int qd_con_channel(void);

int main(int argc, char **argv)
{
	int ch = qd_con_channel(), con = qd_con_number(ch);
	char line[130];
	int n, c, got = 0;

	if (argc > 1 && strcmp(argv[1], "cooked") == 0)
		qd_con_mode(ch, QD_CON_COOKED | QD_CON_ECHO);
	printf("CONINP ready %d\n", con);
	for (;;) {
		n = 0;
		for (;;) {
			c = qd_con_getc(ch);
			if (c == QD_EOF) {
				printf("CONINP EOF %d\n", con);
				return 3;
			}
			if (c == '\r' || c == '\n')
				break;
			if (n < (int) sizeof line - 1)
				line[n++] = (char) c;
		}
		line[n] = 0;
		got++;
		printf("GOT %d: %s\n", con, line);
		if (strcmp(line, "quit") == 0)
			break;
	}
	printf("CONINP DONE %d (%d lines)\n", con, got);
	return 0;
}
