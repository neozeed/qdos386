/*
 * QDOS/386 R11M - virtual text consoles (opt-in: -DQDOS_VCON, which needs -DQDOS_FS_WRITE).
 *
 * VCON_MAX independent 80x25 text consoles.  Each holds its own cells (character and attribute), cursor,
 * attribute, scroll region, input queue and input waiter.  Exactly one console is the FOREGROUND console:
 * the display shows it, and keyboard input goes to it.  Output to a background console only updates its
 * buffer.  Every job belongs to one console (the one of the job that created it, unless a creator chose
 * another with vcon_set_child); its CON channels read and write that console.
 *
 * Interrupt discipline: every function is called with interrupts off (a trap, an interrupt handler, or
 * boot).  The keyboard handler may switch the foreground console and queue input; traps write output
 * and read input.  They never interleave, so no locks are needed.
 *
 * Display and diagnostics: the platform shows the foreground console (vdisp_*) and carries the
 * diagnostic stream (diag_putc: debug port 0x402 / serial / the harness byte port).  See
 * docs/R11M-CONSOLE-ABI.md.  The file is empty unless QDOS_VCON is defined, so default images are unchanged.
 */
#ifndef QDOS386_VCON_H
#define QDOS386_VCON_H

#define VCON_MAX   4
#define VCON_COLS  80
#define VCON_ROWS  25
#define VCON_CELLS (VCON_COLS * VCON_ROWS)
#define VCON_QSIZE 128                 /* input queue: VCON_QSIZE - 1 bytes */
#define VCON_LINE  120                 /* longest cooked line (without the newline) */
#define VCON_ATTR_DEFAULT 0x07u        /* light grey on black */

/* Console modes (vcon_set_mode).  The default, 0, is the legacy behaviour: bytes are delivered as typed and
 * the program echoes. */
#define VC_ECHO    0x01u               /* typed characters are shown by the kernel */
#define VC_COOKED  0x02u               /* line discipline: BS and ^U edit, input is delivered a line at a time */
#define VC_MODE_MASK 0x03u

/* Diagnostic mirror (vcon_set_diag): which console output also goes to the diagnostic stream. */
#define VDIAG_CON0 0                   /* console 0 only (default: the legacy transcript) */
#define VDIAG_NONE 1                   /* kernel messages only */
#define VDIAG_ALL  2                   /* every console; lines of consoles 1.. start with "[Cn] " */

/* Clear selectors (vcon_clear). */
#define VCLR_ALL   0                   /* whole console, cursor home */
#define VCLR_EOL   1                   /* from the cursor to the end of the line */
#define VCLR_LINE  2                   /* the cursor's line */
#define VCLR_EOS   3                   /* from the cursor to the end of the console */

/* Query selectors (vcon_query). */
#define VQ_SIZE    0                   /* cols | rows << 8 */
#define VQ_CURSOR  1                   /* col | row << 8 | visible << 16 */
#define VQ_ATTR    2                   /* attribute */
#define VQ_SELF    3                   /* console number | foreground << 8 */
#define VQ_MODE    4                   /* mode flags */
#define VQ_QUEUED  5                   /* bytes waiting in the input queue */
#define VQ_FG      6                   /* the foreground console number */
#define VQ_REGION  7                   /* top | bottom << 8 */

#define VCON_ERR_BADCON  (-14)         /* console number out of range / bad argument (CHAN_ERR_RANGE) */

void vcon_init(void);                  /* once, at boot: clear all consoles, console 0 foreground */

/* ---- output ---- */
void vcon_write(int con, const unsigned char *buf, unsigned int n);   /* program output (diagnostic mirror per mode) */
void vcon_kputc(unsigned char c);      /* kernel output: console 0 and always the diagnostic stream */
int vcon_goto(int con, unsigned int row, unsigned int col);   /* 0-based; clamped to the console; -14 never */
void vcon_clear(int con, unsigned int what);
unsigned int vcon_set_attr(int con, unsigned int attr);       /* returns the old attribute */
void vcon_set_cursor_visible(int con, int visible);
int vcon_set_region(int con, unsigned int top, unsigned int bottom);   /* 0 on success, -14 if top > bottom or too big */
long vcon_query(int con, unsigned int what);
int vcon_snap_row(int con, unsigned int row, unsigned char *out, unsigned int size);   /* text of a row, NUL-terminated, trailing blanks trimmed */

/* ---- focus ---- */
int vcon_switch(int con);              /* make con the foreground console; 0, or -14 */
int vcon_fg(void);

/* ---- input ---- */
void vcon_input(unsigned char c);      /* one decoded byte from the keyboard or the polled backend: to the foreground console */
void vcon_eof_all(void);               /* the input backend ended */
int vcon_get(int con, unsigned char *c);   /* 1 and *c, or 0 if the queue is empty */
unsigned int vcon_count(int con);
int vcon_at_eof(int con);              /* eof reported and the queue is drained */
unsigned int vcon_set_mode(int con, unsigned int mode);   /* returns the old mode */
void vcon_set_diag(int mode);
int vcon_get_diag(void);

