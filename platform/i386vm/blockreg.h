#ifndef QDOS386_I386VM_BLOCKREG_H
#define QDOS386_I386VM_BLOCKREG_H

/*
 * R11I: byte registers of the i386VM host-file block device.  The harness
 * (patches/i386vm-block.patch, harness/i386vm-block/blockdev.h) defines the
 * same addresses; the protocol is docs/R11I-BLOCK-PROTOCOL.md.  Every access
 * is one byte wide: the harness refuses 16- and 32-bit accesses here.
 */
#define BLK_REG_CMD     0x00800010UL
#define BLK_REG_LBA0    0x00800011UL   /* LBA0..LBA3 at +0..+3 */
#define BLK_REG_STATUS  0x00800015UL
#define BLK_REG_DATA    0x00800016UL
#define BLK_REG_COUNT0  0x00800017UL   /* COUNT0..COUNT3 at +0..+3 */

#define BLK_CMD_READ    0x01
#define BLK_ST_OK       0x00

#endif
