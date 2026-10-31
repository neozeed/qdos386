/*
 * sb_error.c - error message table (texts copied from the JS ROM sys_mess.s
 * ordering; these are the ROM's strings, not new text).
 */
#include "sb_error.h"

static const char *const sb_error_texts[] = {
    "not complete",
    "invalid Job",
    "out of memory",
    "out of range",
    "buffer full",
    "channel not open",
    "not found",
    "already exists",
    "in use",
    "end of file",
    "drive full",
    "bad name",
    "Xmit error",
    "format failed",
    "bad parameter",
    "bad or changed medium",
    "error in expression",
    "overflow",
    "not implemented",
    "read only",
    "bad line"
};

#define SB_ERROR_COUNT ((int)(sizeof(sb_error_texts) / sizeof(sb_error_texts[0])))

const char *sb_error_text(int code)
{
    int idx;
    if (code == SB_E_IO) {
        return "channel I/O failed (hosted)";
    }
    if (code > -1 || code < -SB_ERROR_COUNT) {
        return "unknown error";
    }
    idx = -code - 1;
    return sb_error_texts[idx];
}
