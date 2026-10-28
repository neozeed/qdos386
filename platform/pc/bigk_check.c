/*
 * R11K TEST ONLY (-DQDOS_TEST_BIGK): verify the padding data built by tools/r11k-gen-bigk.py, which
 * sits at the END of the kernel image.  Proves that every kernel sector was read, checksummed and
 * copied to the right place by the loader.  Output: "BIGK: <n> bytes verified" or the first bad offset.
 */
#include "platform.h"

extern unsigned long bigk_start[];
extern unsigned long bigk_end[];
extern unsigned long bigk_words;

static void put_hex(unsigned long v)
{
	int i;

	con_puts("0x");
	for (i = 28; i >= 0; i -= 4)
		con_putc("0123456789abcdef"[(v >> i) & 15]);
}

int bigk_check(void)
{
	unsigned long n = bigk_words;
	unsigned long i;
	unsigned long want;

	if ((unsigned long) (bigk_end - bigk_start) != n) {
		con_puts("BIGK: layout mismatch\n");
		return -1;
	}
	for (i = 0; i < n; i++) {
		want = i * 2654435761UL + 0x1234567UL;
		if (bigk_start[i] != want) {
			con_puts("BIGK: MISMATCH at byte offset ");
			con_putdec(i * 4);
			con_putc('\n');
			return -1;
		}
	}
	con_puts("BIGK: ");
	con_putdec(n * 4);
	con_puts(" bytes verified, base ");
	put_hex((unsigned long) bigk_start);
	con_putc('\n');
	return 0;
}
