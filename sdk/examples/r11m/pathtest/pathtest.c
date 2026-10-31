/* PATHT: R11M guest test of the native path layer, through libc and libqdos (C89).
 * Creates /PT, works in it, and removes everything it made.  Legacy WIN1_ flat names are checked
 * against the same files.  Banner "QDOS/386 R11M PATHT PASS" only when every check passed. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "qdos386.h"
#include "../../r11b/check.h"

/* The libc POSIX layer is what programs use; its prototypes are declared here (no header ships them). */
int mkdir(const char *path, int mode);
int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int qd_fsname(const char *in, char *out, size_t outsz);

static char buf[4096];

/* libqdos calls take channel names; map a program path the way fopen does */
static int mstat(const char *p, struct qd_stat *st)
{
	char full[FILENAME_MAX];
	if (qd_fsname(p, full, sizeof full) != 0)
		return -1;
	return qd_stat(full, st);
}

static int mdir(const char *p, int with_dirs)
{
	char full[FILENAME_MAX];
	if (qd_fsname(p, full, sizeof full) != 0)
		return -1;
	return qd_dir_open_path(full, with_dirs);
}

static int cwdis(const char *want)
{
	char c[80];
	return getcwd(c, sizeof c) != NULL && strcmp(c, want) == 0;
}

static int fails_(int r, int err, int want, const char *what)
{
	if (r == -1 && err == want)
		return 1;
	printf("  (%s: result %d errno %d, wanted -1 / %d)\n", what, r, err, want);
	return 0;
}

/* a call that must fail with a given errno; the value is captured before anything else can change it */
static int fails_r;
#define FAILS(call, e) (fails_r = (call), fails_(fails_r, errno, (e), #call))

static int put(const char *name, int n, int seed)
{
	FILE *f = fopen(name, "wb");
	int i, w;
	if (f == NULL)
		return -1;
	for (i = 0; i < n; i++)
		buf[i % 4096] = (char) ('A' + (i * 7 + seed) % 26);
	w = 0;
	for (i = 0; i < n; i += 100) {
		int c = n - i < 100 ? n - i : 100;
		int k;
		char tmp[100];
		for (k = 0; k < c; k++)
			tmp[k] = (char) ('A' + ((i + k) * 7 + seed) % 26);
		if (fwrite(tmp, 1, (size_t) c, f) != (size_t) c)
			break;
		w += c;
	}
	if (fclose(f) != 0)
		return -2;
	return w == n ? 0 : -3;
}

static int same(const char *name, int n, int seed)
{
	FILE *f = fopen(name, "rb");
	int i, ok = 1;
	if (f == NULL)
		return 0;
	for (i = 0; i < n; i++) {
		int c = fgetc(f);
		if (c != 'A' + (i * 7 + seed) % 26) {
			ok = 0;
			break;
		}
	}
	if (ok && fgetc(f) != EOF)
		ok = 0;
	fclose(f);
	return ok;
}

static int count_dir(const char *path, int with_dirs)
{
	int ch = mdir(path, with_dirs);
	struct qd_dirent e;
	int n = 0, r;
	if (ch < 0)
		return ch;
	while ((r = qd_dir_next(ch, &e)) > 0)
		n++;
	qd_close(ch);
	return r < 0 ? r : n;
}

static int exists(const char *p)
{
	struct qd_stat st;
	return mstat(p, &st) == 0;
}

