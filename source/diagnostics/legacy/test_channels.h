/*
 * QDOS/386 R4 - guest channel tests (see test_channels.c).
 *
 * channel_job_phase runs in SYSTEM after the R3 job phase.  It starts three
 * application jobs, CHAN-A (owner, full sequence), CHAN-B (intruder) and
 * CHAN-C (exits holding an open channel), and then the IRQ classification
 * checks.  Every result is recorded with r4_check and printed by IDLE.
 */
#ifndef QDOS386_TEST_CHANNELS_H
#define QDOS386_TEST_CHANNELS_H

#define R4_MAX_CHECKS 256

struct r4_check {
	const char *name;
	int ok;
	int na;                       /* not applicable on this platform */
};

extern struct r4_check r4_checks[R4_MAX_CHECKS];
extern int r4_nchecks;
extern volatile int r4_phase_done;

void channel_job_phase(void);
void r4_add(const char *name, int ok);

void test_job_chan_a(void);
void test_job_chan_b(void);
void test_job_chan_c(void);

#endif