/* ---- sessions: which console a job belongs to ---- */
int vcon_job_con(int slot);
void vcon_job_inherit(int slot, int creator);   /* a new job in slot: the creator's console, or the creator's pending choice */
void vcon_job_gone(int slot);                   /* the job in slot ended: waiters and focus are repaired */
int vcon_set_child(int slot, int con);          /* the next job slot's job creates goes to con (-1 clears); 0, or -14 */
int vcon_job_set(int slot, int con);            /* put a job on a console (used by the job creation path) */

#ifdef QDOS_TASKMGR
/* ---- R12C: the task selector overlay (kernel/taskmgr.c) ---- */
void vcon_overlay(int on);                      /* 1: the selector owns the display; 0: redraw the foreground console */
int vcon_users(int con);                        /* live jobs on the console */
int taskmgr_key(unsigned char c);               /* 1 if the open selector took the key */
void taskmgr_hotkey(void);                      /* Ctrl+Esc (keyboard IRQ) */
void taskmgr_console_key(int con);              /* Alt+F1..F4 (keyboard IRQ): close the selector, show console con */
#endif

#ifdef QDOS_SURFACES
/* ---- R13: sessions beyond the four legacy consoles (docs/R13-DISPLAY-ABI.md) ---- */
#define VSESS_MAX      20              /* sessions in all: 0..3 legacy consoles, 4..19 private sessions */
#define VCON_CHILD_NEW 0xfe            /* vcon_set_child: the next job the caller creates gets a new private session */
#define VSESS_OWNER    0x7E00u         /* heap owner of session and surface blocks (never a job id) */
#define VSESS_ERR_FULL (-5)            /* no free session */
#define VSESS_ERR_MEM  (-3)            /* no kernel heap for the session */
struct gsurf;
int vsess_new(unsigned int owner_id);           /* -> session index, or VSESS_ERR_*; interrupts off */
int vsess_valid(int idx);
unsigned int vsess_tag(int idx);                /* changes every time the index is reused (0: no session) */
int vsess_move(int slot, int idx);              /* the job in slot, and its CON channels, to session idx */
struct gsurf *vsess_gfx(int idx);               /* the session's INDEX8 surface, or 0 (TEXT80x25) */
void vsess_set_gfx(int idx, struct gsurf *g);
int vsess_users(int idx);
int vsess_dynamic(int idx);                     /* 1 for a private session (4..19) */
unsigned int vsess_owner(int idx);              /* tagged id of the job that created a private session */
int vsess_count(void);                          /* sessions in use */
int vsess_new_failures(void);                   /* VCON_CHILD_NEW requests that fell back to the creator's session */
int vcon_overlay_on(void);
void vcon_present_text(void);                   /* the presenter is in text mode again: draw what the display shows */
int vcon_write_cells(int con, unsigned int row, unsigned int col, const unsigned short *cells, unsigned int n);
#ifdef QDOS_R13A
#define VSESS_CM_VGA   0u              /* R13A console modes: the VGA's own text mode (the R13 behaviour) */
#define VSESS_CM_GFX   1u              /*   the cells rendered with a bitmap font by the presenter */
struct kfont;
struct kfont *vsess_font(int idx);
void vsess_set_font(int idx, struct kfont *f);
unsigned int vsess_cmode(int idx);
void vsess_set_cmode(int idx, unsigned int m);
void vsess_set_cmode_default(unsigned int m);   /* for sessions created from now on */
const unsigned short *vsess_cells(int idx, unsigned int *cursor);   /* cursor: col | row << 8 | shown << 16 */
#endif
/* Provided by kernel/surface.c: a surface leaves its session (closed, or its session or owner ended). */
void surf_release(struct gsurf *g);
void surf_job_ended(unsigned int job_id, int idx);
/* Provided by the presenter (platform/pc/vgapres.c on VGA builds, stubs otherwise; kernel/present.h). */
#endif

/* ---- waiter (one per console) ---- */
int vcon_waiter(int con);                       /* job slot or -1 */
void vcon_set_waiter(int con, int slot);        /* slot, or -1 to clear */

/* Provided by jobs.c: mark a job that sleeps in QT_CON_WAIT runnable (never schedules). */
void vcon_wake_job(int slot);

/* Provided by the platform (display and diagnostics). */
void vdisp_put(unsigned int row, unsigned int col, unsigned short cell);
void vdisp_load(const unsigned short *cells);
void vdisp_cursor(unsigned int row, unsigned int col, int visible);
void diag_putc(unsigned char c);

#endif
