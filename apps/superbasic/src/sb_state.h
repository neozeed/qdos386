/*
 * sb_state.h - the complete interpreter state.
 *
 * There are no file-scope variables in the SuperBASIC core.  Everything the
 * interpreter remembers lives in one SbState, so two interpreters can run
 * side by side and a test can create and destroy one freely.
 *
 * Field meanings are documented in docs/INTERPRETER-STATE.md.
 */
#ifndef SB_STATE_H
#define SB_STATE_H

#include <stddef.h>

#include "sb_chan.h"
#include "sb_mem.h"
#include "sb_value.h"

/* Channels #0 to #3 (the ROM's channel numbers, see docs/SB3-CHANNEL-ABI.md). */
#define SB_NCHAN 4

/* One numbered program line.  text is the canonical form (keywords
 * upper-cased, see sb_lex_canonical); it does not include the number. */
typedef struct SbLine {
    long number;
    char *text;
    size_t len;
    struct SbLine *next;
} SbLine;

/* One numeric or string variable.  name is upper-cased and includes the
 * trailing '$' for string variables. */
typedef struct SbVar {
    char *name;
    size_t namelen;
    SbValue val;
    struct SbVar *next;
    int local;           /* made by a call: a parameter or LOCal (SB4)    */
    int frame_no;        /* call depth that owns a local, see SbFrame     */
    SbValue *ref;        /* parameter bound by reference: the caller's    */
                         /* storage; NULL means the value is val itself   */
} SbVar;

/* One active FOR loop.  line_no is the program line that holds the FOR,
 * -1 when the FOR was typed in direct mode, pos is the offset in that
 * text just after the FOR clause. */
typedef struct SbFor {
    char *name;
    size_t namelen;
    double limit;
    double step;
    long line_no;
    size_t pos;
    int done;            /* the loop has finished (SB4, see FOR records) */
    int depth;           /* call depth of the FOR statement (SB4)        */
    struct SbFor *next;
} SbFor;

/* Hosted safeguard, not ROM behaviour: a top-level command that makes more
 * than this many control transfers stops with SB_E_OUT_OF_RANGE. */
#define SB_DEFAULT_STEP_LIMIT 1000000L

/* Hosted bounds, not ROM behaviour.  The ROM accepted 3000 nested GOSUBs
 * and was limited only by free memory; SB_MAX_GOSUB is where the hosted
 * core stops with SB_E_OUT_OF_MEMORY.  DIM accepts up to SB_MAX_DIMS
 * subscripts (the ROM accepted 4). */
#define SB_MAX_GOSUB 4096
#define SB_MAX_DIMS  8
#define SB_MAX_ARRAY_ELEMENTS 1000000L

/* One numeric array made by DIM.  Dimension i takes subscripts 0..sizes[i]
 * (so sizes[i]+1 elements); elements are stored row-major in vals. */
typedef struct SbArray {
    char *name;
    size_t namelen;
    int ndims;
    long sizes[SB_MAX_DIMS];
    size_t count;
    SbValue *vals;
    struct SbArray *next;
    int local;           /* LOCal array of a call (SB4)                  */
    int frame_no;
} SbArray;

/* One string made by DIM name$(n): a fixed-length string of len characters,
 * initially NUL bytes (verified).  a$(i) and a$(i TO j) address this storage, not the
 * scalar a$ (verified against the JS ROM; see docs/JS-SB3-MAP.md). */
typedef struct SbStrArray {
    char *name;
    size_t namelen;
    size_t len;
    char *str;
    struct SbStrArray *next;
    int local;           /* LOCal string array of a call (SB4)           */
    int frame_no;
} SbStrArray;

/* DATA cursor data_pos value meaning "past the end of line data_line" (the
 * cursor then moves on to the next DATA line, or RESTORE set it that way). */
#define SB_DATA_POS_DONE ((size_t)-1)

/* Hosted bound, not ROM behaviour.  The JS ROM ran out of memory (-3)
 * between 5000 and 6000 nested calls of a one-parameter function (probe
 * sb4probe/p11).  The hosted core stops at SB_MAX_CALL_DEPTH with -3. */
