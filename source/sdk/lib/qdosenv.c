/* QDOS/386 R12B - libqdos environment calls (kernels built with -DQDOS_ENV).  A separate archive member.
 * On an older kernel the traps are unknown and every call answers -1.  See docs/R12B-ENV-PROCESS-ABI.md. */
#include "qdos386.h"

#define QD_TRAP_ENV_GET  0x35u
#define QD_TRAP_ENV_SET  0x36u
#define QD_TRAP_ENV_LIST 0x37u

int qd_env_get(const char *name, char *buf, unsigned int size)
{
	return (int) qtrap(QD_TRAP_ENV_GET, (unsigned int) name, (unsigned int) buf, size);
}

int qd_env_set(const char *name, const char *value)
{
	if (value == 0)
		return QD_ERR_RANGE;
	return (int) qtrap(QD_TRAP_ENV_SET, (unsigned int) name, (unsigned int) value, 0);
}

int qd_env_unset(const char *name)
{
	return (int) qtrap(QD_TRAP_ENV_SET, (unsigned int) name, 0, 0);
}

int qd_env_list(unsigned int index, char *buf, unsigned int size)
{
	return (int) qtrap(QD_TRAP_ENV_LIST, index, (unsigned int) buf, size);
}

int qd_env_summary(unsigned int *count, unsigned int *bytes)
{
	int r = (int) qtrap(QD_TRAP_ENV_LIST, 0xFFFFFFFFu, 0, 0);

	if (r < 0)
		return r;
	if (count)
		*count = (unsigned int) r >> 16;
	if (bytes)
		*bytes = (unsigned int) r & 0xFFFFu;
	return 0;
}
