/*
 * QDOS/386 R7 - PC keyboard: the i8042 controller and IRQ1.
 *
 * Init (interrupts still off, before the timer starts):
 *   disable keyboard and aux ports, flush stale output, read the command byte,
 *   set it explicitly (IRQ1 on, keyboard clock on, translation on, aux IRQ12
 *   off), write it back, enable the keyboard port.
 * IRQ1: read one byte from port 0x60 when the output buffer holds keyboard
 *   data, decode it (kernel/kbd.c), store the result (kernel/cin.c), send EOI.
 *   The handler never schedules.
 * Polling: none on the PC; input_poll always reports INPUT_NONE.
 */
#include "platform.h"
#include "kbd.h"
#include "cin.h"
#ifdef QDOS_VCON
#include "vcon.h"
#ifdef QDOS_R13A
#include "kev.h"
#endif
#endif

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
extern void pic_init(void);

#define KBD_DATA     0x60
#define KBD_STATUS   0x64
#define ST_OUT_FULL  0x01
#define ST_IN_FULL   0x02
#define ST_AUX       0x20             /* output byte came from the aux port */

static struct kbd_decoder dec;
volatile unsigned long kbd_scancodes = 0;   /* keyboard bytes read by IRQ1 */
volatile unsigned long kbd_timeouts = 0;    /* controller waits that gave up */
volatile unsigned long kbd_flushed = 0;     /* stale bytes discarded at init */

static void wait_in_empty(void)
{
	unsigned long n;
	for (n = 0; n < 100000ul; n++)
		if ((inb(KBD_STATUS) & ST_IN_FULL) == 0)
			return;
	kbd_timeouts++;
}

static int wait_out_full(void)
{
	unsigned long n;
	for (n = 0; n < 100000ul; n++)
		if ((inb(KBD_STATUS) & ST_OUT_FULL) != 0)
			return 1;
	kbd_timeouts++;
	return 0;
}

static void write_command(unsigned int cmd)
{
	wait_in_empty();
	outb(KBD_STATUS, cmd);
}

static void write_data(unsigned int v)
{
	wait_in_empty();
	outb(KBD_DATA, v);
}

void input_init(void)
{
	unsigned int cfg;
	unsigned int n;

	pic_init();                          /* master: IRQ0 and IRQ1 unmasked */
	kbd_reset(&dec);

	write_command(0xad);                 /* disable keyboard port */
	write_command(0xa7);                 /* disable aux port */
	for (n = 0; n < 16; n++) {           /* discard stale output */
		if ((inb(KBD_STATUS) & ST_OUT_FULL) == 0)
			break;
		(void) inb(KBD_DATA);
		kbd_flushed++;
	}

	write_command(0x20);                 /* read command byte */
	if (!wait_out_full())
		cfg = 0x40;                      /* no answer: assume translation */
	else
		cfg = inb(KBD_DATA);
	cfg |= 0x01;                         /* IRQ1 enabled */
	cfg |= 0x40;                         /* translation to set 1 */
	cfg &= ~0x10u;                       /* keyboard clock enabled */
	cfg &= ~0x02u;                       /* IRQ12 disabled */
	write_command(0x60);                 /* write command byte */
	write_data(cfg);

	write_command(0xae);                 /* enable keyboard port */
#ifdef QDOS_VCON
	cin_features |= 0x0001u;             /* R12: this driver delivers key codes (CON_CAP_KEYS) */
#endif
}

void input_irq1(void)
{
	unsigned int st = inb(KBD_STATUS);
	unsigned char sc;
	unsigned char c;

	if ((st & (ST_OUT_FULL | ST_AUX)) == ST_OUT_FULL) {
		sc = (unsigned char) inb(KBD_DATA);
		kbd_scancodes++;
#ifdef QDOS_VCON
		{
			int r = kbd_feed(&dec, sc, &c);
#ifdef QDOS_R13A
			{
				static int e0;              /* R13A: native key events (kernel/kev.h) */
				if (sc == 0xe0)
					e0 = 1;
				else if (sc != 0xe1) {
					kev_scancode(sc, e0, r == 2 || r == 3, r == 1 ? c : 0u,
					             (dec.shift ? KEV_SHIFT : 0u) | (dec.ctrl ? KEV_CTRL : 0u) | (dec.alt ? KEV_ALT : 0u));
					e0 = 0;
				}
			}
#endif
			if (r == 1)
				cin_put(c);
#ifdef QDOS_TASKMGR
			else if (r == 2)
				taskmgr_console_key((int) c);  /* R12C: Alt+Fn (closes an open task selector first) */
			else if (r == 3)
				taskmgr_hotkey();              /* R12C: Ctrl+Esc */
#else
			else if (r == 2)
				(void) vcon_switch((int) c);    /* Alt+Fn: the focus moves, no byte is queued */
#endif
		}
#else
		if (kbd_feed(&dec, sc, &c))
			cin_put(c);
#endif
	}
	outb(0x20, 0x20);                    /* EOI, master */
}

int input_poll(void)
{
	return INPUT_NONE;
}

/* Aux bytes are discarded: the mouse is disabled.  Their pending output would
 * otherwise hold the shared output buffer full, and the controller then gives
 * no IRQ1 for keyboard bytes (see docs/R7-RUNTIME-REPORT.md). */
volatile unsigned long kbd_aux_drained = 0;

void input_tick(void)
{
	unsigned int st = inb(KBD_STATUS);
	if ((st & (ST_OUT_FULL | ST_AUX)) == (ST_OUT_FULL | ST_AUX)) {
		(void) inb(KBD_DATA);
		kbd_aux_drained++;
	}
}
