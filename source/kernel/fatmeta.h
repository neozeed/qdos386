/* Shared bounded FAT metadata inspection, not a second pathname/file driver.
 * Uses the production FAT engine's sector callback ABI. All LBAs are physical. */
#ifndef QDOS_FATMETA_H
#define QDOS_FATMETA_H
#include "fat16.h"
typedef char fm_word_must_be_32_bits[(sizeof(unsigned int)==4)?1:-1];
#define FM_MAX_CLUSTERS 262144u
#define FM_MAX_DEPTH 32u
struct fm_device {
 void *ctx;
 fat_read_fn read;
 int (*write)(void *, unsigned int, const unsigned char *);
 int (*flush)(void *);
 unsigned int sectors, identity;
};
struct fm_partition { unsigned int start, count, type, active; };
struct fm_volume {
 struct fm_device *dev;
 unsigned int start, count, bits, spc, reserved, nfats, spf;
 unsigned int root, root_entries, data, clusters, active, mirrored, info, backup;
 unsigned char boot[512], fat_cache[512];
 unsigned int fat_cached;
};
unsigned int fm_u16(const unsigned char *, unsigned int);
unsigned int fm_u32(const unsigned char *, unsigned int);
void fm_p16(unsigned char *, unsigned int, unsigned int);
void fm_p32(unsigned char *, unsigned int, unsigned int);
int fm_read(struct fm_device *, unsigned int, unsigned char *);
int fm_write(struct fm_device *, unsigned int, const unsigned char *);
int fm_mbr(const unsigned char *, unsigned int, struct fm_partition *);
int fm_table_valid(unsigned int, const struct fm_partition *);
void fm_table_encode(unsigned char *, const struct fm_partition *);
int fm_mount(struct fm_volume *, struct fm_device *, unsigned int, unsigned int);
int fm_format(struct fm_device *, unsigned int, unsigned int, unsigned int);
/* Issue classes: 1 chain,2 cross-link,3 lost,4 directory,5 long name,
 * 6 FAT copies,7 size,8 FSInfo,9 backup BPB,10 reserved FAT entries.
 * Repair suggestions are emitted only AFTER the complete scan succeeds.
 * A negative scan return means I/O/unsupported, never 'clean'. */
struct fm_check {
 unsigned int *owners, capacity, errors, files, dirs, free_count, lost;
 void (*issue)(void *, unsigned int, unsigned int, const char *);
 void *ctx;
};
int fm_scan(struct fm_volume *, struct fm_check *);
int fm_fix_info(struct fm_volume *, unsigned int);
#endif
