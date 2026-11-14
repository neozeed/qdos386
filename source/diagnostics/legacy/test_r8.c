/*
 * QDOS/386 R8 - kernel-side negative checks for the streaming loader.
 *
 * Each case feeds app_load_src a reader that fails, runs short, or describes
 * a bad image or argument block, and checks: the error code, that the slot is
 * still free and unowned, that no job or heap block was left behind (the heap
 * free count is restored), and that a good load still works afterwards.
 *
 * The source image is the HELLO application from the transfer area, which is
 * linked for slot 0.
 */
#include "aout.h"
#include "jobs.h"
#include "memory.h"
#include "test_channels.h"
#include "test_r8.h"

#define R8_IDX_HELLO   0
#define R8_READ_FAULT  (-77)
#define R8_IDX_HELLO_STATUS 7

struct r8_src {
	const unsigned char *img;
	unsigned int len;
	unsigned int calls;
	unsigned int fail_call;   /* 0 = never; else fail on this call (1-based) */
};

/* Reader over a memory image.  Refuses a read past the image, like a
 * truncated file would. */
static int r8_rd(void *ctx, unsigned int off, unsigned char *dst, unsigned int n)
{
	struct r8_src *s = (struct r8_src *) ctx;
	unsigned int i;

	s->calls++;
	if (s->fail_call != 0 && s->calls == s->fail_call)
		return R8_READ_FAULT;
	if (off > s->len || n > s->len - off)
		return APP_ERR_SIZE;
	for (i = 0; i < n; i++)
		dst[i] = s->img[off + i];
	return APP_OK;
}

static unsigned int r8_free(void)
{
	struct mem_stats st;
	mem_stats(&st);
	return st.free;
}

static void wr_le32(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
	p[2] = (unsigned char) ((v >> 16) & 0xff);
	p[3] = (unsigned char) ((v >> 24) & 0xff);
}

/* a.out header: a_magic at 0, a_text at 4, a_data at 8, a_bss at 12. */
#define R8_HDR_TEXT 4
#define R8_HDR_DATA 8
#define R8_HDR_BSS  12

static unsigned int r8_rd32(const unsigned char *p)
{
	return (unsigned int) p[0] | ((unsigned int) p[1] << 8) |
	       ((unsigned int) p[2] << 16) | ((unsigned int) p[3] << 24);
}

/* A copy of the source image with one header field changed. */
static unsigned char r8_copy[1024];

#ifdef QDOS_LAYOUT_R9
/* R9: slot 1 takes a 1 MiB image.  PREP is linked for slot 1 and does not use
 * its BSS, so its BSS can be raised to the exact limit: accepted, it runs and
 * exits 42; one byte more is refused with the slot left free and the heap
 * restored.  Requires the PREP image (transfer index 1). */
#define R9_IDX_PREP    1
#define R9_PREP_STATUS 42
static unsigned char r9_copy[16384];

static void r9_loader_phase(void)
{
	const unsigned char *img;
	unsigned int len, lim, tdb, i, free0, id;
	struct r8_src s;
	int rc, n;

	if (app_image_bytes(R9_IDX_PREP, &img, &len) != APP_OK || len > sizeof r9_copy) {
		r4_add("R9: PREP image available for slot 1 checks", 0);
		return;
	}
	lim = APP_IMAGE_LIMIT_OF(1);
	tdb = r8_rd32(img + R8_HDR_TEXT) + r8_rd32(img + R8_HDR_DATA);
	free0 = r8_free();

	for (i = 0; i < len; i++)
		r9_copy[i] = img[i];
	wr_le32(r9_copy + R8_HDR_BSS, lim - tdb + 1);
	s.img = r9_copy; s.len = len; s.calls = 0; s.fail_call = 0;
	rc = app_load_src(r8_rd, &s, len, 1, 0, 0);
	r4_add("R9 slot 1: one byte past the 1 MiB limit: APP_ERR_LAYOUT", rc == APP_ERR_LAYOUT);
	r4_add("R9 slot 1: layout refusal leaves slot 1 unowned", app_slot_owner(1) == 0);
	r4_add("R9 slot 1: layout refusal restores the heap", r8_free() == free0);

	wr_le32(r9_copy + R8_HDR_BSS, lim - tdb);
	s.calls = 0;
	id = (unsigned int) app_load_src(r8_rd, &s, len, 1, 0, 0);
	r4_add("R9 slot 1: image of exactly the limit is accepted", (int) id > 0 && app_slot_owner(1) == (int) id);
	if ((int) id > 0) {
		while (qtrap(QT_JOB_EXIT, id, 0, 0) == JOB_ERR_RUNNING)
			qtrap(QT_SUSPEND, 1, 0, 0);
		n = (int) qtrap(QT_JOB_EXIT, id, 0, 0);
		r4_add("R9 slot 1: image of exactly the limit runs: exit status 42", n == R9_PREP_STATUS);
		while (jobs[id & 0xffff].in_use)
			qtrap(QT_SUSPEND, 1, 0, 0);
	}
	r4_add("R9 slot 1: released after the exact-limit run", app_slot_owner(1) == 0);
	r4_add("R9 slot 1: heap restored after the exact-limit run", r8_free() == free0);
}
#endif

