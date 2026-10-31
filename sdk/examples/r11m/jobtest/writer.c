/* WRITER: R11M job-control test child.  WRITER <file> [lines [ticks]]: appends "line N" to <file> every <ticks> ticks,
 * flushing each line, keeping the file open between lines.  Used to kill a job that holds a writable file. */
#include <stdio.h>
#include <stdlib.h>
#include "qdos386.h"

int main(int argc, char **argv)
{
	int lines = 1000, ticks = 25, i;
	FILE *f;

	if (argc < 2)
		return 2;
	if (argc > 2)
		lines = atoi(argv[2]);
	if (argc > 3)
		ticks = atoi(argv[3]);
	f = fopen(argv[1], "w");
	if (f == NULL)
		return 3;
	for (i = 1; i <= lines; i++) {
		fprintf(f, "line %d\n", i);
		fflush(f);
		qd_yield((unsigned int) ticks);
	}
	fclose(f);
	return 0;
}
