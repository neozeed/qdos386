/*
 * QDOS/386 R1 - flat GDT and IDT (shared by both platforms).
 *   GDT: null, code 0x08 (flat 4 GiB, DPL 0), data 0x10 (flat 4 GiB).
 *   IDT: IRQ0 at vector 0x20, spurious IRQ7 at 0x27 and 0x2F, and the service
 *        trap at 0x30 (interrupt gates, type 0x8E, so IF is cleared on entry).
 *        Other vectors not present.
 */
#include "jobs.h"

extern void irq0_stub(void);
extern void trap_stub(void);
extern void spurious_stub(void);
extern void load_descriptors(unsigned int gdtr, unsigned int idtr);

#define IRQ0_VECTOR 0x20

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

static void set_gate(int vector, unsigned long handler)
{
	unsigned char *g = idt + vector * 8;
	put16(g, handler & 0xffff);
	put16(g + 2, 0x08);
	g[4] = 0;
	g[5] = 0x8e;
	put16(g + 6, (handler >> 16) & 0xffff);
}

void descriptors_init(void)
{
	int i;

	put32(gdt + 8, 0x0000ffffUL);       /* code: base 0, limit low      */
	put32(gdt + 12, 0x00cf9a00UL);      /* G=1 D=1, access 9A, limit hi */
	put32(gdt + 16, 0x0000ffffUL);      /* data                         */
	put32(gdt + 20, 0x00cf9200UL);      /* G=1 D=1, access 92           */
	put16(gdtr, 24 - 1);
	put32(gdtr + 2, (unsigned long) gdt);

	for (i = 0; i < 256 * 8; i++)
		idt[i] = 0;
	set_gate(IRQ0_VECTOR, (unsigned long) irq0_stub);
	set_gate(TRAP_VECTOR, (unsigned long) trap_stub);
	set_gate(0x27, (unsigned long) spurious_stub);      /* master IRQ7 */
	set_gate(0x2f, (unsigned long) spurious_stub);      /* slave IRQ7 */
	put16(idtr, 256 * 8 - 1);
	put32(idtr + 2, (unsigned long) idt);

	load_descriptors((unsigned int) gdtr, (unsigned int) idtr);
}
