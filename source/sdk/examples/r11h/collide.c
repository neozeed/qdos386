/* R11H COLLIDE: linked for slot 2 (0x380000), the same origin as MULTIA.  The
 * launcher must be refused while MULTIA is resident, so this must never run. */
#include <stdio.h>
#include "qdos386.h"

int main(void)
{
	printf("COLLIDE ran: origin occupied, this is a failure\n");
	return 1;
}
