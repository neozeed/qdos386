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

/* Disk sector source for the FAT16 volume (R6).  Read-only use: the kernel
 * only ever calls disk_read_sector.  Returns 0 on success, negative on error.
 * disk_sector_count is the device size in 512-byte sectors (0 if absent). */
int disk_read_sector(unsigned int lba, unsigned char *buf);
unsigned long disk_sector_count(void);

/* Console input (R7).  input_init configures the keyboard device before
 * interrupts are enabled; on PC it sets up the i8042 and unmasks IRQ1.
 * input_irq1 is the IRQ1 body (PC; empty on i386VM, which has no PIC).
 * input_poll returns the next raw byte without blocking, INPUT_NONE when
 * none is waiting, or INPUT_EOF at end of input.  The PC always returns
 * INPUT_NONE: its bytes arrive by IRQ1. */
#define INPUT_NONE  (-1)
#define INPUT_EOF   (-2)
void input_init(void);
void input_irq1(void);
int input_poll(void);
/* Called on every timer tick, interrupts off.  The PC discards stale mouse
 * output that would otherwise hold the shared i8042 output buffer, and
 * with it the keyboard's IRQ1.  i386VM does nothing. */
void input_tick(void);

/* Stop the machine: finish the run with the given status. */
void halt_with(int status);

#endif
