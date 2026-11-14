/*
 * QDOS/386 R6 - FAT16 file phase: WIN1_ channels, the a.out loader from a
 * file, and the negative FAT16 tests.
 */
#ifndef QDOS386_TEST_FS_H
#define QDOS386_TEST_FS_H

extern volatile int r6_phase_done;   /* set last by fs_phase (see apps_done) */
void fs_phase(void);

#endif
