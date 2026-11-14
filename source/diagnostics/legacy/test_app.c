/*
 * QDOS/386 R5 - application phase.
 *
 * Loads the two independently linked a.out programs from the transfer area
 * (hello at slot 0, prep at slot 1), runs them as ordinary jobs and checks
 * their exit statuses, channel output and preemption.  Malformed images are
 * fed to the pure validator from heap copies, and the slots and heap are
 * compared before and after.  The transfer area itself is never modified.
 */
#include "platform.h"
#include "jobs.h"
#include "memory.h"
#include "channel.h"
#include "aout.h"
#include "test_channels.h"
#include "test_app.h"

extern char bss_end[];                  /* linker: end of kernel BSS */

#define APP_HELLO_STATUS 7              /* hello.c: exit status on success */
#define APP_PREP_STATUS  42             /* prep.c: exit status on success */
#define XFER_IDX_HELLO   0
#define XFER_IDX_PREP    1
#define XFER_IDX_HELLO2  2              /* second copy of hello: slot 0 busy test */

static volatile int hold_go;

static void hold_job(void)
{
	while (!hold_go)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

static void wait_job_done(int id)
{
	while (qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0) == JOB_ERR_RUNNING)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

static void wait_job_reclaimed(int id)
{
	if (id <= 0)
		return;
	while (jobs[id & 0xffff].in_use)
		qtrap(QT_SUSPEND, 1, 0, 0);
}

static unsigned int slot_sum(unsigned int base, unsigned int n)
{
	unsigned int s = 0, i;
	const unsigned char *p = (const unsigned char *) base;
	for (i = 0; i < n; i++)
		s = s * 31u + p[i];
	return s;
}

static unsigned int rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) |
	       ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

static void wr32(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) v;
	p[1] = (unsigned char) (v >> 8);
	p[2] = (unsigned char) (v >> 16);
	p[3] = (unsigned char) (v >> 24);
}

/* Fill the heap with blocks until nothing else fits, so a 4 KiB job stack
 * cannot be allocated.  Pointers go into blk[]; returns the count. */
#define PRESS_MAX 512
static void *blk[PRESS_MAX];
static int press_fill(void)
{
	static const unsigned int sizes[] = { 8192, 4096, 1024, 256, 64, 16 };
	int n = 0, s;
	for (s = 0; s < 6; s++) {
		for (;;) {
			void *p;
			if (n >= PRESS_MAX)
				return n;
			p = mem_alloc(sizes[s], MEM_OWNER_KERNEL);
			if (mem_is_error((unsigned int) p))
				break;
			blk[n++] = p;
		}
	}
	return n;
}

static void press_release(int n)
{
	int i;
	for (i = 0; i < n; i++)
		mem_free(blk[i], MEM_OWNER_KERNEL);
}

/* Malformed-image case: copy the image, change it, and expect one error. */
static unsigned char *mimg;
static unsigned int mimg_len;

static int mcase(unsigned int newlen)
{
	struct app_image info;
	return aout_check(mimg, newlen, &info);
}

static void check_expect(const char *name, int got, int want)
{
	r4_add(name, got == want);
}

