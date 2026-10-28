#ifdef LEGACY_HOST64
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
extern void fatals(const char *, ...);
#endif

#ifdef MSDOS
#include <stdlib.h>
#include <io.h>
#endif /* MSDOS */

#ifdef USG
#include <string.h>
#else /* not USG */
#ifdef LEGACY_HOST64
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
extern void fatals(const char *, ...);
#endif

#ifdef MSDOS
#include <string.h>
#else
#include <strings.h>
#endif /* not MSDOS */
#endif /* not USG */
