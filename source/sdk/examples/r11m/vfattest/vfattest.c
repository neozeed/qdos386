/* VFATT: R11M Stage 4 guest test of VFAT long file names, through libc and libqdos (C89).
 * Works in /VT and removes everything it made.  Needs a kernel built with -DQDOS_VFAT.
 * Banner "QDOS/386 R11B VFATT PASS" only when every check passed. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "qdos386.h"
#include "../../r11b/check.h"

int mkdir(const char *path, int mode);
int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int qd_fsname(const char *in, char *out, size_t outsz);

static int put(const char *name, int n, int seed)
{
	FILE *f = fopen(name, "wb");
	int i;
	if (f == NULL)
		return -1;
	for (i = 0; i < n; i++)
		if (fputc('A' + (i * 7 + seed) % 26, f) == EOF)
			break;
	if (fclose(f) != 0)
		return -2;
	return i == n ? 0 : -3;
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

static int mstat(const char *p, struct qd_stat *st)
{
	char full[FILENAME_MAX];
	if (qd_fsname(p, full, sizeof full) != 0)
		return -1;
	return qd_stat(full, st);
}

static int exists(const char *p)
{
	struct qd_stat st;
	return mstat(p, &st) == 0;
}

static int cwdis(const char *want)
{
	char c[160];
	return getcwd(c, sizeof c) != NULL && strcmp(c, want) == 0;
}

/* a long listing of a directory: the records into le[], their count (or a negative error) */
#define LMAX 40
static struct qd_dirent_long le[LMAX];

static int list_long(const char *path)
{
	char full[FILENAME_MAX];
	int ch, n = 0, r;

	if (qd_fsname(path, full, sizeof full) != 0)
		return -1;
	ch = qd_dir_open_long(full);
	if (ch < 0)
		return ch;
	while ((r = qd_dir_next_long(ch, &le[n < LMAX ? n : LMAX - 1])) > 0)
		n++;
	qd_close(ch);
	return r < 0 ? r : n;
}

static int find_name(int n, const char *name)
{
	int i;
	for (i = 0; i < n && i < LMAX; i++)
		if (strcmp(le[i].name, name) == 0)
			return i;
	return -1;
}

static int find_alias(int n, const char *alias)
{
	int i;
	for (i = 0; i < n && i < LMAX; i++)
		if (strcmp(le[i].alias, alias) == 0)
			return i;
	return -1;
}

