/*
 * QDOS/386 R11M - shell commands for directories and files (included by shell.c with -DR11M_SHELL).
 *
 * Paths are native paths: "SRC/HELLO.C", "/BIN", "..", "WIN1:/SRC" - the shell hands "WIN1:" + the text to
 * the kernel, which reads it relative to this job's current directory and folds case.  Commands:
 *   PWD   CD [path]   MD path   RD path   DIR [path]   TYPE path   DEL path   REN old new
 *   COPY src dst [/Y]   (dst may be a directory; an existing file is refused unless /Y)
 * RUN looks in the current directory, then in /BIN.
 */

#define FS_REMOVE 1
#define FS_RENAME 2
#define FS_MKDIR  6
#define FS_RMDIR  7
#define FS_CHDIR  8
#define FS_GETCWD 9
#define FS_STAT   10
#define OPEN_READ   0x01u
#define OPEN_WRITE  0x02u
#define OPEN_CREATE 0x04u
#define OPEN_TRUNC  0x08u
#define OPEN_EXCL   0x20u
#define OPEN_DIRS   0x40u
#define SH_PATH_ARG 64                 /* longest path the kernel accepts */
#define SH_NATIVE_MAX 72               /* "WIN1:" + path + NUL */

static char npath[SH_NATIVE_MAX];      /* native channel name built by native() */
static char npath2[SH_NATIVE_MAX];
static char cwdbuf[80];
static unsigned char cpbuf[256];
static unsigned char statbuf[8];

/* Quoting, for names with blanks (VFAT long names): a double-quoted run may contain blanks.  tok() returns the next
 * blank-separated word of *pp (quotes kept, terminated in place); uq() removes the quotes from a word in place. */
static char *tok(char **pp)
{
	char *p = *pp, *s;
	int q = 0;
	while (*p == ' ')
		p++;
	s = p;
	while (*p && (q || *p != ' ')) {
		if (*p == '"')
			q = !q;
		p++;
	}
	if (*p)
		*p++ = 0;
	*pp = p;
	return s;
}

static char *uq(char *s)
{
	char *r = s, *w = s;
	for (; *r; r++)
		if (*r != '"')
			*w++ = *r;
	*w = 0;
	return s;
}

/* Is arg already a channel name ("WIN1_NAME" legacy flat, "WIN1:path" native, either case)? */
static int has_volume_prefix(const char *a)
{
	unsigned int i;
	const char *p = "WIN1";
	for (i = 0; i < 4; i++) {
		char c = a[i];
		if (c >= 'a' && c <= 'z')
			c = (char) (c - 'a' + 'A');
		if (c != p[i])
			return 0;
	}
	return a[4] == '_' || a[4] == ':';
}

/* Build the channel name for a shell argument into dst: "WIN1:" + arg for a path, the argument itself
 * when it already carries a volume prefix.  0, or -1 if the path is too long. */
static int native_to(char *dst, const char *arg)
{
	unsigned int k = 0, i;
	const char *p = "WIN1:";
	if (slen(arg) > SH_PATH_ARG)
		return -1;
	if (has_volume_prefix(arg)) {
		for (i = 0; arg[i]; i++)
			dst[k++] = arg[i];
		dst[k] = 0;
		return 0;
	}
	for (i = 0; p[i]; i++)
		dst[k++] = p[i];
	for (i = 0; arg[i]; i++)
		dst[k++] = arg[i];
	dst[k] = 0;
	return 0;
}

static void say_fs_error(int rc)
{
	switch (rc) {
	case -44: outs("no such file or directory\n"); break;
	case -85: outs("write failed\n"); break;
	case -87: outs("not a directory\n"); break;
	case -88: outs("is a directory\n"); break;
	case -89: outs("directory not empty\n"); break;
	case -90: outs("path too long or too deep (64 characters, 16 levels)\n"); break;
	case -91: outs("not allowed on that name\n"); break;
	case -92: outs("a directory cannot be moved to another parent\n"); break;
	case -83: outs("already exists\n"); break;
	case -84: outs("in use (open elsewhere, or a job's current directory)\n"); break;
	case -81: outs("disk full\n"); break;
	case -82: outs("directory full\n"); break;
	case -80: outs("volume is write-protected\n"); break;
	case -46: outs("invalid name\n"); break;
	case -47: outs("invalid name\n"); break;
	case -43: outs("bad cluster chain\n"); break;
	case -15: outs("not supported by this kernel (no writable volume)\n"); break;
	case -1:  outs("not supported by this kernel\n"); break;
	default:
		say_name_error(rc);
	}
}

static int get_cwd(void)
{
	int r = (int) qtrap(APP_QT_FS_OP, FS_GETCWD, (unsigned int) cwdbuf, sizeof cwdbuf);
	if (r < 0) {
		cwdbuf[0] = '/';
		cwdbuf[1] = 0;
	}
	return r;
}

static void cmd_pwd(void)
{
	get_cwd();
	outs(cwdbuf);
	outc('\n');
}

