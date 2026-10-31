/*
 * QDOS/386 R8 - streaming RUN and boot loads.  See imgload.h.
 */
#include "imgload.h"
#include "aout.h"
#include "channel.h"
#include "fat16.h"
#include "win1.h"
#ifdef QDOS_PAGING
#include "paging.h"
#endif
#ifdef QDOS_DYNPROC
#include "proc.h"
#endif

#ifdef QDOS_FS_WRITE
#define IMG_NAME_MAX 66u           /* R11M: a path of up to 64 characters, NUL included (relative to the caller's directory) */
#else
#define IMG_NAME_MAX 18u           /* name, NUL included: full 8.3 (R11E) */
#endif

/* Name plus arguments, as the kernel passes them to the loader.  Kernel static:
 * only the trap that fills it uses it, and traps do not nest. */
static unsigned char img_packed[APP_ARG_STR_MAX];

/* Byte source for the loader: a FAT16 file, read at any offset. */
struct fat_src {
	struct fat16_file *f;
};

static int fat_rd(void *ctx, unsigned int off, unsigned char *dst, unsigned int n)
{
	struct fat_src *s = (struct fat_src *) ctx;
	unsigned int got = 0;
	int rc;

	if (n == 0)
		return APP_OK;
	rc = fat16_seek(s->f, (unsigned long) off);
	if (rc != FAT_OK)
		return rc;
	while (n > 0) {
		rc = fat16_read(s->f, dst, n, &got);
		if (rc != FAT_OK)
			return rc;
		if (got == 0)                    /* chain shorter than the size */
			return FAT_ERR_SIZE;
		dst += got;
		n -= got;
	}
	return APP_OK;
}

/* Copy a user name (at most IMG_NAME_MAX-1 characters). */
static int copy_user_name(unsigned int addr, char *out)
{
	unsigned int i;
	for (i = 0; i < IMG_NAME_MAX; i++) {
		if (!chan_user_range_ok(addr + i, 1))
			return 0;
		out[i] = *(char *) (addr + i);
		if (out[i] == 0)
			return 1;
	}
	return 0;
}

#ifdef QDOS_ENV
#include "env.h"
#include "jobs.h"
#include "pathres.h"
#ifdef QDOS_R13B
#include "jobctl.h"
#endif
/* R12B: every load (RUN, START, QT_IMG_START, qd_spawn, BASIC EXEC) searches with path_resolve (pathres.h) using the
 * caller's PATH; candidates are opened like a name given directly (win1_open_img: relative to the caller's directory,
 * without regard to case). */
static char img_cand[PATH_CAND_MAX];
#ifdef QDOS_TITLES
static int img_cand_ok;
#endif

static int img_open_cb(void *ctx, const char *cand)
{
	return win1_open_img(cand, (struct fat16_file *) ctx);
}

static int img_resolve_open(const char *name, struct fat16_file *f)
{
	unsigned int vlen = 0;
	const char *pv = env_value_of(JOB_CTX_SLOT(), "PATH", &vlen);   /* R13B: the launch context's PATH */

#ifdef QDOS_TITLES
	{
		int rc = path_resolve(name, pv, pv ? vlen : 0u, img_open_cb, f, img_cand);
		img_cand_ok = rc == 0;
		return rc;
	}
#else
	return path_resolve(name, pv, pv ? vlen : 0u, img_open_cb, f, img_cand);
#endif
}

#ifdef QDOS_TITLES
/* R12C: the file the last successful load actually opened (e.g. "/GAMES/PHOON.AOU" for "PHOON"), or 0.  Read by
 * jobctl_note_launch right after the load, before any other load can run (traps do not nest). */
const char *img_resolved_name(void)
{
	return img_cand_ok ? img_cand : 0;
}
#endif
#endif /* QDOS_ENV */

#ifdef QDOS_TITLES
/* R12C: the compiled title (_prog_name) of the image the last successful load used, read from its symbol table after
 * the load (aout_prog_name_src); "" when it has none.  Read by jobctl_note_launch like img_resolved_name. */
static char img_symtitle[64];

const char *img_symbol_title(void)
{
	return img_symtitle;
}

