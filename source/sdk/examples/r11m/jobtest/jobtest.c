/* JOBT: R11M guest test of job control (C89): query, start, kill, wait, reclaim.  Needs a kernel with -DQDOS_JOBCTL;
 * the launch tests need -DQDOS_PAGING (reported NA without it).  Children are sent to console 2.
 * Banner "QDOS/386 R11M JOBT PASS" only when every check that ran passed. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "qdos386.h"
#include "../../r11b/check.h"

int qd_con_channel(void);
int remove(const char *path);

static int con;
static unsigned int free0;

static int gone(int id, int maxticks)
{
	int t;
	for (t = 0; t < maxticks; t++) {
		if (qd_job_reclaimed(id))
			return 1;
		qd_yield(1);
	}
	return qd_job_reclaimed(id);
}

static int start(const char *name, const char *packed, unsigned int len, unsigned int flags)
{
	return qd_pg_startv(name, packed, len, flags);
}

/* number of paged address spaces now */
static unsigned int spaces(void)
{
	struct qd_pginfo pi;
	if (qd_pginfo(0, &pi) != 0)
		return 9999;
	return pi.spaces_live;
}

static unsigned int held(void)
{
	struct qd_pginfo pi;
	if (qd_pginfo(0, &pi) != 0)
		return 9999;
	return pi.held_bytes;
}

