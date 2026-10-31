/* QLTerm C: source-derived port of Jan Bredenbeek's QLTerm 3.00b2.
 * GPL-3.0-or-later. See LICENSE and reference/. */
#ifndef QLTERM_H
#define QLTERM_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <qdos.h>
/* The supplied libc declares colour_t as a byte, but these stubs fetch a
 * WORD from the argument slot. GCC leaves byte-argument padding undefined.
 * Call their actual word ABI to avoid random stipple/colour values. */
#undef sd_setin
#undef sd_setst
#undef sd_setpa
#undef sd_wdef
#define sd_setin(ch,to,c) (((int (*)(chanid_t,timeout_t,short))_sd_setin)((ch),(to),(short)(c)))
#define sd_setst(ch,to,c) (((int (*)(chanid_t,timeout_t,short))_sd_setst)((ch),(to),(short)(c)))
#define sd_setpa(ch,to,c) (((int (*)(chanid_t,timeout_t,short))_sd_setpa)((ch),(to),(short)(c)))
#define sd_wdef(ch,to,c,w,r) (((int (*)(chanid_t,timeout_t,short,short,QLRECT_t *))_sd_wdef)((ch),(to),(short)(c),(w),(r)))
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
#define ESC 27
#define CAN 24
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 21
#define IAC 255
#define P_CUSTOM 4
#define P_TCP 5
#define P_TELNET 6
#define XMODEM 0
#define XMODEM1K 1
#define YMODEM 2
#define YMODEMG 3
#define QL_EPOCH 283996800UL
#define QLT_NAME_MAX 48
#include "config.h"
typedef struct {
    chanid_t mainwin, cmdwin, guardwin;
    volatile chanid_t in, out, log;
    jobid_t screenjob, serialjob;
    U8 *queue;
    U32 qsize;
    volatile U32 head, tail;
    volatile U8 qlock, txlock;
    volatile int eof, connected, transfer, pause_screen, xoff;
    int redirected, pipes, status;
    int baud, echo, terminal, colour, convlf, cooked, port;
    int enterkey, delkey, protocol, buflen, bell, disable_tra;
    int cols, rows, saved_x, saved_y, inverse, ink, strip, paper;
    int escstate, paramlen, vtcmd, vtleft, space;
    char params[80];
    int got_iac, got_cmd, got_sub, sub_oper;
    char comname[QLT_NAME_MAX], hostname[38], filename[QLT_NAME_MAX];
    char dldir[50];
    int name_offset;
} Terminal;
extern Terminal t;
extern const U8 gr_set1[];
U16 be16(const U8 *p);
U32 be32(const U8 *p);
void put16(U8 *p, U16 n);
void put32(U8 *p, U32 n);
void wait_frames(int n);
int try_lock(volatile U8 *p);
void lock(volatile U8 *p);
void unlock(volatile U8 *p);
void screen_byte(U8 c);
void screen_reset(void);
void screen_job(void);
void serial_job(void);
int queue_put(U8 c);
int rxchar(int timeout, U8 *c);
int tx_raw(const void *p, int n);
int tx_data(const void *p, int n, int echo);
int tx_char(int c, int echo);
void key_send(int key);
void telnet_start(void);
void aux(const char *s);
void status_line(void);
int menu(const char *prompt, const char *const *items, int n, int current);
int ask_yes(const char *s);
int ask_file(int downloading);
chanid_t open_download(const char *name);
chanid_t open_upload(char *name);
void file_error(int code);
void upload(void);
void download(void);
U16 crc16(const U8 *p, int n);
void transfer_mode(int on);
int progress(const char *msg, U32 done, long total, U32 start);
void wait_media(const char *name);
void close_connection(void);
int open_connection(void);
void set_baud(void);
#endif
