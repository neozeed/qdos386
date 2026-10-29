/*
 * basic.c - BASIC.AOU: the SuperBASIC/C89 interpreter on QDOS/386.
 *
 *   BASIC [/Q] [/S] [/L:steps] [/M:kib] [file]
 *
 * Reads typed lines from the console (line editor, history) and hands each to
 * the unchanged core with sb_execute(), exactly as a typed line.  A file
 * argument is executed line by line first, as if typed (numbered lines go
 * into the program; INPUT then reads the keyboard); with /Q the program
 * ends after the file.  QUIT, EXIT or BYE alone on a line, or Ctrl+D on an
 * empty line, leaves.  /S prints the private stack high-water mark and the
 * count of core heap blocks still live at the end.  /L:n sets the core's
 * step safeguard (default SB_DEFAULT_STEP_LIMIT); /M:kib sets the size of the
 * interpreter's private stack (default 256, 128..384; qb_stack.c).
 *
 * The interpreter runs on a private stack (qb_stack.c).  See
 * docs/R12-SUPERBASIC-PORT.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"
#include "sb_chan.h"
#include "sb_error.h"
#include "sb_interp.h"
#include "sb_mem.h"
#include "sb_state.h"
#include "qb.h"
#ifdef QB_R12B
#include "qb_ext.h"
#endif

struct ctx {
	const char *file;
	int quit_after;
	int stats;
	long limit;
	int status;
	long live;
	int steps_set;
	unsigned int kib;
};

static int ci_word(const char *s, int n, const char *w)
{
	int i = 0;

	while (i < n && (s[i] == ' ' || s[i] == '\t'))
		i++;
	while (*w) {
		char c;

		if (i >= n)
			return 0;
		c = s[i++];
		if (c >= 'a' && c <= 'z')
			c = (char) (c - 'a' + 'A');
		if (c != *w++)
			return 0;
	}
	while (i < n && (s[i] == ' ' || s[i] == '\t'))
		i++;
	return i == n;
}

static int is_quit(const char *s, int n)
{
	return ci_word(s, n, "QUIT") || ci_word(s, n, "EXIT") || ci_word(s, n, "BYE");
}

/* Execute the lines of a text file as typed lines. */
static int load_file(SbState *st, const char *name)
{
	FILE *fp = fopen(name, "r");
	char *buf = NULL;
	unsigned int cap = 0, n = 0, i, start, lineno = 0;
	int c;

	if (fp == NULL) {
		qb_puts("BASIC: cannot open ");
		qb_puts(name);
		qb_puts("\n");
		return 2;
	}
	while ((c = getc(fp)) != EOF) {
		if (n + 1 >= cap) {
			char *nb;

			cap = cap ? cap * 2u : 1024u;
			nb = (char *) realloc(buf, cap);
			if (nb == NULL) {
				free(buf);
				fclose(fp);
				qb_puts("BASIC: out of memory reading the file\n");
				return 2;
			}
			buf = nb;
		}
		buf[n++] = (char) c;
	}
	fclose(fp);
	start = 0;
	for (i = 0; i <= n; i++) {
		if (i == n || buf[i] == '\n') {
			unsigned int e = i;

			if (e > start && buf[e - 1] == '\r')
				e--;
			lineno++;
			if (e - start > QB_LINE_MAX) {
				/* the stack budget (qb_stack.c) assumes lines of at most QB_LINE_MAX characters */
				char t[64];

				sprintf(t, "BASIC: line %u of the file is too long (limit %d), skipped\n", lineno, QB_LINE_MAX);
				qb_puts(t);
			} else if (e > start || i < n) {
				sb_execute(st, buf + start, e - start);
			}
			start = i + 1;
		}
	}
	free(buf);
	return 0;
}

static int run(void *p)
{
	struct ctx *cx = (struct ctx *) p;
	SbAlloc mem;
	SbState st;
	char buf[QB_LINE_MAX + 4];
	int n;

	sb_alloc_default_init(&mem, &cx->live);
	sb_init(&st, &mem);
	if (qb_attach(&st) != SB_OK) {
		qb_puts("BASIC: out of memory\n");
		sb_destroy(&st);
		cx->status = 1;
		return 1;
	}
	if (cx->steps_set)
		st.step_limit = cx->limit;
#ifdef QB_R12B
	qb_ext_install(&st);         /* R12B: LOAD SAVE MERGE DIR EXEC EXEC_W ... (qb_ext.c) */
#endif
	if (cx->file != NULL)
		cx->status = load_file(&st, cx->file);
	if (!cx->quit_after && cx->status == 0) {
		qb_puts("QDOS/386 SuperBASIC (SB4 core)  -  QUIT leaves, Ctrl+C breaks a printing program\n");
		for (;;) {
			n = qb_getline(buf, (int) sizeof(buf));
			if (n <= 0)
				break;
			while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
				n--;
			if (n == 0)
				continue;
			if (is_quit(buf, n))
				break;
			sb_execute(&st, buf, (size_t) n);
			qb_break_seen();
		}
	}
	qb_detach(&st);
	sb_destroy(&st);
	return cx->status;
}

int main(int argc, char **argv)
{
	struct ctx cx;
	int i;

	memset(&cx, 0, sizeof(cx));
	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '/' && (a[1] == 'Q' || a[1] == 'q') && a[2] == 0) {
			cx.quit_after = 1;
		} else if (a[0] == '/' && (a[1] == 'S' || a[1] == 's') && a[2] == 0) {
			cx.stats = 1;
		} else if (a[0] == '/' && (a[1] == 'L' || a[1] == 'l') && a[2] == ':') {
			cx.limit = atol(a + 3);
			cx.steps_set = 1;
		} else if (a[0] == '/' && (a[1] == 'M' || a[1] == 'm') && a[2] == ':') {
			cx.kib = (unsigned int) atol(a + 3);
		} else if (a[0] == '/') {
			printf("usage: BASIC [/Q] [/S] [/L:steps] [/M:kib] [file]\n");
			return 1;
		} else if (cx.file == NULL) {
			cx.file = a;
		} else {
			printf("usage: BASIC [/Q] [/S] [/L:steps] [/M:kib] [file]\n");
			return 1;
		}
	}
	if (qb_io_init() < 0) {
		printf("BASIC: needs a console\n");
		return 1;
	}
	if (cx.kib == 0)
		cx.kib = QB_STACK_DEFAULT_KIB;
	if (qb_stack_call(cx.kib, run, &cx) < 0 && qb_stack_size() == 0) {
		qb_puts("BASIC: out of memory for the interpreter stack\n");
		qb_io_done();
		return 1;
	}
	if (!qb_stack_intact())
		qb_puts("BASIC: WARNING private stack overflowed\n");
	if (cx.stats) {
		char t[96];

		sprintf(t, "BASIC stats: stack %u of %u, depth limit %d, live blocks %ld\n",
		        qb_stack_used(), qb_stack_size(), qb_depth_limit, cx.live);
		qb_puts(t);
	}
	qb_stack_free();
	qb_io_done();
	return cx.status;
}
