/* DIRFUL: R11M guest test - leave only a few dozen clusters free (one large file made with qd_truncate), then
 * fill a subdirectory with small files until the volume is full, so the directory itself grows cluster by
 * cluster; check that the failure is clean (ENOSPC), nothing is lost, space comes back when files are removed,
 * and the volume is left as it was.  Run it on a scratch volume. */
#define LEAVE 60
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "qdos386.h"
#include "../../r11b/check.h"

int mkdir(const char *path, int mode);
int rmdir(const char *path);

static void name_of(char *out, int i)
{
	sprintf(out, "/FULL/F%04d", i);
}

static int make(int i)
{
	char nm[32];
	FILE *f;
	name_of(nm, i);
	f = fopen(nm, "w");
	if (f == NULL)
		return -1;
	fputc('a' + i % 26, f);
	if (fclose(f) != 0)
		return -2;
	return 0;
}

static int has(int i)
{
	char nm[32];
	FILE *f;
	int c;
	name_of(nm, i);
	f = fopen(nm, "r");
	if (f == NULL)
		return 0;
	c = fgetc(f);
	if (c != 'a' + i % 26 || fgetc(f) != EOF) {
		fclose(f);
		return 0;
	}
	fclose(f);
	return 1;
}

int main(void)
{
	long free0 = qd_fs_free_clusters();
	long free1;
	int n, i, bad, err = 0;
	char nm[32];

	long cb = qd_fs_cluster_bytes();
	int fch;

	CHECK("free-known", free0 > LEAVE + 100 && cb > 0);
	fch = qd_open_ex("WIN1:/FILL", QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC);
	CHECK("fill-open", fch > 0);
	CHECK("fill-grow", qd_truncate(fch, (unsigned long) (free0 - LEAVE) * (unsigned long) cb) == 0);
	CHECK("fill-close", qd_close(fch) == 0);
	CHECK("left-free", qd_fs_free_clusters() == LEAVE);
	CHECK("mkdir", mkdir("/FULL", 0) == 0);
	for (n = 0; n < 9000; n++) {
		int r = make(n);
		if (r != 0) {
			err = errno;
			break;
		}
	}
	printf("DIRFUL: created %d files, errno %d, free clusters %ld\n", n, err, qd_fs_free_clusters());
	CHECK("filled-until-failure", n > 20 && n < LEAVE);
	CHECK("failure-is-ENOSPC", err == ENOSPC);
	CHECK("no-free-clusters-left", qd_fs_free_clusters() <= 1);
	bad = 0;
	for (i = 0; i < n; i++)
		if (!has(i))
			bad++;
	CHECK("created-files-intact", bad == 0 && has(n - 1) && has(0));
	CHECK("failed-name-absent", !has(n));
	CHECK("another-create-still-fails", make(n) != 0 && errno == ENOSPC);
	CHECK("mkdir-fails-when-full", mkdir("/FULL/SUB", 0) == -1 && errno == ENOSPC);
	CHECK("rmdir-nonempty", rmdir("/FULL") == -1 && errno == ENOTEMPTY);

	/* space comes back */
	bad = 0;
	for (i = 0; i < n; i += 2) {
		name_of(nm, i);
		if (remove(nm) != 0)
			bad++;
	}
	CHECK("removed-half", bad == 0);
	CHECK("space-after-remove", qd_fs_free_clusters() >= n / 2 - 2);
	bad = 0;
	for (i = 1; i < n; i += 2)
		if (!has(i))
			bad++;
	CHECK("other-half-intact", bad == 0);
	for (i = 0; i < n / 2 - 4; i += 2)
		if (make(i) != 0)
			break;
	CHECK("recreate-after-remove", i >= n / 2 - 6);

	/* clean up completely */
	for (i = 0; i <= n; i++) {      /* n itself may exist as an empty file: the create succeeded, its data did not */
		name_of(nm, i);
		remove(nm);
	}
	CHECK("rmdir-empty", rmdir("/FULL") == 0);
	CHECK("remove-fill", remove("/FILL") == 0);
	free1 = qd_fs_free_clusters();
	printf("DIRFUL: free clusters before %ld after %ld\n", free0, free1);
	CHECK("space-restored", free1 == free0);
	return finish("DIRFUL");
}
