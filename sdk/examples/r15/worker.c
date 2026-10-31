/* WORKER.AOU (R15 AMP0 demo): an ordinary QDOS/386 program that has work done on the worker CPU through the QT_AMP
 * service - it never touches the LAPIC or any shared kernel memory; the kernel copies its buffers in and out.
 *   WORKER FILE name     CRC-32 of a file, computed on the worker (synchronous: submit, sleep-poll, result)
 *   WORKER VECTOR n      a 16 KiB vector transform on the worker while this program keeps counting on CPU 0
 *                        (asynchronous: submit, do own work, poll, result), then checked here byte for byte
 *   WORKER BUSY          long computations one after another until it is ended with KILL (the kernel then
 *                        reclaims the request only after the worker has acknowledged cancellation)
 * On a kernel without the service (or without a second CPU) it says so and exits with status 3. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qdos386.h"
#include "qdos386amp.h"

char _prog_name[] = "Worker CPU demo";

static unsigned char buf[AMP_BUF_BYTES], res[AMP_BUF_BYTES], chk[AMP_BUF_BYTES];

static int amp(unsigned int op, unsigned int b, unsigned int c) { return qtrap(QT_AMP, op, b, c); }

static int start(unsigned int cmd, unsigned int len, unsigned int a0, unsigned int a1, unsigned int *gen)
{
	struct amp_submit s;
	int rc;
	s.cmd = cmd; s.in_ptr = (unsigned int) buf; s.in_len = len; s.out_max = AMP_BUF_BYTES; s.arg0 = a0; s.arg1 = a1; s.gen = 0;
	rc = amp(AMP_OP_SUBMIT, 0, (unsigned int) &s);
	*gen = s.gen;
	return rc;
}

static int finish(unsigned int gen, struct amp_result *r)
{
	int st;
	while ((st = amp(AMP_OP_POLL, gen, 0)) == (int) RQ_QUEUED || st == (int) RQ_RUNNING)
		qd_yield(1);                       /* sleep a tick: CPU 0 runs the other programs */
	if (st < 0)
		return st;
	r->out_ptr = (unsigned int) res;
	r->out_max = AMP_BUF_BYTES;
	return amp(AMP_OP_RESULT, gen, (unsigned int) r);
}

int main(int argc, char **argv)
{
	unsigned int w[AMP_INFO_WORDS], gen, i, n, s, x;
	unsigned long spins = 0;
	struct amp_result r;
	FILE *f;
	int rc;

	if (amp(AMP_OP_INFO, 0, (unsigned int) w) != 0 || w[1] != 1) {
		printf("WORKER: no worker CPU service on this system\n");
		return 3;
	}
	if (argc > 2 && strcmp(argv[1], "FILE") == 0) {
		f = fopen(argv[2], "rb");
		if (!f) {
			printf("WORKER: cannot open %s\n", argv[2]);
			return 2;
		}
		n = (unsigned int) fread(buf, 1, AMP_BUF_BYTES, f);
		fclose(f);
		rc = start(AMP_CMD_CRC32, n, 0, 0, &gen);
		rc = rc == 0 ? finish(gen, &r) : rc;
		printf("WORKER FILE %s: %u bytes, CRC-32 %08x (computed on CPU %u), rc %d\n", argv[2], n, r.r0, w[6], rc);
		return rc == 0 ? 0 : 1;
	}
	if (argc > 1 && strcmp(argv[1], "VECTOR") == 0) {
		n = argc > 2 ? (unsigned int) atol(argv[2]) : 1;
		for (i = 0, x = n | 1; i < AMP_BUF_BYTES; i++) {
			x = x * 69069u + 1u;
			buf[i] = (unsigned char) (x >> 24);
		}
		rc = start(AMP_CMD_XFORM, AMP_BUF_BYTES, n, 0, &gen);
		while (rc == 0 && amp(AMP_OP_POLL, gen, 0) == (int) RQ_QUEUED)
			spins++;                           /* own work on CPU 0 while the worker starts */
		while (rc == 0 && amp(AMP_OP_POLL, gen, 0) == (int) RQ_RUNNING)
			spins++;                           /* ... and while it computes */
		rc = rc == 0 ? finish(gen, &r) : rc;
		for (i = 0, s = n; i < AMP_BUF_BYTES; i++) {
			s = s * 1103515245u + 12345u;
			chk[AMP_BUF_BYTES - 1 - i] = (unsigned char) (buf[i] ^ (s >> 16));
		}
		printf("WORKER VECTOR %u: %u bytes transformed on the worker, %lu polls of own work meanwhile, %s\n", n, r.out_len,
		       spins, rc == 0 && r.out_len == AMP_BUF_BYTES && memcmp(res, chk, AMP_BUF_BYTES) == 0 ? "result verified" : "MISMATCH");
		return rc == 0 && memcmp(res, chk, AMP_BUF_BYTES) == 0 ? 0 : 1;
	}
	if (argc > 1 && strcmp(argv[1], "BUSY") == 0) {
		for (i = 1; ; i++) {                   /* one long computation after another, until KILLed */
			rc = start(AMP_CMD_NUMERIC, 0, AMP_NUM_MAX / 4u, i, &gen);
			printf("WORKER BUSY: request %u submitted (rc %d)\n", gen, rc);
			if (rc != 0 || finish(gen, &r) != 0)
				return 1;
		}
	}
	printf("usage: WORKER FILE name | VECTOR n | BUSY\n");
	return 2;
}
