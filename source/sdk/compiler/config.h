/* config.h for the QDOS/386 build of the GCC 1.40 runtime helpers.
   The historical gnulib sources include "config.h"; this file supplies the
   i386 values they need: little-endian, 32-bit words.  It is not the GCC
   config.h and is not used by the compiler itself. */
#define BITS_PER_WORD 32
#define SItype long int
#define BITS_PER_UNIT 8
