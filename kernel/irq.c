/*
 * QDOS/386 R4 - hardware interrupt classification (see irq.h).
 */
#include "irq.h"
#include "platform.h"

volatile unsigned long irq_spurious_master7 = 0;
volatile unsigned long irq_genuine_master7 = 0;
volatile unsigned long irq_spurious_slave15 = 0;
volatile unsigned long irq_genuine_slave15 = 0;

int irq_isr_bit_set(unsigned int isr, int bit)
{
	return (isr >> bit) & 1u;
}

void irq7_dispatch(void)
{
	if (pic_present() && irq_isr_bit_set(pic_master_isr(), 7)) {
		irq_genuine_master7++;
		pic_eoi_master();
	} else {
		irq_spurious_master7++;      /* spurious: no EOI */
	}
}

void irq15_dispatch(void)
{
	if (pic_present() && irq_isr_bit_set(pic_slave_isr(), 7)) {
		irq_genuine_slave15++;
		pic_eoi_slave();
		pic_eoi_master();
	} else {
		irq_spurious_slave15++;
		pic_eoi_master();            /* master IRQ2 is in service */
	}
}