#ifdef SB_CALL_DEPTH_VAR            /* QDOS/386 port: the limit is a variable set from the stack the host gives the core */
extern int SB_CALL_DEPTH_VAR;
#define SB_MAX_CALL_DEPTH SB_CALL_DEPTH_VAR
#endif
#ifndef SB_MAX_CALL_DEPTH
#define SB_MAX_CALL_DEPTH 5500
#endif

/* One active DEFine PROCedure or DEFine FuNction call (SB4).  Calls nest
 * exactly as the C recursion of run_body does.  The frame's locals are the
 * SbVar, SbArray and SbStrArray nodes whose frame_no is its depth.  dead is
 * set when CLEAR or NEW runs inside the call: the frame's storage may then
 * be gone, so the call ends with -7 (verified: CLEAR inside a procedure,
 * then END DEFine, gave -7; statements between the two were not probed). */
typedef struct SbFrame {
    int is_func;
    int dead;
    long gosub_base;     /* gosub_depth when the call began              */
    int depth;           /* 1 for the outermost call                     */
    int body_started;    /* a statement other than LOCal has run         */
    struct SbFrame *next;
} SbFrame;

/* One pending GOSUB return.  line_no is -1 when the GOSUB came from a typed
 * line (such frames are dropped when that line ends); pos is where RETURN
 * resumes in that line. */
typedef struct SbGosub {
    long line_no;
    size_t pos;
    struct SbGosub *next;
} SbGosub;

/* One DEFine name remembered from the program (SB4).  The JS ROM keeps the
 * name when its DEFine line is replaced or deleted: a later call then gives
 * -12, not -7 (probes p9 and p12).  valid is 0 after that. */
typedef struct SbDefEntry {
    char *name;
    size_t namelen;
    long number;
    int func;
    int valid;
    struct SbDefEntry *next;
} SbDefEntry;

typedef struct SbState {
    SbAlloc mem;
    SbChan *chan[SB_NCHAN];  /* host-owned handles for #0 to #3  */
    SbChan *diag;            /* host-owned handle for error reports */

    SbLine *prog;         /* program, sorted by line number            */
    SbVar *vars;          /* variables, most recently created first    */
    SbFor *fors;          /* active FOR loops, innermost first         */
    SbArray *arrays;      /* numeric arrays made by DIM                */
    SbGosub *gosubs;      /* pending GOSUB returns, innermost first    */
    long gosub_depth;     /* number of frames in gosubs                */
    SbStrArray *strarrays; /* strings made by DIM name$(n), see SbStrArray */
    long data_line;       /* DATA cursor: line number, -1 before the first */
    size_t data_pos;      /* DATA cursor: offset in that line, or SB_DATA_POS_DONE */
    int data_set;         /* nonzero once RESTORE has positioned the cursor */

    int col;              /* output column, for comma tabs             */
    int err;              /* ERNUM: last error code                    */
    long cur_line;        /* program line executing, -1 in direct mode */
    int in_direct;        /* nonzero while a typed line is executing   */

    SbFrame *frames;      /* active DEFine calls, innermost first (SB4) */
    int call_depth;       /* number of frames in frames                  */
    SbValue ret_val;      /* value passed from RETurn to the call site    */
    SbDefEntry *defs;     /* DEFine names seen in the program (SB4)       */
    int jmp_gosub;        /* the pending jump is a GOSUB or its RETURN    */

    long stop_line;       /* where STOP happened, -1 if none           */
    size_t stop_pos;

    long jmp_line;        /* control transfer requested by a statement */
    size_t jmp_pos;
    long steps;           /* jumps taken since the top-level command began */
    long step_limit;      /* hosted safeguard, see SB_DEFAULT_STEP_LIMIT  */
} SbState;

/* Start a fresh interpreter using @a mem (copied).  Channels start unset. */
void sb_init(SbState *st, const SbAlloc *mem);

/* Install a channel handle as #n (0 to 3), or the error report handle.  The
 * core never frees a handle; the host creates it and frees it with
 * sb_chan_free after the state is destroyed or the channel replaced. */
void sb_set_channel(SbState *st, int n, SbChan *ch);
void sb_set_diag(SbState *st, SbChan *ch);

/* Free every program line, variable and FOR record. */
void sb_destroy(SbState *st);

#endif /* SB_STATE_H */
