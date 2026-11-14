/* GCC.AOU - QDOS/386 R14B guest-native compiler driver for the historical GCC 1.40 toolchain.
 *
 * NOT the historical gcc.c: that one needs fork, exec and pipes.  This is a small C89 replacement (derived from the
 * R11L CC.AOU, sdk/examples/r11l/qdcc.c) that runs the REAL historical tools one at a time as child processes
 * (qd_spawn / qd_wait, R12A), stops at the first failure, and finds everything through the environment the shell
 * hands down (R12B):
 *
 *   tools     CCCP, CC1, AS, LD by bare name: the KERNEL's program search (current directory, each PATH entry, then /;
 *             NAME then NAME.AOU - docs/R13B-EXEC-RESOLUTION-ABI.md).  -Bprefix puts a prefix in front of the names.
 *   headers   -I directories first, then every INCLUDE entry (as -I); cccp always gets -nostdinc so that its built-in
 *             Unix directories (/usr/include ...) are never searched.  -nostdinc here drops INCLUDE.
 *   startup   the link script SLOT1.LD, CRT0.O and LIBM.A LIBC.A LIBGCC-QD.A LIBQDOS.A are looked up in every -L
 *             directory, then every LIB entry, and so is libNAME.a for -lNAME.  -nostdlib drops CRT0.O and the four
 *             libraries (not the script).  A link too long for one QDOS/386 command line (15 arguments, 432 bytes)
 *             goes through a link script written for the run (the script's text + INPUT(...)).
 *   temps     the .I/.S/.O of each source go to TEMP, else TMP, else the current directory, named after this process's
 *             job id (Q<id><n>.I ...), never a name that already exists; removed at the end, also after a failure.
 *
 * Options (GCC 1.40 meanings; anything else is refused, nothing modern is invented):
 *   -c -S -E  stop after assembling (NAME.O) / compiling (NAME.S) in the current directory, or after preprocessing (to
 *             the console); -o FILE names the output instead           -o FILE  default A.OUT (CC.AOU: SOURCE.AOU)
 *   -O  optimise (cc1 -O, cpp -D__OPTIMIZE__)        -g -w -W* -ansi -traditional -pedantic -f*  to cc1 (some to cpp)
 *   -Idir -Dname[=v] -Uname -nostdinc -C -P -trigraphs  cccp       -Ldir -lname -e sym -T file -u sym -nostdlib  link
 *   -v  print the version and every tool command     -Bprefix  tool name prefix (e.g. -B/SDK/BIN/)
 *   Inputs: .c -> cccp, cc1, as;  .i -> cc1, as;  .s -> as;  anything else (.o, .a) goes to the linker unchanged.
 *
 * Safety: the output this command makes is removed BEFORE the first tool runs, and every tool's output is checked to
 * exist and be non-empty (the historical as exits 0 for a missing input), so a failed build never leaves a runnable
 * old program.  An output name equal to an input is refused.  Only files this run created in the temp directory are
 * removed; nothing else is touched.
 * Exit status: 0 done; 1 usage; 2 an input is missing; 10 cccp, 11 cc1, 12 as, 13 ld failed; 14 the link script,
 * CRT0.O or a library not found; 15 a bad TEMP/TMP; 20 a tool could not be started; 21 this kernel cannot start
 * processes.
 * Classification: guest-native driver (host built with the SDK); the tools it starts are the historical programs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef GCCDRV_HOST
#include "hostshim.h"
#else
#include "qdos386.h"
#endif

#define VERSION   "GCC driver R14B (QDOS/386) for GCC 1.40, GAS 1.38, ld (binutils 1.94)"
#define ARGS_MAX  15            /* kernel: 16 argv entries including the program name (kernel/aout.h APP_ARG_MAX) */
#define PACK_MAX  432           /* kernel: argument strings including the program name and its NUL */
#define PATHLEN   64            /* the native path limit (QD_PATH_MAX) */
#define MAXIN     24
#define MAXDIR    12
#define MAXOPT    24
#define POOLSZ    16384

#define E_USAGE   1
#define E_INPUT   2
#define E_CPP     10
#define E_CC1     11
#define E_AS      12
#define E_LD      13
#define E_FIND    14
#define E_TEMP    15
#define E_START   20
#define E_KERNEL  21

