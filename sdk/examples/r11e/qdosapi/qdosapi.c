/* QDOSAPI: exercises the R11E libqdos wrappers over the kernel traps.  Jobs,
 * heap, channels and the WIN1_ directory are checked for their documented
 * results, including the error cases.  Cross-job tests cannot run from a RUN
 * program (one child slot); see the child-jobs section. */
#include <stdio.h>
#include <string.h>
#include "../check.h"
#include "qdos386.h"

int main(void)
{
	int self, ch, r;
	unsigned int inc, inc0 = 0, acc, free0, free1;
	void *p, *q;
	struct qd_dirent e;
	int found_lines, found_empty, found_long, count;
	unsigned long t0, t1;

	/* ---- jobs ---- */
	self = qd_self();
	CHECK("qd_self is a live id", self >= 0);
	CHECK("qd_job_slot finds self", qd_job_slot(self & 0xffff) == self);
	CHECK("qd_job_slot out of range is QD_ERR_NOJOB", qd_job_slot(99) == QD_ERR_NOJOB);
	CHECK("stale tag is rejected by info", qd_job_info(self ^ 0x10000, &inc, &acc) == QD_ERR_NOJOB);
	CHECK("info of self succeeds", qd_job_info(self, &inc, &acc) == 0);
	/* A zero increment makes a job inactive (the scheduler stops running it), so
	 * this test only moves its own priority between non-zero values. */
	CHECK("info of self before change", qd_job_info(self, &inc0, &acc) == 0 && inc0 != 0);
	r = qd_job_chprio(self, inc0 + 2);
	CHECK("chprio of self", r == 0);
	CHECK("info shows new increment", qd_job_info(self, &inc, &acc) == 0 && inc == inc0 + 2);
	CHECK("activate of an active job fails", qd_job_activate(self, 1) == -1);
	CHECK("chprio restored", qd_job_chprio(self, inc0) == 0 && qd_job_info(self, &inc, &acc) == 0 && inc == inc0);
	CHECK("reactivate of a stale id fails", qd_job_reactivate(self ^ 0x10000) == QD_ERR_NOJOB);
	CHECK("exit status of a running job", qd_job_exit_status(self) == QD_ERR_RUNNING);
	CHECK("reclaimed is 0 for a live job", qd_job_reclaimed(self) == 0);
	CHECK("stale id is reclaimed", qd_job_reclaimed(self ^ 0x10000) == 1);
	CHECK("stale id has no exit status", qd_job_exit_status(self ^ 0x10000) == QD_ERR_NOJOB);

	t0 = qd_ticks();
	qd_yield(5);
	t1 = qd_ticks();
	CHECK("ticks advance over a yield", t1 > t0);
	CHECK("suspend self for 1 tick returns 0", qd_job_suspend(1) == 0);

	/* ---- heap ---- */
	free0 = qd_free_bytes();
	CHECK("alloc of zero bytes", qd_alloc_ex(0, &q) == -4 && q == 0);
	p = 0;
	CHECK("alloc of 16 bytes", qd_alloc_ex(16, &p) == 0 && p != 0);
	CHECK("heap validates after alloc", qd_mem_validate() == 0);
	CHECK("free of an interior pointer is BADPTR", qd_free((char *) p + 1) == -6);
	CHECK("free of a block", qd_free(p) == 0);
	CHECK("second free is DOUBLE", qd_free(p) == -7);
	CHECK("alloc too large fails", qd_alloc_ex(0x7fffffffu, &q) < 0 && q == 0);
	free1 = qd_free_bytes();
	CHECK("no heap leak after alloc and free", free1 == free0);
	CHECK("qd_alloc returns NULL on failure", qd_alloc(0x7fffffffu) == 0);

	/* ---- channels ---- */
	CHECK("open of a missing file fails", qd_open("WIN1_NOPE.TXT") < 0);
	CHECK("open of an unknown driver fails", qd_open("BOGUS") == -7);
	CHECK("overlong name is rejected", qd_open("WIN1_ABCDEFGHI.TXT") == -14);
	CHECK("12-character name is accepted", qd_open("WIN1_ABCDEFGH.TXT") > 0);
	ch = qd_open("WIN1_ABCDEFGH.TXT");
	if (ch > 0) {
		char b[16];
		int n = qd_read(ch, b, 16);
		CHECK("12-character file reads its contents", n == 6 && memcmp(b, "twelve", 6) == 0);
		CHECK("12-character file reaches end", qd_read(ch, b, 16) == 0);
		CHECK("write on a read-only channel is NOTSUP", qd_write(ch, "x", 1) == -15);
		CHECK("putc on a read-only channel is NOTSUP", qd_putc(ch, 'x') == -15);
		CHECK("close of an open channel", qd_close(ch) == 0);
		CHECK("close of a closed channel is BADID", qd_close(ch) == -12);
	}
	CHECK("read of a bad channel id", qd_read(999, qd_alloc(4), 4) == -12);
	ch = qd_open("WIN1_LINES.TXT");
	CHECK("open LINES.TXT", ch > 0);
	/* User buffers must lie in the user range.  0x1000 is below the heap arena
	 * (the kernel image and BSS) on both targets, so it is refused; 0x100000 is
	 * a heap address on i386VM and must not be used here.  The hold pointer of
	 * QT_SUSPEND is checked before any sleep, so these calls return at once. */
	if (ch > 0)
		CHECK("read into kernel memory is refused", qd_read(ch, (void *) 0x00001000u, 4) == -14);
	CHECK("suspend with a kernel hold pointer is refused", qtrap(QD_TRAP_SUSPEND, 1, 0x00001000u, 0) == -14);
	if (ch > 0) {
		CHECK("getc returns 'a'", qd_getc(ch) == 'a');
		CHECK("seek to 16", qd_seek(ch, 16UL) == 0);
		CHECK("getc at 16 is 'a' (gamma)", qd_getc(ch) == 'a');
		CHECK("getc at the end is QD_EOF", qd_getc(ch) == QD_EOF && qd_getc(ch) == QD_EOF);
		CHECK("seek past the end is refused", qd_seek(ch, 999UL) < 0);
		qd_close(ch);
	}

	/* ---- directory ---- */
	ch = qd_dir_open();
	CHECK("open WIN1_ root", ch > 0);
	found_lines = found_empty = found_long = count = 0;
	if (ch > 0) {
		while ((r = qd_dir_next(ch, &e)) == 1) {
			count++;
			if (strcmp(e.name, "LINES.TXT") == 0 && e.size == 17UL)
				found_lines = 1;
			if (strcmp(e.name, "EMPTY.TXT") == 0 && e.size == 0UL)
				found_empty = 1;
			if (strcmp(e.name, "ABCDEFGH.TXT") == 0 && e.size == 6UL)
				found_long = 1;
		}
		CHECK("directory ends with 0", r == 0);
		qd_close(ch);
	}
	CHECK("directory lists LINES.TXT with its size", found_lines);
	CHECK("directory lists EMPTY.TXT with size 0", found_empty);
	CHECK("directory lists a 12-character name", found_long);
	CHECK("directory has entries", count >= 8);

	/* ---- child jobs ----
	 * A program started by RUN occupies child slot 1, and qd_run loads into that
	 * slot.  So a RUN program cannot start another job: the call fails with
	 * APP_ERR_SLOTBUSY (-28).  Missing and non-program files fail earlier, in the
	 * file or header checks. */
	CHECK("run of a missing program fails", qd_run("NOPE.AOU", 0) < 0);
	CHECK("run of a non-program fails", qd_run("LINES.TXT", 0) < 0);
	CHECK("run of a program from slot 1 is APP_ERR_SLOTBUSY", qd_run("CHILD1.AOU", "CHILD1.AOU x") == -28);
	CHECK("run with a bad argument address is refused", qd_run("CHILD1.AOU", (const char *) 0x00001000u) == -14);
	CHECK("heap intact after refused runs", qd_heap_ok() == 1);

	return finish("QDOSAPI");
}
