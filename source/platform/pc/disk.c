/*
 * QDOS/386 R6 - Tiny386 PC disk: the FAT16 volume on the primary slave of the
 * PIIX3 IDE controller (ports 0x1F0-0x1F7, Tiny386 "hdb=" option).
 *
 * Only IDENTIFY DEVICE (0xEC) and READ SECTORS (0x20) are issued, both with
 * LBA28 addressing and polled PIO.  Nothing is written.  The data port is read
 * one 16-bit word at a time.
 */
#include "platform.h"
#ifdef QDOS_APOLLO_R6
#include "r6boot.h"
#endif
#ifdef QDOS_R20
#include "r20w.h"
/* R20: the original routines are renamed; the wrappers at the end wait for the storage worker's claim (the
 * IDE controller has one owner at a time) and keep the sector cache coherent (kernel/r20w.c). */
#define disk_read_sector  disk_read_body
#define disk_write_sector disk_write_body
#define disk_flush        disk_flush_body
#define disk_sector_count disk_sector_count_body
#endif

extern void outb(unsigned int port, unsigned int val);
extern unsigned int inb(unsigned int port);
extern unsigned int inw(unsigned int port);

#define IDE_DATA     0x1f0
#define IDE_COUNT    0x1f2
#define IDE_LBA0     0x1f3
#define IDE_LBA1     0x1f4
#define IDE_LBA2     0x1f5
#define IDE_DRIVE    0x1f6
#define IDE_STATUS   0x1f7        /* read: status */
#define IDE_COMMAND  0x1f7        /* write: command */

#define ST_ERR  0x01
#define ST_DRQ  0x08
#define ST_DF   0x20
#define ST_BSY  0x80

#define IDE_CMD_READ      0x20
#define IDE_CMD_WRITE     0x30        /* R11L, QDOS_FS_WRITE only */
#define IDE_CMD_IDENTIFY  0xec
#ifdef QDOS_APOLLO_R6
static unsigned int ide_selected = 0xf0; /* legacy fallback; immutable after boot selection */
#define IDE_SLAVE_LBA ide_selected
#else
#define IDE_SLAVE_LBA    0xf0    /* LBA mode, slave drive (bit 4) */
#endif
#define IDE_POLL_LIMIT   2000000u

static int ide_state = 0;        /* 0 unknown, 1 present, -1 absent */
static unsigned long ide_sectors = 0;

/* Wait until BSY clears.  Returns the status, or -1 on timeout. */
static int ide_wait_ready(void)
{
	unsigned int i;
	for (i = 0; i < IDE_POLL_LIMIT; i++) {
		unsigned int st = inb(IDE_STATUS);
		if (!(st & ST_BSY))
			return (int) st;
	}
	return -1;
}

/* Wait until DRQ is set (data ready).  Returns 0, or negative on error. */
static int ide_wait_drq(void)
{
	unsigned int i;
	for (i = 0; i < IDE_POLL_LIMIT; i++) {
		int st = ide_wait_ready();
		if (st < 0)
			return -1;
		if (st & (ST_ERR | ST_DF))
			return -2;
		if (st & ST_DRQ)
			return 0;
	}
	return -3;
}

static void ide_read_words(unsigned char *buf, int words)
{
	int i;
	for (i = 0; i < words; i++) {
		unsigned int w = inw(IDE_DATA);
		buf[2 * i] = (unsigned char) (w & 0xff);
		buf[2 * i + 1] = (unsigned char) (w >> 8);
	}
}

/* IDENTIFY DEVICE on the slave: records the LBA28 sector count. */
static void ide_identify(void)
{
	unsigned char id[512];
	int st;

	ide_state = -1;
	outb(IDE_DRIVE, IDE_SLAVE_LBA);
	st = ide_wait_ready();
	if (st <= 0 || st == 0xff)           /* no drive answers */
		return;
	outb(IDE_COMMAND, IDE_CMD_IDENTIFY);
	if (ide_wait_drq() != 0)
		return;
	ide_read_words(id, 256);
	ide_sectors = (unsigned long) id[120] | ((unsigned long) id[121] << 8) |
	              ((unsigned long) id[122] << 16) | ((unsigned long) id[123] << 24);
	ide_state = (ide_sectors != 0) ? 1 : -1;
}

unsigned long disk_sector_count(void)
{
	if (ide_state == 0)
		ide_identify();
	return ide_state == 1 ? ide_sectors : 0;
}

int disk_read_sector(unsigned int lba, unsigned char *buf)
{
	if (disk_sector_count() == 0 || lba >= ide_sectors)
		return -1;
	if (ide_wait_ready() < 0)
		return -2;
	outb(IDE_DRIVE, (unsigned int) (IDE_SLAVE_LBA | ((lba >> 24) & 0x0f)));
	outb(IDE_COUNT, 1);
	outb(IDE_LBA0, lba & 0xff);
	outb(IDE_LBA1, (lba >> 8) & 0xff);
	outb(IDE_LBA2, (lba >> 16) & 0xff);
	outb(IDE_COMMAND, IDE_CMD_READ);
	if (ide_wait_drq() != 0)
		return -3;
	ide_read_words(buf, 256);
	return 0;
}

