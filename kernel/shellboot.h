/*
 * QDOS/386 R7 - shell boot.  Built only with -DR7_SHELL_MODE (make i386vm-shell
 * and make pc-shell); the default images run the R6 test suite instead.
 *
 * Boot sequence: banner, descriptors, console input (IRQ1 on the PC), the
 * scheduler, the heap, the WIN1_ volume.  Then SHELL.AOU is loaded into slot 0
 * by the same loader RUN uses, and the IDLE job and timer are started.  SYSTEM
 * (the boot context) sleeps until the shell job ends, then halts with the
 * shell's exit status.  Children started with RUN run in slot 1; the shell
 * itself is never replaced.
 */
#ifndef QDOS386_SHELLBOOT_H
#define QDOS386_SHELLBOOT_H

#define SHELL_IMAGE_NAME "SHELL.AOU"
#define SHELL_HALT_NOSTATUS 100     /* the shell job ended without QT_EXIT */
#define SHELL_HALT_BOOTFAIL 101     /* the shell could not be loaded */

int shell_boot(void);

#endif