static void cmd_cd(char *arg)
{
	int rc;
	if (*arg == 0) {
		cmd_pwd();
		return;
	}
	if (native_to(npath, arg) != 0) {
		say_fs_error(-90);
		return;
	}
	rc = (int) qtrap(APP_QT_FS_OP, FS_CHDIR, (unsigned int) npath, 0);
	if (rc < 0)
		say_fs_error(rc);
}

static void cmd_name_op(unsigned int op, char *arg, const char *usage)
{
	int rc;
	if (*arg == 0) {
		outs(usage);
		return;
	}
	if (native_to(npath, arg) != 0) {
		say_fs_error(-90);
		return;
	}
	rc = (int) qtrap(APP_QT_FS_OP, op, (unsigned int) npath, 0);
	if (rc < 0)
		say_fs_error(rc);
}

static void cmd_ren(char *arg)
{
	char *p = arg, *a, *b;
	int rc;
	a = tok(&p);
	b = tok(&p);
	if (*a == 0 || *b == 0) {
		outs("usage: REN <old> <new>   (use \"double quotes\" around a name with blanks)\n");
		return;
	}
	uq(a);
	uq(b);
	if (native_to(npath, a) != 0 || native_to(npath2, b) != 0) {
		say_fs_error(-90);
		return;
	}
	rc = (int) qtrap(APP_QT_FS_OP, FS_RENAME, (unsigned int) npath, (unsigned int) npath2);
	if (rc < 0)
		say_fs_error(rc);
}

static void cmd_del(char *arg)
{
	cmd_name_op(FS_REMOVE, arg, "usage: DEL <file>\n");
}

static void cmd_dir_m(char *arg)
{
	int ch, n, i, files = 0, dirs = 0;
	unsigned int c;

	if (*arg == 0) {
		get_cwd();
		outs(" Directory of ");
		outs(cwdbuf);
		outc('\n');
		npath[0] = 0;
		native_to(npath, "");
	} else {
		outs(" Directory of ");
		outs(arg);
		outc('\n');
		if (native_to(npath, arg) != 0) {
			say_fs_error(-90);
			return;
		}
	}
	ch = (int) qtrap(APP_QT_CH_OPENX, (unsigned int) npath, OPEN_READ | OPEN_DIRS, 0);
	if (ch <= 0) {
		say_fs_error(ch);
		return;
	}
	/* a kernel with VFAT long names answers CTL_LISTLONG: 96-byte records with the long name and the alias */
	if ((int) qtrap(APP_QT_CH_CTL, (unsigned int) ch, 5, 0) == 0) {
		for (;;) {
			n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) recbuf, sizeof recbuf);
			if (n <= 0)
				break;
			for (i = 0; i + 96 <= n; i += 96) {
				unsigned long size;
				unsigned int col = 0;
				const unsigned char *al = recbuf + i + 64;
				const unsigned char *nm = recbuf + i;
				for (c = 0; c < 12 && al[c] != 0; c++) {
					outc((char) al[c]);
					col++;
				}
				while (col < 14) {
					outc(' ');
					col++;
				}
				size = (unsigned long) recbuf[i + 80] | ((unsigned long) recbuf[i + 81] << 8) |
				       ((unsigned long) recbuf[i + 82] << 16) | ((unsigned long) recbuf[i + 83] << 24);
				if (size == 0xffffffffUL) {
					outs("<DIR>");
					dirs++;
				} else {
					outnum((long) size);
					files++;
				}
				if (recbuf[i + 88] == 1) {            /* a long name: shown after the alias and size */
					outs("  ");
					for (c = 0; c < 63 && nm[c] != 0; c++)
						outc((char) nm[c]);
				} else if (recbuf[i + 88] == 2) {
					outs("  (long name not supported here)");
				}
				outc('\n');
			}
		}
		qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
		if (n < 0)
			say_fs_error(n);
		outnum(files);
		outs(files == 1 ? " file, " : " files, ");
		outnum(dirs);
		outs(dirs == 1 ? " directory\n" : " directories\n");
		return;
	}
	for (;;) {
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) recbuf, sizeof recbuf);
		if (n <= 0)
			break;
		for (i = 0; i + SH_REC <= n; i += SH_REC) {
			unsigned long size;
			unsigned int col = 0;
			for (c = 0; c < 12 && recbuf[i + c] != 0; c++) {
				outc((char) recbuf[i + c]);
				col++;
			}
			while (col < 14) {
				outc(' ');
				col++;
			}
			size = (unsigned long) recbuf[i + 12] | ((unsigned long) recbuf[i + 13] << 8) |
			       ((unsigned long) recbuf[i + 14] << 16) | ((unsigned long) recbuf[i + 15] << 24);
			if (size == 0xffffffffUL) {
				outs("<DIR>");
				dirs++;
			} else {
				outnum((long) size);
				files++;
			}
			outc('\n');
		}
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	if (n < 0)
		say_fs_error(n);
	outnum(files);
	outs(files == 1 ? " file, " : " files, ");
	outnum(dirs);
	outs(dirs == 1 ? " directory\n" : " directories\n");
}

