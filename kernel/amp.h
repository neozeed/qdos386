/*
 * QDOS/386 R15 AMP0 - master/worker multiprocessing, shared definitions (docs/R15-AMP-MAILBOX-ABI.md).
 *
 * CPU 0 (the BSP) owns the whole QDOS/386 kernel and every device.  CPU 1 (one application processor, the WORKER) runs
 * only kernel/amp_ap.c + kernel/amp_ap_s.s: flat protected mode, paging OFF (physical addressing; everything it touches
 * is kernel BSS, identity-mapped for CPU 0), its own GDT, IDT and stack, IF always clear.  The two meet only in the
 * mailbox below and its two buffers.  Opt-in: compiled only with -DQDOS_AMP (make qdos386-amp-pc).
 *
 * Mailbox field ownership (who may WRITE; anyone may read):
 *   BSP before start          boot_gen, amp_ap_stack_top_v, the AP IDT gates (amp_ap_idt), the trampoline page
 *   AP only                   ap_magic, ap_version, ap_apic_id, ap_cpuid_id, ap_lapic_base, ap_boot_gen, ap_stage,
 *                             heartbeat, ap_completed, ap_errors, burn_*, fault record, done_gen, status, out_len, r0, r1,
 *                             out buffer (while RUNNING)
 *   ap_state                  AP writes (xchg); BSP writes only while the AP is known halted (FAULTED/QUIESCED/never ran)
 *   BSP only                  ap_ctl, cancel_gen, request fields (gen, cmd, in_len, out_max, arg0, arg1) and the in buffer
 *                             (only while state == IDLE)
 *   state                     IDLE -> QUEUED (BSP, xchg); QUEUED -> RUNNING (AP, lock cmpxchg); RUNNING -> DONE / ERROR /
 *                             CANCELLED (AP, xchg); DONE / ERROR / CANCELLED -> IDLE (BSP, xchg, after copying out)
 * x86 ordering: every state change is a LOCKed instruction (xchg or lock cmpxchg: full barrier), so the fields written
 * before it are visible to the other CPU before the new state; every shared access goes through an out-of-line asm
 * function (a compiler barrier for GCC 1.40, which has no memory clobber).  docs/R15-AMP-MAILBOX-ABI.md.
 */
#ifndef QDOS386_AMP_H
#define QDOS386_AMP_H

#include "../sdk/include/qdos386amp.h"   /* the application ABI: QT_AMP, operations, commands, states, errors */

#define AMP_ABI_VERSION   1u
#define AMP_AP_MAGIC      0x31504D41u      /* "AMP1" */
#define AMP_TRAMP_PA      0x00008000u      /* real-mode trampoline page (SIPI vector 0x08), free after boot */
#define AMP_AP_STACK      8192u

/* ap_state (AP-owned) */
#define AP_ST_OFF         0u               /* never started / reset by the BSP before INIT */
#define AP_ST_ENTRY       1u               /* reached 32-bit code */
#define AP_ST_READY       2u               /* idle loop, polling the mailbox */
#define AP_ST_FAULTED     3u               /* fault recorded; the AP has halted (cli; hlt) - positive acknowledgement */
#define AP_ST_QUIESCED    4u               /* quiesce acknowledged; the AP has halted - positive acknowledgement */

/* ap_ctl (BSP-owned) */
#define AP_CTL_RUN        0u
#define AP_CTL_QUIESCE    1u

struct amp_mbox {
	volatile unsigned int ap_magic, ap_version, ap_apic_id, ap_cpuid_id, ap_lapic_base, ap_boot_gen, ap_stage;
	volatile unsigned int ap_state, ap_ctl, boot_gen;
	volatile unsigned int heartbeat, ap_completed, ap_errors, burn_iter, burn_bad;
	volatile unsigned int f_vec, f_err, f_eip, f_cr2, f_esp, f_eflags, f_apic;
	volatile unsigned int state, gen, cmd, in_len, out_max, arg0, arg1, cancel_gen;
	volatile unsigned int done_gen, status, out_len, r0, r1;
};

extern struct amp_mbox amp_mbox;
extern unsigned char amp_in[AMP_BUF_BYTES];
extern unsigned char amp_out[AMP_BUF_BYTES];
extern unsigned int amp_ap_idt[64];           /* AP IDT: 32 interrupt gates, filled by the BSP before start */
extern unsigned int amp_ap_stack_top_v;       /* read by ap_entry32 */
extern unsigned char amp_trampoline[], amp_trampoline_end[];
extern unsigned int ap_exc_table[32];

/* reasons (INFO word 2) */
#define AMP_R_NONE        0
#define AMP_R_NOCPUID     1
#define AMP_R_NOAPIC      2
#define AMP_R_APICOFF     3
#define AMP_R_NOTABLES    4
#define AMP_R_ONECPU      5
#define AMP_R_ICR         6
#define AMP_R_TIMEOUT     7
#define AMP_R_IDENTITY    8
#define AMP_R_PIT         9
#define AMP_R_DISABLED   10

/* BSP side (kernel/amp.c) */
void amp_boot(void);
int amp_trap(unsigned int a, unsigned int b, unsigned int c);
void amp_job_ended(unsigned int id);

#endif
