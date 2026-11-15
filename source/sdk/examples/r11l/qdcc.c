/* CC.AOU - R11L guest-native build driver ("qdcc").  NOT the historical gcc driver: that one needs fork,
 * exec and pipes, which QDOS/386 does not have.  This is a small, clearly labelled replacement that runs
 * the REAL historical tools, one at a time, as separate paged jobs (R11K paged launcher), waits for each,
 * reads its exit status, and stops at the first failure:
 *
 *   cccp  -nostdinc -undef -Di386 -D__i386__ -I. [-D.. -I..] NAME.C NAME.I      (GNU CPP 1.40)
 *   cc1   -quiet [-O] NAME.I -o NAME.S                                          (GCC 1.40 proper)
 *   as    -o NAME.O NAME.S                                                      (GAS 1.38)
 *   ld    -N -T SLOT1.LD -e _start -o OUT.AOU CRT0.O NAME.O ... LIBM.A LIBC.A LIBGCC.A LIBQDOS.A  (i386 ld)
 *
 *   CC [-O] [-c] [-v] [-r] [-o OUT.AOU] [-Dname[=v]] [-Idir] SRC.C [SRC.C ...]
 *     -O  pass -O to cc1 (GCC 1.40 has one optimisation level)       -c  compile only (no link)
 *     -v  print every tool command line                               -r  remove NAME.I and NAME.S after success
 *     -o  output name (default: the first source's name with .AOU)
 *
 * Rules: sources are in the volume root with 8.3 names and a .C extension (any case); the intermediates
 * NAME.I, NAME.S, NAME.O sit beside the source.  Before a build the old NAME.I/.S/.O and the output are
 * DELETED, so a failed build can never leave an output that looks current.  After each tool its output is
 * checked (it must exist and not be empty: the historical as exits 0 for a missing input).  On a failure the
 * failed step's partial output and everything after it are deleted.  CRT0.O is assembled from CRT0.S when it
 * is missing.  Kernel requirement: paging (the driver is an ordinary slot 1 program; each tool is a paged child).
 *
 * Exit status: 0 built; 1 usage; 2 source missing or bad name; 10 cccp failed; 11 cc1 failed; 12 as failed;
 * 13 ld failed; 14 CRT0 assembly failed or CRT0.S/CRT0.O missing; 20 a tool could not be started;
 * 21 this kernel has no paged launch.
 * Classification: guest-native driver; the tools it starts are the historical programs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "qdos386.h"

#define MAXSRC   6
#define MAXOPT   4
#define PACKMAX  420             /* the loader's argument block: 432 bytes with the tool name */

static int verbose;

static int exists(const char *name)          /* size in bytes, or -1 if the file is missing */
{
	FILE *fp = fopen(name, "r");
	long n;

	if (fp == NULL)
		return -1;
	if (fseek(fp, 0L, SEEK_END) != 0) {
		fclose(fp);
		return -1;
	}
	n = ftell(fp);
	fclose(fp);
	return (int) n;
}

/* Start TOOL (a WIN1_ program name) paged with the given arguments, wait, return its exit status, or a value
 * below -1000 if it could not be started.  Prints nothing but a start failure. */
static int run_tool(const char *tool, int argc, const char **argv)
{
	char packed[PACKMAX];
	unsigned int len = 0, n;
	int i, id, st;

	for (i = 0; i < argc; i++) {
		n = (unsigned int) strlen(argv[i]) + 1;
		if (len + n > sizeof packed) {
			printf("CC: arguments for %s are too long\n", tool);
			return -1001;
		}
		memcpy(packed + len, argv[i], n);
		len += n;
	}
	if (verbose) {
		printf("CC: %s", tool);
		for (i = 0; i < argc; i++)
			printf(" %s", argv[i]);
		printf("\n");
	}
	fflush(stdout);
	id = qd_pg_startv(tool, packed, len, 0);
	if (id <= 0) {
		printf("CC: cannot start %s (kernel error %d)\n", tool, id);
		return -1000 + (id < 0 ? id : 0) - 1;
	}
	while ((st = qd_job_exit_status(id)) == QD_ERR_RUNNING)
		qd_yield(1);
	while (!qd_job_reclaimed(id))
		qd_yield(1);
	return st;
}

static void rm(const char *name)
{
	(void) remove(name);
}

static int ends_c(const char *s, char *base)    /* "NAME.C" in any case -> base "NAME"; 0 if not a valid source name */
{
	size_t n = strlen(s), i;

	if (n < 3 || n > 12 || (s[n - 1] != 'c' && s[n - 1] != 'C') || s[n - 2] != '.')
		return 0;
	if (n - 2 > 8)
		return 0;
	for (i = 0; i < n - 2; i++) {
		if (!isalnum((unsigned char) s[i]) && s[i] != '_')
			return 0;
		base[i] = (char) toupper((unsigned char) s[i]);
	}
	base[n - 2] = '\0';
	return 1;
}

