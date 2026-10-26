/*
 * QDOS/386 R1 - platform services.  Implemented in platform/i386vm or
 * platform/pc.  The shared kernel uses only this interface.
 */
#ifndef QDOS386_PLATFORM_H
#define QDOS386_PLATFORM_H

void con_putc(unsigned char c);
void con_puts(const char *s);
void con_putdec(unsigned long v);

/* Start periodic IRQ0.  Units are platform-defined; see each platform. */
void timer_start(unsigned int period_units);
void timer_stop(void);

/* Device-side tick count, if the platform exposes one (returns 0 if not). */
int timer_device_present(void);
unsigned int timer_device_count(void);

/* End of interrupt for IRQ0. */
void timer_ack(void);

/* Cascaded 8259 access for interrupt classification (kernel/irq.c).
 * pic_present() is 0 on platforms without a PIC; the other calls are then
 * not made by the kernel for classification, and their results are 0. */
int pic_present(void);
unsigned int pic_master_isr(void);      /* in-service register, master */
unsigned int pic_slave_isr(void);       /* in-service register, slave */
void pic_eoi_master(void);
void pic_eoi_slave(void);

/* Stop the machine: finish the run with the given status. */
void halt_with(int status);

#endif
