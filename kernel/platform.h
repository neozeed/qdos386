/*
 * QDOS/386 R1 - platform services for the i386VM harness.
 * Console and timer are byte MMIO registers (see README.md for the map).
 */
#ifndef QDOS386_PLATFORM_H
#define QDOS386_PLATFORM_H

void con_putc(unsigned char c);
void con_puts(const char *s);
void con_putdec(unsigned long v);

void timer_start(unsigned char period_units);   /* period = units * 1024 steps */
void timer_stop(void);
unsigned int timer_delivered(void);             /* device count, low 8 bits */

void descriptors_init(void);                    /* flat GDT, IDT with IRQ0 gate */
unsigned int read_cr0(void);
void halt_with(int status);

#define IRQ0_VECTOR 0x20

#endif
