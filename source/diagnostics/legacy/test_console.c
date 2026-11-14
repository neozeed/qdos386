/*
 * QDOS/386 R7 - console input phase.
 *
 * Non-interactive and labelled as such: these checks inject bytes with
 * cin_put.  They test the decoder, the ring, the CON read and the blocking
 * wait.  Keys typed by a person are never injected here; see the R7 runtime
 * report for the interactive and scripted sessions.
 */
#include "test_console.h"
#include "cin.h"
#include "kbd.h"
#include "jobs.h"
#include "channel.h"
#include "platform.h"

volatile int r7c_phase_done = 0;

static int ck_dec = 0, ck_ring = 0, ck_read_empty = 0, ck_read_data = 0;
static int ck_wait_sleeps = 0, ck_wake = 0, ck_wait_immediate = 0;

static volatile unsigned char wflag = 0;     /* the waiter's flag (QT_CON_WAIT b) */
static volatile int w_state = 0;             /* 0 running, 1 got a byte, -1 failed */
static volatile unsigned char w_byte = 0;
static volatile unsigned long w_retries = 0;

/* Decode a scancode sequence; copy produced bytes to out.  Returns the count. */
static unsigned int dec_run(const unsigned char *seq, unsigned int n,
                            unsigned char *out)
{
	struct kbd_decoder d;
	unsigned int i, got = 0;
	unsigned char c;
	kbd_reset(&d);
	for (i = 0; i < n; i++)
		if (kbd_feed(&d, seq[i], &c))
			out[got++] = c;
	out[got] = 0;
	return got;
}

static int same(const unsigned char *a, unsigned int na, const char *want)
{
	unsigned int i;
	for (i = 0; i < na; i++)
		if (want[i] == 0 || a[i] != (unsigned char) want[i])
			return 0;
	return want[na] == 0;
}

static int decoder_checks(void)
{
	static const unsigned char s1[] = { 0x1e, 0x9e };
	static const unsigned char s2[] = { 0x2a, 0x1e, 0x9e, 0xaa, 0x1e, 0x9e };
	static const unsigned char s3[] = { 0x1c, 0x0e, 0x39, 0x0f };
	static const unsigned char s4[] = { 0xe0, 0x1c, 0xe0, 0x48, 0xe0, 0x9c };
	static const unsigned char s5[] = { 0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5, 0x1e, 0x9e };
	static const unsigned char s6[] = { 0x02, 0x2a, 0x02, 0xaa, 0x2a, 0x35, 0xaa, 0x2a, 0x2b, 0xaa };
	static const unsigned char s7[] = { 0x3b, 0xbb, 0x58, 0xd8 };
	static const unsigned char s8[] = { 0x2a, 0xe0, 0xaa, 0x1e };
	unsigned char out[16];
	unsigned int n;
	int ok = 1;

	n = dec_run(s1, sizeof s1, out);  ok = ok && same(out, n, "a");
	n = dec_run(s2, sizeof s2, out);  ok = ok && same(out, n, "Aa");
	n = dec_run(s3, sizeof s3, out);  ok = ok && same(out, n, "\r\x08 \t");
	n = dec_run(s4, sizeof s4, out);  ok = ok && same(out, n, "\r");
	n = dec_run(s5, sizeof s5, out);  ok = ok && same(out, n, "a");
	n = dec_run(s6, sizeof s6, out);  ok = ok && same(out, n, "1!?|");
	n = dec_run(s7, sizeof s7, out);  ok = ok && same(out, n, "");
	n = dec_run(s8, sizeof s8, out);  ok = ok && same(out, n, "A");
	return ok;
}

static int ring_checks(void)
{
	unsigned long ovf0 = cin_overflows;
	unsigned int i;
	unsigned char c;
	int ok = 1;

	while (cin_get(&c))
		;
	for (i = 0; i < CIN_SIZE - 1; i++)
		if (cin_put((unsigned char) (i + 1)) != 1)
			ok = 0;
	if (cin_put(0x55) != 0)              /* full: dropped */
		ok = 0;
	if (cin_overflows != ovf0 + 1)
		ok = 0;
	for (i = 0; i < CIN_SIZE - 1; i++) {  /* FIFO order */
		if (!cin_get(&c) || c != (unsigned char) (i + 1))
			ok = 0;
	}
	if (cin_get(&c))
		ok = 0;
	for (i = 0; i < 300; i++) {          /* wrap-around */
		cin_put((unsigned char) i);
		if (!cin_get(&c) || c != (unsigned char) i)
			ok = 0;
	}
	return ok;
}