#define K_C    1
#define K_I    2
#define K_S    3
#define K_LINK 4                /* a file for the linker */
#define K_LIB  5                /* -lNAME (kept in command-line order, as GCC 1.40 does) */

#define NOSTART (-10000)        /* run(): the tool was not started */

static char pool[POOLSZ];
static unsigned int poolused;
static int verbose, ccmode;
static const char *bprefix = "";
static const char *me = "GCC";

static const char *in_name[MAXIN];
static int in_kind[MAXIN];
static const char *in_obj[MAXIN];     /* the file each input gives the linker (an object, or a resolved -l) */
static int nin;
static const char *cpp_opt[MAXOPT], *cc1_opt[MAXOPT], *ld_opt[MAXOPT];
static int ncpp, ncc1, nld;
static const char *ldirs[MAXDIR], *idirs[MAXDIR];
static int nldir, nidir;
static const char *temps[4 * MAXIN + 8];
static int ntemp;

static const char *av[ARGS_MAX + 1];  /* the arguments of the next tool */
static int ac, acover;

static void die_oom(void)
{
	fprintf(stderr, "%s: command line too long for the driver's buffers\n", me);
	exit(E_USAGE);
}

static char *cat2(const char *a, const char *b)        /* a + b, in the pool */
{
	unsigned int la = (unsigned int) strlen(a), lb = (unsigned int) strlen(b);
	char *p;

	if (poolused + la + lb + 1 > POOLSZ)
		die_oom();
	p = pool + poolused;
	memcpy(p, a, la);
	memcpy(p + la, b, lb + 1);
	poolused += la + lb + 1;
	return p;
}

static int ieq(const char *a, const char *b)            /* equal, ignoring ASCII case */
{
	while (*a && toupper((unsigned char) *a) == toupper((unsigned char) *b))
		a++, b++;
	return toupper((unsigned char) *a) == toupper((unsigned char) *b);
}

/* qd_stat takes a kernel channel name: a program's file name becomes the native form "WIN1:" + path (absolute, or
 * relative to the current directory - docs/R11M-FAT-PATH-ABI.md), as the C library does for fopen/remove. */
static int stat_name(const char *path, struct qd_stat *st)
{
	char buf[PATHLEN + 16];

	/* already a channel name ("WIN1_..." / "WIN1:..."): each character is tested only after the one before it
	 * matched, so a short name is never read past its end */
	if (toupper((unsigned char) path[0]) == 'W' && toupper((unsigned char) path[1]) == 'I' &&
	    toupper((unsigned char) path[2]) == 'N' && path[3] == '1' && (path[4] == '_' || path[4] == ':'))
		return qd_stat(path, st);
	if (strlen(path) > PATHLEN)
		return -90;
	strcpy(buf, "WIN1:");
	strcat(buf, path);
	return qd_stat(buf, st);
}

static int ftype(const char *path)                     /* 1 regular file, 2 directory, 0 missing (or any error) */
{
	struct qd_stat st;

	if (stat_name(path, &st) < 0)
		return 0;
	return st.is_dir ? 2 : 1;
}

static long fsize(const char *path)                    /* size of a regular file, -1 if there is none */
{
	struct qd_stat st;

	if (stat_name(path, &st) < 0 || st.is_dir)
		return -1;
	return (long) st.size;
}

static const char *suffix(const char *name)            /* the last extension (".c"), or "" */
{
	const char *p = strrchr(name, '.'), *s = strrchr(name, '/');

	if (p == NULL || (s != NULL && p < s) || p == name || p[-1] == '/')
		return "";
	return p;
}

static const char *basepart(const char *name)          /* "dir/NAME.C" -> "NAME" */
{
	const char *s = strrchr(name, '/');
	char *r;
	unsigned int n;

	s = s ? s + 1 : name;
	if (s[0] != '\0' && s[1] == ':')                    /* "X:NAME" is not a QDOS/386 form; be safe */
		s += 2;
	r = cat2(s, "");
	n = (unsigned int) (strlen(s) - strlen(suffix(s)));
	r[n] = '\0';
	return r;
}