static int img_title_after(int rc, struct fat_src *src, unsigned int size)
{
	if (rc >= 0)
		(void) aout_prog_name_src(fat_rd, src, size, img_symtitle, sizeof img_symtitle);
	return rc;
}
#define IMG_TITLE_CLEAR() (img_symtitle[0] = 0)
#define IMG_TITLE_AFTER(rc, src, size) img_title_after((rc), (src), (size))
#else
#define IMG_TITLE_CLEAR() ((void) 0)
#define IMG_TITLE_AFTER(rc, src, size) (rc)
#endif

#ifdef QDOS_ENV
#define IMG_OPEN(name, f) img_resolve_open((name), (f))
#else
#define IMG_OPEN(name, f) win1_open_img((name), (f))
#endif

/* Load the file called name into want_slot, with packed as its arguments. */
static int img_load_named(const char *name, int want_slot,
                          const unsigned char *packed, unsigned int plen)
{
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	IMG_TITLE_CLEAR();
	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
#ifdef QDOS_FS_WRITE
	rc = IMG_OPEN(name, &f);
#else
	rc = fat16_open_root(v, name, &f);
#endif
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_FILE_MAX)
		return APP_ERR_TOOBIG;
	src.f = &f;
#ifdef QDOS_DYNPROC
	/* R12A: every load is a new process; only the boot shell stays in its slot until it becomes a process. */
	if (want_slot == 0 && !proc_shell_is_process())
		return IMG_TITLE_AFTER(app_load_src(fat_rd, &src, (unsigned int) f.size, 0, packed, plen), &src, (unsigned int) f.size);
	return IMG_TITLE_AFTER(proc_load(fat_rd, &src, (unsigned int) f.size, want_slot, packed, plen, 0), &src, (unsigned int) f.size);
#else
	return IMG_TITLE_AFTER(app_load_src(fat_rd, &src, (unsigned int) f.size, want_slot, packed, plen), &src, (unsigned int) f.size);
#endif
}

int img_load_kernel(const char *name, unsigned int want_slot)
{
	unsigned int n = 0;

	while (name[n] != 0 && n < IMG_NAME_MAX - 1)
		n++;
	if (name[n] != 0)
		return FAT_ERR_NAME;
	/* argv = { name }: the name, its NUL. */
	{
		unsigned int i;
		for (i = 0; i <= n; i++)
			img_packed[i] = (unsigned char) name[i];
		return img_load_named(name, want_slot, img_packed, n + 1);
	}
}

int img_load_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i, total;

	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	if (args_len == 0) {
		for (i = 0; i <= n; i++)
			img_packed[i] = (unsigned char) name[i];
		return img_load_named(name, 1, img_packed, n + 1);
	}
	if (args_len > APP_ARG_STR_MAX || n + 1 + args_len > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	if (!chan_user_range_ok(args_addr, args_len))
		return CHAN_ERR_RANGE;
	if (*(unsigned char *) (args_addr + args_len - 1) != 0)
		return APP_ERR_ARGS;          /* arguments must end with NUL */
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < args_len; i++)
		img_packed[total + i] = *(unsigned char *) (args_addr + i);
	total += args_len;
	return img_load_named(name, 1, img_packed, total);
}

/* R11H: start the named WIN1_ root file at a profile (0..3), or any (-1), with
 * no arguments.  Returns the tagged job id, or a negative code. */
int img_start_request(unsigned int name_addr, int profile)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i;

	if (profile < -1 || profile >= APP_PROFILES)
		return APP_ERR_NOPROFILE;
	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	return img_load_named(name, profile, img_packed, n + 1);
}

#ifdef QDOS_PAGING
/* R11J: QT_PG_START.  The named WIN1_ root file, linked for slot 1, is loaded into a new
 * address space: private frames at the common window PG_VBASE.  Arguments as for RUN. */
int img_pg_start_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i, total;
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	int held = 0;

	if (args_len & PG_START_FLAGS) {
		if ((args_len & PG_START_FLAGS) != PG_START_HELD)
			return APP_ERR_ARGS;
		held = 1;
		args_len &= ~PG_START_FLAGS;
	}
	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	if (args_len > APP_ARG_STR_MAX || n + 1 + args_len > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	if (args_len != 0) {
		if (!chan_user_range_ok(args_addr, args_len))
			return CHAN_ERR_RANGE;
		if (*(unsigned char *) (args_addr + args_len - 1) != 0)
			return APP_ERR_ARGS;
	}
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < args_len; i++)
		img_packed[total + i] = *(unsigned char *) (args_addr + i);
	total += args_len;

	IMG_TITLE_CLEAR();
	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
