/* Apollo R6 BIOS contract v1. All words are little-endian32; no compiler packing. */
#ifndef QDOS386_APOLLO_R6BOOT_H
#define QDOS386_APOLLO_R6BOOT_H
#define R6_BOOT_ADDR 0x900u
#define R6_BOOT_MAGIC 0x49423652u
struct r6_boot_info {
 unsigned int magic, version, length, bios_drive;
 unsigned int partition, start, sectors, disk_id;
 unsigned int flags, type, disk_sectors, checksum;
};
extern struct r6_boot_info r6_boot;
extern int r6_boot_state; /*0 legacy,1 validated contract,-1 malformed R6 contract*/
void r6_boot_capture(void);
int disk_select_boot(const struct r6_boot_info *b);
#endif