static const char *join(const char *dir, const char *name)     /* dir + "/" + name ("." gives just name) */
{
	if (strcmp(dir, ".") == 0)
		return cat2(name, "");
	if (dir[0] != '\0' && dir[strlen(dir) - 1] == '/')
		return cat2(dir, name);
	return cat2(cat2(dir, "/"), name);
}

/* Split an environment list (';'-separated; blanks around an entry and empty entries skipped, as the kernel does for
 * PATH) into dirs[]; a trailing '/' is dropped except for "/". */
static void split_list(const char *var, const char *val, const char **dirs, int *n)
{
	char buf[260];
	const char *p = val;

	while (*p) {
		unsigned int k = 0;
		while (*p == ' ' || *p == '\t')
			p++;
		while (*p && *p != ';') {
			if (k < sizeof buf - 1)
				buf[k++] = *p;
			p++;
		}
		if (*p == ';')
			p++;
		while (k > 0 && (buf[k - 1] == ' ' || buf[k - 1] == '\t'))
			k--;
		while (k > 1 && buf[k - 1] == '/')
			k--;
		buf[k] = '\0';
		if (k == 0)
			continue;
		if (*n >= MAXDIR) {
			fprintf(stderr, "%s: too many directories (more than %d) with %s\n", me, MAXDIR, var);
			exit(E_USAGE);
		}
		dirs[(*n)++] = cat2(buf, "");
	}
}

static const char *find_lib(const char *name)          /* NAME in the -L directories, then LIB; NULL if absent */
{
	int i;

	for (i = 0; i < nldir; i++) {
		const char *p = join(ldirs[i], name);
		if (ftype(p) == 1)
			return p;
	}
	return NULL;
}

static void show_dirs(const char *what, const char **dirs, int n)
{
	int i;

	fprintf(stderr, "%s: %s:", me, what);
	if (n == 0)
		fprintf(stderr, " (none)");
	for (i = 0; i < n; i++)
		fprintf(stderr, " %s", dirs[i]);
	fprintf(stderr, "\n");
}

/* The historical ld does not take its argv as separate words: it joins them with blanks and lexes that line
 * (binutils-1.94 ld/ldlex.l parse_args), so a file name holding a blank must reach it in double quotes, which its
 * lexer reads as one name.  cccp, cc1 and as take ordinary argv and get the name unquoted. */
static const char *ldq(const char *s)
{
	return strchr(s, ' ') != NULL ? cat2(cat2("\"", s), "\"") : s;
}

static void arg0(void)
{
	ac = 0;
	acover = 0;
}

static void arg(const char *s)
{
	if (ac < ARGS_MAX + 1)
		av[ac++] = s;
	else
		acover = 1;
}

/* Start TOOL with av[0..ac-1] (the arguments only), wait, and return its exit status (>= 0), a negative kernel status
 * (-17 ended without a status, -66 killed), or NOSTART after printing why it could not be started. */
static int run(const char *tool)
{
	char packed[PACK_MAX];
	char name[PATHLEN + 16];
	unsigned int len = 0, n;
	int i, id, st;

	if (strlen(bprefix) + strlen(tool) >= sizeof name) {
		fprintf(stderr, "%s: -B prefix too long\n", me);
		return NOSTART;
	}
	strcpy(name, bprefix);
	strcat(name, tool);
	if (acover || ac > ARGS_MAX) {
		fprintf(stderr, "%s: %s would need more than %d arguments, the QDOS/386 limit\n", me, name, ARGS_MAX);
		return NOSTART;
	}
	for (i = 0; i < ac; i++) {
		n = (unsigned int) strlen(av[i]) + 1;
		if (strlen(name) + 1 + len + n > PACK_MAX) {
			fprintf(stderr, "%s: the arguments for %s are longer than the QDOS/386 limit (%d bytes)\n", me, name,
			        PACK_MAX);
			return NOSTART;
		}
		memcpy(packed + len, av[i], n);
		len += n;
	}
	if (verbose) {
		fprintf(stderr, "%s: %s", me, name);
		for (i = 0; i < ac; i++)
			fprintf(stderr, strchr(av[i], ' ') && av[i][0] != '"' ? " \"%s\"" : " %s", av[i]);
		fprintf(stderr, "\n");
	}
	fflush(stdout);
	fflush(stderr);
	id = qd_spawn(name, packed, len, 0, QD_SPAWN_CON_OWN);
	if (id < 0) {
		const char *path = getenv("PATH");
		if (id == -44 || id == -46 || id == -47 || id == -87 || id == -88 || id == -90)
			fprintf(stderr, "%s: cannot find the program %s (searched the current directory, PATH=%s, then /)\n",
			        me, name, path ? path : "(not set)");
		else if (id <= -20 && id >= -25)
			fprintf(stderr, "%s: %s is not a valid QDOS/386 program (code %d)\n", me, name, id);
		else if (id == -60 || id == -30 || id == -3)
			fprintf(stderr, "%s: cannot start %s: out of memory (code %d)\n", me, name, id);
		else
			fprintf(stderr, "%s: cannot start %s (code %d)\n", me, name, id);
		return NOSTART;
	}
	st = qd_wait(id, -1L);
	if (st < 0)
		fprintf(stderr, "%s: %s ended without an exit status (code %d%s)\n", me, name, st,
		        st == -66 ? ", killed" : st == -17 ? ", fault" : "");
	return st;
}

