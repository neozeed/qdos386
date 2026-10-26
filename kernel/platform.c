/*
 * QDOS/386 R1 - console, synthetic timer and descriptor tables.
 */
#include "platform.h"
#include "jobs.h"

#define CON_PORT ((volatile unsigned char *) 0x800001)
#define TIMER_PORT ((volatile unsigned char *) 0x800003)

/* From kernel/context.s */
extern void irq0_stub(void);
extern void load_descriptors(unsigned int gdtr, unsigned int idtr);


static unsigned char gdt[24];
static unsigned char idt[256 * 8];
static unsigned char gdtr[6];
static unsigned char idtr[6];

static void put16(unsigned char *p, unsigned int v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
}

static void put32(unsigned char *p, unsigned long v)
{
	p[0] = (unsigned char) (v & 0xff);
	p[1] = (unsigned char) ((v >> 8) & 0xff);
	p[2] = (unsigned char) ((v >> 16) & 0xff);
	p[3] = (unsigned char) ((v >> 24) & 0xff);
}

void con_putc(unsigned char c)
{
	*CON_PORT = c;
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

void timer_start(unsigned char period_units)
{
	*TIMER_PORT = period_units;     /* also clears the device tick count */
}

void timer_stop(void)
{
	*TIMER_PORT = 0;
}

unsigned int timer_delivered(void)
{
	return (unsigned int) *TIMER_PORT;
}

/*
 * Flat 32-bit code (0x08) and data (0x10) segments, base 0, limit 4 GiB.
 * IDT entries are zero (not present) except IRQ0, which uses an interrupt
 * gate (type 0x8E) to clear IF on entry.
 */
void descriptors_init(void)
{
	unsigned long gate;
	int i;

	/* null descriptor is already zero */
	put32(gdt + 8, 0x0000ffffUL);          /* code: limit 0xffff...        */
	put32(gdt + 12, 0x00cf9a00UL);         /* ...G=1 D=1, access 9A, limit 0xfffff */
	put32(gdt + 16, 0x0000ffffUL);         /* data: limit 0xffff...        */
	put32(gdt + 20, 0x00cf9200UL);         /* ...G=1 D=1, access 92        */

	put16(gdtr, 24 - 1);
	put32(gdtr + 2, (unsigned long) gdt);

	for (i = 0; i < 256 * 8; i++)
		idt[i] = 0;
	gate = (unsigned long) irq0_stub;
	put16(idt + IRQ0_VECTOR * 8, gate & 0xffff);
	put16(idt + IRQ0_VECTOR * 8 + 2, 0x08);
	idt[IRQ0_VECTOR * 8 + 4] = 0;
	idt[IRQ0_VECTOR * 8 + 5] = 0x8e;
	put16(idt + IRQ0_VECTOR * 8 + 6, (gate >> 16) & 0xffff);

	put16(idtr, 256 * 8 - 1);
	put32(idtr + 2, (unsigned long) idt);

	load_descriptors((unsigned int) gdtr, (unsigned int) idtr);
}

