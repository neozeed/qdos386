/* Legacy tests are kernel clients, not loaded user applications. Keep the public
 * user-range predicate strict while admitting their kernel-owned service buffers.
 * Built only in the explicit legacy diagnostic composition. */
#include "jobs.h"
#include "aout.h"
#include "channel.h"
extern char bss_end[];

int legacy_service_range_ok(unsigned int addr, unsigned int n)
{
	unsigned int id = job_current_id(), end = addr + n;
	int i;
	if (chan_user_range_ok(addr, n)) return 1;
	if (!addr || end < addr) return 0;
	/* Loaded applications retain the original user-pointer checks. */
	for (i = 0; i < APP_PROFILES; i++)
		if (id && (unsigned int) app_slot_owner(i) == id) return 0;
	return addr >= 0x00100000u && end <= (unsigned int) bss_end;
}
