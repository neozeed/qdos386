/*
 * QDOS/386 R12B - the program search (one resolver for every load).  See docs/R12B-PATH-STARTUP-ABI.md.
 *
 * path_resolve(name, path, plen, open, ctx, cand) tries candidate file names with open(ctx, candidate) until one opens:
 *   - a name holding '/' or ':' (or a WIN1_ / WIN1: channel name) is tried only as given (relative names are relative to
 *     the caller's directory: the opener decides that);
 *   - a bare name is tried as given (the caller's directory), then in each entry of path (plen bytes, ';'-separated, in
 *     order; blanks around an entry and empty entries are skipped), then in the root "/";
 *   - in each place the exact name is tried first and then, when its last component has no '.', name + ".AOU".
 * The search goes on after PATH_GO_ON codes (no such file, not a directory, is a directory, a candidate name the volume
 * cannot hold, a candidate over PATH_CAND_MAX) and stops at the first success or any other error (I/O, chain, no
 * volume), which is returned.  When nothing is found the result is "no such file", or the name error of the name as
 * given when that was the reason.  The first file found is the one used: a malformed image is not skipped.
 * Pure C89 (tests/host/r12b/test_pathres.c).
 */
#ifndef QDOS386_PATHRES_H
#define QDOS386_PATHRES_H

#define PATH_CAND_MAX 128u

#define PATH_OK          0
#define PATH_ERR_NOFILE (-44)
#define PATH_ERR_NAME   (-46)
#define PATH_ERR_RANGE  (-47)
#define PATH_ERR_NOTDIR (-87)
#define PATH_ERR_ISDIR  (-88)
#define PATH_ERR_DEPTH  (-90)

typedef int (*path_open_fn)(void *ctx, const char *candidate);

/* cand: a PATH_CAND_MAX buffer that holds the candidate tried last (the one opened on success). */
int path_resolve(const char *name, const char *path, unsigned int plen, path_open_fn open, void *ctx, char *cand);

extern unsigned long path_searches, path_candidates;

#endif
