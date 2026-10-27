/* PS: lists the live job slots with their identity and priority, using only
 * the public qd_job_* wrappers.  Slots are probed, not read from kernel memory. */
#include <stdio.h>
#include "qdos386.h"

int main(void)
{
	int i, id, self, shown = 0;
	unsigned int inc, acc;

	self = qd_self();
	printf("PS: self %d\n", self);
	for (i = 0; i < 8; i++) {
		id = qd_job_slot((unsigned int) i);
		if (id < 0)
			continue;
		if (qd_job_info(id, &inc, &acc) != 0)
			continue;
		printf("PS slot %d id %d prio %u acc %u %s%s\n", i, id, inc, acc,
		       qd_job_reclaimed(id) ? "free" : "live",
		       id == self ? " (self)" : "");
		shown++;
	}
	printf("PS: %d jobs listed\n", shown);
	return 0;
}