void r8_loader_phase(void)
{
	const unsigned char *img;
	unsigned int len, free0, free1, calls_check, i;
	struct r8_src s;
	struct app_image info;
	unsigned char packed[64];
	int rc, id, n;

	if (app_image_bytes(R8_IDX_HELLO, &img, &len) != APP_OK || len > sizeof r8_copy) {
		r4_add("R8: hello image available for negative checks", 0);
		return;
	}
	free0 = r8_free();
#ifdef QDOS_LAYOUT_R9
	r9_loader_phase();
#endif

	/* 1. Read failure during validation: refused, slot free, nothing left. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 1;
	rc = app_load_src(r8_rd, &s, len, 0, 0, 0);
	r4_add("R8 read failure in validation: its error is returned", rc == R8_READ_FAULT);
	r4_add("R8 read failure in validation: slot 0 free and unowned",
	       app_slot_owner(0) == 0);
	r4_add("R8 read failure in validation: heap restored", r8_free() == free0);

	/* 2. Read failure in the copy: count the validation reads first, then fail
	 * on the first copy read.  The job is cancelled and the slot released. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 0;
	rc = aout_check_src(r8_rd, &s, len, &info);
	calls_check = s.calls;
	r4_add("R8 validation of the hello image alone passes", rc == APP_OK);
	s.calls = 0; s.fail_call = calls_check + 1;
	rc = app_load_src(r8_rd, &s, len, 0, 0, 0);
	r4_add("R8 read failure in the copy: its error is returned", rc == R8_READ_FAULT);
	r4_add("R8 read failure in the copy: slot 0 released", app_slot_owner(0) == 0);
	r4_add("R8 read failure in the copy: heap restored", r8_free() == free0);

	/* 3. Short image: the length given is larger than the bytes present. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 0;
	rc = app_load_src(r8_rd, &s, len + 1, 0, 0, 0);
	r4_add("R8 length one byte past the image: APP_ERR_SIZE", rc == APP_ERR_SIZE);
	s.calls = 0;
	rc = app_load_src(r8_rd, &s, len - 1, 0, 0, 0);
	r4_add("R8 length one byte short: APP_ERR_SIZE", rc == APP_ERR_SIZE);
	r4_add("R8 size refusals leave slot 0 unowned", app_slot_owner(0) == 0);

	/* 4. Wrong slot: a slot 0 image asked for slot 1. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 0;
	rc = app_load_src(r8_rd, &s, len, 1, 0, 0);
	r4_add("R8 slot 0 image for slot 1: APP_ERR_NOTSLOT1", rc == APP_ERR_NOTSLOT1);
	r4_add("R8 wrong slot leaves slot 1 unowned", app_slot_owner(1) == 0);

	/* 5. Layout: text+data+BSS past the slot-0 limit (224 KiB in every map). */
	for (i = 0; i < len; i++)
		r8_copy[i] = img[i];
	wr_le32(r8_copy + R8_HDR_BSS, 0x00040000u);
	s.img = r8_copy; s.len = len; s.calls = 0; s.fail_call = 0;
	rc = app_load_src(r8_rd, &s, len, 0, 0, 0);
	r4_add("R8 image larger than the slot 0 limit of text+data+BSS: APP_ERR_LAYOUT",
	       rc == APP_ERR_LAYOUT);
	r4_add("R8 layout refusal leaves slot 0 unowned", app_slot_owner(0) == 0);

	/* 5b. Exact boundary: text+data+BSS equal to the limit is accepted, and one
	 * byte more is refused.  The code is the hello program, which does not use
	 * its BSS, so the accepted image runs and exits with its status. */
	{
		unsigned int tdb = r8_rd32(img + R8_HDR_TEXT) + r8_rd32(img + R8_HDR_DATA);
		unsigned int exact = (unsigned int) APP_IMAGE_LIMIT_OF(0) - tdb;
		int idb;

		for (i = 0; i < len; i++)
			r8_copy[i] = img[i];
		wr_le32(r8_copy + R8_HDR_BSS, exact + 1);
		s.img = r8_copy; s.len = len; s.calls = 0; s.fail_call = 0;
		rc = app_load_src(r8_rd, &s, len, 0, 0, 0);
		r4_add("R8 text+data+BSS one byte past the limit: APP_ERR_LAYOUT",
		       rc == APP_ERR_LAYOUT);

		wr_le32(r8_copy + R8_HDR_BSS, exact);
		s.calls = 0;
		idb = app_load_src(r8_rd, &s, len, 0, 0, 0);
		r4_add("R8 text+data+BSS exactly at the limit: accepted", idb > 0 && app_slot_owner(0) == idb);
		if (idb > 0) {
			while (qtrap(QT_JOB_EXIT, (unsigned int) idb, 0, 0) == JOB_ERR_RUNNING)
				qtrap(QT_SUSPEND, 1, 0, 0);
			n = (int) qtrap(QT_JOB_EXIT, (unsigned int) idb, 0, 0);
			r4_add("R8 image at the limit runs: exit status 7", n == R8_IDX_HELLO_STATUS);
			while (jobs[idb & 0xffff].in_use)
				qtrap(QT_SUSPEND, 1, 0, 0);
		}
		r4_add("R8 at-limit image: slot 0 released after reclaim", app_slot_owner(0) == 0);
	}

	/* 6. Argument blocks: unterminated, over-long, and too many strings. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 0;
	packed[0] = 'a'; packed[1] = 'b'; packed[2] = 'c';
	rc = app_load_src(r8_rd, &s, len, 0, packed, 3);
	r4_add("R8 argument block not NUL-terminated: APP_ERR_ARGS", rc == APP_ERR_ARGS);

	for (i = 0; i < 64; i++)
		packed[i] = 'x';
	packed[63] = 0;
	s.calls = 0;
	rc = app_load_src(r8_rd, &s, len, 0, packed, APP_ARG_STR_MAX + 1);
	r4_add("R8 argument block past 432 bytes: APP_ERR_ARGS", rc == APP_ERR_ARGS);

	for (i = 0; i < 17 * 2; i += 2) {
		packed[i] = 'a';
		packed[i + 1] = 0;
	}
	s.calls = 0;
	rc = app_load_src(r8_rd, &s, len, 0, packed, 17 * 2);
	r4_add("R8 seventeen argument strings: APP_ERR_ARGS", rc == APP_ERR_ARGS);
	r4_add("R8 argument refusals leave slot 0 unowned", app_slot_owner(0) == 0);
	r4_add("R8 argument refusals: heap restored", r8_free() == free0);

	/* 7. The loader still works after all refusals: one good load and reclaim. */
	s.img = img; s.len = len; s.calls = 0; s.fail_call = 0;
	id = app_load_src(r8_rd, &s, len, 0, 0, 0);
	r4_add("R8 good load after the refusals", id > 0 && app_slot_owner(0) == id);
	if (id > 0) {
		while (qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0) == JOB_ERR_RUNNING)
			qtrap(QT_SUSPEND, 1, 0, 0);
		n = (int) qtrap(QT_JOB_EXIT, (unsigned int) id, 0, 0);
		r4_add("R8 good load after the refusals: exit status 7", n == R8_IDX_HELLO_STATUS);
		while (jobs[id & 0xffff].in_use)
			qtrap(QT_SUSPEND, 1, 0, 0);
	}
	free1 = r8_free();
	r4_add("R8 negative phase: heap restored", free1 == free0);
	r4_add("R8 negative phase: both slots unowned", app_slot_owner(0) == 0 && app_slot_owner(1) == 0);
}