int main(void)
{
	char cwd[80];
	struct qd_stat st;
	FILE *f;
	int r, i;
	char longp[200];

	r = chdir("/");
	CHECK("chdir-root", r == 0);
	CHECK("getcwd-root", cwdis("/"));
	CHECK("pt-absent-at-start", !exists("/PT"));

	/* --- directories ------------------------------------------------------------------------- */
	CHECK("mkdir-PT", mkdir("/PT", 0) == 0);
	CHECK("mkdir-PT-twice", FAILS(mkdir("/PT", 0), EEXIST));
	CHECK("mkdir-nested", mkdir("/PT/A", 0) == 0 && mkdir("PT/A/B", 0) == 0);
	CHECK("mkdir-missing-parent", FAILS(mkdir("/PT/NOPE/X", 0), ENOENT));
	CHECK("mkdir-lower-ci", mkdir("/pt/a/c", 0) == 0);
	CHECK("stat-dir", mstat("/PT/A", &st) == 0 && st.is_dir && st.size == 0);
	CHECK("stat-dotdot", mstat("/PT/A/../A/./B", &st) == 0 && st.is_dir);
	CHECK("stat-root", mstat("/", &st) == 0 && st.is_dir);
	CHECK("stat-root-dotdot", mstat("/PT/../..", &st) == 0 && st.is_dir);

	/* --- files by absolute path -------------------------------------------------------------- */
	CHECK("create-abs", put("/PT/A/H.C", 3000, 1) == 0);
	CHECK("stat-file", mstat("/PT/A/H.C", &st) == 0 && !st.is_dir && st.size == 3000);
	CHECK("read-abs", same("/PT/A/H.C", 3000, 1));
	CHECK("read-ci", same("/pt/a/h.c", 3000, 1));
	CHECK("read-dots", same("/PT/A/B/../H.C", 3000, 1) && same("/PT/./A/./H.C", 3000, 1));

	/* --- relative paths and the current directory -------------------------------------------- */
	CHECK("chdir-PT", chdir("/PT") == 0 && cwdis("/PT"));
	CHECK("read-rel", same("A/H.C", 3000, 1) && same("./A/H.C", 3000, 1) && same("A/B/../H.C", 3000, 1));
	CHECK("chdir-rel", chdir("A/B") == 0 && cwdis("/PT/A/B"));
	CHECK("read-up", same("../H.C", 3000, 1) && same("../../A/H.C", 3000, 1));
	CHECK("create-rel", put("N.TXT", 700, 2) == 0 && same("/PT/A/B/N.TXT", 700, 2));
	CHECK("chdir-dotdot", chdir("..") == 0 && cwdis("/PT/A"));
	CHECK("chdir-file-fails", FAILS(chdir("H.C"), ENOTDIR));
	CHECK("chdir-missing", FAILS(chdir("NOPE"), ENOENT) && cwdis("/PT/A"));

	/* --- the volume root is the ceiling ------------------------------------------------------ */
	CHECK("chdir-above-root", chdir("/..") == 0 && cwdis("/"));
	CHECK("many-dotdot", chdir("/PT/A/../../../../..") == 0 && cwdis("/"));
	CHECK("stat-above-root", mstat("../../PT/A/H.C", &st) == 0 && st.size == 3000);

	/* --- rename, move, remove ---------------------------------------------------------------- */
	CHECK("rename-same-dir", rename("/PT/A/H.C", "/PT/A/H2.C") == 0 && !exists("/PT/A/H.C") && same("/PT/A/H2.C", 3000, 1));
	CHECK("move-file-across", rename("/PT/A/H2.C", "/PT/A/B/H3.C") == 0 && same("/PT/A/B/H3.C", 3000, 1));
	CHECK("move-file-up-rel", chdir("/PT/A") == 0 && rename("B/H3.C", "../M.C") == 0 && same("/PT/M.C", 3000, 1));
	CHECK("rename-onto-existing", put("/PT/E.C", 10, 3) == 0 && rename("/PT/M.C", "/PT/E.C") == -1 && errno == EEXIST && same("/PT/M.C", 3000, 1));
	CHECK("rename-missing", rename("/PT/NONE.C", "/PT/X.C") == -1 && errno == ENOENT);
	CHECK("rename-dir-same-parent", rename("/PT/A/C", "/PT/A/CC") == 0 && exists("/PT/A/CC") && !exists("/PT/A/C"));
	CHECK("rename-dir-to-other-parent", rename("/PT/A/CC", "/PT/CC") == -1 && errno == EXDEV);
	CHECK("remove-file", remove("/PT/E.C") == 0 && !exists("/PT/E.C"));
	CHECK("remove-missing", remove("/PT/E.C") == -1 && errno == ENOENT);
	CHECK("remove-dir-by-remove", remove("/PT/A/CC") != 0 && exists("/PT/A/CC"));

	/* --- directory rules --------------------------------------------------------------------- */
	CHECK("fopen-dir-fails", fopen("/PT/A", "rb") == NULL);
	CHECK("open-file-as-dir", mdir("/PT/M.C", 1) < 0);
	CHECK("rmdir-nonempty", chdir("/") == 0 && FAILS(rmdir("/PT/A"), ENOTEMPTY));
	CHECK("rmdir-cwd-busy", chdir("/PT/A/CC") == 0 && FAILS(rmdir("/PT/A/CC"), EBUSY));
	CHECK("rmdir-dot", rmdir(".") != 0 && rmdir("..") != 0 && rmdir("/") != 0);
	CHECK("rmdir-file", chdir("/") == 0 && FAILS(rmdir("/PT/M.C"), ENOTDIR));

	/* --- listing ----------------------------------------------------------------------------- */
	CHECK("list-files-only", count_dir("/PT", 0) == 1);
	CHECK("list-with-dirs", count_dir("/PT", 1) == 2);	/* A and M.C; "." and ".." are not listed */
	CHECK("list-nested", count_dir("/PT/A/B", 1) == 1);

	/* --- legacy WIN1_ flat names ------------------------------------------------------------- */
	CHECK("legacy-create", put("WIN1_PTL.TXT", 500, 4) == 0 && exists("/PTL.TXT") && same("WIN1:/PTL.TXT", 500, 4));
	CHECK("legacy-ignores-cwd", chdir("/PT/A") == 0 && same("WIN1_PTL.TXT", 500, 4) && !exists("PTL.TXT"));
	CHECK("legacy-underscore-name", put("WIN1_PT_X.TXT", 40, 5) == 0 && exists("/PT_X.TXT") && !exists("/PT/X.TXT"));
	CHECK("legacy-8char-nodot", put("WIN1_ZORK1_Z3", 90, 6) == 0 && exists("/ZORK1_Z3") && same("WIN1:/ZORK1_Z3", 90, 6));
	CHECK("legacy-underscore-is-not-a-dir", fopen("WIN1_PT_M.C", "rb") == NULL);
	CHECK("legacy-prefix-slash-is-root-path", same("WIN1_PT/M.C", 3000, 1));
	CHECK("legacy-lowercase-folded", same("WIN1_ptl.txt", 500, 4));
	CHECK("legacy-remove", remove("WIN1_PTL.TXT") == 0 && remove("WIN1_PT_X.TXT") == 0 && remove("WIN1_ZORK1_Z3") == 0);
	CHECK("legacy-gone", !exists("/PTL.TXT") && !exists("/PT_X.TXT") && !exists("/ZORK1_Z3"));
	CHECK("native-prefix-abs", same("WIN1:/PT/M.C", 3000, 1));
	CHECK("native-prefix-rel", chdir("/PT") == 0 && same("WIN1:M.C", 3000, 1));

	/* --- invalid and oversized paths --------------------------------------------------------- */
	memset(longp, 'A', sizeof longp - 1);
	longp[sizeof longp - 1] = 0;
	CHECK("path-too-long", mstat(longp, &st) < 0 && fopen(longp, "rb") == NULL);
	strcpy(longp, "/");
	for (i = 0; i < 20; i++)
		strcat(longp, "D/");
	CHECK("path-too-deep", mstat(longp, &st) < 0);
	/* "A*B" is never a valid name.  "A B" and "TOOLONGNAME9" are refused by a kernel without VFAT; a VFAT kernel
	 * (R11M Stage 4) creates them as long names, so both must behave alike and be removable again. */
	{
		int r1 = mkdir("/PT/A B", 0), r2 = mkdir("/PT/TOOLONGNAME9", 0);

		CHECK("path-bad-chars", mkdir("/PT/A*B", 0) < 0 && ((r1 < 0 && r2 < 0) ||
		      (r1 == 0 && r2 == 0 && rmdir("/PT/A B") == 0 && rmdir("/PT/TOOLONGNAME9") == 0)));
	}
	CHECK("path-empty", mstat("", &st) < 0 && mkdir("", 0) < 0);
	CHECK("path-double-slash", same("/PT//M.C", 3000, 1) || !exists("/PT//M.C"));

	/* --- file I/O in a subdirectory: append, truncate, size ---------------------------------- */
	f = fopen("/PT/A/APP.TXT", "w");
	CHECK("open-w", f != NULL);
	if (f != NULL) {
		fputs("one\n", f);
		fclose(f);
	}
	f = fopen("/PT/A/APP.TXT", "a");
	if (f != NULL) {
		fputs("two\n", f);
		fclose(f);
	}
	CHECK("append-size", mstat("/PT/A/APP.TXT", &st) == 0 && st.size == 8);
	f = fopen("/PT/A/APP.TXT", "w");
	if (f != NULL)
		fclose(f);
	CHECK("truncate", mstat("/PT/A/APP.TXT", &st) == 0 && st.size == 0);

	/* --- cleanup ----------------------------------------------------------------------------- */
	chdir("/");
	remove("/PT/A/APP.TXT");
	remove("/PT/A/B/N.TXT");
	remove("/PT/M.C");
	CHECK("cleanup-dirs", rmdir("/PT/A/CC") == 0 && rmdir("/PT/A/B") == 0 && rmdir("/PT/A") == 0 && rmdir("/PT") == 0);
	CHECK("cleanup-gone", !exists("/PT"));
	return finish("PATHT");
}
