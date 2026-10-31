/*
 * QDOS/386 R10-VGA - VGA text console for the PC platform (interface).
 * Active only when the kernel is built with -DQDOS_CON_VGA.  See vga.c.
 */
#ifndef QDOS_PC_VGA_H
#define QDOS_PC_VGA_H

#define VGA_DEFAULT_ATTR 0x07u      /* light grey on black */

void vga_init(void);                /* clear the screen, cursor to (0,0) */
void vga_putc(unsigned char c);     /* one byte of console output */
void vga_clear(void);

#endif
