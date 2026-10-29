/* R12B shell: the pure text rules (no traps), shared by the shell (shell_env.inc) and the host test
 * (tests/host/r12b/test_shparse.c).  Strict C89.  See docs/R12B-ENV-PROCESS-ABI.md section 5 and
 * docs/R12B-PATH-STARTUP-ABI.md section 3. */

static int env_namec(int c, int first)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || (!first && c >= '0' && c <= '9');
}

/* Expand %NAME% in in[] into out[] (capacity cap, NUL included).  One pass: an expanded value is not scanned again.
 * %% gives one %.  Nothing between double quotes is expanded (the quotes are copied).  A % that does not start %NAME%
 * (NAME = 1-31 name characters, the first not a digit, followed by %) is copied as it is.  lookup(name, buf, size)
 * returns the value length (the copy NUL-terminated in buf) or a negative code for an unset NAME, which expands to
 * nothing.  Returns 0, or -1 when the result does not fit (out is then not usable). */
static int sh_expand(const char *in, char *out, unsigned int cap,
                     int (*lookup)(const char *name, char *buf, unsigned int size))
{
	static char val[260];
	unsigned int o = 0, n, k;
	int q = 0, r;
	const char *p = in;
	char name[32];

	while (*p) {
		if (*p == '"')
			q = !q;
		if (!q && *p == '%') {
			if (p[1] == '%') {
				if (o + 1 >= cap)
					return -1;
				out[o++] = '%';
				p += 2;
				continue;
			}
			for (n = 0; n < 31 && env_namec((unsigned char) p[1 + n], n == 0); n++)
				name[n] = p[1 + n];
			if (n > 0 && p[1 + n] == '%') {
				name[n] = 0;
				r = lookup(name, val, sizeof val);
				if (r > (int) sizeof val - 1)
					r = (int) sizeof val - 1;
				if (r > 0) {
					if (o + (unsigned int) r >= cap)
						return -1;
					for (k = 0; k < (unsigned int) r; k++)
						out[o++] = val[k];
				}
				p += n + 2;
				continue;
			}
		}
		if (o + 1 >= cap)
			return -1;
		out[o++] = *p++;
	}
	out[o] = 0;
	return 0;
}

/* Script text: TAB becomes a space; returns n when every byte is printable, CR, LF (or TAB), else the offset of the
 * first other control byte (the script is then not run at all). */
static unsigned int sc_prepare(char *b, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char) b[i];
		if (c == '\t')
			b[i] = ' ';
		else if (c < 32 && c != '\r' && c != '\n')
			return i;
	}
	return n;
}

/* The next line of b[0..n) from *pos into lb (capacity cap): LF or CR LF ends a line, the last line may lack it.
 * 1 = a line (++*lineno), 0 = no more text, -1 = the line has cap or more characters (++*lineno; not copied). */
static int sc_line(const char *b, unsigned int n, unsigned int *pos, char *lb, unsigned int cap, int *lineno)
{
	unsigned int s = *pos, e, k, i;

	if (s >= n)
		return 0;
	e = s;
	while (e < n && b[e] != '\n')
		e++;
	k = e;
	if (k > s && b[k - 1] == '\r')
		k--;
	*pos = e + 1;
	(*lineno)++;
	if (k - s > cap - 1)
		return -1;
	for (i = 0; i < k - s; i++)
		lb[i] = b[s + i];
	lb[i] = 0;
	return 1;
}

/* A blank line or a REM line (REM, any case, alone or followed by a space) is skipped. */
static int sc_skip(const char *lb)
{
	while (*lb == ' ')
		lb++;
	if (*lb == 0)
		return 1;
	return (lb[0] == 'R' || lb[0] == 'r') && (lb[1] == 'E' || lb[1] == 'e') && (lb[2] == 'M' || lb[2] == 'm') &&
	       (lb[3] == 0 || lb[3] == ' ');
}
