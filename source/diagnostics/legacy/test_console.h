/*
 * QDOS/386 R7 - console input phase (non-interactive unit checks).
 */
#ifndef QDOS386_TEST_CONSOLE_H
#define QDOS386_TEST_CONSOLE_H

extern volatile int r7c_phase_done;   /* set last by console_phase (see apps_done) */
void console_phase(void);

#endif
