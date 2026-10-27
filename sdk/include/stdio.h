/* stdio.h - QDOS/386 libc (R9): a bounded subset.  See docs/R9-LIBC-ABI.md
 * for what is and is not provided.  Only read-only files (WIN1_ volume). */
#ifndef QDOS386_STDIO_H
#define QDOS386_STDIO_H
#include <stddef.h>
#include <stdarg.h>
#define EOF (-1)
#define BUFSIZ 256
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define FILENAME_MAX 16
typedef struct qd_file FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
int putchar(int c);
int puts(const char *s);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);
/* snprintf and vsnprintf are C99, not C89: a bounded extension of this SDK. */
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int getchar(void);
int fgetc(FILE *fp);
int getc(FILE *fp);
int ungetc(int c, FILE *fp);
char *fgets(char *buf, int n, FILE *fp);
FILE *fopen(const char *name, const char *mode);
int fclose(FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
int fseek(FILE *fp, long off, int whence);
long ftell(FILE *fp);
int feof(FILE *fp);
int ferror(FILE *fp);
void clearerr(FILE *fp);
#endif
