/* qb_ext.h - QDOS/386 R12B: SuperBASIC host procedures and functions (LOAD, SAVE, MERGE, DIR, EXEC, EXEC_W, EXEC_ID,
 * EXEC_STATUS, JOB_WAIT, JOB_KILL, JOBS).  See qb_ext.c.  Strict C89. */
#ifndef QB_EXT_H
#define QB_EXT_H

#include "sb_state.h"

void qb_ext_install(SbState *st);

#endif
