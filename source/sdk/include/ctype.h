/* ctype.h - QDOS/386 libc (R11A): C89 character classification, C locale.
 * Arguments are int: pass unsigned char values, or EOF.  Any value outside
 * 0..127 (including EOF and negative values) is classified as false and is
 * returned unchanged by tolower and toupper.  Functions only, no macros. */
#ifndef QDOS386_CTYPE_H
#define QDOS386_CTYPE_H

int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#endif
