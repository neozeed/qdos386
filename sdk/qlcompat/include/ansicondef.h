/*
 * QDOS/386 R14A - <ansicondef.h> of the C68 runtime, for QL C programs that select the LibVT console emulation
 * (Hack 1.03's QL port: main.c and termqd.c include it).  docs/R14A-LIBVT-ANSI.md, docs/R14A-QL-C-ABI.md.
 *
 * The declarations are those of C68 libc 4.24.5 include/ansicondef.h and include/fcntl.h (the library the QL Hack
 * binary was linked with: it carries "C68 libc.a v4.24.5 (gcc)"; source supplied with R11 as libc-4.24.5.tar.bz2):
 * the emulation bit values, the two-char ANSICONDEF and the console write vector
 *     int (*_conwrite)(struct UFB *, void *, unsigned int)
 * with C68's return protocol: > 0 this many bytes may be written unchanged, < 0 this many bytes were consumed,
 * 0 all done.  LibVT itself (ANSI_conwrite's terminal emulation) was not supplied; QDOS/386's ANSI_conwrite
 * (sdk/qlcompat/c68/c68vt.c) implements what Hack emits, on the process's own console session.
 * Differences: _conwrite is declared here and not in <fcntl.h> (QDOS/386's POSIX fcntl.h is used); struct UFB has
 * only the leading C68 fields (no termios) and is filled for the console stream only; the default vector is NULL
 * (plain QDOS/386 console output) instead of C68's default_conwrite.
 */
#ifndef _ANSICON_H_
#define _ANSICON_H_
#define VT52 0x01
#define VT100 0x02
#define ANSI1 0x04
#define ANSI2 0x08
#define ANSI3 0x10
#define ANSI4 0x20
typedef struct {
char emulation;
char csi;
} ANSICONDEF,*PANSICONDEF;
extern ANSICONDEF __ANSICONF__;
extern int ANSI_conwrite();

/* C68 <fcntl.h>: the user file block (leading fields only) and the console write vector */
#ifndef QDOS386_C68_UFB
#define QDOS386_C68_UFB
struct UFB {
	char ufbtyp;                 /* D_CON (2) for the console */
	char ufbst;
	short ufbflg;                /* 0: translation wanted (C68's UFB_NT is never set here) */
	long ufbfh;                  /* the QDOS/386 CON channel */
	long ufbfh1;
	long ufbjob;
	char *ufbnam;
};
typedef struct UFB UFB_t;
#define D_CON 2
extern int (*_conwrite)(struct UFB *, void *, unsigned int);
#endif
#endif
