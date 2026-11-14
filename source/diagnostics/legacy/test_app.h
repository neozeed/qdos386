/*
 * QDOS/386 R5 - application phase (see test_app.c).
 * Runs in SYSTEM after the R4 channel phase.  Results go into the R4 check
 * list (r4_add) and are printed by the report.
 */
#ifndef QDOS386_TEST_APP_H
#define QDOS386_TEST_APP_H

void app_phase(void);

#endif
