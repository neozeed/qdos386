/*
 * QDOS/386 R5 application 1: channel output and exit.
 * Opens the console channel through QT_CH_OPEN, writes one message through
 * QT_CH_WRITE, closes the channel and returns 7 (the value SYSTEM checks).
 */
#include "appcalls.h"

static const char msg[] = "Hello from an independently linked i386 a.out!\n";

int main(void)
{
	int ch, n, rc = 0;

	ch = qtrap(APP_QT_CH_OPEN, (unsigned int) "CON", 0, 0);
	if (ch <= 0)
		return 1;
	n = qtrap(APP_QT_CH_WRITE, (unsigned int) ch, (unsigned int) msg, sizeof msg - 1);
	if (n != (int) (sizeof msg - 1))
		rc |= 2;
	if (qtrap(APP_QT_CH_CLOSE, (unsigned int) ch, 0, 0) != 0)
		rc |= 4;
	if (rc != 0)
		return rc;
	return 7;
}