static void cmd_type_m(char *arg)
{
	int ch, n;
	if (*arg == 0) {
		outs("usage: TYPE <file>\n");
		return;
	}
	if (native_to(npath, arg) != 0) {
		say_fs_error(-90);
		return;
	}
	ch = (int) qtrap(APP_QT_CH_OPEN, (unsigned int) npath, 0, 0);
	if (ch <= 0) {
		say_fs_error(ch);
		return;
	}
	n = (int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) npath, (unsigned int) statbuf);
	if (n == 1) {                      /* a directory: its listing is not text */
		qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
		outs("is a directory\n");
		return;
	}
	for (;;) {
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) ch, (unsigned int) tbuf, sizeof tbuf);
		if (n <= 0)
			break;
		out(tbuf, (unsigned int) n);
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0);
	if (n < 0)
		say_fs_error(n);
}

/* COPY src dst [/Y] */
static void cmd_copy(char *arg)
{
	char *src, *dst, *opt = 0, *p = arg;
	int sch, dch, n, rc, yes = 0, st;
	unsigned long total = 0;
	unsigned int i, k;
	const char *base;

	src = tok(&p);
	dst = tok(&p);
	while (*p == ' ')
		p++;
	if (*p)
		opt = tok(&p);
	if (*src == 0 || *dst == 0) {
		outs("usage: COPY <source> <destination> [/Y]   (/Y: replace an existing destination; \"quotes\" for blanks)\n");
		return;
	}
	uq(src);
	uq(dst);
	if (opt != 0) {
		fold(opt);
		if (same(opt, "/Y"))
			yes = 1;
		else {
			outs("unknown option (only /Y)\n");
			return;
		}
	}
	if (native_to(npath, src) != 0 || native_to(npath2, dst) != 0) {
		say_fs_error(-90);
		return;
	}
	st = (int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) npath, (unsigned int) statbuf);
	if (st < 0) {
		say_fs_error(st);
		return;
	}
	if (st == 1) {
		outs("cannot copy a directory\n");
		return;
	}
	/* destination a directory: copy into it under the source's own name */
	st = (int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) npath2, (unsigned int) statbuf);
	if (st == 1) {
		base = src;
		for (i = 0; src[i]; i++)
			if (src[i] == '/')
				base = src + i + 1;
		if (*base == 0) {
			outs("invalid source name\n");
			return;
		}
		k = slen(npath2);
		if (npath2[k - 1] != '/' && npath2[k - 1] != ':') {
			if (k + 1 >= SH_NATIVE_MAX) {
				say_fs_error(-90);
				return;
			}
			npath2[k++] = '/';
		}
		for (i = 0; base[i]; i++) {
			if (k + 1 >= SH_NATIVE_MAX) {
				say_fs_error(-90);
				return;
			}
			npath2[k++] = base[i];
		}
		npath2[k] = 0;
		st = (int) qtrap(APP_QT_FS_OP, FS_STAT, (unsigned int) npath2, (unsigned int) statbuf);
		if (st == 1) {
			outs("destination is a directory\n");
			return;
		}
	}
	if (st == 0 && !yes) {
		outs("destination exists (nothing copied; COPY ... /Y replaces it)\n");
		return;
	}
	if (st < 0 && st != -44) {
		say_fs_error(st);
		return;
	}
	sch = (int) qtrap(APP_QT_CH_OPENX, (unsigned int) npath, OPEN_READ, 0);
	if (sch <= 0) {
		say_fs_error(sch);
		return;
	}
	dch = (int) qtrap(APP_QT_CH_OPENX, (unsigned int) npath2,
	                  OPEN_WRITE | OPEN_CREATE | (yes ? OPEN_TRUNC : OPEN_EXCL), 0);
	if (dch <= 0) {
		qtrap(APP_QT_CH_CLOSE, (unsigned int) sch, 0, 0);
		if (dch == -84)
			outs("source and destination are the same file, or the destination is open\n");
		else
			say_fs_error(dch);
		return;
	}
	rc = 0;
	for (;;) {
		n = (int) qtrap(APP_QT_CH_READ, (unsigned int) sch, (unsigned int) cpbuf, sizeof cpbuf);
		if (n < 0) {
			rc = n;
			break;
		}
		if (n == 0)
			break;
		k = (unsigned int) qtrap(APP_QT_CH_WRITE, (unsigned int) dch, (unsigned int) cpbuf, (unsigned int) n);
		if ((int) k != n) {
			rc = ((int) k < 0) ? (int) k : -81;
			break;
		}
		total += (unsigned long) n;
	}
	qtrap(APP_QT_CH_CLOSE, (unsigned int) sch, 0, 0);
	i = (unsigned int) qtrap(APP_QT_CH_CLOSE, (unsigned int) dch, 0, 0);
	if (rc == 0 && (int) i < 0)
		rc = (int) i;
	if (rc != 0) {
		say_fs_error(rc);
		if (!yes) {
			qtrap(APP_QT_FS_OP, FS_REMOVE, (unsigned int) npath2, 0);
			outs("copy failed; the partial destination was removed\n");
		} else {
			outs("copy failed; the destination was replaced and is incomplete\n");
		}
		return;
	}
	outnum((long) total);
	outs(" bytes copied\n");
}
