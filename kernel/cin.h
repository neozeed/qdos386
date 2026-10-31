/*
 * QDOS/386 R7 - console input: a fixed ring of decoded bytes.
 *
 * Producers: the PC IRQ1 handler (platform/pc/i8042.c) and, when polling is
 * enabled, the timer tick on i386VM (cin_poll_tick).  A producer stores one
 * byte under irq_save and never schedules; it then calls con_input_arrived,
 * which only marks a waiting job runnable.
 *
 * Consumer: the CON channel read, inside a trap (IF clear).  A full ring
 * drops the new byte and counts it in cin_overflows; stored bytes are never
 * overwritten.
 */
#ifndef QDOS386_CIN_H
#define QDOS386_CIN_H

#define CIN_SIZE 128                 /* ring capacity is CIN_SIZE - 1 bytes */
#define CIN_TICK_MAX 16              /* polled bytes per timer tick, at most */

/* Store a byte.  Returns 1 if stored, 0 if the ring was full (dropped). */
int cin_put(unsigned char c);

/* Remove the oldest byte.  Returns 1 and sets *c, or 0 if the ring is empty. */
int cin_get(unsigned char *c);

unsigned int cin_count(void);        /* bytes waiting */

/* End of input (the backend reported that no more bytes will come).  Sticky. */
void cin_set_eof(void);
int cin_at_eof(void);

/* Polling backend (i386VM): cin_poll_tick pulls bytes from input_poll on each
 * timer tick, but only after cin_enable_poll.  Tests that inject bytes
 * themselves never enable it, so a closed or empty stdin cannot end them. */
void cin_enable_poll(void);
void cin_poll_tick(void);

/* Wake the job waiting in QT_CON_WAIT, if any (jobs.c). */
void con_input_arrived(void);

extern volatile unsigned long cin_overflows;

#ifdef QDOS_VCON
/* R12: what the input device delivers (CON_CAP_* bits of kernel/channel.h).  The PC keyboard driver sets CON_CAP_KEYS; polled
 * byte backends leave it 0, so a program can tell whether cursor keys can ever arrive. */
extern volatile unsigned int cin_features;
#endif
extern volatile unsigned long cin_stored;

#endif
