/* Current production entry. shell_boot owns SYSTEM, idle/AP/worker and device initialization. */
#include "shellboot.h"

int main(void)
{
	return shell_boot();
}