static void malformed_phase(const unsigned char *src, unsigned int len)
{
	struct app_image info;
	unsigned char *buf;
	unsigned int text, data, syms, stroff, i;
	unsigned int snap0, snap1, mfree0, mfree1;
	struct mem_stats st;

	mem_stats(&st);
	mfree0 = st.free;
	snap0 = slot_sum(APP_SLOT0_BASE, APP_SLOT_SIZE) ^ slot_sum(APP_SLOT1_BASE, APP_SLOT_SIZE);

	if (len < XFER_DIR_BYTES)
		return;
	buf = (unsigned char *) mem_alloc(len + 64, MEM_OWNER_KERNEL);
	if (mem_is_error((unsigned int) buf)) {
		r4_add("malformed-image tests: scratch buffer allocated", 0);
		return;
	}
	for (i = 0; i < len + 64; i++)
		buf[i] = 0;
	for (i = 0; i < len; i++)
		buf[i] = src[i];
	mimg = buf;
	text = rd32(src + 4);
	data = rd32(src + 8);
	syms = rd32(src + 16);
	stroff = AOUT_HDR_BYTES + text + data + syms;

	r4_add("unmodified hello image passes the validator", aout_check(buf, len, &info) == APP_OK);

	for (i = 0; i < len; i++)
		buf[i] = src[i];
	check_expect("truncated by 16 bytes refused (size)", mcase(len - 16), APP_ERR_SIZE);
	check_expect("truncated to 20 bytes refused (size)", mcase(20), APP_ERR_SIZE);
	check_expect("zero-length image refused (size)", mcase(0), APP_ERR_SIZE);
	check_expect("trailing bytes after the string table refused (size)", mcase(len + 16), APP_ERR_SIZE);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf, 0x108);
	check_expect("wrong magic refused", mcase(len), APP_ERR_MAGIC);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 24, 8);
	check_expect("text relocation records refused", mcase(len), APP_ERR_RELOC);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 28, 8);
	check_expect("data relocation records refused", mcase(len), APP_ERR_RELOC);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 20, 0x200010u);
	check_expect("entry inside a slot but not at its base refused", mcase(len), APP_ERR_ENTRY);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 20, 0x220000u);
	check_expect("entry outside both slots refused", mcase(len), APP_ERR_ENTRY);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 20, APP_SLOT1_BASE);
	check_expect("entry at slot 1 base but linked for slot 0 refused", mcase(len), APP_ERR_ENTRY);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 4, text - 4);
	check_expect("text size not a multiple of 16 refused (layout)", mcase(len), APP_ERR_LAYOUT);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 12, 0x40000u);
	check_expect("BSS larger than the slot refused (oversized)", mcase(len), APP_ERR_LAYOUT);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 4, 0xFFFFFFF0u);
	check_expect("text size overflow refused (layout)", mcase(len), APP_ERR_LAYOUT);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 8, 0xFFFFFFF0u);
	check_expect("data size overflow refused (layout)", mcase(len), APP_ERR_LAYOUT);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 16, syms + 4);
	check_expect("symbol table not a multiple of 12 refused (format)", mcase(len), APP_ERR_FORMAT);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + 16, 0x00C00000u);
	check_expect("symbol table past the end of the file refused (size)", mcase(len), APP_ERR_SIZE);

	for (i = 0; i < len; i++) buf[i] = src[i];
	wr32(buf + stroff, len + 4);
	check_expect("string table size past the end refused (size)", mcase(len), APP_ERR_SIZE);

	for (i = 0; i < len; i++) buf[i] = src[i];
	{
		unsigned int n = syms / AOUT_SYM_BYTES, k;
		for (k = 0; k < n; k++)
			wr32(buf + AOUT_HDR_BYTES + text + data + k * AOUT_SYM_BYTES + 8, 0x200004u);
	}
	check_expect("_start value disagreeing with the entry refused (entry)", mcase(len), APP_ERR_ENTRY);

	/* Directory validation on a copy of the transfer directory. */
	{
		struct app_xfer_entry tab[XFER_MAX_IMAGES];
		unsigned char *d = buf;
		for (i = 0; i < XFER_DIR_BYTES; i++)
			d[i] = ((const unsigned char *) XFER_BASE)[i];
		wr32(d + 8, 5);
		r4_add("transfer directory with five images refused",
		       xfer_parse(d, XFER_DIR_BYTES, tab, XFER_MAX_IMAGES) == APP_ERR_XFER);
		for (i = 0; i < XFER_DIR_BYTES; i++)
			d[i] = ((const unsigned char *) XFER_BASE)[i];
		wr32(d + 16 + 0, 64);             /* payload inside the directory */
		r4_add("transfer entry overlapping the directory refused",
		       xfer_parse(d, XFER_DIR_BYTES, tab, XFER_MAX_IMAGES) == APP_ERR_XFER);
	}

	mem_free(buf, MEM_OWNER_KERNEL);
	snap1 = slot_sum(APP_SLOT0_BASE, APP_SLOT_SIZE) ^ slot_sum(APP_SLOT1_BASE, APP_SLOT_SIZE);
	mem_stats(&st);
	mfree1 = st.free;
	r4_add("malformed images left both slots and the heap unchanged",
	       snap0 == snap1 && mfree0 == mfree1);
}

static int text_data_match(unsigned int slot_base, const unsigned char *img)
{
	unsigned int text = rd32(img + 4), data = rd32(img + 8), i;
	const unsigned char *s = (const unsigned char *) slot_base;
	for (i = 0; i < text; i++)
		if (s[i] != img[AOUT_HDR_BYTES + i])
			return 0;
	for (i = 0; i < data; i++)
		if (s[text + i] != img[AOUT_HDR_BYTES + text + i])
			return 0;
	return 1;
}

