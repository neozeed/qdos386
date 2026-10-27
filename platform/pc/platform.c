/*
 * QDOS/386 R1 - Tiny386 PC platform.
 *   Console: Tiny386 debug port 0x402 (one byte per OUT).
 *   Timer:   8254 PIT channel 0 (mode 3) on IRQ0 through an 8259 PIC.
 *            The PIT runs on host time, so tick-based output order varies;
 *            the checks in main.c are written to allow for this.
 *   Exit:    the "Shutdown" byte sequence to port 0x8900 stops the emulator.
 */
#include "platform.h"

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);

#define PIT_CLOCK 1193182UL

static int pic_ready = 0;
static unsigned int pit_divisor = 11932;   /* ~100 Hz */

#include "vga.h"

void con_putc(unsigned char c)
{
#ifdef QDOS_CON_VGA
	static int vga_ready = 0;
#endif

	outb(0x402, c);                    /* Tiny386 debug port: always */
#ifdef QDOS_CON_SERIAL
	/* Optional diagnostic backend (not the default): also send the byte to
	 * the first 16550 UART at 0x3F8, so QEMU can capture it with -serial. */
	outb(0x3f8, c);
#endif
#ifdef QDOS_CON_VGA
	/* Optional human console: 80x25 text at 0xB8000 (platform/pc/vga.c). */
	if (!vga_ready) {
		vga_init();
		vga_ready = 1;
	}
	vga_putc(c);
#endif
}

void con_puts(const char *s)
{
	while (*s)
		con_putc((unsigned char) *s++);
}

void con_putdec(unsigned long v)
{
	char buf[12];
	int n = 0;
	int i;

	if (v == 0)
		con_putc('0');
	while (v != 0) {
		buf[n++] = (char) ('0' + (int) (v % 10));
		v = v / 10;
	}
	for (i = n - 1; i >= 0; i--)
		con_putc((unsigned char) buf[i]);
}

void pic_init(void)
{
	/* Master 8259 at 0x20/0x21: IRQ0 -> vector 0x20; IRQ0 and IRQ1 unmasked. */
	outb(0x20, 0x11);      /* ICW1: edge, cascade, ICW4 needed */
	outb(0x21, 0x20);      /* ICW2: vector base 0x20 */
	outb(0x21, 0x04);      /* ICW3: slave on IRQ2 */
	outb(0x21, 0x01);      /* ICW4: 8086 mode */
	outb(0x21, 0xfc);      /* OCW1: IRQ0 (timer) and IRQ1 (keyboard) */
	/* Slave 8259 at 0xA0/0xA1: all masked. */
	outb(0xa0, 0x11);
	outb(0xa1, 0x28);
	outb(0xa1, 0x02);
	outb(0xa1, 0x01);
	outb(0xa1, 0xff);
	pic_ready = 1;
}

/* The PIT input clock divided by the programmed divisor (about 100.0017 Hz). */
void timer_rate(unsigned long *num, unsigned long *den)
{
	*num = PIT_CLOCK;
	*den = (unsigned long) pit_divisor;
}

/* Period units are ignored on the PC: 1 unit = 10 ms (100 Hz). */
void timer_start(unsigned int period_units)
{
	(void) period_units;
	if (!pic_ready)
		pic_init();
	outb(0x43, 0x36);                       /* channel 0, lo/hi, mode 3 */
	outb(0x40, pit_divisor & 0xff);
	outb(0x40, (pit_divisor >> 8) & 0xff);
}

void timer_stop(void)
{
	outb(0x43, 0x30);                       /* channel 0, one-shot: stop */
	outb(0x40, 0);
	outb(0x40, 0);
}

int timer_device_present(void)
{
	return 0;     /* no tick counter register on the PC platform */
}

unsigned int timer_device_count(void)
{
	return 0;
}

void timer_ack(void)
{
	outb(0x20, 0x20);                       /* non-specific EOI, master */
}

int pic_present(void)
{
	return 1;
}

/* OCW3 0x0B selects the in-service register for the next read of the
 * command port. */
unsigned int pic_master_isr(void)
{
	outb(0x20, 0x0b);
	return inb(0x20);
}

unsigned int pic_slave_isr(void)
{
	outb(0xa0, 0x0b);
	return inb(0xa0);
}

void pic_eoi_master(void)
{
	outb(0x20, 0x20);
}

void pic_eoi_slave(void)
{
	outb(0xa0, 0x20);
}

void halt_with(int status)
{
	const char *s = "Shutdown";
	(void) status;
	while (*s)
		outb(0x8900, (unsigned int) (unsigned char) *s++);
#ifdef QDOS_HALT_DEBUG_EXIT
	/* Optional diagnostic exit for QEMU's isa-debug-exit device (port 0xF4):
	 * QEMU exits with status (0x10 << 1) | 1 = 33. Not the default. */
	outb(0xf4, 0x10);
#endif
	for (;;)
		;
}
