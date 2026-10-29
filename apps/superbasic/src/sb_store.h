/*
 * sb_store.h - program line store and variable table.
 */
#ifndef SB_STORE_H
#define SB_STORE_H

#include <stddef.h>

#include "sb_state.h"

/* Program lines, kept sorted by number. */
SbLine *sb_prog_find(SbState *st, long number);
int sb_prog_store(SbState *st, long number, const char *src, size_t len);
void sb_prog_delete(SbState *st, long number);
void sb_prog_clear(SbState *st);

/* Variables.  Names compare case-insensitively.  sb_var_set copies string
 * values, so the caller keeps ownership of @a val. */
SbVar *sb_var_find(SbState *st, const char *name, size_t namelen);
int sb_var_set(SbState *st, const char *name, size_t namelen, const SbValue *val);
int sb_var_set_undef(SbState *st, const char *name, size_t namelen);
void sb_var_clear(SbState *st);

/* Numeric arrays.  sb_arr_dim replaces any array of the same name; the new
 * elements are all zero.  sb_arr_offset maps subscripts (already rounded)
 * to a row-major element index: SB_E_OUT_OF_RANGE for a subscript outside
 * 0..size, SB_E_EXPRESSION when the number of subscripts is wrong. */
SbArray *sb_arr_find(SbState *st, const char *name, size_t namelen);
int sb_arr_dim(SbState *st, const char *name, size_t namelen, int ndims,
               const long *sizes);
int sb_arr_offset(const SbArray *a, const long *idx, int n, size_t *off);
void sb_arr_clear(SbState *st);

/* Fixed-length strings made by DIM name$(len).  sb_sarr_dim replaces any
 * string of the same name and fills it with NUL bytes (verified against the
 * ROM); len must be at least 1.
 * The ROM accepted only one subscript here; see KNOWN-LIMITATIONS. */
SbStrArray *sb_sarr_find(SbState *st, const char *name, size_t namelen);
int sb_sarr_dim(SbState *st, const char *name, size_t namelen, long len);
void sb_sarr_clear(SbState *st);

/* SB4: the value a variable names.  For a parameter bound by reference
 * this is the caller's storage; otherwise it is the variable's own value.
 * Every read and write of a variable goes through this. */
SbValue *sb_var_slot(SbVar *v);

/* SB4: locals made by a call at call depth @a depth.  Each one is inserted
 * at the head of its list, so it shadows any global of the same name for as
 * long as the call lasts (dynamic scope).  sb_frame_unbind removes every
 * local made at that depth. */
int sb_local_var(SbState *st, const char *name, size_t namelen, int depth,
                 SbValue **slot);
int sb_local_ref(SbState *st, const char *name, size_t namelen, int depth,
                 SbValue *ref);
int sb_local_arr(SbState *st, const char *name, size_t namelen, int depth,
                 int ndims, const long *sizes);
int sb_local_sarr(SbState *st, const char *name, size_t namelen, int depth,
                  long len);
void sb_frame_unbind(SbState *st, int depth);

/* SB4: DEFine names.  sb_def_note and sb_def_drop are called by the program
 * store.  sb_def_lookup gives SB_OK with the line number of the DEFine that
 * is in force; SB_E_NOT_FOUND (-7) when the name was defined but its DEFine
 * line has since been replaced or deleted (ROM: probes p16, p18); and
 * SB_E_BAD_NAME (-12) when the name was never defined (ROM: probes p16-p18).
 * sb_def_seen reports whether the name was ever defined, valid or not. */
void sb_def_note(SbState *st, long number, const char *text, size_t len);
void sb_def_set(SbState *st, const char *name, size_t nlen, int func,
                long number, int valid);
void sb_def_drop(SbState *st, long number);
int sb_def_lookup(SbState *st, const char *name, size_t namelen,
                  long *number, int *func);
int sb_def_seen(SbState *st, const char *name, size_t namelen, int *func);

#endif /* SB_STORE_H */