void app_phase(void)
{
	const unsigned char *himg, *pimg;
	unsigned int hlen, plen, base_chk_faults;
	struct mem_stats st;
	unsigned int free0;
	unsigned long opened0, closed0;
	int n, id_h, id_p, id_h2, id_p2, id_h3, rc, n_hold;
	int hold_ids[JOB_MAX];
	unsigned int preempts, xfer_sum0;

	r4_add("transfer directory valid at XFER_BASE", app_xfer_init() > 0);
	r4_add("kernel BSS ends below the transfer area", (unsigned int) bss_end <= XFER_BASE);
	r4_add("transfer area carries the two application images",
	       app_image_count() >= 2);

	if (app_image_bytes(XFER_IDX_HELLO, &himg, &hlen) != APP_OK ||
	    app_image_bytes(XFER_IDX_PREP, &pimg, &plen) != APP_OK) {
		r4_add("application images present in the transfer area", 0);
		r4_phase_done = 1;
		return;
	}

	mem_stats(&st);
	free0 = st.free;
	xfer_sum0 = slot_sum(XFER_BASE, XFER_SIZE);
	malformed_phase(himg, hlen);

	/* Job-slot exhaustion: two holders take slots 1 and 2 (IDLE holds 3),
	 * so a free application slot still cannot get a job. */
	/* R10: JOB_MAX is larger than the R4 table, so the holders fill whatever
	 * slots are free at this point, not a fixed two. */
	hold_go = 0;
	n_hold = 0;
	while (n_hold < JOB_MAX &&
	       (hold_ids[n_hold] = qtrap(QT_CREATE, (unsigned int) hold_job, 2,
	                                 (unsigned int) "HOLD")) > 0)
		n_hold++;
	rc = app_load(XFER_IDX_HELLO);
	r4_add("no free job slot: load refused with APP_ERR_NOJOB", n_hold > 0 && rc == APP_ERR_NOJOB);
	hold_go = 1;
	for (n = 0; n < n_hold; n++)
		wait_job_reclaimed(hold_ids[n]);

	/* Heap exhaustion: the job stack cannot be allocated, so the load is refused
	 * and slot 1 stays untouched. */
	{
		unsigned int before = slot_sum(APP_SLOT1_BASE, APP_SLOT_SIZE);
		int held = press_fill();
		rc = app_load(XFER_IDX_PREP);
		press_release(held);
		r4_add("heap exhausted: load refused with APP_ERR_NOMEM", rc == APP_ERR_NOMEM);
		r4_add("refused load left slot 1 untouched and unowned",
		       before == slot_sum(APP_SLOT1_BASE, APP_SLOT_SIZE) && app_slot_owner(1) == 0);
	}

	/* Load both applications. */
	opened0 = chan_opened;
	closed0 = chan_closed_on_exit;
	base_chk_faults = (unsigned int) stack_faults;
	id_h = app_load(XFER_IDX_HELLO);
	id_p = app_load(XFER_IDX_PREP);
	r4_add("hello loaded into slot 0", id_h > 0 && app_slot_owner(0) == id_h);
	r4_add("prep loaded into slot 1", id_p > 0 && app_slot_owner(1) == id_p);
	r4_add("loaded text and data match the transfer bytes",
	       text_data_match(APP_SLOT0_BASE, himg) && text_data_match(APP_SLOT1_BASE, pimg));
	rc = app_load(XFER_IDX_HELLO2);
	r4_add("busy slot refused with APP_ERR_SLOTBUSY", rc == APP_ERR_SLOTBUSY);

	wait_job_done(id_h);
	wait_job_done(id_p);
	n = (int) qtrap(QT_JOB_EXIT, (unsigned int) id_h, 0, 0);
	preempts = jobs[id_p & 0xffff].preempts;
	r4_add("hello: channel output, close, exit status 7", n == APP_HELLO_STATUS);
	n = (int) qtrap(QT_JOB_EXIT, (unsigned int) id_p, 0, 0);
	r4_add("prep: initialised data, zero BSS, stack, loop, exit status 42", n == APP_PREP_STATUS);
	r4_add("prep preempted by the timer while CPU-bound (no yield)", preempts >= 1);
	r4_add("application channels closed by the applications",
	       chan_opened - opened0 == 2 && chan_closed_on_exit == closed0);
	r4_add("no stack canary faults during the application phase",
	       stack_faults == base_chk_faults);

	wait_job_reclaimed(id_h);
	wait_job_reclaimed(id_p);
	r4_add("application slots released after reclaim",
	       app_slot_owner(0) == 0 && app_slot_owner(1) == 0);
	r4_add("exit status readable after the job is reclaimed",
	       (int) qtrap(QT_JOB_EXIT, (unsigned int) id_h, 0, 0) == APP_HELLO_STATUS &&
	       (int) qtrap(QT_JOB_EXIT, (unsigned int) id_p, 0, 0) == APP_PREP_STATUS);

	/* Repeated load and exit reuses both slots. */
	id_h2 = app_load(XFER_IDX_HELLO);
	id_p2 = app_load(XFER_IDX_PREP);
	wait_job_done(id_h2);
	wait_job_done(id_p2);
	r4_add("repeated load into reused slots: statuses 7 and 42",
	       (int) qtrap(QT_JOB_EXIT, (unsigned int) id_h2, 0, 0) == APP_HELLO_STATUS &&
	       (int) qtrap(QT_JOB_EXIT, (unsigned int) id_p2, 0, 0) == APP_PREP_STATUS);
	wait_job_reclaimed(id_h2);
	wait_job_reclaimed(id_p2);

	/* One more single load to confirm slot 0 is usable after reclaim. */
	id_h3 = app_load(XFER_IDX_HELLO);
	wait_job_done(id_h3);
	r4_add("slot 0 reusable after reclaim", id_h3 > 0 &&
	       (int) qtrap(QT_JOB_EXIT, (unsigned int) id_h3, 0, 0) == APP_HELLO_STATUS);
	wait_job_reclaimed(id_h3);

	mem_stats(&st);
	r4_add("heap restored after the application phase", st.free == free0);
	r4_add("heap invariants hold after the application phase", mem_validate() == MEM_OK);

	r4_add("transfer area unchanged through the application phase",
	       slot_sum(XFER_BASE, XFER_SIZE) == xfer_sum0);

	/* Last step: releases the report (see apps_done in main.c). */
	r4_phase_done = 1;
}
