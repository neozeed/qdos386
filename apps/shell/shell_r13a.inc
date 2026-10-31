/*
 * shell_r13a.inc - R13A shell commands (-DR13A_SHELL): fonts and rendered text consoles (docs/R13A-GRAPHICS-ABI.md).
 *   FONT                       list the registered fonts
 *   FONT path [/CON|/SURF]     load a QFNT file and make it the default (both, or only consoles / surface text)
 *   FONT n [/CON|/SURF]        make registered font n the default
 *   FONT /FREE                 release every font this shell loaded (a font stays while a default, a session or a
 *                              surface still uses it)
 *   SET FONT=path              the same as FONT path (so a startup script can choose the font at every boot)
 *   CONSOLE                    show this session's console mode
 *   CONSOLE GFX|VGA [/ALL]     rendered text (640x480x16, the font) or the VGA's text mode; /ALL: every session
 *                              and the default for new ones
 */
#define APP_QT_SURFACE13A 0x3Bu
static unsigned int fbuf[16], flist[8];

static int sur(unsigned int op, unsigned int b, unsigned int c)
{
	return (int) qtrap(APP_QT_SURFACE13A, op, b, c);
}

static void font_table(void)
{
	int n = sur(27u, (unsigned int) flist, 8u), i, k;

	if (n < 0) {
		outs("this kernel has no loadable fonts\n");
		return;
	}
	outs("ID        GLYPH CELL  GLYPHS REFS FLAGS     NAME\n");
	for (i = 0; i < n; i++) {
		char nm[21];

		if (sur(25u, flist[i], (unsigned int) fbuf) != 0)
			continue;
		out_pad("", 0);
		outnum((long) fbuf[1]);
		outs(fbuf[1] < 10 ? "         " : fbuf[1] < 1000 ? "       " : "     ");
		outnum((long) fbuf[2]);
		outc('x');
		outnum((long) fbuf[3]);
		outs(fbuf[3] < 10 ? "   " : "  ");
		outnum((long) fbuf[4]);
		outc('x');
		outnum((long) fbuf[5]);
		outs(fbuf[5] < 10 ? "  " : " ");
		outnum((long) fbuf[6]);
		outs(fbuf[6] < 10 ? "      " : fbuf[6] < 100 ? "     " : "    ");
		outnum((long) fbuf[8]);
		outs("    ");
		outs((fbuf[9] & 1u) ? "built-in " : "loaded   ");
		outs((fbuf[9] & 4u) ? "S" : "-");
		outs((fbuf[9] & 8u) ? "C " : "- ");
		for (k = 0; k < 20; k++)
			nm[k] = (char) ((fbuf[11 + k / 4] >> (8 * (k % 4))) & 0xffu);
		nm[20] = 0;
		outs(nm);
		outc('\n');
	}
	outs("FLAGS: S default for surface text, C default for rendered consoles\n");
}

static void font_err(int rc)
{
	if (rc == -4)
		outs("not a valid QFNT v1 font file (the previous font stays)\n");
	else if (rc == -5)
		outs("font table full\n");
	else if (rc == -3)
		outs("no kernel memory for the font\n");
	else if (rc == -2)
		outs("no such font\n");
	else {
		outs("cannot load the font file (error ");
		outnum(rc);
		outs(")\n");
	}
}

/* Make font id the default: which 0 both, 1 consoles, 2 surface text. */
static int font_use(long id, int which)
{
	int rc = 0;

	if (which != 2)
		rc = sur(26u, 1u, (unsigned int) id);
	if (rc == 0 && which != 1)
		rc = sur(26u, 0u, (unsigned int) id);
	return rc;
}

static void font_path(char *path, int which)
{
	int id = sur(36u, (unsigned int) path, 0u), rc;

	if (id < 0) {
		font_err(id);
		return;
	}
	rc = font_use(id, which);
	if (rc != 0) {
		font_err(rc);
		return;
	}
	outs("font ");
	outnum(id);
	outs(" loaded and in use\n");
}

static void cmd_font(char *arg)
{
	char *p = arg, *opt;
	int which = 0, rc;

	if (*arg == 0) {
		font_table();
		return;
	}
	if (same(arg, "/FREE")) {
		int n = sur(27u, (unsigned int) flist, 8u), i, k = 0;

		for (i = 0; i < n; i++)
			if (sur(25u, flist[i], (unsigned int) fbuf) == 0 && (fbuf[9] & 2u) && sur(23u, flist[i], 0u) == 0)
				k++;
		outnum(k);
		outs(" font(s) released\n");
		return;
	}
	while (*p && *p != ' ')
		p++;
	opt = p;
	while (*opt == ' ')
		opt++;
	*p = 0;
	if (*opt) {
		if (same(opt, "/CON"))
			which = 1;
		else if (same(opt, "/SURF"))
			which = 2;
		else {
			outs("usage: FONT [path|n] [/CON|/SURF]\n");
			return;
		}
	}
	if (arg[0] >= '0' && arg[0] <= '9') {
		long id = 0;
		for (p = arg; *p >= '0' && *p <= '9'; p++)
			id = id * 10 + (*p - '0');
		rc = font_use(id, which);
		if (rc != 0)
			font_err(rc);
		return;
	}
	font_path(arg, which);
}

static void font_env_hook(const char *name, const char *value)
{
	char nm[8], v[130];
	unsigned int i;

	for (i = 0; i < 5 && name[i]; i++)
		nm[i] = (char) (name[i] >= 'a' && name[i] <= 'z' ? name[i] - 32 : name[i]);
	nm[i] = 0;
	if (i != 4 || name[4] != 0 || nm[0] != 'F' || nm[1] != 'O' || nm[2] != 'N' || nm[3] != 'T' || value[0] == 0)
		return;
	for (i = 0; i < 129 && value[i]; i++)
		v[i] = value[i];
	v[i] = 0;
	font_path(v, 0);
}

static void cmd_console(char *arg)
{
	char *p = arg, *opt;
	int all = 0, rc;
	unsigned int m;

	if (*arg == 0) {
		rc = sur(28u, 0xFFFFFFFFu, 0u);
		outs(rc == 1 ? "this session: rendered text (CONSOLE GFX)\n" : rc == 0 ? "this session: VGA text mode (CONSOLE VGA)\n"
		                                                            : "this kernel has no rendered consoles\n");
		return;
	}
	while (*p && *p != ' ')
		p++;
	opt = p;
	while (*opt == ' ')
		opt++;
	*p = 0;
	if (*opt && same(opt, "/ALL"))
		all = 1;
	else if (*opt) {
		outs("usage: CONSOLE [GFX|VGA] [/ALL]\n");
		return;
	}
	if (same(arg, "GFX"))
		m = 1u;
	else if (same(arg, "VGA"))
		m = 0u;
	else {
		outs("usage: CONSOLE [GFX|VGA] [/ALL]\n");
		return;
	}
	rc = sur(28u, m, all ? 1u : 0u);
	if (rc == -7)
		outs("no rendered text on this display: VGA text mode stays\n");
	else if (rc < 0) {
		outs("this kernel has no rendered consoles\n");
	}
}
