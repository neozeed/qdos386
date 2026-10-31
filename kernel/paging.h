/*
 * QDOS/386 R11J/R11K - paging (opt-in: -DQDOS_PAGING; ABI version 2 in R11K).
 *
 * Nothing here is compiled into a default image.  See docs/R11J-ADDRESS-SPACE-ABI.md.
 *
 * Model
 *   - One kernel template directory identity-maps physical 0 .. PG_MAP_TOP (16 MiB) with
 *     4 KiB supervisor pages.  Kernel, heap, job stacks, IDT/GDT, boot stack, device MMIO
 *     and the transfer area are reached through it at their physical addresses.
 *   - A paged process has its own directory: the template, except that the common
 *     application window [PG_VBASE, PG_VBASE + PG_VSIZE) maps a private, physically
 *     contiguous frame run, and the frame pool [PG_POOL_LO, PG_POOL_HI) is not mapped.
 *     Page tables whose 4 MiB range touches neither are shared with the template.
 *   - Unpaged jobs (SYSTEM, IDLE, the shell, ordinary RUN applications) use the template.
 *   - Every stack and every kernel heap block is in the template's shared low memory, so
 *     a CR3 switch never removes the stack the switch code is running on.
 *   - Ring 0 only.  U/S bits are set as a convention (kernel pages supervisor, window
 *     pages user) but protect nothing at CPL 0.  This is not memory protection.
 */
#ifndef QDOS386_PAGING_H
#define QDOS386_PAGING_H

#include "appmap.h"
#include "aout.h"

#define PG_PAGE       0x1000u
#define PG_P          0x001u
#define PG_RW         0x002u
#define PG_US         0x004u
#define PG_MAP_TOP    0x01000000u            /* identity-mapped physical range */
#define PG_DIR_SPAN   0x00400000u            /* bytes per directory entry */
#define PG_DIRS_USED  (PG_MAP_TOP / PG_DIR_SPAN)

/* The common application window: the slot 1 profile (R9 map). */
#define PG_VBASE      APP_SLOT1_BASE
#define PG_VSIZE      APP_SLOT1_SIZE
#define PG_VEND       (PG_VBASE + PG_VSIZE)

/* Frame pool.  i386VM block mode: the R11I spare span.  Elsewhere (Tiny386 PC, QEMU,
 * i386VM legacy): the profile 2 and 3 spans, which the region allocator already holds
 * FREE.  QDOS_TEST_PG_POOL_BYTES caps the pool for the frame-exhaustion test image. */
#ifdef QDOS_I386VM_BLOCK
#define PG_POOL_LO    SPARE_BASE
#define PG_POOL_HI0   SPARE_END
#else
#define PG_POOL_LO    APP_PROFILE2_BASE
#define PG_POOL_HI0   (APP_PROFILE3_BASE + APP_PROFILE3_SIZE)
#endif
#ifdef QDOS_TEST_PG_POOL_BYTES
#define PG_POOL_HI    (PG_POOL_LO + QDOS_TEST_PG_POOL_BYTES)
#else
#define PG_POOL_HI    PG_POOL_HI0
#endif

#define PG_PROC_MAX     3                    /* live paged processes */
#define PG_PROC_TABLES  4                    /* private page tables per process, at most */
#define PG_OWNER_BASE   0x7F00u              /* region owner of a record's frames (< any job id) */

/* Service traps (R11J; paging builds only).  R11K adds QT_PG_CAPS and, in test builds, QT_PG_TEST. */
#define QT_PG_START   0x25   /* a = name, b = args (NUL-terminated strings), c = args length -> job id */
#define QT_PG_INFO    0x26   /* a = job id (0 = caller), b = struct pg_info pointer -> 0 */
#define QT_PG_CAPS    0x27   /* R11K: -> (PG_ABI_VERSION << 24) | (max paged spaces << 16) | capability bits */
#define QT_PG_CANCEL  0x29   /* R11K: a = job id of a HELD paged job that has never run -> 0 (frames, record, job released) */
#define QT_PG_TEST    0x28   /* R11K TEST ONLY (QDOS_TEST_PG_TASKS): a = counter index -> value */

/* ABI version 1 = R11J (QT_PG_START, QT_PG_INFO; any #PF in the window ends the job; other exceptions halt).
 * ABI version 2 = R11K (adds QT_PG_CAPS; every exception raised in a paged job's own window ends only that job). */
#define PG_ABI_VERSION  2
#define PG_CAP_START    0x0001u   /* QT_PG_START */
#define PG_CAP_INFO     0x0002u   /* QT_PG_INFO */
#define PG_CAP_ANYEXC   0x0004u   /* any recoverable exception in the window ends only that job */
#define PG_CAP_HOLD     0x0008u   /* hold-flag writes are made in the owner's address space */
#define PG_CAP_TASKS    0x0010u   /* poll/schedule-list tasks run under the template directory */
#define PG_CAP_HELD     0x0020u   /* QT_PG_START flag PG_START_HELD and QT_PG_CANCEL */

