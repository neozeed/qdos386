/*
 * qdos386amp.h - R15 AMP0 worker-CPU service, application ABI (docs/R15-AMP-MAILBOX-ABI.md).  Opt-in kernels only
 * (make qdos386-amp-pc); on any other kernel QT_AMP is an unknown trap.  Call with qtrap(QT_AMP, op, b, c).
 * The kernel COPIES the caller's buffers into and out of pinned worker memory; the worker CPU never sees a pointer.
 */
#ifndef QDOS386AMP_H
#define QDOS386AMP_H

#define AMP_BUF_BYTES     16384u           /* in and out buffers, each */
#define AMP_NUM_MAX       0x40000000u      /* NUMERIC iteration bound (about 8 s of worker time under QEMU TCG) */

/* request state */
#define RQ_IDLE           0u
#define RQ_QUEUED         1u
#define RQ_RUNNING        2u
#define RQ_DONE           3u
#define RQ_ERROR          4u
#define RQ_CANCELLED      5u               /* the worker saw cancel_gen == gen, stopped and acknowledged */

/* commands */
#define AMP_CMD_CRC32     1u               /* r0 = CRC-32 (IEEE, reflected) of in[0..in_len), r1 = byte sum */
#define AMP_CMD_XFORM     2u               /* out[len-1-i] = in[i] ^ LCG(arg0) byte i; out_len = in_len; r0 = LCG state */
#define AMP_CMD_NUMERIC   3u               /* xorshift32 from arg1 for arg0 iterations: r0 = x, r1 = sum; cancellable */
#define AMP_CMD_T_STALE   0xF0u            /* TEST: completes with a wrong done_gen */
#define AMP_CMD_T_FAULT   0xF1u            /* TEST: executes an invalid opcode on the AP (#UD) */
#define AMP_CMD_T_SLOW    0xF2u            /* TEST: arg0 iterations that never look at cancel_gen */

/* status (with RQ_DONE / RQ_ERROR / RQ_CANCELLED) */
#define AMP_S_OK          0u
#define AMP_S_BADCMD      1u
#define AMP_S_BOUNDS      2u
#define AMP_S_CANCELLED   3u

/* the trap */
#define QT_AMP            0x3D             /* a = AMP_OP_*, b, c */
#define AMP_OP_INFO       0                /* c = AMP_INFO_WORDS-word buffer -> 0 */
#define AMP_OP_SUBMIT     1                /* c = struct amp_submit * -> 0 (gen written back) or AMP_ERR_* */
#define AMP_OP_POLL       2                /* b = gen -> RQ_* or AMP_ERR_* */
#define AMP_OP_RESULT     3                /* b = gen, c = struct amp_result * -> 0 or AMP_ERR_* ; frees the slot */
#define AMP_OP_CANCEL     4                /* b = gen -> 0 (cancellation pending until the worker acknowledges) */
#define AMP_OP_TEST       5                /* b = AMP_T_*, c = argument / struct pointer (diagnostic operations) */
#define AMP_T_SETGEN      1                /* c = next generation (wraparound test) */
#define AMP_T_MALFORMED   2                /* publish a request whose in_len exceeds the buffer (bypasses the BSP check) */
#define AMP_T_QUIESCE     3                /* ask the worker to quiesce (acknowledged by AP_ST_QUIESCED) */
#define AMP_T_RESTART     4                /* INIT/SIPI again; only after a positive halt acknowledgement */
#define AMP_T_PROOF       5                /* c = struct amp_proof *: submit NUMERIC and spin with IF clear until done */

#define AMP_INFO_WORDS    32

#define AMP_ERR_UNAVAIL   (-80)
#define AMP_ERR_BUSY      (-81)
#define AMP_ERR_RANGE     (-82)            /* user buffer not in the caller's memory, or length over AMP_BUF_BYTES */
#define AMP_ERR_STALE     (-83)            /* generation does not match the slot, or the worker answered another one */
#define AMP_ERR_OWNER     (-84)            /* the slot belongs to another job */
#define AMP_ERR_NOTREADY  (-85)            /* RESULT before DONE / ERROR / CANCELLED */
#define AMP_ERR_STATE     (-86)            /* operation not allowed in the current worker state */
#define AMP_ERR_TIMEOUT   (-87)            /* PROOF: no completion within the spin bound (nothing is reclaimed) */
#define AMP_ERR_FAULT     (-88)            /* the worker faulted while this request was RUNNING */

struct amp_submit { unsigned int cmd, in_ptr, in_len, out_max, arg0, arg1, gen; };
struct amp_result { unsigned int out_ptr, out_max, state, status, out_len, r0, r1, gen; };
struct amp_proof  { unsigned int iters, seed, r0, r1, spins, hb_before, hb_after, ticks_before, ticks_after, status, gen; };

#endif
