/* COUNTER: R11M demo job.  Prints "COUNTER <console> <n>" lines at a fixed pace, in its own console, and exits.
 *   COUNTER [count [ticks]]      defaults: 40 lines, one every 25 ticks.  TICKER is the same program built with
 *                                a fixed header line for the progress bar demo.
 * Exit status 0 after the last line; "COUNTER DONE" is the completion marker. */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

int qd_con_channel(void);

int main(int argc, char **argv)
{
	int count = 40, ticks = 25, i, con;
	int ch = qd_con_channel();

	if (argc > 1)
		count = atoi(argv[1]);
	if (argc > 2)
		ticks = atoi(argv[2]);
	if (count < 1)
		count = 1;
	if (count > 100000)
		count = 100000;
	if (ticks < 1)
		ticks = 1;
	con = qd_con_number(ch);
#ifdef TICKER
	qd_con_clear(ch, QD_CLR_ALL);
	qd_con_attr(ch, QD_ATTR(14, 1));
	printf("TICKER on console %d", con);
	fflush(stdout);
	qd_con_clear(ch, QD_CLR_EOL);
	qd_con_attr(ch, QD_ATTR(7, 0));
	printf("\n");
	fflush(stdout);
	for (i = 1; i <= count; i++) {
		qd_con_goto(ch, 2, 0);
		printf("tick %d of %d", i, count);
		fflush(stdout);                      /* the cursor operations below act at once */
		qd_con_clear(ch, QD_CLR_EOL);
		qd_yield((unsigned int) ticks);
	}
	printf("\nTICKER DONE\n");
#else
	for (i = 1; i <= count; i++) {
		printf("COUNTER %d %d\n", con, i);
		qd_yield((unsigned int) ticks);
	}
	printf("COUNTER DONE %d\n", con);
#endif
	return 0;
}
