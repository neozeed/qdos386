/* Native host declarations; object records are serialized separately. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

void as_warn(const char *, ...);
void as_bad(const char *, ...);
void as_fatal(const char *, ...);
void md_number_to_chars(char *, long, int);
#if defined(SPARC) || defined(I860)
struct fix;
void md_number_to_imm(char *, long, int, struct fix *, int);
#else
void md_number_to_imm(char *, long, int);
#endif
void md_number_to_disp(char *, long, int);
#if defined(I386) || defined(VAX)
void md_number_to_field(char *, long, int);
#elif defined(NS32K)
struct bit_fix;
void md_number_to_field(char *, long, struct bit_fix *);
#else
void md_number_to_field(char *, long, void *);
#endif

char *xmalloc(size_t), *xrealloc(char *, size_t);

long get_absolute_expression(void);