/* QT_PG_START: c = args_len | flags.  Bits 0..15 are the argument length; bits 16..31 are flags.
 * A kernel of ABI version 1 (R11J) refuses any flag with -34 (its length check), so a caller needs no
 * special case.  PG_START_HELD: the job is created inactive (increment 0): it holds its frames and a
 * job slot but is never scheduled until qd_job_activate(), or removed with QT_PG_CANCEL. */
#define PG_START_HELD   0x00010000u
#define PG_START_FLAGS  0xFFFF0000u

/* Errors. */
#define PG_ERR_NOFRAMES (-60)   /* no contiguous frame run free in the pool */
#define PG_ERR_ORIGIN   (-61)   /* image not linked at PG_VBASE (slot 1) */
#define PG_ERR_REFUSED  (-62)   /* operation not available to a paged process */
#define PG_ERR_NOREC    (-63)   /* no free address-space record */
#define PG_ERR_ACTIVE   (-64)   /* internal: directory still active, not released */
#define PG_ERR_NOTHELD  (-65)   /* QT_PG_CANCEL: the job is not a held paged job that has never run */

/* QT_PG_INFO result: 18 words. */
#define PG_INFO_WORDS 18
struct pg_info {
	unsigned int paged;        /*  0: 1 if the job has its own directory */
	unsigned int cr3;          /*  1: its directory (physical) */
	unsigned int frame_base;   /*  2: physical base of its window frames, 0 if unpaged */
	unsigned int frame_bytes;  /*  3 */
	unsigned int vbase;        /*  4: PG_VBASE */
	unsigned int vsize;        /*  5: PG_VSIZE */
	unsigned int held_bytes;   /*  6: frame bytes held by all paged processes now */
	unsigned int cr3_loads;    /*  7: CR3 loads since paging was enabled */
	unsigned int preempts;     /*  8: timer preemptions of that job */
	unsigned int dispatches;   /*  9: dispatches of that job */
	unsigned int region_count; /* 10: live regions in the application pool */
	unsigned int region_bytes; /* 11: bytes they hold */
	unsigned int pte_vbase;    /* 12: the PTE that maps PG_VBASE in that job's directory */
	unsigned int cur_cr3;      /* 13: CR3 active while the trap ran (the caller's) */
	unsigned int holds_deferred; /* 14: hold-flag writes deferred to the owner's CR3 */
	unsigned int records_live; /* 15: live address-space records */
	unsigned int int27_count;  /* 16: IRQ7 vector entries (TEST: int 0x27 around each CR3 load) */
	unsigned int pool_lo;      /* 17: PG_POOL_LO */
};

/* Boot: build the template, load CR3, set CR0.PG.  Prints one line. */
void pg_init(void);

/* Scheduler hooks (IF off). */
void pg_activate(int slot);           /* load slot's directory if not already active */
void pg_after_switch(int slot);       /* deliver a deferred hold-flag write */
void pg_hold_release(int slot);       /* *jobs[slot].hold = 0xFF, in the owner's space */
int pg_slot_paged(int slot);

/* Pointer check for the calling job: 1 ok, 0 refused, -1 caller is not paged. */
int pg_user_range_ok(unsigned int addr, unsigned int n);

/* Address-space records (loader). */
int pg_rec_alloc(void);                                   /* record index or PG_ERR_NOREC */
void pg_rec_build(int rec, unsigned int frames);          /* directory for those frames */
void pg_rec_bind(int rec, int slot, unsigned int job_id, long frames_handle,
                 unsigned int frames);
void pg_rec_free(int rec);                                /* never bound (failed load) */
int pg_release_owner(unsigned int job_id, int slot);      /* reclaim path; PG_ERR_ACTIVE = retry */
unsigned int pg_enter_template(void);                     /* run poll/schedule tasks */
void pg_leave_template(unsigned int was);
unsigned int pg_held_bytes(void);

/* QT_PG_CAPS body. */
int pg_caps(void);

#ifdef QDOS_TEST_PG_TASKS
/* TEST ONLY: kernel-registered poll and schedule-list tasks that record the CR3 they run under. */
void pg_test_tasks_register(void);
int pg_test_value(unsigned int idx);
#endif

/* QT_PG_INFO body.  id 0 = calling job. */
int pg_info_fill(unsigned int id, unsigned int user_addr);

/* Exception entry (paging.s): returns saved_sp | fmt for the job to resume. */
unsigned int pg_exception(unsigned int sp);

/* imgload.c / aout.c (QDOS_PAGING): QT_PG_START and the paged loader. */
int img_pg_start_request(unsigned int name_addr, unsigned int args_addr, unsigned int args_len);
int app_load_paged(app_rd_fn rd, void *ctx, unsigned int len,
                   const unsigned char *packed, unsigned int plen, int held);
int pg_cancel_held(unsigned int job_id);   /* QT_PG_CANCEL body */

/* paging.s */
void pg_load_cr3(unsigned int dir);
unsigned int pg_read_cr3(void);
unsigned int pg_read_cr2(void);
void pg_enable(void);                 /* CR0 |= PG */

#endif
