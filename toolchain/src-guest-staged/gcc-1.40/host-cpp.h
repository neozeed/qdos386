/* Shared declarations for the native preprocessor and its generated parser. */
struct hashnode;
struct hashnode *lookup();
int error(const char *, ...), warning(const char *, ...);

#include <stddef.h>
char *xcalloc(size_t, size_t);
