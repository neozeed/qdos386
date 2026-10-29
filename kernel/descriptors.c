/*
 * QDOS/386 R1 - flat GDT and IDT (shared by both platforms).
 *   GDT: null, code 0x08 (flat 4 GiB, DPL 0), data 0x10 (flat 4 GiB).
 *   IDT: IRQ0 at vector 0x20, IRQ7 at 0x27, IRQ15 at 0x2F, and the service
 *        trap at 0x30 (interrupt gates, type 0x8E, so IF is cleared on entry).
 *        Other vectors not present.
 */
#include "jobs.h"

extern void irq0_stub(void);
extern void trap_stub(void);
extern void irq7_stub(void);
extern void irq1_stub(void);
extern void irq15_stub(void);
extern void load_descriptors(unsigned int gdtr, unsigned int idtr);

#define IRQ0_VECTOR 0x20

#ifdef QDOS_DYNPROC
/* R12A: two more descriptors, 0x18 = main TSS (loaded into TR), 0x20 = #DF TSS (task gate at vector 8). */
#define GDT_BYTES 40
#else
#define GDT_BYTES 24
#endif
static unsigned char gdt[GDT_BYTES];
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

#ifdef QDOS_DYNPROC
/* ---- R12A task state segments ----------------------------------------------------------------------------------
 * kx_tss_main is the TSS loaded into TR at boot.  Nothing reads it in normal operation (no privilege change ever
 * happens); the CPU stores the interrupted state into it when #DF switches to kx_tss_df, and proc_double_fault
 * (proc.c) may rewrite that state before the task return.  Word index = byte offset / 4 (cr3 7, eip 8, eflags 9,
 * esp 14, es 18, cs 19, ss 20, ds 21, fs 22, gs 23, ldt 24, I/O map base in the high half of 25). */
extern void kx_ltr(unsigned int sel);
extern void kx_df_entry(void);
unsigned int kx_tss_main[26];
unsigned int kx_tss_df[26];
static unsigned int kx_df_stack[1024];             /* 4 KiB, kernel BSS (identity-mapped everywhere) */

static void tss_desc(unsigned char *d, unsigned long base)
{
	put32(d, ((base & 0xffffUL) << 16) | 103UL);   /* limit 103 (104-byte TSS) */
	put32(d + 4, (base & 0xff000000UL) | 0x00008900UL | ((base >> 16) & 0xffUL));   /* P, DPL 0, type 9 */
}

static void kx_tss_descriptors(void)
{
	int i;

	for (i = 0; i < 26; i++) {
		kx_tss_main[i] = 0;
		kx_tss_df[i] = 0;
	}
	kx_tss_main[25] = 104u << 16;
	kx_tss_df[8] = (unsigned int) kx_df_entry;
	kx_tss_df[9] = 0x2u;
	kx_tss_df[14] = (unsigned int) &kx_df_stack[1020];
	kx_tss_df[19] = 0x08u;
	kx_tss_df[18] = kx_tss_df[20] = kx_tss_df[21] = kx_tss_df[22] = kx_tss_df[23] = 0x10u;
	kx_tss_df[25] = 104u << 16;
	tss_desc(gdt + 0x18, (unsigned long) kx_tss_main);
	tss_desc(gdt + 0x20, (unsigned long) kx_tss_df);
}

/* proc.c, once the template directory exists: the #DF task runs on it. */
void kx_tss_init(unsigned int template_cr3)
{
	kx_tss_df[7] = template_cr3;
}
#endif

void descriptors_init(void)
{
	int i;

	put32(gdt + 8, 0x0000ffffUL);       /* code: base 0, limit low      */
	put32(gdt + 12, 0x00cf9a00UL);      /* G=1 D=1, access 9A, limit hi */
	put32(gdt + 16, 0x0000ffffUL);      /* data                         */
	put32(gdt + 20, 0x00cf9200UL);      /* G=1 D=1, access 92           */
#ifdef QDOS_DYNPROC
	kx_tss_descriptors();
#endif
	put16(gdtr, GDT_BYTES - 1);
	put32(gdtr + 2, (unsigned long) gdt);

	for (i = 0; i < 256 * 8; i++)
		idt[i] = 0;
	set_gate(IRQ0_VECTOR, (unsigned long) irq0_stub);
	set_gate(TRAP_VECTOR, (unsigned long) trap_stub);
	set_gate(0x21, (unsigned long) irq1_stub);          /* master IRQ1: keyboard */
	set_gate(0x27, (unsigned long) irq7_stub);          /* master IRQ7 */
	set_gate(0x2f, (unsigned long) irq15_stub);         /* slave IRQ7 = IRQ15 */
#ifdef QDOS_PAGING
	/* R11J: exception vectors 0..19 report and end a faulting paged job (paging.c);
	 * 20..31 are reserved and stay not present. */
	{
		extern unsigned long pg_exc_table[20];
		for (i = 0; i < 20; i++)
			set_gate(i, pg_exc_table[i]);
	}
#endif
#ifdef QDOS_DYNPROC
	/* #DF through a task gate: the CPU switches to the #DF TSS (own stack, template directory), so a stack overflow into
	 * an unmapped guard page does not become a triple fault.  P=1, DPL=0, type 5 (task gate), selector 0x20. */
	{
		unsigned char *g = idt + 8 * 8;
		put16(g, 0);
		put16(g + 2, 0x20);
		g[4] = 0;
		g[5] = 0x85;
		put16(g + 6, 0);
	}
#endif
	put16(idtr, 256 * 8 - 1);
	put32(idtr + 2, (unsigned long) idt);

	load_descriptors((unsigned int) gdtr, (unsigned int) idtr);
#ifdef QDOS_DYNPROC
	kx_ltr(0x18);
#endif
}
