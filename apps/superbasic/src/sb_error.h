/*
 * sb_error.h - QDOS/SuperBASIC error codes used by the C89 core.
 *
 * Codes are negative, as in the JS ROM.  Message texts are the ROM's own
 * strings from sys_mess.s (the ROM table is ordered; code = -(index+1)).
 *
 * Verification status (see docs/JS-SUPERBASIC-MAP.md):
 *   -17 "error in expression"  verified against the ROM (ERNUM)
 *   -18 "overflow"             verified against the ROM (ERNUM)
 *   others                     mapping inferred from table order
 */
#ifndef SB_ERROR_H
#define SB_ERROR_H

#define SB_OK                   0

#define SB_E_OUT_OF_MEMORY     (-3)
#define SB_E_OUT_OF_RANGE      (-4)
#define SB_E_NOT_OPEN         (-6)
#define SB_E_NOT_FOUND         (-7)
#define SB_E_END_OF_DATA      (-10)   /* READ past the last DATA item */
#define SB_E_BAD_NAME         (-12)
#define SB_E_BAD_PARAMETER    (-15)
#define SB_E_EXPRESSION       (-17)
#define SB_E_OVERFLOW         (-18)
#define SB_E_NOT_IMPLEMENTED  (-19)
#define SB_E_BAD_LINE         (-21)
/* Hosted only, not a ROM code: a channel adapter failed or made no progress
 * (see sb_chan.h).  Chosen outside the ROM range so it cannot be confused
 * with a ROM error. */
#define SB_E_IO              (-40)

/* Return the ROM message text for @a code, or "unknown error". */
const char *sb_error_text(int code);

#endif /* SB_ERROR_H */
