/* CONSOL: console input through getchar and fgets(stdin).  The driver types,
 * after "RUN CONSOL.AOU": the line "abc" and then the line "hello world".
 * Expected reads: 'a', 'b', 'c', '\n' with getchar, then the line
 * "hello world\n" with fgets.  The program must wait (yielding) and must not
 * spin.  stdout is flushed before each wait, so the prompt is visible. */
#include <stdio.h>
#include <string.h>
#include "../check.h"

int main(void)
{
	char line[64];
	int c;

	printf("CONSOL: type abc then Enter, then hello world then Enter\n");
	c = getchar();
	CHECK("getchar reads a", c == 'a');
	CHECK("getchar reads b", getchar() == 'b');
	CHECK("getchar reads c", getchar() == 'c');
	CHECK("getchar reads newline", getchar() == '\n');
	CHECK("fgets on stdin reads a line", fgets(line, sizeof line, stdin) != NULL);
	CHECK("fgets line is hello world", strcmp(line, "hello world\n") == 0);
	CHECK("stdin not at eof", feof(stdin) == 0);
	CHECK("stdin not in error", ferror(stdin) == 0);
	CHECK("stdout is not readable", fgetc(stdout) == EOF && ferror(stdout) != 0);
	clearerr(stdout);
	CHECK("ungetc on stdin then getchar", ungetc('Z', stdin) == 'Z' && getchar() == 'Z');
	return finish("CONSOL");
}
