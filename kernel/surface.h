/*
 * QDOS/386 R13 - display surfaces: the versioned kernel call QT_SURFACE (opt-in: -DQDOS_SURFACES, needs QDOS_VCON,
 * QDOS_JOBCTL, QDOS_TASKMGR).  A surface belongs to a session; a session belongs to the jobs on it (the job that
 * created a private session is its owner).  TEXT80x25 is the session's cells (the console API); INDEX8 is an
 * optional framebuffer the session shows instead.  Only the foreground session is presented and receives keys.
 * See docs/R13-DISPLAY-ABI.md.
 */
#ifndef QDOS386_SURFACE_H
#define QDOS386_SURFACE_H

#define QT_SURFACE        0x3B    /* a = SURF_OP_*, b, c -> see below */
#define SURF_VERSION      1u

#define SURF_OP_CAPS      0u      /* -> (version << 16) | SURF_CAP_* */
#define SURF_OP_INFO      1u      /* b = surface id (0: the caller's), c = 16-word buffer -> 0 */
#define SURF_OP_NEW       2u      /* the caller moves to a new private TEXT80x25 session -> its surface id */
#define SURF_OP_GFX_OPEN  3u      /* b = w | h << 16 (0: 320x200), c = SURF_GF_* -> surface id */
#define SURF_OP_GFX_CLOSE 4u      /* -> 0: the session shows its text again */
#define SURF_OP_FILL      5u      /* b = struct surf_rect {x, y, w, h, colour} -> pixels changed */
#define SURF_OP_BLIT      6u      /* b = struct surf_rect {x, y, w, h, src, src_stride} -> pixels copied */
#define SURF_OP_READ      7u      /* b = struct surf_rect {x, y, w, h, dst, dst_stride} -> pixels read */
#define SURF_OP_PAL_SET   8u      /* b = first | count << 16 (count 1..256), c = 3*count bytes R,G,B 0..255 -> count */
#define SURF_OP_PAL_GET   9u      /* as PAL_SET, c = buffer */
#define SURF_OP_TEXT      10u     /* b = struct surf_text {x, y, ink, paper, attr, str, len} -> cells drawn */
#define SURF_OP_PRINT     11u     /* b = string, c = length (<= 256): the text stream of the INDEX8 surface -> c */
#define SURF_OP_CURSOR    12u     /* b = col | row << 8 | visible << 16; b = 0xFFFFFFFF: query -> 0 / position */
#define SURF_OP_SCROLL    13u     /* b = struct surf_rect {x, y, w, h, dy, fill} -> pixels changed */
#define SURF_OP_FOREGROUND 14u    /* b = surface id (0: the caller's): show it (own session only) -> 0 */
#define SURF_OP_FOCUS     15u     /* -> 1 if the caller's session is shown and gets the keys, else 0 */
#define SURF_OP_SUM       16u     /* -> FNV-1a checksum of the caller's INDEX8 pixels and palette */
#define SURF_OP_LIST      17u     /* b = buffer, c = bytes: 16-byte header + 32-byte records -> count */
#define SURF_OP_CELLS     18u     /* b = struct surf_cells {row, col, n, cells}: TEXT80x25 cells (char | attr << 8) */
#define SURF_OP_COLOURS   19u     /* b = ink | paper << 8 | attr << 16: the text stream's colours -> 0 */
#define SURF_OP_FRAME     20u     /* count one frame (programs that animate) -> the new count */
#define SURF_OP_STATS     21u     /* b = 8-word buffer: surfaces open, opened, refused, freed, presenter counters */

#define SURF_CAP_TEXT     0x01u   /* TEXT80x25 sessions */
#define SURF_CAP_INDEX8   0x02u   /* INDEX8 surfaces */
#define SURF_CAP_SHOW_TEXT 0x04u  /* the display presents text sessions (VGA builds) */
#define SURF_CAP_SHOW_GFX 0x08u   /* the display presents INDEX8 surfaces in 320x200x256 (VGA builds) */

#define SURF_GF_FOREGROUND 0x01u  /* GFX_OPEN: also show the surface now */
#define SURF_GF_SHARE      0x02u  /* GFX_OPEN: keep a shared session (default: move to a private one first) */

#define SURF_KIND_TEXT    1u
#define SURF_KIND_INDEX8  2u

#define SURF_INFO_WORDS   16u     /* version, id, kind, w, h, stride, cols, rows, cursor, flags, users, owner,
                                   * pal_gen, gen, frames, session index */
#define SURF_FL_SHOWN     0x01u   /* foreground (shown and keyed) */
#define SURF_FL_PRIVATE   0x02u   /* private session */
#define SURF_FL_OVERLAY   0x04u   /* the task selector covers it now */
#define SURF_FL_OWNER     0x08u   /* the caller opened this INDEX8 surface */

#define SURF_LIST_ENTRY   32u     /* id, index, kind, w, h, users, owner, flags */

#define SURF_GFX_MAX      4       /* INDEX8 surfaces at once */

#define SURF_ERR_STALE    (-2)    /* no such surface (ended, or the id is from an earlier one) */
#define SURF_ERR_ARG      (-4)    /* bad op, size, rectangle, palette range or stride */
#define SURF_ERR_FULL     (-5)    /* SURF_GFX_MAX surfaces, or VSESS_MAX sessions, in use */
#define SURF_ERR_BUSY     (-6)    /* the session already has a graphics surface / the caller owns one */
#define SURF_ERR_NOGFX    (-7)    /* the caller's session shows text: no INDEX8 surface */
#define SURF_ERR_OWNER    (-8)    /* not the caller's surface */
#define SURF_ERR_MEM      (-3)    /* no kernel heap */
#define SURF_ERR_RANGE    (-14)   /* a pointer outside the caller's memory */

/* Argument records (all words 32-bit). */
struct surf_rect { long x, y, w, h; unsigned long p, q; };        /* p: colour / pointer / dy; q: stride / fill */
struct surf_text { long x, y; unsigned long ink, paper, attr, str, len; };
struct surf_cells { unsigned long row, col, n, cells; };

int surface_trap(unsigned int op, unsigned int b, unsigned int c);
void surface_poll(void);          /* the SYSTEM job: frees released surfaces (never while the presenter copies) */

#endif
