/*
 * QDOS/386 R8 - loader negative checks (see test_r8.c).
 * Runs in SYSTEM after the R5 application phase; results go into the R4
 * check list (r4_add) like the other phases.
 */
#ifndef QDOS386_TEST_R8_H
#define QDOS386_TEST_R8_H

void r8_loader_phase(void);

#endif
