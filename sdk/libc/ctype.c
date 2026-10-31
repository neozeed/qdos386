/* ctype.c - QDOS/386 libc (R11A): C89 character classes for the C locale.
 *
 * Written for this SDK from the C89 definitions.  The historical C68 libc
 * (ctype/*.c) uses a 68000 table and macros, and is only the semantic
 * reference: which classes exist and how EOF is treated. */
#include <ctype.h>

#define QD_ASCII(c) ((c) >= 0 && (c) <= 127)

int isupper(int c)  { return QD_ASCII(c) && c >= 'A' && c <= 'Z'; }
int islower(int c)  { return QD_ASCII(c) && c >= 'a' && c <= 'z'; }
int isdigit(int c)  { return QD_ASCII(c) && c >= '0' && c <= '9'; }
int isalpha(int c)  { return isupper(c) || islower(c); }
int isalnum(int c)  { return isalpha(c) || isdigit(c); }
int iscntrl(int c)  { return QD_ASCII(c) && (c < 32 || c == 127); }
int isprint(int c)  { return QD_ASCII(c) && c >= 32 && c <= 126; }
int isgraph(int c)  { return QD_ASCII(c) && c >= 33 && c <= 126; }
int ispunct(int c)  { return isgraph(c) && !isalnum(c); }
int isspace(int c)
{
	return QD_ASCII(c) && (c == ' ' || (c >= '\t' && c <= '\r'));
}
int isxdigit(int c)
{
	return isdigit(c) || (QD_ASCII(c) && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')));
}
int tolower(int c)  { return isupper(c) ? c + ('a' - 'A') : c; }
int toupper(int c)  { return islower(c) ? c - ('a' - 'A') : c; }
