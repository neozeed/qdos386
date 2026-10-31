/*
 * QDOS/386 R7 - console input ring.  See cin.h.
 */
#include "cin.h"
#include "platform.h"
#ifdef QDOS_VCON
#include "vcon.h"
#endif

extern unsigned int irq_save(void);
extern void irq_restore(unsigned int flags);

#ifdef QDOS_VCON
/* R11M: input belongs to the virtual consoles (kernel/vcon.c).  This file keeps the producer entry points:
 * a byte goes to the foreground console, end of input to every console.  The polled backend also decodes
 * the console hotkey ESC 1..ESC 4 (the PC keyboard path uses Alt+Fn in kbd.c instead). */
int cin_put(unsigned char c)
{
	unsigned int flags = irq_save();

	vcon_input(c);
	irq_restore(flags);
	return 1;
}

int cin_get(unsigned char *c)
{
	unsigned int flags = irq_save();
	int r = vcon_get(vcon_fg(), c);

	irq_restore(flags);
	return r;
}

unsigned int cin_count(void)
{
	return vcon_count(vcon_fg());
}

void cin_set_eof(void)
{
	unsigned int flags = irq_save();

	vcon_eof_all();
	irq_restore(flags);
}

int cin_at_eof(void)
{
	return vcon_at_eof(vcon_fg());
}

volatile unsigned long cin_overflows = 0;
volatile unsigned long cin_stored = 0;
volatile unsigned int cin_features = 0;
static volatile int polling = 0;
static int esc_pending = 0;

void cin_enable_poll(void)
{
	polling = 1;
}

void cin_poll_tick(void)
{
	unsigned int i;

	input_tick();
	if (!polling)
		return;
	for (i = 0; i < CIN_TICK_MAX; i++) {
		int r;

		if (cin_count() >= VCON_QSIZE - 8)
			break;                     /* leave bytes in the backend */
		r = input_poll();
		if (r == INPUT_NONE)
			break;
		if (r == INPUT_EOF) {
			if (esc_pending) {
				esc_pending = 0;
				cin_put(0x1b);
			}
			cin_set_eof();
			break;
		}
		if (esc_pending) {
			esc_pending = 0;
			if (r >= '1' && r < '1' + VCON_MAX) {
				(void) vcon_switch(r - '1');
				continue;
			}
			cin_put(0x1b);
		} else if (r == 0x1b) {
			esc_pending = 1;
			continue;
		}
		cin_put((unsigned char) r);
	}
}
#else
static volatile unsigned char ring[CIN_SIZE];
static volatile unsigned int head = 0;     /* next write */
static volatile unsigned int tail = 0;     /* next read */
static volatile int eof = 0;
static volatile int polling = 0;

volatile unsigned long cin_overflows = 0;
volatile unsigned long cin_stored = 0;

int cin_put(unsigned char c)
{
	unsigned int flags = irq_save();
	unsigned int next = (head + 1) % CIN_SIZE;
	int stored = 0;

	if (next == tail) {
		cin_overflows++;
	} else {
		ring[head] = c;
		head = next;
		cin_stored++;
		stored = 1;
	}
	irq_restore(flags);
	if (stored)
		con_input_arrived();
	return stored;
}

int cin_get(unsigned char *c)
{
	unsigned int flags = irq_save();
	int got = 0;

	if (head != tail) {
		*c = ring[tail];
		tail = (tail + 1) % CIN_SIZE;
		got = 1;
	}
	irq_restore(flags);
	return got;
}

unsigned int cin_count(void)
{
	return (head + CIN_SIZE - tail) % CIN_SIZE;
}

void cin_set_eof(void)
{
	eof = 1;
	con_input_arrived();
}

int cin_at_eof(void)
{
	return eof;
}

void cin_enable_poll(void)
{
	polling = 1;
}

void cin_poll_tick(void)
{
	unsigned int i;

	input_tick();                      /* always: drains stale aux output */
	if (!polling)
		return;
	for (i = 0; i < CIN_TICK_MAX; i++) {
		int r;
		if (eof || cin_count() >= CIN_SIZE - 1)
			break;                     /* leave bytes in the backend */
		r = input_poll();
		if (r == INPUT_NONE)
			break;
		if (r == INPUT_EOF) {
			cin_set_eof();
			break;
		}
		cin_put((unsigned char) r);
	}
}
#endif /* QDOS_VCON */
