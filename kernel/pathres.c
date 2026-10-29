/*
 * QDOS/386 R12B - the program search.  See pathres.h.  Strict C89, no kernel dependencies.
 */
#include "pathres.h"

unsigned long path_searches, path_candidates;

static int go_on(int rc)
{
	return rc == PATH_ERR_NOFILE || rc == PATH_ERR_NOTDIR || rc == PATH_ERR_ISDIR || rc == PATH_ERR_NAME ||
	       rc == PATH_ERR_RANGE || rc == PATH_ERR_DEPTH;
}

/* dir (dlen bytes; 0 = as given) + '/' + name [+ ".AOU"] */
static int try1(const char *dir, unsigned int dlen, const char *name, int suffix, path_open_fn open, void *ctx,
                char *cand)
{
	unsigned int k = 0, i;
	const char *s = ".AOU";

	for (i = 0; i < dlen; i++) {
		if (k + 1 >= PATH_CAND_MAX)
			return PATH_ERR_DEPTH;
		cand[k++] = dir[i];
	}
	if (dlen > 0 && dir[dlen - 1] != '/' && dir[dlen - 1] != ':') {
		if (k + 1 >= PATH_CAND_MAX)
			return PATH_ERR_DEPTH;
		cand[k++] = '/';
	}
	for (i = 0; name[i]; i++) {
		if (k + 1 >= PATH_CAND_MAX)
			return PATH_ERR_DEPTH;
		cand[k++] = name[i];
	}
	for (i = 0; suffix && s[i]; i++) {
		if (k + 1 >= PATH_CAND_MAX)
			return PATH_ERR_DEPTH;
		cand[k++] = s[i];
	}
	cand[k] = 0;
	path_candidates++;
	return open(ctx, cand);
}

static int place(const char *dir, unsigned int dlen, const char *name, int dotless, path_open_fn open, void *ctx,
                 char *cand)
{
	int rc = try1(dir, dlen, name, 0, open, ctx, cand), r2;

	if (rc == PATH_OK || !go_on(rc) || !dotless)
		return rc;
	r2 = try1(dir, dlen, name, 1, open, ctx, cand);
	if (r2 == PATH_ERR_NOFILE && (rc == PATH_ERR_NAME || rc == PATH_ERR_RANGE))
		return rc;                       /* keep the more telling reason */
	return r2;
}

int path_resolve(const char *name, const char *path, unsigned int plen, path_open_fn open, void *ctx, char *cand)
{
	unsigned int i, n, s, e;
	int direct = 0, dotless = 1, rc, first;

	path_searches++;
	for (n = 0; name[n]; n++) {
		if (name[n] == '/' || name[n] == ':') {
			direct = 1;
			dotless = 1;
		} else if (name[n] == '.')
			dotless = 0;
	}
	if (n >= 5 && (name[0] == 'W' || name[0] == 'w') && (name[1] == 'I' || name[1] == 'i') &&
	    (name[2] == 'N' || name[2] == 'n') && name[3] == '1' && (name[4] == '_' || name[4] == ':'))
		direct = 1;
	if (n == 0)
		return PATH_ERR_NAME;
	first = place(0, 0, name, dotless, open, ctx, cand);
	if (first == PATH_OK || !go_on(first) || direct)
		return first;
	for (s = 0; path != 0 && s < plen; s = e + 1) {
		e = s;
		while (e < plen && path[e] != ';')
			e++;
		i = s;
		while (i < e && path[i] == ' ')
			i++;
		n = e;
		while (n > i && path[n - 1] == ' ')
			n--;
		if (n == i)
			continue;
		rc = place(path + i, n - i, name, dotless, open, ctx, cand);
		if (rc == PATH_OK || !go_on(rc))
			return rc;
	}
	rc = place("/", 1, name, dotless, open, ctx, cand);
	if (rc == PATH_OK || !go_on(rc))
		return rc;
	return (first == PATH_ERR_NAME || first == PATH_ERR_RANGE) ? first : PATH_ERR_NOFILE;
}