static void rm_if(const char *path)                    /* remove a regular file if there is one */
{
	if (path != NULL && ftype(path) == 1)
		(void) remove(path);
}

static void cleanup(void)                              /* remove this run's temporaries */
{
	int i;

	for (i = 0; i < ntemp; i++)
		rm_if(temps[i]);
	ntemp = 0;
}

static void temp(const char *t)
{
	temps[ntemp++] = t;
}

/* A step failed: say so, remove its (partial) output and the temporaries; returns the driver's exit status. */
static int fail(const char *tool, int st, const char *out, int code)
{
	if (st > 0)
		fprintf(stderr, "%s: %s failed (exit status %d)\n", me, tool, st);
	else if (st == 0)
		fprintf(stderr, "%s: %s made no output %s\n", me, tool, out ? out : "");
	rm_if(out);
	cleanup();
	return st == NOSTART ? E_START : code;
}

static int ok_out(int st, const char *out)             /* status 0 and a non-empty output file */
{
	return st == 0 && fsize(out) > 0;
}

static const char *optarg_of(int *i, int argc, char **argv)    /* "-Xvalue" or "-X value" */
{
	const char *s = argv[*i];

	if (s[2] != '\0')
		return s + 2;
	if (*i + 1 >= argc) {
		fprintf(stderr, "%s: %s needs an argument\n", me, s);
		exit(E_USAGE);
	}
	return argv[++*i];
}

static void addopt(const char **v, int *n, const char *s)
{
	if (*n >= MAXOPT) {
		fprintf(stderr, "%s: too many options\n", me);
		exit(E_USAGE);
	}
	v[(*n)++] = s;
}

static void addin(const char *s, int kind)
{
	if (nin >= MAXIN) {
		fprintf(stderr, "%s: at most %d input files\n", me, MAXIN);
		exit(E_USAGE);
	}
	in_name[nin] = s;
	in_kind[nin++] = kind;
}