#ifdef QDOS_APOLLO_R6
static unsigned int boot_word(const unsigned char *p)
{ return (unsigned int)p[0] | ((unsigned int)p[1]<<8) | ((unsigned int)p[2]<<16) | ((unsigned int)p[3]<<24); }
int disk_select_boot(const struct r6_boot_info *b)
{
 unsigned char m[512];
 const unsigned char *e;
 int unit, found = -1, matches = 0;
 /* Boot-only, before any cached disk access or worker start; inspect raw devices. */
 for (unit = 0; unit < 2; unit++) {
  ide_selected = 0xe0u | ((unsigned int)unit << 4); ide_state = 0; ide_sectors = 0;
  if (disk_sector_count() != b->disk_sectors || disk_read_sector(0,m) != 0) continue;
  e = m + 446u + 16u*b->partition;
  if (m[510]!=0x55 || m[511]!=0xaa || boot_word(m+440)!=b->disk_id ||
      e[0]!=0x80u || e[4]!=b->type || boot_word(e+8)!=b->start || boot_word(e+12)!=b->sectors) continue;
  found = unit; matches++;
 }
 if (matches != 1) return -1;
 ide_selected = 0xe0u | ((unsigned int)found << 4); ide_state = 0; ide_sectors = 0;
 if (disk_sector_count() != b->disk_sectors) return -1;
 return found;
}
#endif

#ifdef QDOS_FS_WRITE
/* R11L: WRITE SECTORS (0x30), LBA28, polled PIO, one sector.  After the data the controller goes busy
 * while it writes; a clear BSY with ERR/DF clear means the sector was accepted.  A write-protected
 * disk (Tiny386 opens hdb read-write; QEMU read-only drives) answers ERR: reported as -2. */
static void ide_write_words(const unsigned char *buf, int words)
{
	int i;
	for (i = 0; i < words; i++) {
		unsigned int w = (unsigned int) buf[2 * i] | ((unsigned int) buf[2 * i + 1] << 8);
		__asm__ volatile ("outw %%ax,%%dx" : : "a" (w), "d" (IDE_DATA));
	}
}

#ifdef QDOS_SYNC
#define IDE_ERROR         0x1f1
#define IDE_CMD_FLUSH     0xe7
#define IDE_FLUSH_LIMIT   50000000u      /* a host fsync can take long; still bounded */

int disk_flush(unsigned int *err)
{
	unsigned int i, st = ST_BSY;

	*err = 0;
	if (disk_sector_count() == 0)
		return -1;
	if (ide_wait_ready() < 0)
		return -1;
	outb(IDE_DRIVE, IDE_SLAVE_LBA);
	outb(IDE_COMMAND, IDE_CMD_FLUSH);
	for (i = 0; i < IDE_FLUSH_LIMIT; i++) {
		st = inb(IDE_STATUS);
		if (!(st & ST_BSY))
			break;
	}
	if (st & ST_BSY)
		return -1;
	if (st & (ST_ERR | ST_DF)) {
		*err = inb(IDE_ERROR) & 0xffu;
		return -2;
	}
	return 0;
}
#endif

int disk_write_sector(unsigned int lba, const unsigned char *buf)
{
	int st;

	if (disk_sector_count() == 0 || lba >= ide_sectors)
		return -1;
	if (ide_wait_ready() < 0)
		return -1;
	outb(IDE_DRIVE, (unsigned int) (IDE_SLAVE_LBA | ((lba >> 24) & 0x0f)));
	outb(IDE_COUNT, 1);
	outb(IDE_LBA0, lba & 0xff);
	outb(IDE_LBA1, (lba >> 8) & 0xff);
	outb(IDE_LBA2, (lba >> 16) & 0xff);
	outb(IDE_COMMAND, IDE_CMD_WRITE);
	st = ide_wait_ready();
	if (st < 0)
		return -1;
	if (st & (ST_ERR | ST_DF))
		return -2;
	if (!(st & ST_DRQ))
		return -1;
	ide_write_words(buf, 256);
	st = ide_wait_ready();
	if (st < 0)
		return -1;
	if (st & ST_DF)
		return -1;
	if (st & ST_ERR)
		return -2;
	return 0;
}
#endif

#ifdef QDOS_R20
#undef disk_read_sector
#undef disk_write_sector
#undef disk_flush
#undef disk_sector_count

unsigned long disk_sector_count(void)
{
	if (r20c_wait_dev() != 0)
		return 0;
	return disk_sector_count_body();
}

/* The application-facing sector read.  Probe (kernel/r20w.c): a cache hit, or a restart request on a miss. */
int disk_read_sector(unsigned int lba, unsigned char *buf)
{
	return r20c_read(lba, buf);
}

/* The storage worker's own read: the worker holds the controller claim and runs with interrupts off. */
int disk_read_worker(unsigned int lba, unsigned char *buf)
{
	return disk_read_body(lba, buf);
}

#ifdef QDOS_FS_WRITE
int disk_write_sector(unsigned int lba, const unsigned char *buf)
{
	return r20c_write(lba, buf);
}
#endif

