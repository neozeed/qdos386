/* CONWAIT: waits for console input with QT_CON_WAIT, reads one character from
 * stdin and exits with its value.  The harness types "Z" after its RUN line. */
#include <stdio.h>
#include "qdos386.h"

int main(void)
{
	unsigned char flag = 0;
	int c;

	qd_con_wait(&flag);
	c = getchar();
	printf("CONWAIT got %d\n", c);
	return c;
}
