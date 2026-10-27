/* LIBQDO: the native QDOS wrappers (libqdos.a) used directly: channels,
 * memory, timer and stale or invalid handles.  Stale and invalid handles must
 * fail with a negative code, never corrupt another channel or the heap. */
#include <stdio.h>
#include <string.h>
#include "../check.h"
#include "qdos386.h"

int main(void)
{
	int ch;
	int ch2;
	char buf[32];
	void *p;
	void *q;
	unsigned int before;
	unsigned int during;
	unsigned long t0;
	unsigned long t1;
	int got;

	ch = qd_open("WIN1_LINES.TXT");
	CHECK("qd_open WIN1_LINES.TXT", ch > 0);
	got = qd_read(ch, buf, 5);
	CHECK("qd_read 5 bytes", got == 5 && memcmp(buf, "alpha", 5) == 0);
	CHECK("qd_seek to 6", qd_seek(ch, 6UL) >= 0);
	got = qd_read(ch, buf, 4);
	CHECK("qd_read after seek", got == 4 && memcmp(buf, "beta", 4) == 0);
	CHECK("qd_close", qd_close(ch) >= 0);
	CHECK("stale channel read fails", qd_read(ch, buf, 1) < 0);
	CHECK("stale channel close fails", qd_close(ch) < 0);
	CHECK("stale channel seek fails", qd_seek(ch, 0UL) < 0);

	CHECK("missing file open fails", qd_open("WIN1_NOSUCH.TXT") <= 0);
	CHECK("negative channel read fails", qd_read(-5, buf, 1) < 0);

	ch = qd_open("WIN1_LINES.TXT");
	ch2 = qd_open("WIN1_LINES.TXT");
	CHECK("two channels on one file are distinct", ch > 0 && ch2 > 0 && ch != ch2);
	CHECK("first channel still valid", qd_read(ch, buf, 1) == 1 && buf[0] == 'a');
	CHECK("second channel independent", qd_read(ch2, buf, 1) == 1 && buf[0] == 'a');
	qd_close(ch);
	qd_close(ch2);

	before = qd_free_bytes();
	p = qd_alloc(1000);
	CHECK("qd_alloc 1000", p != NULL);
	during = qd_free_bytes();
	CHECK("free bytes drop after alloc", during < before);
	if (p != NULL) {
		memset(p, 0x5a, 1000);
		CHECK("qd_free returns 0", qd_free(p) == 0);
	}
	CHECK("free bytes return after free", qd_free_bytes() == before);
	CHECK("double free fails", p == NULL || qd_free(p) < 0);
	CHECK("heap invariants hold", qd_heap_ok() == 1);
	q = qd_alloc(0x7fffffffu);
	CHECK("oversized alloc returns NULL", q == NULL);
	CHECK("heap still valid after failed alloc", qd_heap_ok() == 1);

	t0 = qd_ticks();
	qd_yield(2);
	t1 = qd_ticks();
	CHECK("ticks advance across a yield", t1 >= t0 + 1UL);
	return finish("LIBQDO");
}
