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

/* Stop the machine: finish the run with the given status. */
void halt_with(int status);

#endif
