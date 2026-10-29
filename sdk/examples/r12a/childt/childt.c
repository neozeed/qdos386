/* CHILDT.AOU (R12A): the child for SPAWNT.  usage: CHILDT tag ticks status [read]
 * Allocates and checks a private heap block, sleeps `ticks` in steps, prints CHILDT <tag> done <status> and ends with
 * `status`.  With "read" it first reads one line from its console and prints it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"

int main(int argc, char **argv)
{
	int ticks, status, i, bad = 0;
	char line[64], *p;
	struct qd_procinfo pi;

	if (argc < 4) {
		printf("usage: CHILDT tag ticks status [read]\n");
		return 1;
	}
	ticks = atoi(argv[2]);
	status = atoi(argv[3]);
	p = (char *) malloc(5000);
	if (p == 0)
		return 98;
	memset(p, argv[1][0], 5000);
	if (argc > 4 && strcmp(argv[4], "read") == 0) {
		printf("CHILDT %s reading a line\n", argv[1]);
		if (fgets(line, sizeof line, stdin) == 0)
			line[0] = 0;
		line[strcspn(line, "\r\n")] = 0;
		printf("CHILDT %s got '%s'\n", argv[1], line);
	}
	for (i = 0; i < ticks; i += 5)
		qd_yield(5);
	for (i = 0; i < 5000; i++)
		if (p[i] != argv[1][0])
			bad++;
	free(p);
	qd_proc_info(0, &pi);
	printf("CHILDT %s done %d parent=%lu origin=%06lx space=%08lx\n", argv[1], bad ? 97 : status, pi.parent, pi.origin,
	       pi.space);
	return bad ? 97 : status;
}