int main(void)
{
	char nm[100];
	struct qd_stat st;
	int n, i, k;

	chdir("/");
	CHECK("vt-absent", !exists("/VT"));
	CHECK("mkdir-VT", mkdir("/VT", 0) == 0);
	CHECK("create-legacy-root", put("LEGACY.TXT", 30, 9) == 0);
	CHECK("chdir-VT", chdir("/VT") == 0 && cwdis("/VT"));

	/* --- the gate: a long name, read by long name and by alias ------------------------------- */
	CHECK("create-long", put("SuperBasic_Interpreter.c", 1500, 3) == 0);
	CHECK("read-long", same("SuperBasic_Interpreter.c", 1500, 3));
	CHECK("read-long-upper", same("SUPERBASIC_INTERPRETER.C", 1500, 3));
	CHECK("read-alias", same("SUPERB~1.C", 1500, 3));
	CHECK("read-abs-long", same("/VT/superbasic_interpreter.c", 1500, 3));
	CHECK("read-abs-alias", same("/VT/superb~1.c", 1500, 3));
	CHECK("stat-long-size", mstat("SuperBasic_Interpreter.c", &st) == 0 && st.size == 1500 && !st.is_dir);
	n = list_long("/VT");
	CHECK("list-long-count", n == 1);
	CHECK("list-long-name", find_name(n, "SuperBasic_Interpreter.c") == 0);
	CHECK("list-long-alias", find_alias(n, "SUPERB~1.C") == 0);
	CHECK("list-long-flags", n == 1 && le[0].flags == QD_DIRL_HASLFN && le[0].size == 1500);

	/* --- short names stay short ------------------------------------------------------------- */
	CHECK("create-short", put("notes.txt", 50, 1) == 0);
	CHECK("short-upper", same("NOTES.TXT", 50, 1));
	CHECK("short-legacy", same("WIN1_LEGACY.TXT", 30, 9) && same("/LEGACY.TXT", 30, 9) && !exists("LEGACY.TXT"));
	n = list_long("/VT");
	CHECK("short-no-lfn", n == 2 && find_alias(n, "NOTES.TXT") >= 0 && le[find_alias(n, "NOTES.TXT")].flags == 0);

	/* --- aliases and collisions ------------------------------------------------------------- */
	CHECK("create-hw", put("Hello World.txt", 10, 4) == 0);
	CHECK("create-twin", put("Hello Wonderful.txt", 10, 5) == 0);
	CHECK("create-twin3", put("Hello World Again.txt", 10, 6) == 0);
	n = list_long("/VT");
	CHECK("alias-1", find_alias(n, "HELLOW~1.TXT") >= 0 && strcmp(le[find_alias(n, "HELLOW~1.TXT")].name, "Hello World.txt") == 0);
	CHECK("alias-2", find_alias(n, "HELLOW~2.TXT") >= 0 && strcmp(le[find_alias(n, "HELLOW~2.TXT")].name, "Hello Wonderful.txt") == 0);
	CHECK("alias-3", find_alias(n, "HELLOW~3.TXT") >= 0);
	CHECK("by-alias-2", same("HELLOW~2.TXT", 10, 5));
	CHECK("overwrite-long", put("HELLO WORLD.TXT", 20, 7) == 0 && same("Hello World.txt", 20, 7));
	n = list_long("/VT");
	CHECK("overwrite-no-dup", n == 5);

	/* --- rename ------------------------------------------------------------------------------ */
	CHECK("ren-long-long", rename("Hello World.txt", "Goodbye Cruel World.txt") == 0);
	CHECK("ren-old-gone", !exists("Hello World.txt") && !exists("HELLOW~1.TXT"));
	CHECK("ren-new-ok", same("Goodbye Cruel World.txt", 20, 7) && same("GOODBY~1.TXT", 20, 7));
	CHECK("ren-long-short", rename("Goodbye Cruel World.txt", "bye.txt") == 0 && same("BYE.TXT", 20, 7));
	CHECK("ren-short-long", rename("bye.txt", "Bye Bye Blackbird.txt") == 0 && same("BYEBYE~1.TXT", 20, 7));
	CHECK("ren-onto-existing", rename("Bye Bye Blackbird.txt", "hello wonderful.TXT") == -1 && errno == EEXIST);

	/* --- directories with blanks in the name ------------------------------------------------- */
	CHECK("mkdir-long", mkdir("My Documents", 0) == 0);
	CHECK("mkdir-long-again", mkdir("my documents", 0) == -1 && errno == EEXIST);
	CHECK("chdir-long", chdir("My Documents") == 0);
	CHECK("getcwd-long", cwdis("/VT/My Documents"));
	CHECK("file-in-long-dir", put("Quarterly Report 2026.txt", 300, 2) == 0 && same("QUARTE~1.TXT", 300, 2));
	CHECK("dotdot", chdir("..") == 0 && cwdis("/VT"));
	CHECK("path-through-long-dir", same("My Documents/Quarterly Report 2026.txt", 300, 2) && same("MYDOCU~1/QUARTE~1.TXT", 300, 2));
	CHECK("move-into-long-dir", rename("Bye Bye Blackbird.txt", "My Documents/Moved Here.txt") == 0 &&
	      same("My Documents/Moved Here.txt", 20, 7));
	CHECK("rmdir-nonempty", rmdir("My Documents") == -1 && errno == ENOTEMPTY);
	CHECK("ren-dir", rename("My Documents", "Your Documents") == 0 && !exists("My Documents") && exists("Your Documents"));
	CHECK("chdir-renamed", chdir("Your Documents") == 0 && cwdis("/VT/Your Documents") && chdir("/VT") == 0);
	CHECK("rm-in-dir", remove("Your Documents/Moved Here.txt") == 0 && remove("/VT/Your Documents/Quarterly Report 2026.txt") == 0);
	CHECK("rmdir-long", rmdir("Your Documents") == 0 && !exists("YOURDO~1"));

	/* --- names that are refused ------------------------------------------------------------- */
	CHECK("bad-star", fopen("a*b.txt", "wb") == NULL);
	CHECK("bad-quest", fopen("what?.txt", "wb") == NULL);
	CHECK("bad-leading-dot", fopen(".hidden", "wb") == NULL);
	CHECK("bad-trailing-space", fopen("trail ", "wb") == NULL);
	memset(nm, 'z', 64);
	nm[64] = 0;
	CHECK("bad-64-chars", fopen(nm, "wb") == NULL);
	memset(nm, 'y', 63);
	nm[63] = 0;
	CHECK("ok-63-chars", put(nm, 5, 1) == 0 && same(nm, 5, 1) && remove(nm) == 0);
	CHECK("bad-legacy-long", fopen("WIN1_Long Name.txt", "wb") == NULL);
	CHECK("none-added", list_long("/VT") == 4);

	/* --- delete: both entries go, the name can be reused ------------------------------------ */
	CHECK("rm-long", remove("SuperBasic_Interpreter.c") == 0 && !exists("SUPERB~1.C") && !exists("SuperBasic_Interpreter.c"));
	CHECK("rm-by-alias", remove("HELLOW~2.TXT") == 0 && !exists("Hello Wonderful.txt"));
	CHECK("recreate-reuses-alias", put("SuperBasic_Interpreter.c", 7, 1) == 0 && same("SUPERB~1.C", 7, 1));
	CHECK("rm-recreated", remove("superb~1.c") == 0);
	CHECK("rm-rest", remove("Hello World Again.txt") == 0 && remove("NOTES.TXT") == 0);
	CHECK("dir-empty", list_long("/VT") == 0);

	/* --- many names: a cycle of create and remove does not leak directory entries ----------- */
	for (i = 0; i < 30; i++) {
		sprintf(nm, "Cycle file number %d with a longer name.dat", i);
		k = put(nm, 3, i);
		if (k != 0 || remove(nm) != 0)
			break;
	}
	CHECK("cycle-30", i == 30);
	for (i = 0; i < 16; i++) {
		sprintf(nm, "Group member %d of the same prefix.dat", i);
		if (put(nm, 2, i) != 0)
			break;
	}
	CHECK("group-create", i == 16);
	n = list_long("/VT");
	CHECK("group-listed", n >= 16 && find_alias(n, "GROUPM~1.DAT") >= 0 && find_alias(n, "GROUP~16.DAT") >= 0 &&
	      strcmp(le[find_alias(n, "GROUP~16.DAT")].name, "Group member 15 of the same prefix.dat") == 0);
	for (i = 0; i < 16; i++) {
		sprintf(nm, "Group member %d of the same prefix.dat", i);
		if (!same(nm, 2, i) || remove(nm) != 0)
			break;
	}
	CHECK("group-read-remove", i == 16);

	/* --- cleanup ----------------------------------------------------------------------------- */
	chdir("/");
	CHECK("cleanup-legacy", remove("WIN1_LEGACY.TXT") == 0 && !exists("/LEGACY.TXT"));
	CHECK("cleanup-empty", list_long("/VT") == 0);
	CHECK("cleanup-rmdir", rmdir("/VT") == 0 && !exists("/VT"));
	return finish("VFATT");
}