int main(int argc, char **argv)
{
	static const char *libname[4] = { "LIBM.A", "LIBC.A", "LIBGCC-QD.A", "LIBQDOS.A" };
	const char *out = NULL, *script = NULL, *entry = "_start", *tdir, *crt0 = NULL, *libs[4], *v;
	int stop = 0;                  /* 'c', 'S', 'E' or 0 (link) */
	int optO = 0, nostdinc = 0, nostdlib = 0, ncompile = 0, serial = 0, caps, i, k, st;
	unsigned long jid;

	/* CC.AOU is the same program: the R11L compatible name, whose default output is FIRST-SOURCE.AOU */
	if (argc > 0 && argv[0] != NULL) {
		const char *b = strrchr(argv[0], '/');
		b = b ? b + 1 : argv[0];
		if (toupper((unsigned char) b[0]) == 'C' && toupper((unsigned char) b[1]) == 'C' && (b[2] == '\0' || b[2] == '.')) {
			ccmode = 1;
			me = "CC";
		}
	}

	for (i = 1; i < argc; i++) {
		const char *s = argv[i];

		if (s[0] != '-' || s[1] == '\0') {
			const char *x = suffix(s);
			addin(s, ieq(x, ".c") ? K_C : ieq(x, ".i") ? K_I : ieq(x, ".s") ? K_S : K_LINK);
		} else if (strcmp(s, "-c") == 0 || strcmp(s, "-S") == 0 || strcmp(s, "-E") == 0) {
			if (stop == 0 || s[1] == 'E' || (s[1] == 'S' && stop == 'c'))
				stop = s[1];                         /* GCC 1.40: -E over -S over -c */
		} else if (strcmp(s, "-o") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "%s: -o needs a file name\n", me);
				return E_USAGE;
			}
			out = argv[++i];
		} else if (strcmp(s, "-O") == 0)
			optO = 1;
		else if (strcmp(s, "-v") == 0)
			verbose = 1;
		else if (strcmp(s, "-r") == 0 && ccmode)
			;                                        /* R11L CC "remove NAME.I/.S": they are temporaries now */
		else if (strcmp(s, "-nostdinc") == 0)
			nostdinc = 1;
		else if (strcmp(s, "-nostdlib") == 0)
			nostdlib = 1;
		else if (s[1] == 'B')
			bprefix = s + 2;
		else if (s[1] == 'I') {
			if (nidir >= MAXDIR) {
				fprintf(stderr, "%s: too many -I directories\n", me);
				return E_USAGE;
			}
			idirs[nidir++] = optarg_of(&i, argc, argv);
		} else if (s[1] == 'D' || s[1] == 'U') {
			const char *val = optarg_of(&i, argc, argv);
			addopt(cpp_opt, &ncpp, cat2(s[1] == 'D' ? "-D" : "-U", val));
		} else if (strcmp(s, "-C") == 0 || strcmp(s, "-P") == 0 || strcmp(s, "-trigraphs") == 0)
			addopt(cpp_opt, &ncpp, s);
		else if (strcmp(s, "-g") == 0 || strcmp(s, "-w") == 0 || strcmp(s, "-ansi") == 0 ||
		         strcmp(s, "-traditional") == 0 || strcmp(s, "-pedantic") == 0 || s[1] == 'W' || s[1] == 'f') {
			addopt(cc1_opt, &ncc1, s);
			if (strcmp(s, "-ansi") == 0) {
				addopt(cpp_opt, &ncpp, "-trigraphs");
				addopt(cpp_opt, &ncpp, "-D__STRICT_ANSI__");
			} else if (s[1] != 'f' && s[1] != 'g')
				addopt(cpp_opt, &ncpp, s);           /* -traditional -pedantic -w -W...: cpp takes them too */
		} else if (s[1] == 'L') {
			if (nldir >= MAXDIR) {
				fprintf(stderr, "%s: too many -L directories\n", me);
				return E_USAGE;
			}
			ldirs[nldir++] = optarg_of(&i, argc, argv);
		} else if (s[1] == 'l' && s[2] != '\0') {
			addin(s, K_LIB);
		} else if (strcmp(s, "-e") == 0 || strcmp(s, "-T") == 0 || strcmp(s, "-u") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "%s: %s needs an argument\n", me, s);
				return E_USAGE;
			}
			if (s[1] == 'e')
				entry = argv[++i];
			else if (s[1] == 'T')
				script = argv[++i];
			else {
				addopt(ld_opt, &nld, s);
				addopt(ld_opt, &nld, argv[++i]);
			}
		} else {
			fprintf(stderr, "%s: unknown option %s (GCC 1.40 options only)\n", me, s);
			return E_USAGE;
		}
	}
	if (verbose)
		fprintf(stderr, "%s\n", VERSION);
	if (nin == 0) {
		fprintf(stderr, "%s: no input files\nusage: %s [-c|-S|-E] [-o OUT] [-O] [-v] [-Idir] [-Dname[=v]] [-Ldir] [-lname] FILE...\n",
		        me, me);
		return E_USAGE;
	}
	caps = qd_proc_caps();
	if (caps < 0 || (caps & QD_PROC_CAP_SPAWN) == 0) {
		fprintf(stderr, "%s: this kernel cannot start processes (needs the R12A process services)\n", me);
		return E_KERNEL;
	}

	/* the inputs must exist; count the files that produce an output of their own */
	for (i = 0; i < nin; i++) {
		if (in_kind[i] == K_LIB)
			continue;
		k = ftype(in_name[i]);
		if (k != 1) {
			fprintf(stderr, "%s: %s: %s\n", me, in_name[i], k == 2 ? "is a directory" : "no such file");
			return E_INPUT;
		}
		if (stop == 'E' ? in_kind[i] == K_C : stop == 'S' ? (in_kind[i] == K_C || in_kind[i] == K_I)
		                                                  : in_kind[i] != K_LINK)
			ncompile++;
	}
	if (stop != 0 && out != NULL && ncompile > 1) {
		fprintf(stderr, "%s: -o with -%c and more than one input would overwrite the output\n", me, stop);
		return E_USAGE;
	}
	if (stop == 0 && out == NULL) {
		if (ccmode) {
			for (i = 0; i < nin - 1 && in_kind[i] == K_LIB; i++)
				;
			out = cat2(basepart(in_name[i]), ".AOU");
		} else
			out = "A.OUT";                           /* GCC 1.40's default, a.out */
	}
	if (out != NULL)
		for (i = 0; i < nin; i++)
			if (in_kind[i] != K_LIB && ieq(out, in_name[i])) {
				fprintf(stderr, "%s: the output %s is also an input\n", me, out);
				return E_USAGE;
			}

	/* temporary directory: TEMP, else TMP, else the current directory.  A variable that is set but names no
	 * directory is an error (no silent fallback). */
	v = getenv("TEMP");
	if (v == NULL || *v == '\0')
		v = getenv("TMP");
	tdir = ".";
	if (v != NULL && *v != '\0') {
		const char *dl[MAXDIR];
		int nd = 0;
		split_list("TEMP", v, dl, &nd);
		if (nd > 0)
			tdir = dl[0];
		if (ftype(tdir) != 2) {
			fprintf(stderr, "%s: the temporary directory %s (TEMP/TMP) does not exist or is not a directory\n", me, tdir);
			return E_TEMP;
		}
	}
	if (strlen(tdir) + 14 > PATHLEN) {
		fprintf(stderr, "%s: the temporary directory %s is too long for a QDOS/386 path\n", me, tdir);
		return E_TEMP;
	}
	{
		struct qd_procinfo pi;
		jid = qd_proc_info(0, &pi) >= 0 ? pi.id : 0UL;
	}

	/* search lists: -I / -L first, then INCLUDE / LIB */
	if (!nostdinc && (v = getenv("INCLUDE")) != NULL)
		split_list("INCLUDE", v, idirs, &nidir);
	if ((v = getenv("LIB")) != NULL)
		split_list("LIB", v, ldirs, &nldir);
	if (stop != 'S' || ncompile > 0)
		for (i = 0; i < nidir; i++)
			if (ftype(idirs[i]) != 2)
				fprintf(stderr, "%s: warning: header directory %s does not exist\n", me, idirs[i]);
	if (verbose) {
		show_dirs("headers (-I, then INCLUDE)", idirs, nidir);
		show_dirs("libraries (-L, then LIB)", ldirs, nldir);
		fprintf(stderr, "%s: temporary files in %s\n", me, tdir);
	}

	/* link: find the script, the startup file and the libraries before any tool runs */
	if (stop == 0) {
		for (i = 0; i < nldir; i++)
			if (ftype(ldirs[i]) != 2)
				fprintf(stderr, "%s: warning: library directory %s does not exist\n", me, ldirs[i]);
		if ((script == NULL && (script = find_lib("SLOT1.LD")) == NULL) ||
		    (!nostdlib && (crt0 = find_lib("CRT0.O")) == NULL)) {
			fprintf(stderr, "%s: cannot find %s\n", me, script == NULL ? "the link script SLOT1.LD" : "the startup file CRT0.O");
			show_dirs("searched (-L, then LIB)", ldirs, nldir);
			if (getenv("LIB") == NULL)
				fprintf(stderr, "%s: LIB is not set\n", me);
			return E_FIND;
		}
		if (!nostdlib)
			for (k = 0; k < 4; k++)
				if ((libs[k] = find_lib(libname[k])) == NULL) {
					fprintf(stderr, "%s: cannot find the library %s\n", me, libname[k]);
					show_dirs("searched (-L, then LIB)", ldirs, nldir);
					return E_FIND;
				}
		/* -lNAME: libNAME.a in the -L directories, then LIB (what ld's -l does with -L), resolved here so that ld
		 * gets one file name instead of -L options, and a missing library is reported before anything runs */
		for (i = 0; i < nin; i++)
			if (in_kind[i] == K_LIB && (in_obj[i] = find_lib(cat2(cat2("lib", in_name[i] + 2), ".a"))) == NULL) {
				fprintf(stderr, "%s: cannot find %s (lib%s.a)\n", me, in_name[i], in_name[i] + 2);
				show_dirs("searched (-L, then LIB)", ldirs, nldir);
				return E_FIND;
			}
	}

	/* No stale outputs: remove what this command is going to make, before the first tool runs. */
	if (out != NULL)
		rm_if(out);
	else if (stop == 'c' || stop == 'S')
		for (i = 0; i < nin; i++)
			if (in_kind[i] == K_C || in_kind[i] == K_I || (stop == 'c' && in_kind[i] == K_S))
				rm_if(cat2(basepart(in_name[i]), stop == 'c' ? ".O" : ".S"));

	for (i = 0; i < nin; i++) {
		const char *src = in_name[i], *ti = NULL, *ts = NULL, *to = NULL, *fin;
		int kind = in_kind[i];

		if (kind != K_LIB)
			in_obj[i] = src;
		if (kind == K_LIB || kind == K_LINK) {
			if (stop != 0 && kind == K_LINK)
				fprintf(stderr, "%s: warning: %s: linker input not used (no link with -%c)\n", me, src, stop);
			continue;
		}
		if (stop == 'E' && kind != K_C) {
			fprintf(stderr, "%s: warning: %s: not preprocessed (-E applies to .c files)\n", me, src);
			continue;
		}
		if (stop == 'S' && kind == K_S)
			continue;                               /* GCC 1.40: nothing to do for a .s with -S */
		if (stop != 'E') {
			/* a fresh temporary base for this input: Q<job id><serial>; none of its .I .S .O may exist */
			for (;;) {
				char nm[16];
				const char *b;
				if (serial > 0xFF) {
					fprintf(stderr, "%s: no free temporary name in %s\n", me, tdir);
					cleanup();
					return E_TEMP;
				}
				sprintf(nm, "Q%05lX%02X", jid & 0xFFFFFUL, (unsigned int) serial++);
				b = join(tdir, nm);
				ti = cat2(b, ".I");
				ts = cat2(b, ".S");
				to = cat2(b, ".O");
				if (ftype(ti) == 0 && ftype(ts) == 0 && ftype(to) == 0)
					break;
			}
		}
		if (kind == K_C) {                              /* preprocess */
			arg0();
			arg("-nostdinc");
			arg("-undef");
			arg("-D__GNUC__");
			arg("-Di386");
			arg("-D__i386__");
			if (optO)
				arg("-D__OPTIMIZE__");
			for (k = 0; k < ncpp; k++)
				arg(cpp_opt[k]);
			for (k = 0; k < nidir; k++)
				arg(cat2("-I", idirs[k]));
			arg(src);
			fin = stop == 'E' ? out : ti;
			if (fin != NULL)
				arg(fin);
			if (fin == ti)
				temp(ti);
			st = run("CCCP");
			if (st != 0 || (fin != NULL && fsize(fin) <= 0)) {
				if (st > 0 && getenv("INCLUDE") == NULL && !nostdinc)
					fprintf(stderr, "%s: note: INCLUDE is not set (no standard header directory)\n", me);
				return fail("CCCP", st, fin, E_CPP);
			}
			if (stop == 'E')
				continue;
		}
		if (kind == K_C || kind == K_I) {               /* compile */
			fin = stop == 'S' ? (out ? out : cat2(basepart(src), ".S")) : ts;
			arg0();
			arg(kind == K_C ? ti : src);
			arg("-quiet");
			if (optO)
				arg("-O");
			for (k = 0; k < ncc1; k++)
				arg(cc1_opt[k]);
			if (verbose)
				arg("-version");
			arg("-o");
			arg(fin);
			if (fin == ts)
				temp(ts);
			st = run("CC1");
			if (!ok_out(st, fin))
				return fail("CC1", st, fin, E_CC1);
			rm_if(ti);
			if (stop == 'S')
				continue;
		}
		fin = stop == 'c' ? (out ? out : cat2(basepart(src), ".O")) : to;    /* assemble */
		arg0();
		arg("-o");
		arg(fin);
		arg(kind == K_S ? src : ts);
		if (fin == to)
			temp(to);
		st = run("AS");
		if (!ok_out(st, fin))
			return fail("AS", st, fin, E_AS);
		rm_if(ts);
		in_obj[i] = to;
	}
	if (stop != 0) {
		cleanup();
		return 0;
	}

	/* link: LD -N -T SCRIPT -e ENTRY [-u sym] -o OUT CRT0.O INPUTS-IN-ORDER LIBM.A LIBC.A LIBGCC-QD.A LIBQDOS.A
	 * (what sdk/bin/qdos386-gcc links on the host).  When that is more than QDOS/386 passes to a program (15
	 * arguments, 432 bytes), the same files go into a link script made for this run: the script's text followed by
	 * INPUT("CRT0.O" "..." ...), and ld gets "-N -T TEMP/Q...LD -e ENTRY -o OUT" (byte-identical output). */
	{
		const char *files[MAXIN + 8];
		int nfiles = 0, base;
		unsigned int bytes = (unsigned int) strlen(bprefix) + 3;

		if (crt0 != NULL)
			files[nfiles++] = crt0;
		for (i = 0; i < nin; i++)
			files[nfiles++] = in_obj[i];
		if (!nostdlib)
			for (k = 0; k < 4; k++)
				files[nfiles++] = libs[k];
		for (k = 0; k < nfiles; k++)
			files[k] = ldq(files[k]);
		arg0();
		arg("-N");
		arg("-T");
		arg(ldq(script));
		arg("-e");
		arg(entry);
		for (k = 0; k < nld; k++)
			arg(ld_opt[k]);
		arg("-o");
		arg(ldq(out));
		base = ac;
		for (k = 0; k < ac; k++)
			bytes += (unsigned int) strlen(av[k]) + 1;
		for (k = 0; k < nfiles; k++)
			bytes += (unsigned int) strlen(files[k]) + 1;
		if (base + nfiles <= ARGS_MAX && bytes <= PACK_MAX && !acover) {
			for (k = 0; k < nfiles; k++)
				arg(files[k]);
		} else {
			char nm[16];
			const char *ls;
			FILE *in, *lf;
			int c;

			do {
				sprintf(nm, "Q%05lX%02X.LD", jid & 0xFFFFFUL, (unsigned int) serial++);
				ls = join(tdir, nm);
			} while (ftype(ls) != 0 && serial <= 0xFF);
			if (ftype(ls) != 0) {
				fprintf(stderr, "%s: no free temporary name in %s\n", me, tdir);
				cleanup();
				return E_TEMP;
			}
			if ((in = fopen(script, "r")) == NULL) {
				fprintf(stderr, "%s: cannot read the link script %s\n", me, script);
				cleanup();
				return E_FIND;
			}
			temp(ls);
			if ((lf = fopen(ls, "w")) == NULL) {
				fclose(in);
				fprintf(stderr, "%s: cannot write %s\n", me, ls);
				cleanup();
				return E_TEMP;
			}
			while ((c = getc(in)) != EOF)
				putc(c, lf);
			fclose(in);
			fprintf(lf, "\n/* GCC: the inputs of this link, in order */\nINPUT(");
			for (k = 0; k < nfiles; k++)
				fprintf(lf, files[k][0] == '"' ? "%s%s" : "%s\"%s\"", k ? " " : "", files[k]);
			fprintf(lf, ")\n");
			if (fclose(lf) != 0 || fsize(ls) <= 0) {
				fprintf(stderr, "%s: cannot write %s\n", me, ls);
				cleanup();
				return E_TEMP;
			}
			av[2] = ldq(ls);
			if (verbose)
				fprintf(stderr, "%s: %d link inputs are more than one QDOS/386 command line holds: link script %s\n",
				        me, nfiles, ls);
		}
		st = run("LD");
		if (!ok_out(st, out))
			return fail("LD", st, out, E_LD);
	}
	cleanup();
	return 0;
}
