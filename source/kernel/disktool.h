/* Apollo R7 private native disk-maintenance ABI. C89, 32-bit words. */
#ifndef QDOS_DISKTOOL_H
#define QDOS_DISKTOOL_H
#define QT_DISKTOOL 0x46u
#define DT_INFO 0u
#define DT_READ 1u
#define DT_CLAIM 2u
#define DT_WRITE 3u
#define DT_FLUSH 4u
#define DT_RELEASE 5u
#define DT_BOOT 1u
#define DT_ERR_IO (-110)
#define DT_ERR_RANGE (-111)
#define DT_ERR_ID (-112)
#define DT_ERR_BUSY (-113)
#define DT_ERR_LIVE (-114)
#define DT_ERR_ARG (-115)
struct dt_request {
 unsigned int unit, identity, sectors, flags, lba;
 unsigned char data[512];
};
#ifdef QDOS_APOLLO_R7
int disktool_trap(unsigned int op, unsigned int addr, unsigned int owner);
#endif
#endif
