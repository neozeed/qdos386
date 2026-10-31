/*
 * sb_interp.h - public entry point of the SuperBASIC/C89 core.
 *
 * A host creates an SbState with sb_init(), feeds it one source line at a
 * time with sb_execute() (typed lines run immediately; lines that begin with
 * a number are stored in the program), and releases it with sb_destroy().
 *
 * Output (PRINT, LIST) goes to the channel handles installed with
 * sb_set_channel (default #1).  Error reports go to the diagnostic handle
 * installed with sb_set_diag, as "<message>" or "<message> at line <n>".
 * See docs/SB3-CHANNEL-ABI.md.
 */
#ifndef SB_INTERP_H
#define SB_INTERP_H

#include <stddef.h>

#include "sb_lex.h"
#include "sb_state.h"

/* Execute one typed line.  Returns SB_OK or the error code of the failure
 * (the same value ERNUM reports afterwards). */
int sb_execute(SbState *st, const char *text, size_t len);

/* RUN the stored program from its first line (a host convenience). */
int sb_run(SbState *st);

/* SB4: what a name is, as a DEFine in the program: SB_DEF_NONE, SB_DEF_PROC
 * or SB_DEF_FUNC.  A name whose DEFine line has been removed is SB_DEF_NONE. */
#define SB_DEF_NONE 0
#define SB_DEF_PROC 1
#define SB_DEF_FUNC 2
int sb_def_kind(SbState *st, const char *name, size_t nlen);

/* SB4: call the DEFine FuNction name from an expression.  lx is at the
 * left parenthesis after the name; *out receives the result (the caller
 * owns it). */
int sb_call_function(SbState *st, SbLexer *lx, const char *name, size_t nlen,
                     SbValue *out);

/* QDOS/386 R12B: the host extension entry called name (a procedure when
 * func is 0, a function otherwise), or NULL; see SbExtName in sb_state.h. */
const SbExtName *sb_ext_find(const SbState *st, const char *name, size_t nlen,
                             int func);

/* QDOS/386 R12B: what NEW does (program, variables, arrays, loops, GOSUBs,
 * DATA cursor), for a host procedure such as LOAD. */
void sb_new(SbState *st);

/* QDOS/386 R12B: nonzero while a typed (direct) line runs outside any
 * program line or DEFine call; LOAD and MERGE are allowed only then. */
int sb_in_direct_mode(const SbState *st);

#endif /* SB_INTERP_H */
