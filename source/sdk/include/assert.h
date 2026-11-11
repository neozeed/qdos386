/* assert.h - QDOS/386 libc (R11D).  C89 assert: a failing test prints the
 * expression, file and line to stderr, then calls abort().  Defining NDEBUG
 * removes the test. */
#ifndef QDOS386_ASSERT_H
#define QDOS386_ASSERT_H

#ifdef NDEBUG
#define assert(e) ((void) 0)
#else
void __qd_assert(const char *expr, const char *file, int line);
#define assert(e) ((e) ? (void) 0 : __qd_assert(#e, __FILE__, __LINE__))
#endif

#endif