int main(int argc, char **argv)
{
	static char bases[MAXSRC][10];
	static char names[MAXSRC][4][14];     /* .C .I .S .O */
	const char *src[MAXSRC];
	const char *opts[MAXOPT];
	const char *a[16];
	char outname[16];
	char dflag[MAXOPT][40];
	int nsrc = 0, nopt = 0, i, st, optO = 0, compile_only = 0, remove_tmp = 0, na, k;
	const char *out = NULL;

	for (i = 1; i < argc; i++) {
		const char *s = argv[i];

		if (s[0] == '-' && s[1] != '\0') {
			if (strcmp(s, "-O") == 0)
				optO = 1;
			else if (strcmp(s, "-c") == 0)
				compile_only = 1;
			else if (strcmp(s, "-v") == 0)
				verbose = 1;
			else if (strcmp(s, "-r") == 0)
				remove_tmp = 1;
			else if (strcmp(s, "-o") == 0 && i + 1 < argc)
				out = argv[++i];
			else if ((s[1] == 'D' || s[1] == 'I') && s[2] != '\0' && nopt < MAXOPT && strlen(s) < 40) {
				strcpy(dflag[nopt], s);
				opts[nopt] = dflag[nopt];
				nopt++;
			} else {
				printf("CC: unknown option %s\n", s);
				return 1;
			}
		} else {
			if (nsrc >= MAXSRC) {
				printf("CC: at most %d sources\n", MAXSRC);
				return 1;
			}
			src[nsrc++] = s;
		}
	}
	if (nsrc == 0) {
		printf("usage: CC [-O] [-c] [-v] [-r] [-o OUT.AOU] [-Dname] [-Idir] SRC.C [SRC.C ...]\n");
		return 1;
	}
	if (!qd_pg_available()) {
		printf("CC: this kernel has no paged launch; the tools cannot be run (needs -DQDOS_PAGING)\n");
		return 21;
	}
	for (i = 0; i < nsrc; i++) {
		if (!ends_c(src[i], bases[i])) {
			printf("CC: %s is not an 8.3 .C source name\n", src[i]);
			return 2;
		}
		sprintf(names[i][0], "%s.C", bases[i]);
		sprintf(names[i][1], "%s.I", bases[i]);
		sprintf(names[i][2], "%s.S", bases[i]);
		sprintf(names[i][3], "%s.O", bases[i]);
		if (exists(names[i][0]) < 0) {
			printf("CC: %s: no such file\n", src[i]);
			return 2;
		}
	}
	if (out != NULL) {
		size_t n = strlen(out);

		if (n > 12) {
			printf("CC: output name too long\n");
			return 1;
		}
		for (i = 0; out[i] != '\0'; i++)
			outname[i] = (char) toupper((unsigned char) out[i]);
		outname[i] = '\0';
	} else {
		sprintf(outname, "%s.AOU", bases[0]);
	}

	/* No stale outputs: delete what this build is going to make. */
	for (i = 0; i < nsrc; i++) {
		rm(names[i][1]);
		rm(names[i][2]);
		rm(names[i][3]);
	}
	if (!compile_only)
		rm(outname);

	for (i = 0; i < nsrc; i++) {
		na = 0;
		a[na++] = "-nostdinc"; a[na++] = "-undef"; a[na++] = "-Di386"; a[na++] = "-D__i386__"; a[na++] = "-I.";
		for (k = 0; k < nopt; k++)
			a[na++] = opts[k];
		a[na++] = names[i][0]; a[na++] = names[i][1];
		st = run_tool("CCCP.AOU", na, a);
		if (st != 0 || exists(names[i][1]) <= 0) {
			printf("CC: preprocessing %s failed (status %d)\n", names[i][0], st);
			rm(names[i][1]);
			return st < -1000 ? 20 : 10;
		}
		na = 0;
		a[na++] = "-quiet";
		if (optO)
			a[na++] = "-O";
		a[na++] = names[i][1]; a[na++] = "-o"; a[na++] = names[i][2];
		st = run_tool("CC1.AOU", na, a);
		if (st != 0 || exists(names[i][2]) <= 0) {
			printf("CC: compiling %s failed (status %d)\n", names[i][1], st);
			rm(names[i][2]);
			return st < -1000 ? 20 : 11;
		}
		na = 0;
		a[na++] = "-o"; a[na++] = names[i][3]; a[na++] = names[i][2];
		st = run_tool("AS.AOU", na, a);
		if (st != 0 || exists(names[i][3]) <= 0) {
			printf("CC: assembling %s failed (status %d)\n", names[i][2], st);
			rm(names[i][3]);
			return st < -1000 ? 20 : 12;
		}
		if (remove_tmp) {
			rm(names[i][1]);
			rm(names[i][2]);
		}
		printf("CC: %s -> %s\n", names[i][0], names[i][3]);
	}
	if (compile_only)
		return 0;

	if (exists("CRT0.O") <= 0) {
		if (exists("CRT0.S") <= 0) {
			printf("CC: CRT0.O and CRT0.S are both missing\n");
			return 14;
		}
		a[0] = "-o"; a[1] = "CRT0.O"; a[2] = "CRT0.S";
		st = run_tool("AS.AOU", 3, a);
		if (st != 0 || exists("CRT0.O") <= 0) {
			printf("CC: assembling CRT0.S failed (status %d)\n", st);
			rm("CRT0.O");
			return st < -1000 ? 20 : 14;
		}
	}
	na = 0;
	a[na++] = "-N"; a[na++] = "-T"; a[na++] = "SLOT1.LD"; a[na++] = "-e"; a[na++] = "_start";
	a[na++] = "-o"; a[na++] = outname; a[na++] = "CRT0.O";
	for (i = 0; i < nsrc; i++)
		a[na++] = names[i][3];
	a[na++] = "LIBM.A"; a[na++] = "LIBC.A"; a[na++] = "LIBGCC.A"; a[na++] = "LIBQDOS.A";
	st = run_tool("LD.AOU", na, a);
	if (st != 0 || exists(outname) <= 0) {
		printf("CC: linking %s failed (status %d)\n", outname, st);
		rm(outname);
		return st < -1000 ? 20 : 13;
	}
	printf("CC: %s built (%d bytes)\n", outname, exists(outname));
	return 0;
}
