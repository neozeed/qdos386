/*
 * QDOS/386 R14 - PC UART serial ports (8250 / 16450 / 16550A), SER1/SER2 channels and the QT_SER kernel call.
 * Built only with -DQDOS_R14 (docs/R14-SERIAL-ABI.md).
 */
#ifndef QDOS386_UART_H
#define QDOS386_UART_H

#define QT_SER          0x3C      /* a = SER_OP_*, b, c -> see below */

/* operations (a); b is a channel id unless stated */
#define SER_OP_PROBE    0         /* b = port 1|2 -> SER_T_* */
#define SER_OP_CONFIG   1         /* c = struct ser_cfg * -> 0 or error */
#define SER_OP_GETCFG   2         /* c = struct ser_cfg * -> 0 */
#define SER_OP_STATUS   3         /* c = struct ser_status * -> 0 */
#define SER_OP_WAIT     4         /* c = (flags << 24) | ticks (SER_TICKS_FOREVER) -> ready mask, 0 after a sleep, -51, error */
#define SER_OP_POLL     5         /* c = flags -> ready mask & flags, -51 (cancel pending; consumed), or 0 */
#define SER_OP_CANCEL   6         /* b = port 1|2 (any job) -> 0 */
#define SER_OP_FLUSH    7         /* c = SER_FLUSH_* -> 0 */
#define SER_OP_DEFAULT  8         /* b = port 1|2, c = baud: rate used by the next open (0 = 9600) -> 0 or -15 */

#define SER_T_ABSENT    0
#define SER_T_8250      1
#define SER_T_16450     2
#define SER_T_16550A    3

/* wait flags / ready mask */
#define SER_RX          0x01      /* received bytes buffered */
#define SER_TXSPACE     0x02      /* room in the transmit ring */
#define SER_TXEMPTY     0x04      /* transmit ring and UART transmitter empty */
#define SER_KEY         0x08      /* input queued on the caller's console */
#define SER_TICKS_FOREVER 0xFFFFFFu

#define SER_FLUSH_RX    1
#define SER_FLUSH_TX    2

#define SER_ERR_TIMEOUT (-1)      /* QDOS "not complete" */
#define SER_ERR_INUSE   (-9)      /* QDOS "in use": the port is open */
#define SER_ERR_BADPARM (-15)     /* unsupported setting (CHAN_ERR_NOTSUP) */
#define SER_ERR_CANCEL  (-51)     /* the wait was cancelled */

#define SER_RXSZ        4096u
#define SER_TXSZ        2048u

struct ser_cfg {                  /* 8 bytes, the same in libqdos (qdos386.h struct qd_ser_cfg) */
	unsigned long baud;           /* divides 115200 */
	unsigned char data_bits;      /* 5..8 */
	unsigned char parity;         /* 'N' 'O' 'E' 'M' 'S' */
	unsigned char stop_bits;      /* 1, 2 */
	unsigned char handshake;      /* 'I' 'H' */
};

struct ser_status {               /* 16 x 32 bits, the same in libqdos (struct qd_ser_status) */
	unsigned long type;           /* SER_T_* */
	unsigned long rx_bytes, tx_bytes;
	unsigned long rx_queued, tx_queued;
	unsigned long overrun, parity, framing, breaks;
	unsigned long rx_dropped, tx_discarded;
	unsigned long msr, lsr;       /* last modem / line status read */
	unsigned long irqs;           /* interrupts serviced for this port since boot */
	unsigned long cancels;        /* cancels delivered */
	unsigned long rts;            /* 1 if RTS is raised (handshake H) */
};

void ser_boot(void);                    /* probe COM1/COM2, print SERIAL: lines, install the IRQ gates */
int ser_trap(unsigned int a, unsigned int b, unsigned int c, unsigned int *eax);   /* 1 = schedule after a sleep */
void ser_tick(void);                    /* from the timer interrupt: deadlines, transmitter-empty waits */
void ser_irq3(void);                    /* called by the stubs in kernel/uart_s.s */
void ser_irq4(void);

#endif
