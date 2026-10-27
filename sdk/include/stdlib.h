/* stdlib.h - QDOS/386 libc (R9): memory, conversion and exit.
 * malloc and friends are the QDOS kernel heap; see docs/R9-LIBC-ABI.md. */
#ifndef QDOS386_STDLIB_H
#define QDOS386_STDLIB_H
#include <stddef.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767
void *malloc(size_t n);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
void exit(int status);
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
