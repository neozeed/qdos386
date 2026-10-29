/* stdlib.h - QDOS/386 libc (R9): memory, conversion and exit.
 * malloc and friends are the QDOS kernel heap; see docs/R9-LIBC-ABI.md. */
#ifndef QDOS386_STDLIB_H
#define QDOS386_STDLIB_H
#include <stddef.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767
#define MB_CUR_MAX 1

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
void *malloc(size_t n);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
void exit(int status);
void abort(void);
int atexit(void (*fn)(void));
int system(const char *cmd);
char *getenv(const char *name);
/* R12B extensions (not C89): change the calling process's environment.  0 on success, -1 on failure (errno is not
 * set).  A child started later gets a copy; pointers from getenv may be invalid after a change. */
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int rand(void);
void srand(unsigned int seed);
div_t div(int num, int den);
ldiv_t ldiv(long num, long den);
/* Multibyte: the C locale only, one byte is one character.  No wchar_t exists,
 * so the wide-character conversions are not provided (docs/R11D-C89-COVERAGE.md). */
int mblen(const char *s, size_t n);
int abs(int n);
long labs(long n);
int atoi(const char *s);
long atol(const char *s);
long strtol(const char *s, char **end, int base);
void qsort(void *base, size_t nmemb, size_t size, int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*cmp)(const void *, const void *));
unsigned long strtoul(const char *s, char **end, int base);
double strtod(const char *s, char **end);
double atof(const char *s);
#endif
