#include "r6boot.h"
struct r6_boot_info r6_boot;
int r6_boot_state;
void r6_boot_capture(void)
{
 const unsigned int *p = (const unsigned int *) R6_BOOT_ADDR;
 unsigned int *d = (unsigned int *) &r6_boot;
 unsigned int i, sum = 0;
 r6_boot_state = 0;
 if (p[0] != R6_BOOT_MAGIC) return;
 r6_boot_state = -1;
 for (i = 0; i < 12u; i++) { d[i] = p[i]; sum += p[i]; }
 if (sum || sizeof(r6_boot) != 48u || r6_boot.version != 1u || r6_boot.length != 48u)
  return;
 if (r6_boot.partition >= 4u || r6_boot.bios_drive < 0x80u || r6_boot.bios_drive > 0xffu ||
     !r6_boot.disk_id || (r6_boot.flags & ~1u) || !r6_boot.start || !r6_boot.sectors ||
     !r6_boot.disk_sectors || r6_boot.disk_sectors > 0x10000000u ||
     r6_boot.start >= r6_boot.disk_sectors || r6_boot.sectors > r6_boot.disk_sectors - r6_boot.start ||
     (r6_boot.type != 4u && r6_boot.type != 6u && r6_boot.type != 14u)) return;
 r6_boot_state = 1;
}
