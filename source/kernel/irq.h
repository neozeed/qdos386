/*
 * QDOS/386 R4 - hardware interrupt classification for the cascaded 8259 pair.
 *
 * Vector 0x27 is master IRQ7, vector 0x2F is slave IRQ7 (IRQ15).  A real IRQ
 * is in service (its ISR bit is set) when the handler runs.  A spurious IRQ7
 * has no ISR bit set and is not acknowledged as though it were in service.
 *
 *   master IRQ7, genuine : EOI master
 *   master IRQ7, spurious: no EOI
 *   slave IRQ15, genuine : EOI slave, then EOI master (IRQ2 is in service)
 *   slave IRQ15, spurious: EOI master only (IRQ2 is in service, slave ISR is not)
 *
 * The platform supplies the PIC access (platform/pc).  Platforms with no PIC
 * (i386VM) report pic_present() == 0 and every hardware IRQ7 counts as spurious.
 */
#ifndef QDOS386_IRQ_H
#define QDOS386_IRQ_H

extern volatile unsigned long irq_spurious_master7;
extern volatile unsigned long irq_genuine_master7;
extern volatile unsigned long irq_spurious_slave15;
extern volatile unsigned long irq_genuine_slave15;

/* Pure rule: 1 if ISR bit 'bit' (0..7) is set in 'isr', else 0. */
int irq_isr_bit_set(unsigned int isr, int bit);

/* Handlers called from the vector 0x27 and 0x2F stubs (kernel/context.s). */
void irq7_dispatch(void);
void irq15_dispatch(void);

/* Software injection of the two vectors, for the guest tests. */
void sw_int27(void);
void sw_int2f(void);

#endif