/* Waiter job: reads CON; sleeps in QT_CON_WAIT while nothing is waiting. */
static void con_waiter_job(void)
{
	unsigned char *buf = (unsigned char *) qtrap(QT_MEM_ALLOC, 8, 0, 0);
	int ch = (int) qtrap(QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	int r;

	if (buf == 0 || ch < 0) {
		w_state = -1;
		return;
	}
	for (;;) {
		r = (int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 1);
		if (r == 1) {
			w_byte = buf[0];
			w_state = 1;
			break;
		}
		if (r == CHAN_ERR_AGAIN) {
			w_retries++;
			qtrap(QT_CON_WAIT, 0, (unsigned int) &wflag, 0);
			continue;
		}
		w_state = -1;
		break;
	}
	qtrap(QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	qtrap(QT_MEM_FREE, (unsigned int) buf, 0, 0);
}

static int read_checks(void)
{
	unsigned char *buf = (unsigned char *) qtrap(QT_MEM_ALLOC, 8, 0, 0);
	int ch = (int) qtrap(QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	unsigned char c;
	int r;

	if (buf == 0 || ch < 0)
		return 0;
	r = (int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 8);
	ck_read_empty = (r == CHAN_ERR_AGAIN);

	cin_put('x');
	cin_put('y');
	r = (int) qtrap(QT_CH_READ, (unsigned int) ch, (unsigned int) buf, 8);
	ck_read_data = (r == 2) && (buf[0] == 'x') && (buf[1] == 'y');

	/* A pending byte: QT_CON_WAIT returns at once and does not sleep. */
	cin_put('R');
	wflag = 0;
	ck_wait_immediate = (qtrap(QT_CON_WAIT, 0, (unsigned int) &wflag, 0) == 0) &&
	                    (wflag == 0) && (cin_count() == 1);
	while (cin_get(&c))
		;
	qtrap(QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	qtrap(QT_MEM_FREE, (unsigned int) buf, 0, 0);
	return 1;
}

static void wake_checks(void)
{
	unsigned long t1;
	unsigned int id;

	w_state = 0;
	w_retries = 0;
	wflag = 0;
	id = qtrap(QT_CREATE, (unsigned int) con_waiter_job, 3, (unsigned int) "CONW");
	/* Let the waiter run and go to sleep.  The bound is on ticks, not on the
	 * waiter's first dispatch: how soon it runs depends on what ran before it. */
	t1 = sched_ticks;
	while (w_retries == 0 && w_state == 0 && sched_ticks - t1 < 40)
		qtrap(QT_SUSPEND, 1, 0, 0);
	ck_wait_sleeps = (id != 0xffffffffu) && (w_state == 0) && (w_retries >= 1);

	cin_put('Q');                        /* the arrival wakes the waiter */
	t1 = sched_ticks;
	while (w_state == 0 && sched_ticks - t1 < 300)
		qtrap(QT_SUSPEND, 1, 0, 0);
	ck_wake = (w_state == 1) && (w_byte == 'Q') && (wflag == 0xff) &&
	          (sched_ticks - t1 <= 20);
}

void console_phase(void)
{
	int all;

	/* Start from an empty ring; earlier keyboard bytes are discarded. */
	{
		unsigned char c;
		while (cin_get(&c))
			;
	}

	ck_dec = decoder_checks();
	ck_ring = ring_checks();
	read_checks();
	wake_checks();

	con_puts("CHECK keyboard decoder: ");       con_puts(ck_dec ? "ok\n" : "FAIL\n");
	con_puts("CHECK input ring: ");             con_puts(ck_ring ? "ok\n" : "FAIL\n");
	con_puts("CHECK CON read empty: ");         con_puts(ck_read_empty ? "ok\n" : "FAIL\n");
	con_puts("CHECK CON read data: ");          con_puts(ck_read_data ? "ok\n" : "FAIL\n");
	con_puts("CHECK CON wait returns at once: "); con_puts(ck_wait_immediate ? "ok\n" : "FAIL\n");
	con_puts("CHECK CON waiter sleeps: ");      con_puts(ck_wait_sleeps ? "ok\n" : "FAIL\n");
	con_puts("CHECK CON wake on input: ");      con_puts(ck_wake ? "ok\n" : "FAIL\n");

	all = ck_dec && ck_ring && ck_read_empty && ck_read_data &&
	      ck_wait_immediate && ck_wait_sleeps && ck_wake;
	if (all)
		con_puts("\nQDOS/386 R7 CONSOLE PASS\n");
	r7c_phase_done = 1;
}
