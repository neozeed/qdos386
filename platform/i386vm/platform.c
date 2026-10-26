/*
 * QDOS/386 R1 - i386VM harness platform.
 * Console: byte MMIO at 0x800001.  Timer: synthetic register at 0x800003
 * (the i386VM patch).  Exit: HLT with the status in EAX.
 */
#include "platform.h"

#define CON_PORT ((volatile unsigned char *) 0x800001)
#define TIMER_PORT ((volatile unsigned char *) 0x800003)

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

/* period in units of 1024 executed steps; the harness counts steps. */
void timer_start(unsigned int period_units)
{
	*TIMER_PORT = (unsigned char) period_units;
}

void timer_stop(void)
{
	*TIMER_PORT = 0;
}

int timer_device_present(void)
{
	return 1;
}

unsigned int timer_device_count(void)
{
	return (unsigned int) *TIMER_PORT;
}

void timer_ack(void)
{
	/* no interrupt controller in the harness */
}

void halt_with(int status)
{
	extern void halt_with_asm(int status);
	halt_with_asm(status);
	for (;;)
		;
}
