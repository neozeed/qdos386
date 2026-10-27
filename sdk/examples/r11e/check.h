/* check.h - guest-side assertions for the R11E tests (C89, header only).
 * Each check prints "CHECK name: ok" or "CHECK name: FAIL".  finish() prints
 * the banner "QDOS/386 R11E <TAG> PASS" only when every check passed, so a
 * banner is evidence of the guest's own assertions, not of a successful build. */
#ifndef R11E_CHECK_H
#define R11E_CHECK_H
#include <stdio.h>

static int chk_ok = 0;
static int chk_bad = 0;

static void check(const char *name, int ok)
{
	if (ok) {
		chk_ok++;
		printf("CHECK %s: ok\n", name);
	} else {
		chk_bad++;
		printf("CHECK %s: FAIL\n", name);
	}
}

#define CHECK(name, cond) check((name), (cond) ? 1 : 0)

static int finish(const char *tag)
{
	printf("%s: %d checks passed, %d failed\n", tag, chk_ok, chk_bad);
	if (chk_bad == 0) {
		printf("QDOS/386 R11E %s PASS\n", tag);
		return 0;
	}
	printf("QDOS/386 R11E %s FAIL\n", tag);
	return 1;
}

#endif