#ifdef QDOS_SYNC
int disk_flush(unsigned int *err)
{
	if (r20c_wait_dev() != 0) {
		if (err != 0)
			*err = 0xffffffffu;
		return -1;
	}
	return disk_flush_body(err);
}
#endif
#endif

#ifdef QDOS_APOLLO_R7
#include "disktool.h"
#include "channel.h"
#include "jobs.h"
#if !defined(QDOS_APOLLO_R6) || !defined(QDOS_R20) || !defined(QDOS_SYNC)
#error "R7 maintenance requires R6 boot identity, R20 ownership and flush"
#endif
/* Calls execute under the BKL. Waiting for the worker's controller claim is
 * mandatory before any select. No mounted volume exists on the other device.
 * That device deliberately bypasses the boot device's physical-LBA cache.
 * Restore the boot selection/state on EVERY exit before releasing the BKL. */
static unsigned int dt_owner[2];
static unsigned int dt_identity[2];
static unsigned int dt_hash(const unsigned char *id, unsigned int unit)
{
 unsigned int h = 2166136261u, i;
 for (i = 20; i < 40; i++) h = (h ^ id[i]) * 16777619u;
 for (i = 54; i < 94; i++) h = (h ^ id[i]) * 16777619u;
 for (i = 120; i < 124; i++) h = (h ^ id[i]) * 16777619u;
 h = (h ^ unit) * 16777619u;
 return h ? h : 1u;
}
int disktool_trap(unsigned int op, unsigned int addr, unsigned int owner)
{
 struct dt_request *q;
 unsigned char id[512];
 unsigned int saved_sel, hash, count, unit, err;
 unsigned long saved_count;
 int saved_state, rc = DT_ERR_IO, boot;
 if (op > DT_RELEASE || !chan_user_range_ok(addr, sizeof(struct dt_request))) return DT_ERR_ARG;
 q = (struct dt_request *)addr; unit = q->unit;
 if (unit > 1u || r6_boot_state != 1) return DT_ERR_ARG;
 if (r20c_wait_dev() != 0) return DT_ERR_BUSY;
 saved_sel = ide_selected; saved_state = ide_state; saved_count = ide_sectors;
 boot = unit == ((saved_sel >> 4) & 1u);
 ide_selected = 0xe0u | (unit << 4); ide_state = 0; ide_sectors = 0;
 outb(IDE_DRIVE, ide_selected);
 /* ATA select settling: four alternate-status reads (400 ns). */
 inb(0x3f6); inb(0x3f6); inb(0x3f6); inb(0x3f6);
 if (ide_wait_ready() <= 0 || inb(IDE_STATUS) == 0xff) goto done;
 outb(IDE_COUNT,0); outb(IDE_LBA0,0); outb(IDE_LBA1,0); outb(IDE_LBA2,0);
 outb(IDE_COMMAND,IDE_CMD_IDENTIFY);
 if (ide_wait_drq() != 0) goto done;
 ide_read_words(id,256);
 count = boot_word(id+120); hash = dt_hash(id,unit);
 /* LBA28 only, 512-byte logical sectors; reject newer non-512 geometry. */
 if (!(id[99] & 2u) || !count || count > 0x0fffffffu ||
     ((id[213] & 0xc0u) == 0x40u && (id[213] & 0x10u))) { rc=DT_ERR_ARG; goto done; }
 ide_state=1; ide_sectors=count;
 if (dt_owner[unit] && job_lookup(dt_owner[unit]) < 0) dt_owner[unit]=0;
 if (dt_owner[unit] && dt_owner[unit]!=owner) { rc=DT_ERR_BUSY; goto done; }
 if (op==DT_INFO) {
  q->identity=hash; q->sectors=count; q->flags=boot ? DT_BOOT : 0;
  rc=0; goto done;
 }
 if (q->identity!=hash || q->sectors!=count) { rc=DT_ERR_ID; goto done; }
 if (op==DT_READ) {
  if (q->lba>=count) { rc=DT_ERR_RANGE; goto done; }
  rc=disk_read_body(q->lba,q->data); goto done;
 }
 /* Refuse the WHOLE live disk, including unused partitions. */
 if (boot) { rc=DT_ERR_LIVE; goto done; }
 if (op==DT_CLAIM) { dt_owner[unit]=owner; dt_identity[unit]=hash; rc=0; goto done; }
 if (dt_owner[unit]!=owner || dt_identity[unit]!=hash) { rc=DT_ERR_BUSY; goto done; }
 if (op==DT_RELEASE) { dt_owner[unit]=0; rc=0; goto done; }
 if (op==DT_WRITE) {
  if (q->lba>=count) { rc=DT_ERR_RANGE; goto done; }
  rc=disk_write_body(q->lba,q->data);
 } else if (op==DT_FLUSH) rc=disk_flush_body(&err);
 done:
 ide_selected=saved_sel; ide_state=saved_state; ide_sectors=saved_count;
 outb(IDE_DRIVE,saved_sel);
 inb(0x3f6); inb(0x3f6); inb(0x3f6); inb(0x3f6);
 return rc;
}
#endif
