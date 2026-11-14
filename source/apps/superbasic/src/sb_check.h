/*
 * sb_check.h - syntax check of a typed (direct) line, run before any of the
 * line is executed.  Verified against the ROM: a malformed statement stops
 * the whole typed line, so a valid statement before it does not run.
 */
#ifndef SB_CHECK_H
#define SB_CHECK_H

#include <stddef.h>

/* Returns SB_OK if the text is a well-formed sequence of statements, or
 * SB_E_EXPRESSION if any part of it is not.  Values are not computed, so
 * this never reports runtime errors (overflow, undefined variables, string
 * conversion). */
int sb_check_line(const char *src, size_t len);

#endif /* SB_CHECK_H */