#ifdef QDOS_FS_WRITE
	rc = IMG_OPEN(name, &f);
#else
	rc = fat16_open_root(v, name, &f);
#endif
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_FILE_MAX)
		return APP_ERR_TOOBIG;
	src.f = &f;
#ifdef QDOS_DYNPROC
	/* R12A veneer: the same request through the common process loader. */
	rc = IMG_TITLE_AFTER(proc_load(fat_rd, &src, (unsigned int) f.size, 1, img_packed, total, held ? PROC_LOAD_HELD : 0u),
	                     &src, (unsigned int) f.size);
	return rc == APP_ERR_NOTSLOT1 ? PG_ERR_ORIGIN : rc;
#else
	return app_load_paged(fat_rd, &src, (unsigned int) f.size, img_packed, total, held);
#endif
}
#endif

#if defined(QDOS_R13B) && defined(QDOS_DYNPROC) && defined(QDOS_ENV)
int img_launch_as(int ctx, const char *name, const unsigned char *args, unsigned int alen)
{
	unsigned int n = 0, i, total;
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	while (name[n] != 0 && n < IMG_NAME_MAX)
		n++;
	if (n == 0 || n >= IMG_NAME_MAX - 1)
		return FAT_ERR_NAME;
	if (alen > APP_ARG_STR_MAX || n + 1 + alen > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < alen; i++)
		img_packed[total + i] = args[i];
	total += alen;
	IMG_TITLE_CLEAR();
	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
	job_ctx_override = (ctx >= 0 && ctx < JOB_MAX && jobs[ctx].in_use) ? ctx : -1;
	rc = IMG_OPEN(name, &f);
	if (rc == FAT_OK) {
		if (f.size == 0)
			rc = APP_ERR_SIZE;
		else if (f.size > IMG_FILE_MAX)
			rc = APP_ERR_TOOBIG;
		else {
			src.f = &f;
			rc = IMG_TITLE_AFTER(proc_load(fat_rd, &src, (unsigned int) f.size, 1, img_packed, total, 0),
			                     &src, (unsigned int) f.size);
		}
	}
	job_ctx_override = -1;
	if (rc >= 0)
		jobctl_note_launch_k(rc, name, JC_F_PAGED);
	return rc;
}
#endif

#ifdef QDOS_DYNPROC
/* R12A: QT_PROC_SPAWN body after the request was checked (proc.c): the named file, any historical origin, with the
 * packed arguments; flags PROC_LOAD_HELD.  The caller may be any job, including a process. */
int img_spawn_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len, unsigned int flags)
{
	char name[IMG_NAME_MAX];
	unsigned int n = 0, i, total;
	struct fat16_vol *v;
	struct fat16_file f;
	struct fat_src src;
	int rc;

	if (!copy_user_name(name_addr, name))
		return FAT_ERR_NAME;
	while (name[n] != 0)
		n++;
	if (args_len > APP_ARG_STR_MAX || n + 1 + args_len > APP_ARG_STR_MAX)
		return APP_ERR_ARGS;
	if (args_len != 0) {
		if (!chan_user_range_ok(args_addr, args_len))
			return CHAN_ERR_RANGE;
		if (*(unsigned char *) (args_addr + args_len - 1) != 0)
			return APP_ERR_ARGS;
	}
	for (i = 0; i <= n; i++)
		img_packed[i] = (unsigned char) name[i];
	total = n + 1;
	for (i = 0; i < args_len; i++)
		img_packed[total + i] = *(unsigned char *) (args_addr + i);
	total += args_len;
	IMG_TITLE_CLEAR();
	v = win1_volume();
	if (v == 0)
		return CHAN_ERR_NOVOL;
	rc = IMG_OPEN(name, &f);
	if (rc != FAT_OK)
		return rc;
	if (f.size == 0)
		return APP_ERR_SIZE;
	if (f.size > IMG_FILE_MAX)
		return APP_ERR_TOOBIG;
	src.f = &f;
	return IMG_TITLE_AFTER(proc_load(fat_rd, &src, (unsigned int) f.size, -1, img_packed, total, flags), &src, (unsigned int) f.size);
}
#endif