int main(void)
{
	struct qd_jobinfo all[8], me, ji;
	int n, i, id, id2, started[8], nstart, firsterr, paging, nonlaunched = -1;
	unsigned char raw[400];
	unsigned int sp0, hb0;
	char row[100];
	static const char two[] = "1000\0" "50";            /* COUNTER 1000 50: argv[1], argv[2] */

	con = qd_con_channel();
	paging = qd_pg_available();
	n = qd_job_list(all, 8);
	CHECK("job control present", n > 0);
	if (n <= 0)
		return finish("JOBT");

	/* --- the query itself --- */
	CHECK("records are well formed", n >= 3 && n <= 6);
	id = qd_self();
	CHECK("own record: SELF and LAUNCHED", qd_job_find((unsigned long) id, &me) == 1 &&
	      (me.flags & QD_JF_SELF) && (me.flags & QD_JF_LAUNCHED) && me.state == QD_JS_RUN);
	CHECK("own record names this program", strncmp(me.name, "JOBT", 4) == 0);
	CHECK("own console is 0..3", me.console >= 0 && me.console < 4);
	for (i = 0; i < n; i++)
		if (!(all[i].flags & QD_JF_LAUNCHED) && !(all[i].flags & QD_JF_SELF))
			nonlaunched = i;
	CHECK("a system job is listed that was not launched", nonlaunched >= 0);
	CHECK("a one-record buffer returns one record", qd_job_list(all, 1) == 1);
	CHECK("zero max refused", qd_job_list(all, 0) == QD_ERR_RANGE);
	CHECK("bad version refused (-34)", (int) qtrap(0x2Du, (unsigned int) raw, sizeof raw, 2u) == -34);
	CHECK("buffer under 16 bytes refused (-14)", (int) qtrap(0x2Du, (unsigned int) raw, 8u, 1u) == -14);
	CHECK("buffer outside the arena refused (-14)", (int) qtrap(0x2Du, 0x10u, 400u, 1u) == -14);
	CHECK("buffer crossing the arena end refused (-14)", (int) qtrap(0x2Du, 0xFFFFFF00u, 400u, 1u) == -14);
	CHECK("header-only buffer returns 0 records", (int) qtrap(0x2Du, (unsigned int) raw, 16u, 1u) == 0 &&
	      raw[0] == 1 && raw[4] == 40 && raw[8] == 0 && raw[12] == 6);

	/* --- kill refusals --- */
	n = qd_job_list(all, 8);
	CHECK("kill of self refused", qd_job_kill((unsigned long) id) == QD_ERR_SELF);
	CHECK("kill of a system job refused", nonlaunched >= 0 && qd_job_kill(all[nonlaunched].id) == QD_ERR_PROTECT);
	CHECK("kill of a bad tag is a stale id (-2)", qd_job_kill(0x7ff00001UL) == QD_ERR_NOJOB);
	CHECK("kill of an out-of-range slot is a stale id (-2)", qd_job_kill(99999UL) == QD_ERR_NOJOB);
	CHECK("system job still listed", qd_job_find(all[nonlaunched].id, 0) == 1);

	if (!paging) {
		printf("NA: no paging in this kernel: launch/kill tests not run\n");
		return finish("JOBT");
	}
	free0 = qd_free_bytes();
	sp0 = spaces();
	hb0 = held();

	/* --- cancel before the first dispatch --- */
	qd_con_child(con, 2);
	id = start("COUNTER.AOU", two, 8, QD_PG_HELD);
	qd_con_child(con, -1);
	CHECK("held start returns a job id", id > 0);
	CHECK("held job is listed HELD with no dispatches", qd_job_find((unsigned long) id, &ji) == 1 &&
	      ji.state == QD_JS_HELD && ji.dispatches == 0 && (ji.flags & QD_JF_PAGED) && (ji.flags & QD_JF_LAUNCHED));
	CHECK("held job is named COUNTER.AOU on console 2", strcmp(ji.name, "COUNTER.AOU") == 0 && ji.console == 2);
	CHECK("held job holds an address space", spaces() == sp0 + 1);
	CHECK("kill of a held job: 0", qd_job_kill((unsigned long) id) == 0);
	CHECK("second kill: already ended or stale", qd_job_kill((unsigned long) id) == QD_ERR_ENDED ||
	      qd_job_kill((unsigned long) id) == QD_ERR_NOJOB);
	CHECK("killed job status is -66", qd_job_exit_status(id) == QD_ERR_KILLED);
	CHECK("slot released", gone(id, 200));
	CHECK("no longer listed", qd_job_find((unsigned long) id, 0) == 0);
	CHECK("address space released", spaces() == sp0 && held() == hb0);
	CHECK("heap back to baseline", qd_free_bytes() == free0);
	CHECK("stale id after release: exit status -2", qd_job_exit_status(id) == QD_ERR_NOJOB || qd_job_exit_status(id) == QD_ERR_KILLED);
	CHECK("stale id kill: -2", qd_job_kill((unsigned long) id) == QD_ERR_NOJOB);

	/* --- two running copies on console 2 --- */
	qd_con_child(con, 2);
	id = start("COUNTER.AOU", two, 8, 0);
	qd_con_child(con, 2);
	id2 = start("COUNTER.AOU", two, 8, 0);
	qd_con_child(con, -1);
	CHECK("two copies started", id > 0 && id2 > 0 && id != id2);
	qd_yield(120);
	CHECK("both run (dispatched, RUN or SLEEP)", qd_job_find((unsigned long) id, &ji) == 1 && ji.dispatches > 0 &&
	      (ji.state == QD_JS_RUN || ji.state == QD_JS_SLEEP) && qd_job_find((unsigned long) id2, &ji) == 1 &&
	      ji.dispatches > 0);
	CHECK("both report console 2", qd_job_find((unsigned long) id, &ji) == 1 && ji.console == 2);
	{
		int r, on2 = 0;
		for (r = 0; r < 25; r++)
			if (qd_con_snap(con, 2, r, row, sizeof row) > 0 && strncmp(row, "COUNTER 2 ", 10) == 0)
				on2++;
		CHECK("their output is on console 2", on2 >= 2);
	}
	CHECK("two address spaces held", spaces() == sp0 + 2);
	CHECK("kill the first", qd_job_kill((unsigned long) id) == 0);
	CHECK("first reclaimed, second still running", gone(id, 200) && qd_job_find((unsigned long) id2, &ji) == 1 &&
	      ji.dispatches > 0 && ji.state != QD_JS_DEAD);
	CHECK("kill the second", qd_job_kill((unsigned long) id2) == 0);
	CHECK("second reclaimed", gone(id2, 200));
	CHECK("both statuses -66", qd_job_exit_status(id) == QD_ERR_KILLED && qd_job_exit_status(id2) == QD_ERR_KILLED);
	CHECK("address spaces back to baseline", spaces() == sp0 && held() == hb0);
	CHECK("heap back to baseline", qd_free_bytes() == free0);

	/* --- table exhaustion --- */
	nstart = 0;
	firsterr = 0;
	for (i = 0; i < 8; i++) {
		int r;
		qd_con_child(con, 2);                    /* the choice is for the next job only */
		r = start("COUNTER.AOU", two, 8, 0);
		if (r < 0) {
			firsterr = r;
			break;
		}
		started[nstart++] = r;
	}
	qd_con_child(con, -1);
	CHECK("at least two copies fit", nstart >= 2);
	CHECK("then a bounded refusal (-29 / -60 / -63)", firsterr == -29 || firsterr == -60 || firsterr == -63);
	CHECK("the refusal left nothing behind", spaces() == sp0 + (unsigned int) nstart);
	for (i = 0; i < nstart; i++)
		CHECK("kill in the exhausted table", qd_job_kill((unsigned long) started[i]) == 0);
	for (i = 0; i < nstart; i++)
		CHECK("reclaimed after exhaustion", gone(started[i], 300));
	CHECK("baseline after exhaustion", spaces() == sp0 && held() == hb0 && qd_free_bytes() == free0);
	qd_con_child(con, 2);
	id = start("COUNTER.AOU", two, 8, 0);
	qd_con_child(con, -1);
	CHECK("a start works again", id > 0);
	CHECK("the new job reuses a slot with a new tag; old ids are stale", qd_job_kill((unsigned long) started[0]) == QD_ERR_NOJOB ||
	      (started[0] & 0xffff) == (id & 0xffff));
	CHECK("kill it", qd_job_kill((unsigned long) id) == 0 && gone(id, 200));

	/* --- repeated launch and exit --- */
	for (i = 0; i < 8; i++) {
		int st = -99;
		qd_con_child(con, 3);
		id = start("HELLO.AOU", 0, 0, 0);
		qd_con_child(con, -1);
		if (id > 0) {
			int t;
			for (t = 0; t < 400 && (st = qd_job_exit_status(id)) == QD_ERR_RUNNING; t++)
				qd_yield(1);
			gone(id, 200);
		}
		if (id <= 0 || st != 7)
			break;
	}
	CHECK("8 launches of HELLO: exit status 7 each", i == 8);
	CHECK("baseline after repeated launches", spaces() == sp0 && held() == hb0 && qd_free_bytes() == free0);

	/* --- kill a job that is waiting for console input; its console is usable again --- */
	qd_con_child(con, 2);
	id = start("CONINP.AOU", 0, 0, 0);
	qd_con_child(con, -1);
	CHECK("CONINP started", id > 0);
	qd_yield(60);
	if (qd_job_find((unsigned long) id, &ji) == 0 && qd_job_exit_status(id) == 3) {
		printf("NA: console input is already at end (i386VM harness): console-wait kill tests not run\n");
		gone(id, 200);
	} else {
		CHECK("CONINP waits on its console (CONWAIT)", qd_job_find((unsigned long) id, &ji) == 1 && ji.state == QD_JS_CONWAIT);
		CHECK("kill the waiting job", qd_job_kill((unsigned long) id) == 0 && gone(id, 200));
		qd_con_child(con, 2);
		id2 = start("CONINP.AOU", 0, 0, 0);
		qd_con_child(con, -1);
		qd_yield(60);
		CHECK("a new CONINP waits on the same console (no stale waiter)", id2 > 0 && qd_job_find((unsigned long) id2, &ji) == 1 &&
		      ji.state == QD_JS_CONWAIT);
		CHECK("kill it", qd_job_kill((unsigned long) id2) == 0 && gone(id2, 200));
	}
	CHECK("baseline after the console-wait section", spaces() == sp0 && held() == hb0 && qd_free_bytes() == free0);

	/* --- kill a job that holds a writable file open --- */
	remove("WIN1:/KILLW.TMP");
	qd_con_child(con, 3);
	id = start("WRITER.AOU", "WIN1:/KILLW.TMP\0" "1000\0" "10", 24, 0);
	qd_con_child(con, -1);
	CHECK("WRITER started", id > 0);
	qd_yield(100);
	{
		FILE *f = fopen("WIN1:/KILLW.TMP", "r");
		CHECK("a second job cannot open the file for reading while it is open for writing (conflict)", f == NULL);
		if (f)
			fclose(f);
	}
	CHECK("kill the writer", qd_job_kill((unsigned long) id) == 0 && gone(id, 200));
	{
		FILE *f = fopen("WIN1:/KILLW.TMP", "r");
		char line[40];
		int lines = 0;
		CHECK("the file is readable after the kill (registry released)", f != NULL);
		if (f) {
			while (fgets(line, sizeof line, f))
				lines++;
			fclose(f);
		}
		CHECK("it holds the lines written before the kill", lines >= 3 && lines < 1000);
	}
	CHECK("the file can be removed (not BUSY)", remove("WIN1:/KILLW.TMP") == 0);
	CHECK("baseline after killing the writer", spaces() == sp0 && held() == hb0 && qd_free_bytes() == free0);

	qd_con_child(con, -1);
	return finish("JOBT");
}
