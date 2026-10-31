/* PINFO.AOU (R12A): process and memory summary.  Prints the process ABI capabilities, the frame pool (total and free
 * frames), the kernel heap's free bytes, the live processes, and this process's own layout. */
#include <stdio.h>
#include "qdos386.h"

int main(void)
{
	struct qd_procinfo p;
	int caps = qd_proc_caps();

	if (caps < 0 || qd_proc_info(0, &p) != 0) {
		printf("PINFO: no R12A process services on this kernel (%d)\n", caps);
		return 1;
	}
	printf("PINFO caps %08x  pool %lu frames (%lu KiB), free %lu  kernel heap free %u  processes %lu\n",
	       (unsigned int) caps, p.pool_frames, p.pool_frames * 4ul, p.pool_free, qd_free_bytes(), p.procs_live);
	printf("PINFO self id %lu origin %06lx image end %06lx heap %08lx..%08lx stack %08lx..%08lx frames %lu\n",
	       p.id, p.origin, p.image_end, p.heap_base, p.heap_brk, p.stack_lo, p.stack_hi, p.frames);
	return 0;
}
