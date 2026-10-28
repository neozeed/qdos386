/* stdio.h - QDOS/386 libc (R11B): C89 stream I/O over the QDOS channels.
 * See docs/R11B-STDIO-ABI.md for the exact behaviour and the deviations.
 * Writable streams are the console only; files on the WIN1_ volume are read
 * only (the FAT16 volume has no write path). */
#ifndef QDOS386_STDIO_H
#define QDOS386_STDIO_H
#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)
#define BUFSIZ 256
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define FILENAME_MAX 18
#define TMP_MAX 25
#define L_tmpnam 16

/* setvbuf modes (C89). */
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

typedef struct qd_file FILE;
typedef long fpos_t;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Output. */
int putchar(int c);
int puts(const char *s);
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int fputs(const char *s, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int fprintf(FILE *fp, const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);
/* snprintf and vsnprintf are C99, not C89: a bounded extension of this SDK. */
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int fflush(FILE *fp);
int setvbuf(FILE *fp, char *buf, int mode, size_t size);
void setbuf(FILE *fp, char *buf);

/* Input. */
int getchar(void);
int fgetc(FILE *fp);
int getc(FILE *fp);
int ungetc(int c, FILE *fp);
char *fgets(char *buf, int n, FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);

/* Files and positioning. */
FILE *fopen(const char *name, const char *mode);
int fclose(FILE *fp);
int fileno(FILE *fp);                       /* R11L: POSIX view, see sdk/libc/posixio.c */
FILE *fdopen(int fd, const char *mode);    /* R11L */
int fseek(FILE *fp, long off, int whence);
long ftell(FILE *fp);
int feof(FILE *fp);
int ferror(FILE *fp);
void clearerr(FILE *fp);
void rewind(FILE *fp);

/* Positioning by fpos_t. */
int fgetpos(FILE *fp, fpos_t *pos);
int fsetpos(FILE *fp, const fpos_t *pos);

/* Input with a format.  scanf reads stdin; sscanf reads a string; vsscanf is a
 * C89 va_list form of sscanf (C99 has the same name; behaviour here is C89). */
int scanf(const char *fmt, ...);
int fscanf(FILE *fp, const char *fmt, ...);
int sscanf(const char *s, const char *fmt, ...);
int vsscanf(const char *s, const char *fmt, va_list ap);
char *gets(char *buf);

/* Errors and files (R11L: remove, rename, tmpnam and freopen work on a writable volume; tmpfile
 * returns NULL). */
void perror(const char *s);
int remove(const char *name);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *s);
FILE *freopen(const char *name, const char *mode, FILE *fp);

#endif
