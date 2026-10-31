/*
 * QDOS/386 R13A - native keyboard events (opt-in per session; docs/R13A-GRAPHICS-ABI.md).  The keyboard IRQ hands
 * every scancode here; a session that enabled events gets key-down / key-up records while it is the foreground (and
 * the task selector is not over it).  Hotkeys (Alt+F1..F4, Alt+1..4, Ctrl+Esc) are consumed: neither their down nor
 * their up reaches a program.  When the focus moves, the session that loses it gets a synthetic key-up for every key
 * it saw go down; a key-up whose key-down went to another session is dropped.  The cooked/raw console input of the
 * session is unchanged.  Freestanding C89; runs with interrupts off (IRQ handler, traps).
 */
#ifndef QDOS386_KEV_H
#define QDOS386_KEV_H

#define KEV_QSIZE     64          /* events per session (oldest kept, newest dropped when full: counted) */
/* An event word: scancode (bits 0..7, set 1 make code without bit 7), bit 8 extended (E0 prefix), bit 9 down,
 * bit 10 synthetic (focus loss), bits 12..14 modifiers (1 Shift, 2 Ctrl, 4 Alt) at the time, bits 16..23 the
 * translated character of a key-down where there is one (the byte the console would get), else 0. */
#define KEV_EXT       0x100u
#define KEV_DOWN      0x200u
#define KEV_SYNTH     0x400u
#define KEV_SHIFT     0x1000u
#define KEV_CTRL      0x2000u
#define KEV_ALT       0x4000u

/* From the IRQ: one scancode byte sc (after the decoder: hot = 1 if it was consumed as a hotkey; ch the byte the
 * console got, 0 if none; mods KEV_SHIFT|KEV_CTRL|KEV_ALT). */
void kev_scancode(unsigned int sc, int ext, int hot, unsigned int ch, unsigned int mods);
void kev_refocus(void);                     /* the foreground (or the selector) changed */
int kev_enable(int idx, int on);            /* -> 0 */
int kev_read(int idx, unsigned int *out, int max);   /* -> events copied */
void kev_session_gone(int idx);
void kev_stats(unsigned int *o);            /* delivered, dropped (queue full), synthetic ups, hotkeys consumed */

#endif
